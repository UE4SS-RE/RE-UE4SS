#include <format>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/Casting.hpp>
#include <Helpers/String.hpp>
#include <LuaLibrary.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/FOutputDevice.hpp>
#include <Unreal/UnrealInitializer.hpp>

namespace RC::LuaLibrary
{
    auto get_outputdevice_ref(const LuaMadeSimple::Lua& lua) -> const Unreal::FOutputDevice*
    {
        if (lua_getglobal(lua.get_lua_state(), "OutputDeviceRef") == LUA_TNIL)
        {
            lua.discard_value(-1);
            return nullptr;
        }

        // Explicitly using the top of the stack (-1) since that's where 'getglobal' puts stuff
        return static_cast<const Unreal::FOutputDevice*>(lua_touserdata(lua.get_lua_state(), -1));
    }

    auto set_outputdevice_ref(const LuaMadeSimple::Lua& lua, Unreal::FOutputDevice* output_device) -> void
    {
        lua_pushlightuserdata(lua.get_lua_state(), output_device);
        lua_setglobal(lua.get_lua_state(), "OutputDeviceRef");
    }

    auto global_print(const LuaMadeSimple::Lua& lua) -> int
    {
        auto* output_device = get_outputdevice_ref(lua);

        StringType formatted_string = STR("[Lua] ");
        StringType outdevice_string;
        if (output_device)
        {
            // Remove stack item from get_outputdevice_ref's lua_getglobal
            lua.discard_value(-1);
            outdevice_string = STR("[Lua] ");
        }

        int32_t stack_size = lua.get_stack_size();
        for (int32_t i = 1; i <= stack_size; i++)
        {
            // lua_tostring (macro of lua_tolstring) is NOT the same as luaL_tolstring
            // luaL_tolstring provides tostring()-ish conversion for any value
            auto lua_str = to_generic_string(luaL_tolstring(lua.get_lua_state(), i, nullptr));

            if (i > 1)
            {
                // Use double tab, as single tab might make the spacing too thin in the console
                formatted_string.append(STR("\t\t"));
                if (output_device) outdevice_string.append(STR("        "));
            }
            formatted_string.append(lua_str);
            if (output_device) outdevice_string.append(lua_str);

            // Remove the stack item produced by luaL_tolstring
            lua.discard_value(-1);
        }

        Output::send(formatted_string);

        if (output_device) output_device->Log(outdevice_string.c_str());

        return 0;
    }

    auto deref_to_int32(const LuaMadeSimple::Lua& lua) -> int
    {
        if (lua.get_stack_size() != 1 || !lua.is_integer())
        {
            Output::send(STR("[Fatal] Lua function 'DerefToInt32' must have only 1 parameter and it must be of type 'int'.\n"));
            lua.set_nil();
            return 1;
        }

        int32_t* int32_ptr = reinterpret_cast<int32_t*>(lua.get_integer());
        int32_t int32_val = Helper::Casting::offset_deref_safe<int32_t>(int32_ptr, 0, GetCurrentProcess());

        if (int32_val == 0)
        {
            Output::send(STR("[Fatal] Address passed to Lua function 'DerefToInt32' was not a valid pointer.\n"));
            lua.set_nil();
            return 1;
        }

        lua.set_integer(int32_val);
        return 1;
    }

    static auto error_handler_for_exported_functions(std::string_view e) -> void
    {
        // If the output system errored out then use printf_s as a fallback
        // Logging will only happen to the debug console but it's something at least
        if (!Output::has_internal_error())
        {
            Output::send(STR("Error: {}\n"), to_wstring(e));
        }
        else
        {
            printf_s("Internal Error: %s\n", e.data());
        }
    }

    static auto exported_function_status_to_string(ExportedFunctionStatus status) -> std::wstring_view
    {
        switch (status)
        {
        case ExportedFunctionStatus::NO_ERROR_TO_EXPORT:
            return L"NO_ERROR_TO_EXPORT | 0";
        case ExportedFunctionStatus::UNKNOWN_ERROR:
            return L"UNKNOWN_ERROR | 7";
        case ExportedFunctionStatus::SUCCESS:
            return L"SUCCESS | 1";
        case ExportedFunctionStatus::VARIABLE_NOT_FOUND:
            return L"VARIABLE_NOT_FOUND | 2";
        case ExportedFunctionStatus::MOD_IS_NULLPTR:
            return L"MOD_IS_NULLPTR | 3";
        case ExportedFunctionStatus::SCRIPT_FUNCTION_RETURNED_FALSE:
            return L"SCRIPT_FUNCTION_RETURNED_FALSE | 4";
        case ExportedFunctionStatus::UNABLE_TO_CALL_SCRIPT_FUNCTION:
            return L"UNABLE_TO_CALL_SCRIPT_FUNCTION | 5";
        case ExportedFunctionStatus::SCRIPT_FUNCTION_NOT_FOUND:
            return L"SCRIPT_FUNCTION_NOT_FOUND | 6";
        case ExportedFunctionStatus::UE4SS_NOT_INITIALIZED:
            return L"UE4SS_NOT_INITIALIZED | 8";
        }

        return L"Missed switch case";
    }

    auto get_lua_state_by_mod_name(const char* mod_name) -> lua_State*
    {
        auto* mod = UE4SSProgram::find_lua_mod_by_name(mod_name);
        if (!mod)
        {
            return nullptr;
        }
        return mod->lua().get_lua_state();
    };

    auto execute_lua_in_mod(const char* mod_name, const char* script, char* output_buffer) -> const char*
    {
        std::string_view output_buffer_view{output_buffer};

        auto* mod = UE4SSProgram::find_lua_mod_by_name(mod_name);
        if (!mod || !mod->is_installed() || !mod->is_started())
        {
            auto error_message = fmt::format("No mod by name '{}' found.", mod_name);
            std::memcpy(output_buffer, error_message.data(), error_message.size());
            return output_buffer;
        }

        try
        {
            if (int status = luaL_loadstring(mod->lua().get_lua_state(), script); status != LUA_OK)
            {
                mod->lua().throw_error(fmt::format("luaL_loadstring returned {}", mod->lua().resolve_status_message(status, true)));
            }

            if (int status = lua_pcall(mod->lua().get_lua_state(), 0, LUA_MULTRET, 0); status != LUA_OK)
            {
                mod->lua().throw_error(fmt::format("lua_pcall returned {}", mod->lua().resolve_status_message(status, true)));
            }
        }
        catch (std::runtime_error& e)
        {
            std::string_view what_view{e.what()};
            std::memcpy(output_buffer, what_view.data(), what_view.size());
            return output_buffer;
        }

        return nullptr;
    };

    auto set_script_variable_int32(const char* mod_name, const char* variable_name, int32_t new_value, ReturnValue& return_struct) -> void
    {
        try
        {
            if (!Unreal::UnrealInitializer::StaticStorage::bIsInitialized)
            {
                return_struct.status = ExportedFunctionStatus::UE4SS_NOT_INITIALIZED;
                Output::send(STR("set_script_variable_int32 | UE4SS is not initialized\n"));
                return;
            }

            // TODO: Remove this in non-debug versions
            /*
            const std::string tmp_var_name = variable_name;
            const std::wstring variable_name_wide = std::wstring(tmp_var_name.begin(), tmp_var_name.end());
            const std::string tmp_mod_name = mod_name;
            const std::wstring mod_name_wide = std::wstring(tmp_mod_name.begin(), tmp_mod_name.end());
            Output::send(STR("Setting variable '{}' in mod '{}' to {}\n"), variable_name_wide, mod_name_wide, new_value);
            //*/

            auto mod = UE4SSProgram::find_lua_mod_by_name(mod_name, UE4SSProgram::IsInstalled::Yes, UE4SSProgram::IsStarted::Yes);
            if (!mod)
            {
                return_struct.status = ExportedFunctionStatus::MOD_IS_NULLPTR;
                Output::send(STR("set_script_variable_int32 | return_struct.status: {}\n"), exported_function_status_to_string(return_struct.status));
                return;
            }

            const LuaMadeSimple::Lua& lua = mod->lua();

            auto type = lua_getglobal(lua.get_lua_state(), variable_name);
            lua_pop(lua.get_lua_state(), 1);

            if (type == LUA_TNIL)
            {
                return_struct.status = ExportedFunctionStatus::VARIABLE_NOT_FOUND;
                Output::send(STR("set_script_variable_int32 | return_struct.status: {}\n"), exported_function_status_to_string(return_struct.status));
                return;
            }

            lua.set_integer(new_value);
            lua_setglobal(lua.get_lua_state(), variable_name);

            return_struct.status = ExportedFunctionStatus::SUCCESS;
            Output::send(STR("set_script_variable_int32 | return_struct.status: {}\n"), exported_function_status_to_string(return_struct.status));
        }
        catch (std::runtime_error& e)
        {
            error_handler_for_exported_functions(e.what());
            return_struct.status = ExportedFunctionStatus::UNKNOWN_ERROR;
        }
    }

    auto set_script_variable_default_data([[maybe_unused]] const char* mod_name,
                                          [[maybe_unused]] const char* variable_name,
                                          [[maybe_unused]] DefaultDataStruct& external_data,
                                          ReturnValue& return_struct) -> void
    {
        try
        {
            if (!Unreal::UnrealInitializer::StaticStorage::bIsInitialized)
            {
                return_struct.status = ExportedFunctionStatus::UE4SS_NOT_INITIALIZED;
                Output::send(STR("set_script_variable_default_data | UE4SS is not initialized\n"));
                return;
            }

            // TODO: Remove this in non-debug versions
            /*
            const std::string tmp_var_name = variable_name;
            const std::wstring variable_name_wide = std::wstring(tmp_var_name.begin(), tmp_var_name.end());
            const std::string tmp_mod_name = mod_name;
            const std::wstring mod_name_wide = std::wstring(tmp_mod_name.begin(), tmp_mod_name.end());

            switch (external_data.data1_type)
            {
                case DefaultDataType::ConstCharPtr:
                {
                    const std::string tmp_data1_value_ansi = external_data.data1.as_string;
                    const std::wstring data1_value_wide = std::wstring(tmp_data1_value_ansi.begin(), tmp_data1_value_ansi.end());
                    Output::send(STR("Setting '{}.data1' as string to '{}' in mod '{}'"), variable_name_wide, data1_value_wide, mod_name_wide);
                    break;
                }
                case DefaultDataType::Float:
                    Output::send(STR("Setting '{}.data1' as float to '{}' in mod '{}'"), variable_name_wide, external_data.data1.as_float, mod_name_wide);
                    break;
            }
            //*/

            auto mod = UE4SSProgram::find_lua_mod_by_name(mod_name, UE4SSProgram::IsInstalled::Yes, UE4SSProgram::IsStarted::Yes);
            if (!mod)
            {
                return_struct.status = ExportedFunctionStatus::MOD_IS_NULLPTR;
                Output::send(STR("set_script_variable_default_data | return_struct.status: {}\n"), exported_function_status_to_string(return_struct.status));
                return;
            }

            const LuaMadeSimple::Lua& lua = mod->lua();
            auto lua_table = lua.prepare_new_table();

            if (external_data.data1.as_string)
            {
                switch (external_data.data1_type)
                {
                case DefaultDataType::ConstCharPtr:
                    lua_table.add_pair("data1", external_data.data1.as_string);
                    break;
                case DefaultDataType::Float:
                    lua_table.add_pair("data1", external_data.data1.as_float);
                    break;
                }
            }

            if (external_data.data2.as_string)
            {
                switch (external_data.data2_type)
                {
                case DefaultDataType::ConstCharPtr:
                    lua_table.add_pair("data2", external_data.data2.as_string);
                    break;
                case DefaultDataType::Float:
                    lua_table.add_pair("data2", external_data.data2.as_float);
                    break;
                }
            }

            if (external_data.data3.as_string)
            {
                switch (external_data.data3_type)
                {
                case DefaultDataType::ConstCharPtr:
                    lua_table.add_pair("data3", external_data.data3.as_string);
                    break;
                case DefaultDataType::Float:
                    lua_table.add_pair("data3", external_data.data3.as_float);
                    break;
                }
            }

            if (external_data.data4.as_string)
            {
                switch (external_data.data4_type)
                {
                case DefaultDataType::ConstCharPtr:
                    lua_table.add_pair("data4", external_data.data4.as_string);
                    break;
                case DefaultDataType::Float:
                    lua_table.add_pair("data4", external_data.data4.as_float);
                    break;
                }
            }

            lua_table.make_global(variable_name);

            return_struct.status = ExportedFunctionStatus::SUCCESS;
        }
        catch (std::runtime_error& e)
        {
            error_handler_for_exported_functions(e.what());
            return_struct.status = ExportedFunctionStatus::UNKNOWN_ERROR;
        }
    }

    auto call_script_function(const char* mod_name, const char* function_name, ReturnValue& return_struct, ScriptFuncReturnValue& script_return_struct) -> void
    {
        try
        {
            if (!Unreal::UnrealInitializer::StaticStorage::bIsInitialized)
            {
                return_struct.status = ExportedFunctionStatus::UE4SS_NOT_INITIALIZED;
                Output::send(STR("call_script_function | UE4SS is not initialized\n"));
                return;
            }

            // TODO: Remove this in non-debug versions
            /*
            const std::string tmp_func_name = function_name;
            const std::wstring func_name_wide = std::wstring(tmp_func_name.begin(), tmp_func_name.end());
            const std::string tmp_mod_name = mod_name;
            const std::wstring mod_name_wide = std::wstring(tmp_mod_name.begin(), tmp_mod_name.end());
            Output::send(STR("Calling script function '{}' in mod '{}'\n"), func_name_wide, mod_name_wide);
            //*/

            auto mod = UE4SSProgram::find_lua_mod_by_name(mod_name, UE4SSProgram::IsInstalled::Yes, UE4SSProgram::IsStarted::Yes);
            if (!mod)
            {
                return_struct.status = ExportedFunctionStatus::MOD_IS_NULLPTR;
                Output::send(STR("call_script_function | return_struct.status: {}\n"), exported_function_status_to_string(return_struct.status));
                return;
            }

            const LuaMadeSimple::Lua& lua = mod->lua();

            try
            {
                lua.prepare_function_call(function_name);
            }
            catch (std::runtime_error&)
            {
                return_struct.status = ExportedFunctionStatus::SCRIPT_FUNCTION_NOT_FOUND;
                Output::send(STR("call_script_function | return_struct.status: {}\n"), exported_function_status_to_string(return_struct.status));
                return;
            }

            lua.call_function(0, 1);

            if (lua.is_nil())
            {
                // Stack is empty or nil, treat as 'false' (default is already 'false')
                // Since we assume that 'nil == false' we don't actually retrieve anything from teh Lua stack
                // Because of that we need to manually remove the 'nil' from the Lua stack
                lua.discard_value();
            }
            else if (lua.is_bool())
            {
                script_return_struct.return_value = lua.get_bool();

                if (!script_return_struct.return_value)
                {
                    return_struct.status = ExportedFunctionStatus::SCRIPT_FUNCTION_RETURNED_FALSE;
                }
            }
            else if (lua.is_integer())
            {
                int64_t script_return_value = lua.get_integer();
                script_return_struct.return_value = script_return_value >= 1;
            }
            else
            {
                // The return type is not something that can be deduced to true/false, lets assume false
            }

            if (return_struct.status == ExportedFunctionStatus::NO_ERROR_TO_EXPORT)
            {
                return_struct.status = ExportedFunctionStatus::SUCCESS;
            }

            Output::send(STR("call_script_function | return_struct.status: {}\n"), exported_function_status_to_string(return_struct.status));
        }
        catch (std::runtime_error& e)
        {
            error_handler_for_exported_functions(e.what());
            return_struct.status = ExportedFunctionStatus::UNKNOWN_ERROR;
        }
    }

    auto is_ue4ss_initialized() -> bool
    {
        return Unreal::UnrealInitializer::StaticStorage::bIsInitialized;
    }
} // namespace RC::LuaLibrary
