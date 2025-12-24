#define NOMINMAX

#include <atomic>
#include <filesystem>
#include <format>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include <DynamicOutput/DynamicOutput.hpp>
#include <ExceptionHandling.hpp>
#include <Helpers/Format.hpp>
#include <Helpers/String.hpp>
#include <Input/Handler.hpp>
#include <LuaLibrary.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <LuaType/LuaAActor.hpp>
#include <LuaType/LuaCustomProperty.hpp>
#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaFText.hpp>
#include <LuaType/LuaUnrealString.hpp>
#include <LuaType/LuaFOutputDevice.hpp>
#include <LuaType/LuaModRef.hpp>
#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaFURL.hpp>
#include <LuaType/LuaThreadId.hpp>
#include <Mod/CppMod.hpp>
#include <Mod/LuaMod.hpp>
#pragma warning(disable : 4005)
#include <GUI/Dumpers.hpp>
#include <UE4SSProgram.hpp>
#include <USMapGenerator/Generator.hpp>
#include <Unreal/Core/HAL/Platform.hpp>
#include <Unreal/FFrame.hpp>
#include <Unreal/FURL.hpp>
#include <Unreal/FWorldContext.hpp>
#include <Unreal/FOutputDevice.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/PackageName.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/CoreUObject/UObject/FStrProperty.hpp>
#include <Unreal/Core/Containers/FUtf8String.hpp>
#include <Unreal/Core/Containers/FAnsiString.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/CoreUObject/UObject/FUtf8StrProperty.hpp>
#include <Unreal/TypeChecker.hpp>
#include <Unreal/UAssetRegistry.hpp>
#include <Unreal/UAssetRegistryHelpers.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/UGameViewportClient.hpp>
#include <Unreal/UKismetSystemLibrary.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UnrealVersion.hpp>
#include <UnrealCustom/CustomProperty.hpp>
#include <UE4SSRuntime.hpp>
#include <Unreal/UnrealInitializer.hpp>

#if PLATFORM_WINDOWS
#include <Unreal/Core/Windows/AllowWindowsPlatformTypes.hpp>
#endif
#pragma warning(default : 4005)

namespace RC
{
    LuaMadeSimple::Lua* LuaStatics::console_executor{};
    bool LuaStatics::console_executor_enabled{};

    auto get_mod_ref(const LuaMadeSimple::Lua& lua) -> LuaMod*
    {
        if (lua_getglobal(lua.get_lua_state(), "ModRef") == LUA_TNIL)
        {
            lua.throw_error("[get_mod_ref] Tried retrieving 'ModRef' global variable but it was nil, please do not override this global");
        }

        // Explicitly using the top of the stack (-1) since that's where 'getglobal' puts stuff
        auto& lua_object = lua.get_userdata<LuaType::LuaModRef>(-1);
        return lua_object.get_remote_cpp_object();
    }

    static auto get_function_name_without_prefix(const StringType& function_full_name) -> StringType
    {
        static constexpr StringViewType function_prefix{STR("Function ")};
        if (auto prefix_pos = function_full_name.find(function_prefix); prefix_pos != function_full_name.npos)
        {
            return function_full_name.substr(prefix_pos + function_prefix.size());
        }
        else
        {
            return function_full_name;
        }
    }

    struct LuaUnrealScriptFunctionData
    {
        Unreal::CallbackId pre_callback_id;
        Unreal::CallbackId post_callback_id;
        Unreal::UFunction* unreal_function;
        const Mod* mod;
        const LuaMadeSimple::Lua& lua;
        const int lua_callback_ref;
        const int lua_post_callback_ref;
        const int lua_thread_ref;

        bool has_return_value{};
        // Will be non-nullptr if the UFunction has a return value
        Unreal::FProperty* return_property{};
        std::atomic<bool> scheduled_for_removal{false};

        LuaUnrealScriptFunctionData(Unreal::CallbackId pre_id,
                                    Unreal::CallbackId post_id,
                                    Unreal::UFunction* func,
                                    const Mod* m,
                                    const LuaMadeSimple::Lua& l,
                                    int cb_ref,
                                    int post_cb_ref,
                                    int thread_ref)
            : pre_callback_id(pre_id), post_callback_id(post_id), unreal_function(func), mod(m), lua(l),
              lua_callback_ref(cb_ref), lua_post_callback_ref(post_cb_ref), lua_thread_ref(thread_ref)
        {
        }
    };
    static std::vector<std::unique_ptr<LuaUnrealScriptFunctionData>> g_hooked_script_function_data{};

    static auto lua_unreal_script_function_hook_pre(Unreal::UnrealScriptFunctionCallableContext context, void* custom_data) -> void
    {
        // Fetch the data corresponding to this UFunction
        auto& lua_data = *static_cast<LuaUnrealScriptFunctionData*>(custom_data);

        // Check if this hook has been scheduled for removal (Lua state may be invalid)
        if (lua_data.scheduled_for_removal) return;

        // Use the stored registry index to put a Lua function on the Lua stack
        // This is the function that was provided by the Lua call to "RegisterHook"
        lua_data.lua.registry().get_function_ref(lua_data.lua_callback_ref);

        // Set up the first param (context / this-ptr)
        // TODO: Check what happens if a static UFunction is hooked since they don't have any context
        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
        LuaType::RemoteUnrealParam::construct(lua_data.lua, &context.Context, s_object_property_name);

        // Attempt at dynamically fetching the params
        const auto FunctionBeingExecuted = lua_data.unreal_function;
        uint16_t return_value_offset = FunctionBeingExecuted->GetReturnValueOffset();

        // 'ReturnValueOffset' is 0xFFFF if the UFunction return type is void
        lua_data.has_return_value = return_value_offset != 0xFFFF;

        uint8_t num_unreal_params = FunctionBeingExecuted->GetNumParms();
        if (lua_data.has_return_value)
        {
            // Subtract one from the number of params if there's a return value
            // This is because Unreal treats the return value as a param, and it's included in the 'NumParms' member variable
            --num_unreal_params;
        }

        bool has_properties_to_process = lua_data.has_return_value || num_unreal_params > 0;
        if (has_properties_to_process && (context.TheStack.Locals() || context.TheStack.OutParms()))
        {
            // int32_t current_param_offset{};

            for (Unreal::FProperty* func_prop : Unreal::TFieldRange<Unreal::FProperty>(FunctionBeingExecuted, Unreal::EFieldIterationFlags::IncludeDeprecated))
            {
                // Skip this property if it's not a parameter
                if (!func_prop->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_Parm))
                {
                    continue;
                }

                // Skip if this property corresponds to the return value
                if (lua_data.has_return_value && func_prop->GetOffset_Internal() == return_value_offset)
                {
                    lua_data.return_property = func_prop;
                    continue;
                }

                Unreal::FName property_type = func_prop->GetClass().GetFName();
                int32_t name_comparison_index = property_type.GetComparisonIndex();

                if (LuaType::StaticState::m_property_value_pushers.contains(name_comparison_index))
                {
                    // Non-typed pointer to the current parameter value
                    void* data{};
                    if (func_prop->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_OutParm))
                    {
                        data = Unreal::FindOutParamValueAddress(context.TheStack, func_prop);
                    }
                    else
                    {
                        data = func_prop->ContainerPtrToValuePtr<void>(context.TheStack.Locals());
                    }

                    // Keeping track of where in the 'Locals' array the next property is
                    // current_param_offset += func_prop->GetSize();

                    // Set up a call to a handler for this type of Unreal property (the param)
                    // The FName is being used as a key for an unordered_map which has the types & corresponding handlers filled right after the dll is injected
                    const LuaType::PusherParams pusher_params{.operation = LuaType::Operation::GetParam,
                                                              .lua = lua_data.lua,
                                                              .base = nullptr,
                                                              .data = data,
                                                              .property = func_prop};
                    LuaType::StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                }
                else
                {
                    lua_data.lua.throw_error(fmt::format(
                            "[unreal_script_function_hook] Tried accessing unreal property without a registered handler. Property type '{}' not supported.",
                            to_string(property_type.ToString())));
                }
            }
        }

        // Call the Lua function with the correct number of parameters & return values
        // Increasing the 'num_params' by one to account for the 'this / context' param
        lua_data.lua.call_function(num_unreal_params + 1, 1);

        // The params for the Lua script will be 'userdata' and they will have get/set functions
        // Use these functions in the Lua script to access & mutate the parameter values

        // After the Lua function has been executed you should call the original function
        // This will execute any internal UE4 scripting functions & native functions depending on the type of UFunction
        // The API will automatically call the original function
        // This function continues in 'lua_unreal_script_function_hook_post' which executes immediately after the original function gets called
    }

    static auto lua_unreal_script_function_hook_post(Unreal::UnrealScriptFunctionCallableContext context, void* custom_data) -> void
    {
        // Fetch the data corresponding to this UFunction
        auto& lua_data = *static_cast<LuaUnrealScriptFunctionData*>(custom_data);

        // Returns true if a hooks were removed.
        auto remove_if_scheduled = [&] -> bool {
            if (lua_data.scheduled_for_removal)
            {
                const auto function_name_no_prefix = get_function_name_without_prefix(lua_data.unreal_function->GetFullName());

                Output::send<LogLevel::Verbose>(STR("Unregistering native pre-hook ({}) for {}\n"), lua_data.pre_callback_id, function_name_no_prefix);
                lua_data.unreal_function->UnregisterHook(lua_data.pre_callback_id);
                luaL_unref(lua_data.lua.get_lua_state(), LUA_REGISTRYINDEX, lua_data.lua_callback_ref);

                Output::send<LogLevel::Verbose>(STR("Unregistering native post-hook ({}) for {}\n"), lua_data.post_callback_id, function_name_no_prefix);
                lua_data.unreal_function->UnregisterHook(lua_data.post_callback_id);
                if (lua_data.lua_post_callback_ref != -1)
                {
                    luaL_unref(lua_data.lua.get_lua_state(), LUA_REGISTRYINDEX, lua_data.lua_post_callback_ref);
                }

                const auto mod = get_mod_ref(lua_data.lua);
                luaL_unref(mod->lua().get_lua_state(), LUA_REGISTRYINDEX, lua_data.lua_thread_ref);
                std::erase_if(g_hooked_script_function_data, [&](const std::unique_ptr<LuaUnrealScriptFunctionData>& elem) {
                    return elem.get() == &lua_data;
                });

                return true;
            }
            else
            {
                return false;
            }
        };

        // Removes pre & post-hook callbacks if UnregisterHook was called in the pre-callback.
        if (remove_if_scheduled())
        {
            return;
        }

        auto process_return_value = [&]() {
            // If 'nil' exists on the Lua stack, that means that the UFunction expected a return value but the Lua script didn't return anything
            // So we can simply clean the stack and let the UFunction decide the return value on its own
            if (lua_data.lua.is_nil())
            {
                lua_data.lua.discard_value();
            }
            else if (lua_data.lua.get_stack_size() > 0 && lua_data.has_return_value && lua_data.return_property && context.RESULT_DECL)
            {
                // Fetch the return value from Lua if the UFunction expects one
                // If no return value exists then assume that the Lua script didn't want to override the original
                // Keep in mind that the if this was a Blueprint UFunction then the entire byte-code will already have executed
                // That means that changing the return value here won't affect the script itself
                // If this was a native UFunction then changing the return value here will have the desired effect

                Unreal::FName property_type_name = lua_data.return_property->GetClass().GetFName();
                int32_t name_comparison_index = property_type_name.GetComparisonIndex();

                if (LuaType::StaticState::m_property_value_pushers.contains(name_comparison_index))
                {
                    const LuaType::PusherParams pusher_params{.operation = LuaType::Operation::Set,
                                                              .lua = lua_data.lua,
                                                              .base = static_cast<Unreal::UObject*>(context.RESULT_DECL),
                                                              .data = context.RESULT_DECL,
                                                              .property = lua_data.return_property};
                    LuaType::StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                }
                else
                {
                    // If the type wasn't supported then we simply clean the Lua stack, output a warning and then do nothing
                    lua_data.lua.discard_value();

                    auto parameter_type_name = property_type_name.ToString();
                    auto parameter_name = lua_data.return_property->GetName();

                    Output::send(
                            STR("Tried altering return value of a hooked UFunction without a registered handler for return type Return property '{}' of type "
                                "'{}' not supported."),
                            parameter_name,
                            parameter_type_name);
                }
            }
        };

        if (lua_data.lua_post_callback_ref != -1)
        {
            // Use the stored registry index to put a Lua function on the Lua stack
            // This is the function that was provided by the Lua call to "RegisterHook"
            lua_data.lua.registry().get_function_ref(lua_data.lua_post_callback_ref);

            // Set up the first param (context / this-ptr)
            static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
            LuaType::RemoteUnrealParam::construct(lua_data.lua, &context.Context, s_object_property_name);

            // Attempt at dynamically fetching the params
            const auto FunctionBeingExecuted = lua_data.unreal_function;
            uint16_t return_value_offset = FunctionBeingExecuted->GetReturnValueOffset();

            // 'ReturnValueOffset' is 0xFFFF if the UFunction return type is void
            lua_data.has_return_value = return_value_offset != 0xFFFF;

            uint8_t num_unreal_params = FunctionBeingExecuted->GetNumParms();
            if (lua_data.has_return_value)
            {
                // Subtract one from the number of params if there's a return value
                // This is because Unreal treats the return value as a param, and it's included in the 'NumParms' member variable
                --num_unreal_params;

                // Set up the return value param so that Lua can access the original return value
                auto return_property = FunctionBeingExecuted->GetReturnProperty();
                auto return_property_type = return_property->GetClass().GetFName();
                int32_t name_comparison_index = return_property_type.GetComparisonIndex();
                if (LuaType::StaticState::m_property_value_pushers.contains(name_comparison_index))
                {
                    const LuaType::PusherParams pusher_params{.operation = LuaType::Operation::GetParam,
                                                              .lua = lua_data.lua,
                                                              .base = nullptr,
                                                              .data = context.RESULT_DECL,
                                                              .property = return_property};
                    LuaType::StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                }
            }

            bool has_properties_to_process = lua_data.has_return_value || num_unreal_params > 0;
            if (has_properties_to_process && context.TheStack.Locals())
            {
                for (Unreal::FProperty* func_prop : Unreal::TFieldRange<Unreal::FProperty>(FunctionBeingExecuted, Unreal::EFieldIterationFlags::IncludeDeprecated))
                {
                    // Skip this property if it's not a parameter
                    if (!func_prop->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_Parm))
                    {
                        continue;
                    }

                    // Skip if this property corresponds to the return value
                    if (lua_data.has_return_value && func_prop->GetOffset_Internal() == return_value_offset)
                    {
                        lua_data.return_property = func_prop;
                        continue;
                    }

                    Unreal::FName property_type = func_prop->GetClass().GetFName();
                    int32_t name_comparison_index = property_type.GetComparisonIndex();

                    if (LuaType::StaticState::m_property_value_pushers.contains(name_comparison_index))
                    {
                        // Non-typed pointer to the current parameter value
                        void* data{};
                        if (func_prop->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_OutParm))
                        {
                            // For out params (including ref params), get the modified value from OutParms
                            data = Unreal::FindOutParamValueAddress(context.TheStack, func_prop);
                        }
                        else
                        {
                            // For regular input params, read from Locals
                            data = func_prop->ContainerPtrToValuePtr<void>(context.TheStack.Locals());
                        }

                        // Keeping track of where in the 'Locals' array the next property is
                        // current_param_offset += func_prop->GetSize();

                        // Set up a call to a handler for this type of Unreal property (the param)
                        // The FName is being used as a key for an unordered_map which has the types & corresponding handlers filled right after the dll is injected
                        const LuaType::PusherParams pusher_params{.operation = LuaType::Operation::GetParam,
                                                                  .lua = lua_data.lua,
                                                                  .base = nullptr,
                                                                  .data = data,
                                                                  .property = func_prop};
                        LuaType::StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                    }
                    else
                    {
                        lua_data.lua.throw_error(fmt::format(
                                "[unreal_script_function_hook] Tried accessing unreal property without a registered handler. Property type '{}' not supported.",
                                to_string(property_type.ToString())));
                    }
                }
            }

            // Call the Lua function with the correct number of parameters & return values
            // Increasing the 'num_params' by one to account for the 'this / context' param
            // Increasing it again if there's a return value because we store that as the second param
            lua_data.lua.call_function(num_unreal_params + (lua_data.has_return_value ? 2 : 1), 1);
        }

        // Processing potential return values from both callbacks.
        // Stack pos 1: return value from callback 1 (nil if nothing returned)
        // Stack pos 2: return value from callback 2
        // We will always have at leaste two return values, either one can be nil, and we need to process both in case one isn't nil.
        process_return_value();
        process_return_value();

        // Removes pre & post-hook callbacks if UnregisterHook was called in the post-hook callback.
        remove_if_scheduled();
    }

    static auto register_input_globals(const LuaMadeSimple::Lua& lua) -> void
    {
        LuaMadeSimple::Lua::Table key_table = lua.prepare_new_table();
        key_table.add_pair("LEFT_MOUSE_BUTTON", static_cast<uint32_t>(Input::Key::LEFT_MOUSE_BUTTON));
        key_table.add_pair("RIGHT_MOUSE_BUTTON", static_cast<uint32_t>(Input::Key::RIGHT_MOUSE_BUTTON));
        key_table.add_pair("CANCEL", static_cast<uint32_t>(Input::Key::CANCEL));
        key_table.add_pair("MIDDLE_MOUSE_BUTTON", static_cast<uint32_t>(Input::Key::MIDDLE_MOUSE_BUTTON));
        key_table.add_pair("XBUTTON_ONE", static_cast<uint32_t>(Input::Key::XBUTTON_ONE));
        key_table.add_pair("XBUTTON_TWO", static_cast<uint32_t>(Input::Key::XBUTTON_TWO));
        key_table.add_pair("BACKSPACE", static_cast<uint32_t>(Input::Key::BACKSPACE));
        key_table.add_pair("TAB", static_cast<uint32_t>(Input::Key::TAB));
        key_table.add_pair("CLEAR", static_cast<uint32_t>(Input::Key::CLEAR));
        key_table.add_pair("RETURN", static_cast<uint32_t>(Input::Key::RETURN));
        key_table.add_pair("PAUSE", static_cast<uint32_t>(Input::Key::PAUSE));
        key_table.add_pair("CAPS_LOCK", static_cast<uint32_t>(Input::Key::CAPS_LOCK));
        key_table.add_pair("IME_KANA", static_cast<uint32_t>(Input::Key::IME_KANA));
        key_table.add_pair("IME_HANGUEL", static_cast<uint32_t>(Input::Key::IME_HANGUEL));
        key_table.add_pair("IME_HANGUL", static_cast<uint32_t>(Input::Key::IME_HANGUL));
        key_table.add_pair("IME_ON", static_cast<uint32_t>(Input::Key::IME_ON));
        key_table.add_pair("IME_JUNJA", static_cast<uint32_t>(Input::Key::IME_JUNJA));
        key_table.add_pair("IME_FINAL", static_cast<uint32_t>(Input::Key::IME_FINAL));
        key_table.add_pair("IME_HANJA", static_cast<uint32_t>(Input::Key::IME_HANJA));
        key_table.add_pair("IME_KANJI", static_cast<uint32_t>(Input::Key::IME_KANJI));
        key_table.add_pair("IME_OFF", static_cast<uint32_t>(Input::Key::IME_OFF));
        key_table.add_pair("ESCAPE", static_cast<uint32_t>(Input::Key::ESCAPE));
        key_table.add_pair("IME_CONVERT", static_cast<uint32_t>(Input::Key::IME_CONVERT));
        key_table.add_pair("IME_NONCONVERT", static_cast<uint32_t>(Input::Key::IME_NONCONVERT));
        key_table.add_pair("IME_ACCEPT", static_cast<uint32_t>(Input::Key::IME_ACCEPT));
        key_table.add_pair("IME_MODECHANGE", static_cast<uint32_t>(Input::Key::IME_MODECHANGE));
        key_table.add_pair("SPACE", static_cast<uint32_t>(Input::Key::SPACE));
        key_table.add_pair("PAGE_UP", static_cast<uint32_t>(Input::Key::PAGE_UP));
        key_table.add_pair("PAGE_DOWN", static_cast<uint32_t>(Input::Key::PAGE_DOWN));
        key_table.add_pair("END", static_cast<uint32_t>(Input::Key::END));
        key_table.add_pair("HOME", static_cast<uint32_t>(Input::Key::HOME));
        key_table.add_pair("LEFT_ARROW", static_cast<uint32_t>(Input::Key::LEFT_ARROW));
        key_table.add_pair("UP_ARROW", static_cast<uint32_t>(Input::Key::UP_ARROW));
        key_table.add_pair("RIGHT_ARROW", static_cast<uint32_t>(Input::Key::RIGHT_ARROW));
        key_table.add_pair("DOWN_ARROW", static_cast<uint32_t>(Input::Key::DOWN_ARROW));
        key_table.add_pair("SELECT", static_cast<uint32_t>(Input::Key::SELECT));
        key_table.add_pair("PRINT", static_cast<uint32_t>(Input::Key::PRINT));
        key_table.add_pair("EXECUTE", static_cast<uint32_t>(Input::Key::EXECUTE));
        key_table.add_pair("PRINT_SCREEN", static_cast<uint32_t>(Input::Key::PRINT_SCREEN));
        key_table.add_pair("INS", static_cast<uint32_t>(Input::Key::INS));
        key_table.add_pair("DEL", static_cast<uint32_t>(Input::Key::DEL));
        key_table.add_pair("HELP", static_cast<uint32_t>(Input::Key::HELP));
        key_table.add_pair("ZERO", static_cast<uint32_t>(Input::Key::ZERO));
        key_table.add_pair("ONE", static_cast<uint32_t>(Input::Key::ONE));
        key_table.add_pair("TWO", static_cast<uint32_t>(Input::Key::TWO));
        key_table.add_pair("THREE", static_cast<uint32_t>(Input::Key::THREE));
        key_table.add_pair("FOUR", static_cast<uint32_t>(Input::Key::FOUR));
        key_table.add_pair("FIVE", static_cast<uint32_t>(Input::Key::FIVE));
        key_table.add_pair("SIX", static_cast<uint32_t>(Input::Key::SIX));
        key_table.add_pair("SEVEN", static_cast<uint32_t>(Input::Key::SEVEN));
        key_table.add_pair("EIGHT", static_cast<uint32_t>(Input::Key::EIGHT));
        key_table.add_pair("NINE", static_cast<uint32_t>(Input::Key::NINE));
        key_table.add_pair("A", static_cast<uint32_t>(Input::Key::A));
        key_table.add_pair("B", static_cast<uint32_t>(Input::Key::B));
        key_table.add_pair("C", static_cast<uint32_t>(Input::Key::C));
        key_table.add_pair("D", static_cast<uint32_t>(Input::Key::D));
        key_table.add_pair("E", static_cast<uint32_t>(Input::Key::E));
        key_table.add_pair("F", static_cast<uint32_t>(Input::Key::F));
        key_table.add_pair("G", static_cast<uint32_t>(Input::Key::G));
        key_table.add_pair("H", static_cast<uint32_t>(Input::Key::H));
        key_table.add_pair("I", static_cast<uint32_t>(Input::Key::I));
        key_table.add_pair("J", static_cast<uint32_t>(Input::Key::J));
        key_table.add_pair("K", static_cast<uint32_t>(Input::Key::K));
        key_table.add_pair("L", static_cast<uint32_t>(Input::Key::L));
        key_table.add_pair("M", static_cast<uint32_t>(Input::Key::M));
        key_table.add_pair("N", static_cast<uint32_t>(Input::Key::N));
        key_table.add_pair("O", static_cast<uint32_t>(Input::Key::O));
        key_table.add_pair("P", static_cast<uint32_t>(Input::Key::P));
        key_table.add_pair("Q", static_cast<uint32_t>(Input::Key::Q));
        key_table.add_pair("R", static_cast<uint32_t>(Input::Key::R));
        key_table.add_pair("S", static_cast<uint32_t>(Input::Key::S));
        key_table.add_pair("T", static_cast<uint32_t>(Input::Key::T));
        key_table.add_pair("U", static_cast<uint32_t>(Input::Key::U));
        key_table.add_pair("V", static_cast<uint32_t>(Input::Key::V));
        key_table.add_pair("W", static_cast<uint32_t>(Input::Key::W));
        key_table.add_pair("X", static_cast<uint32_t>(Input::Key::X));
        key_table.add_pair("Y", static_cast<uint32_t>(Input::Key::Y));
        key_table.add_pair("Z", static_cast<uint32_t>(Input::Key::Z));
        key_table.add_pair("LEFT_WIN", static_cast<uint32_t>(Input::Key::LEFT_WIN));
        key_table.add_pair("RIGHT_WIN", static_cast<uint32_t>(Input::Key::RIGHT_WIN));
        key_table.add_pair("APPS", static_cast<uint32_t>(Input::Key::APPS));
        key_table.add_pair("SLEEP", static_cast<uint32_t>(Input::Key::SLEEP));
        key_table.add_pair("NUM_ZERO", static_cast<uint32_t>(Input::Key::NUM_ZERO));
        key_table.add_pair("NUM_ONE", static_cast<uint32_t>(Input::Key::NUM_ONE));
        key_table.add_pair("NUM_TWO", static_cast<uint32_t>(Input::Key::NUM_TWO));
        key_table.add_pair("NUM_THREE", static_cast<uint32_t>(Input::Key::NUM_THREE));
        key_table.add_pair("NUM_FOUR", static_cast<uint32_t>(Input::Key::NUM_FOUR));
        key_table.add_pair("NUM_FIVE", static_cast<uint32_t>(Input::Key::NUM_FIVE));
        key_table.add_pair("NUM_SIX", static_cast<uint32_t>(Input::Key::NUM_SIX));
        key_table.add_pair("NUM_SEVEN", static_cast<uint32_t>(Input::Key::NUM_SEVEN));
        key_table.add_pair("NUM_EIGHT", static_cast<uint32_t>(Input::Key::NUM_EIGHT));
        key_table.add_pair("NUM_NINE", static_cast<uint32_t>(Input::Key::NUM_NINE));
        key_table.add_pair("MULTIPLY", static_cast<uint32_t>(Input::Key::MULTIPLY));
        key_table.add_pair("ADD", static_cast<uint32_t>(Input::Key::ADD));
        key_table.add_pair("SEPARATOR", static_cast<uint32_t>(Input::Key::SEPARATOR));
        key_table.add_pair("SUBTRACT", static_cast<uint32_t>(Input::Key::SUBTRACT));
        key_table.add_pair("DECIMAL", static_cast<uint32_t>(Input::Key::DECIMAL));
        key_table.add_pair("DIVIDE", static_cast<uint32_t>(Input::Key::DIVIDE));
        key_table.add_pair("F1", static_cast<uint32_t>(Input::Key::F1));
        key_table.add_pair("F2", static_cast<uint32_t>(Input::Key::F2));
        key_table.add_pair("F3", static_cast<uint32_t>(Input::Key::F3));
        key_table.add_pair("F4", static_cast<uint32_t>(Input::Key::F4));
        key_table.add_pair("F5", static_cast<uint32_t>(Input::Key::F5));
        key_table.add_pair("F6", static_cast<uint32_t>(Input::Key::F6));
        key_table.add_pair("F7", static_cast<uint32_t>(Input::Key::F7));
        key_table.add_pair("F8", static_cast<uint32_t>(Input::Key::F8));
        key_table.add_pair("F9", static_cast<uint32_t>(Input::Key::F9));
        key_table.add_pair("F10", static_cast<uint32_t>(Input::Key::F10));
        key_table.add_pair("F11", static_cast<uint32_t>(Input::Key::F11));
        key_table.add_pair("F12", static_cast<uint32_t>(Input::Key::F12));
        key_table.add_pair("F13", static_cast<uint32_t>(Input::Key::F13));
        key_table.add_pair("F14", static_cast<uint32_t>(Input::Key::F14));
        key_table.add_pair("F15", static_cast<uint32_t>(Input::Key::F15));
        key_table.add_pair("F16", static_cast<uint32_t>(Input::Key::F16));
        key_table.add_pair("F17", static_cast<uint32_t>(Input::Key::F17));
        key_table.add_pair("F18", static_cast<uint32_t>(Input::Key::F18));
        key_table.add_pair("F19", static_cast<uint32_t>(Input::Key::F19));
        key_table.add_pair("F20", static_cast<uint32_t>(Input::Key::F20));
        key_table.add_pair("F21", static_cast<uint32_t>(Input::Key::F21));
        key_table.add_pair("F22", static_cast<uint32_t>(Input::Key::F22));
        key_table.add_pair("F23", static_cast<uint32_t>(Input::Key::F23));
        key_table.add_pair("F24", static_cast<uint32_t>(Input::Key::F24));
        key_table.add_pair("NUM_LOCK", static_cast<uint32_t>(Input::Key::NUM_LOCK));
        key_table.add_pair("SCROLL_LOCK", static_cast<uint32_t>(Input::Key::SCROLL_LOCK));
        key_table.add_pair("BROWSER_BACK", static_cast<uint32_t>(Input::Key::BROWSER_BACK));
        key_table.add_pair("BROWSER_FORWARD", static_cast<uint32_t>(Input::Key::BROWSER_FORWARD));
        key_table.add_pair("BROWSER_REFRESH", static_cast<uint32_t>(Input::Key::BROWSER_REFRESH));
        key_table.add_pair("BROWSER_STOP", static_cast<uint32_t>(Input::Key::BROWSER_STOP));
        key_table.add_pair("BROWSER_SEARCH", static_cast<uint32_t>(Input::Key::BROWSER_SEARCH));
        key_table.add_pair("BROWSER_FAVORITES", static_cast<uint32_t>(Input::Key::BROWSER_FAVORITES));
        key_table.add_pair("BROWSER_HOME", static_cast<uint32_t>(Input::Key::BROWSER_HOME));
        key_table.add_pair("VOLUME_MUTE", static_cast<uint32_t>(Input::Key::VOLUME_MUTE));
        key_table.add_pair("VOLUME_DOWN", static_cast<uint32_t>(Input::Key::VOLUME_DOWN));
        key_table.add_pair("VOLUME_UP", static_cast<uint32_t>(Input::Key::VOLUME_UP));
        key_table.add_pair("MEDIA_NEXT_TRACK", static_cast<uint32_t>(Input::Key::MEDIA_NEXT_TRACK));
        key_table.add_pair("MEDIA_PREV_TRACK", static_cast<uint32_t>(Input::Key::MEDIA_PREV_TRACK));
        key_table.add_pair("MEDIA_STOP", static_cast<uint32_t>(Input::Key::MEDIA_STOP));
        key_table.add_pair("MEDIA_PLAY_PAUSE", static_cast<uint32_t>(Input::Key::MEDIA_PLAY_PAUSE));
        key_table.add_pair("LAUNCH_MAIL", static_cast<uint32_t>(Input::Key::LAUNCH_MAIL));
        key_table.add_pair("LAUNCH_MEDIA_SELECT", static_cast<uint32_t>(Input::Key::LAUNCH_MEDIA_SELECT));
        key_table.add_pair("LAUNCH_APP1", static_cast<uint32_t>(Input::Key::LAUNCH_APP1));
        key_table.add_pair("LAUNCH_APP2", static_cast<uint32_t>(Input::Key::LAUNCH_APP2));
        key_table.add_pair("OEM_ONE", static_cast<uint32_t>(Input::Key::OEM_ONE));
        key_table.add_pair("OEM_PLUS", static_cast<uint32_t>(Input::Key::OEM_PLUS));
        key_table.add_pair("OEM_COMMA", static_cast<uint32_t>(Input::Key::OEM_COMMA));
        key_table.add_pair("OEM_MINUS", static_cast<uint32_t>(Input::Key::OEM_MINUS));
        key_table.add_pair("OEM_PERIOD", static_cast<uint32_t>(Input::Key::OEM_PERIOD));
        key_table.add_pair("OEM_TWO", static_cast<uint32_t>(Input::Key::OEM_TWO));
        key_table.add_pair("OEM_THREE", static_cast<uint32_t>(Input::Key::OEM_THREE));
        key_table.add_pair("OEM_FOUR", static_cast<uint32_t>(Input::Key::OEM_FOUR));
        key_table.add_pair("OEM_FIVE", static_cast<uint32_t>(Input::Key::OEM_FIVE));
        key_table.add_pair("OEM_SIX", static_cast<uint32_t>(Input::Key::OEM_SIX));
        key_table.add_pair("OEM_SEVEN", static_cast<uint32_t>(Input::Key::OEM_SEVEN));
        key_table.add_pair("OEM_EIGHT", static_cast<uint32_t>(Input::Key::OEM_EIGHT));
        key_table.add_pair("OEM_102", static_cast<uint32_t>(Input::Key::OEM_102));
        key_table.add_pair("IME_PROCESS", static_cast<uint32_t>(Input::Key::IME_PROCESS));
        key_table.add_pair("PACKET", static_cast<uint32_t>(Input::Key::PACKET));
        key_table.add_pair("ATTN", static_cast<uint32_t>(Input::Key::ATTN));
        key_table.add_pair("CRSEL", static_cast<uint32_t>(Input::Key::CRSEL));
        key_table.add_pair("EXSEL", static_cast<uint32_t>(Input::Key::EXSEL));
        key_table.add_pair("EREOF", static_cast<uint32_t>(Input::Key::EREOF));
        key_table.add_pair("PLAY", static_cast<uint32_t>(Input::Key::PLAY));
        key_table.add_pair("ZOOM", static_cast<uint32_t>(Input::Key::ZOOM));
        key_table.add_pair("PA1", static_cast<uint32_t>(Input::Key::PA1));
        key_table.add_pair("OEM_CLEAR", static_cast<uint32_t>(Input::Key::OEM_CLEAR));
        key_table.make_global("Key");

        LuaMadeSimple::Lua::Table modifier_key_table = lua.prepare_new_table();
        modifier_key_table.add_pair("SHIFT", 0x10);
        modifier_key_table.add_pair("CONTROL", 0x11);
        modifier_key_table.add_pair("ALT", 0x12);
        /*modifier_key_table.add_pair("LEFT_SHIFT", 0xA0);
        modifier_key_table.add_pair("RIGHT_SHIFT", 0xA1);
        modifier_key_table.add_pair("LEFT_CONTROL", 0xA2);
        modifier_key_table.add_pair("RIGHT_CONTROL", 0xA3);
        modifier_key_table.add_pair("LEFT_ALT", 0xA4);
        modifier_key_table.add_pair("RIGHT_ALT", 0xA5);*/
        modifier_key_table.make_global("ModifierKey");
    }

    static auto register_object_flags(const LuaMadeSimple::Lua& lua) -> void
    {
        LuaMadeSimple::Lua::Table object_flags_table = lua.prepare_new_table();
        object_flags_table.add_pair("RF_NoFlags", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_NoFlags));
        object_flags_table.add_pair("RF_Public", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_Public));
        object_flags_table.add_pair("RF_Standalone", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_Standalone));
        object_flags_table.add_pair("RF_MarkAsNative", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_MarkAsNative));
        object_flags_table.add_pair("RF_Transactional", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_Transactional));
        object_flags_table.add_pair("RF_ClassDefaultObject",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_ClassDefaultObject));
        object_flags_table.add_pair("RF_ArchetypeObject", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_ArchetypeObject));
        object_flags_table.add_pair("RF_Transient", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_Transient));
        object_flags_table.add_pair("RF_MarkAsRootSet", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_MarkAsRootSet));
        object_flags_table.add_pair("RF_TagGarbageTemp", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_TagGarbageTemp));
        object_flags_table.add_pair("RF_NeedInitialization",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_NeedInitialization));
        object_flags_table.add_pair("RF_NeedLoad", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_NeedLoad));
        object_flags_table.add_pair("RF_KeepForCooker", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_KeepForCooker));
        object_flags_table.add_pair("RF_NeedPostLoad", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_NeedPostLoad));
        object_flags_table.add_pair("RF_NeedPostLoadSubobjects",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_NeedPostLoadSubobjects));
        object_flags_table.add_pair("RF_NewerVersionExists",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_NewerVersionExists));
        object_flags_table.add_pair("RF_BeginDestroyed", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_BeginDestroyed));
        object_flags_table.add_pair("RF_FinishDestroyed", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_FinishDestroyed));
        object_flags_table.add_pair("RF_BeingRegenerated", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_BeingRegenerated));
        object_flags_table.add_pair("RF_DefaultSubObject", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_DefaultSubObject));
        object_flags_table.add_pair("RF_WasLoaded", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_WasLoaded));
        object_flags_table.add_pair("RF_TextExportTransient",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_TextExportTransient));
        object_flags_table.add_pair("RF_LoadCompleted", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_LoadCompleted));
        object_flags_table.add_pair("RF_InheritableComponentTemplate",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_InheritableComponentTemplate));
        object_flags_table.add_pair("RF_DuplicateTransient",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_DuplicateTransient));
        object_flags_table.add_pair("RF_StrongRefOnFrame", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_StrongRefOnFrame));
        object_flags_table.add_pair("RF_NonPIEDuplicateTransient",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_NonPIEDuplicateTransient));
        object_flags_table.add_pair("RF_Dynamic", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_Dynamic));
        object_flags_table.add_pair("RF_WillBeLoaded", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_WillBeLoaded));
        object_flags_table.add_pair("RF_HasExternalPackage",
                                    static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_HasExternalPackage));
        object_flags_table.add_pair("RF_AllFlags", static_cast<std::underlying_type_t<Unreal::EObjectFlags>>(Unreal::EObjectFlags::RF_AllFlags));
        object_flags_table.make_global("EObjectFlags");

        LuaMadeSimple::Lua::Table object_internal_flags_table = lua.prepare_new_table();
        object_internal_flags_table.add_pair("ReachableInCluster",
                                             static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::ReachableInCluster));
        object_internal_flags_table.add_pair("ClusterRoot",
                                             static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::ClusterRoot));
        object_internal_flags_table.add_pair("Native", static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::Native));
        object_internal_flags_table.add_pair("Async", static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::Async));
        object_internal_flags_table.add_pair("AsyncLoading",
                                             static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::AsyncLoading));
        object_internal_flags_table.add_pair("Unreachable",
                                             static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::Unreachable));
        object_internal_flags_table.add_pair("PendingKill",
                                             static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::PendingKill));
        object_internal_flags_table.add_pair("RootSet", static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::RootSet));
        object_internal_flags_table.add_pair("GarbageCollectionKeepFlags",
                                             static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(
                                                     Unreal::EInternalObjectFlags::GarbageCollectionKeepFlags));
        object_internal_flags_table.add_pair("AllFlags", static_cast<std::underlying_type_t<Unreal::EInternalObjectFlags>>(Unreal::EInternalObjectFlags::AllFlags));
        object_internal_flags_table.make_global("EInternalObjectFlags");
    }

    static auto register_efindname(const LuaMadeSimple::Lua& lua) -> void
    {
        LuaMadeSimple::Lua::Table efindname_table = lua.prepare_new_table();
        efindname_table.add_pair("FNAME_Find", static_cast<std::underlying_type_t<Unreal::EFindName>>(Unreal::EFindName::FNAME_Find));
        efindname_table.add_pair("FNAME_Add", static_cast<std::underlying_type_t<Unreal::EFindName>>(Unreal::EFindName::FNAME_Add));
        efindname_table.add_pair("FNAME_Replace_Not_Safe_For_Threading",
                                 static_cast<std::underlying_type_t<Unreal::EFindName>>(Unreal::EFindName::FNAME_Replace_Not_Safe_For_Threading));
        efindname_table.make_global("EFindName");
    }

    LuaMod::LuaMod(UE4SSProgram& program, StringType&& mod_name, StringType&& mod_path)
        : Mod(program, std::move(mod_name), std::move(mod_path)), m_lua(LuaMadeSimple::new_state())
    {
        // First check for "Scripts" (capital S)
        std::filesystem::path scripts_path = m_mod_path / STR("Scripts");

        // If not found, try with lowercase "scripts"
        if (!std::filesystem::exists(scripts_path))
        {
            std::filesystem::path alt_scripts_path = m_mod_path / STR("scripts");
            if (std::filesystem::exists(alt_scripts_path))
            {
                scripts_path = alt_scripts_path;
            }
        }

        m_scripts_path = scripts_path;

        if (!std::filesystem::exists(m_scripts_path))
        {
            Output::send<LogLevel::Error>(STR("Mod path doesn't exist {}\n"), ensure_str(m_scripts_path));
            set_installable(false);
            return;
        }
    }

    auto LuaMod::global_uninstall() -> void
    {
    }

    template <typename PropertyType>
    auto add_property_type_table(const LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua::Table& property_types_table, std::string_view property_type_name) -> void
    {
        property_types_table.add_key(property_type_name.data());

        auto property_type_table = lua.prepare_new_table();
        property_type_table.add_pair("Name", property_type_name.data());

        if constexpr (Unreal::IsTProperty<PropertyType>)
        {
            // TODO: Update LuaMadeSimple to accept an unsigned long long, and do it with proper bounds checking
            property_type_table.add_pair("Size", static_cast<int64_t>(sizeof(typename PropertyType::TCppType)));
        }
        else
        {
            // Sizes for types are unknown and will only be known dynamically at runtime
            // TODO: The size is used in LuaTArray to calculate the address of an element (element index * size)
            //       Reimplement this by requiring a custom "Size" field in the Lua table
            property_type_table.add_pair("Size", 0);
        }

        // property_type_table.add_pair("Size", PropertyType::size);
        //  This should be a lightuserdata instead of a reinterpret_cast to int64_t
        //  This is not very safe at all, what if the pointer is too large for a signed 64-bit integer ?
        property_type_table.add_pair("FFieldClassPointer", static_cast<int64_t>(PropertyType::StaticClass().HashObject()));
        // TODO: Figure out if the static object pointer is needed
        property_type_table.add_pair("StaticPointer", 0);

        property_types_table.fuse_pair();

        property_type_table.make_local();
    }

    // Private helper: Ensures hook thread exists and returns the registry reference (or LUA_REFNIL if already exists)
    static int ensure_hook_thread_exists(LuaMod* mod)
    {
        if (mod->m_hook_lua == nullptr)
        {
            // First use - create new thread and anchor it in the registry
            mod->m_hook_lua = &mod->lua().new_thread();
            int thread_ref = luaL_ref(mod->lua().get_lua_state(), LUA_REGISTRYINDEX);
            return thread_ref;
        }

        // Thread already exists and is already anchored
        return LUA_REFNIL;
    }

    // Returns the hook lua thread for immediate use (doesn't need registry reference management)
    auto static get_hook_lua(LuaMod* mod) -> LuaMadeSimple::Lua*
    {
        ensure_hook_thread_exists(mod);
        return mod->m_hook_lua;
    }

    // Returns hook state with registry reference (for persistent hooks that need cleanup)
    // auto static make_hook_state(Mod* mod, const LuaMadeSimple::Lua& lua)->std::shared_ptr<LuaMadeSimple::Lua>
    auto static make_hook_state(LuaMod* mod) -> std::pair<LuaMadeSimple::Lua*, int>
    {
        int thread_ref = ensure_hook_thread_exists(mod);
        return {mod->m_hook_lua, thread_ref};

        // Make the hook thread (which is just a separate Lua stack) be a global in its parent.
        // This is needed because otherwise it will be GCd when we don't want it to.
        // lua_setglobal(lua.get_lua_state(), "HookThread");

        // Commenting out until we switch to lua_newstate instead of lua_newthread.
        // For the switch to happen, we need to be able to move or copy Lua types across lua_states which we can't do yet.
        /*
        mod->m_hook_lua->register_function("RegisterHook", [](const LuaMadeSimple::Lua& lua) -> int {
            lua.throw_error("'RegisterHook' is not allowed from the game thread");
            return 0;
        });

        mod->m_hook_lua->register_function("NotifyOnNewObject", [](const LuaMadeSimple::Lua& lua) -> int {
            lua.throw_error("'NotifyOnNewObject' is not allowed in the game thread");
            return 0;
        });
        //*/
        //}
    }

    auto static make_async_state(LuaMod* mod, const LuaMadeSimple::Lua& lua) -> void
    {
        if (!mod->m_async_lua)
        {
            mod->m_async_lua = &lua.new_thread();

            // Make the hook thread (which is just a separate Lua stack) be a global in its parent.
            // This is needed because otherwise it will be GCd when we don't want it to.
            lua_setglobal(lua.get_lua_state(), "AsyncThread");

            // Commenting out until we switch to lua_newstate instead of lua_newthread.
            // For the switch to happen, we need to be able to move or copy Lua types across lua_states which we can't do yet.
            /*
            mod->m_hook_lua->register_function("RegisterHook", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.throw_error("'RegisterHook' is not allowed from the game thread");
                return 0;
            });

            mod->m_hook_lua->register_function("NotifyOnNewObject", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.throw_error("'NotifyOnNewObject' is not allowed in the game thread");
                return 0;
            });
            //*/
        }
    }

    auto static make_main_state(LuaMod* mod, const LuaMadeSimple::Lua& lua) -> void
    {
        if (!mod->m_main_lua)
        {
            mod->m_main_lua = &lua.new_thread();

            // Make the hook thread (which is just a separate Lua stack) be a global in its parent.
            // This is needed because otherwise it will be GCd when we don't want it to.
            lua_setglobal(lua.get_lua_state(), "MainThread");

            // Commenting out until we switch to lua_newstate instead of lua_newthread.
            // For the switch to happen, we need to be able to move or copy Lua types across lua_states which we can't do yet.
            /*
            mod->m_hook_lua->register_function("RegisterHook", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.throw_error("'RegisterHook' is not allowed from the game thread");
                return 0;
            });

            mod->m_hook_lua->register_function("NotifyOnNewObject", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.throw_error("'NotifyOnNewObject' is not allowed in the game thread");
                return 0;
            });
            //*/
        }
    }
    auto static register_all_property_types(const LuaMadeSimple::Lua& lua) -> void
    {
        auto property_types_table = lua.prepare_new_table();

        add_property_type_table<Unreal::FObjectProperty>(lua, property_types_table, "ObjectProperty");
        add_property_type_table<Unreal::FObjectPtrProperty>(lua, property_types_table, "ObjectPtrProperty");
        add_property_type_table<Unreal::FInt8Property>(lua, property_types_table, "Int8Property");
        add_property_type_table<Unreal::FInt16Property>(lua, property_types_table, "Int16Property");
        add_property_type_table<Unreal::FIntProperty>(lua, property_types_table, "IntProperty");
        add_property_type_table<Unreal::FInt64Property>(lua, property_types_table, "Int64Property");
        add_property_type_table<Unreal::FByteProperty>(lua, property_types_table, "ByteProperty");
        add_property_type_table<Unreal::FUInt16Property>(lua, property_types_table, "UInt16Property");
        add_property_type_table<Unreal::FUInt32Property>(lua, property_types_table, "UInt32Property");
        add_property_type_table<Unreal::FUInt64Property>(lua, property_types_table, "UInt64Property");
        add_property_type_table<Unreal::FNameProperty>(lua, property_types_table, "NameProperty");
        add_property_type_table<Unreal::FFloatProperty>(lua, property_types_table, "FloatProperty");
        // add_property_type_table<Unreal::FStrProperty>(lua, property_types_table, "StrProperty");
        add_property_type_table<Unreal::FBoolProperty>(lua, property_types_table, "BoolProperty");
        add_property_type_table<Unreal::FArrayProperty>(lua, property_types_table, "ArrayProperty");
        add_property_type_table<Unreal::FSetProperty>(lua, property_types_table, "SetProperty");
        add_property_type_table<Unreal::FMapProperty>(lua, property_types_table, "MapProperty");
        add_property_type_table<Unreal::FStructProperty>(lua, property_types_table, "StructProperty");
        add_property_type_table<Unreal::FClassProperty>(lua, property_types_table, "ClassProperty");
        add_property_type_table<Unreal::FWeakObjectProperty>(lua, property_types_table, "WeakObjectProperty");
        if (Unreal::Version::IsAtLeast(4, 15))
        {
            add_property_type_table<Unreal::FEnumProperty>(lua, property_types_table, "EnumProperty");
        }
        add_property_type_table<Unreal::FTextProperty>(lua, property_types_table, "TextProperty");
        add_property_type_table<Unreal::FStrProperty>(lua, property_types_table, "StrProperty");
        if (Unreal::Version::IsAtLeast(5, 6))
        {
            add_property_type_table<Unreal::FUtf8StrProperty>(lua, property_types_table, "Utf8StrProperty");
        }

        property_types_table.make_global("PropertyTypes");
    }

    auto LuaMod::setup_custom_module_loader(const LuaMadeSimple::Lua* lua_state) -> void
    {
        lua_State* L = lua_state->get_lua_state();
    
        // Initialize ue4ss_loaded_modules table
        lua_newtable(L);
        lua_setglobal(L, "ue4ss_loaded_modules");
    
        // Get package.searchers table
        lua_getglobal(L, "package");
        if (!lua_istable(L, -1))
        {
            Output::send<LogLevel::Error>(STR("package table not found\n"));
            lua_pop(L, 1);
            return;
        }
    
        lua_getfield(L, -1, "searchers");
        if (!lua_istable(L, -1))
        {
            Output::send<LogLevel::Error>(STR("package.searchers table not found\n"));
            lua_pop(L, 2);
            return;
        }
    
        // Insert our searcher at position 1
        // First, shift existing searchers up
        lua_Integer len = luaL_len(L, -1);
        for (lua_Integer i = len; i >= 1; i--)
        {
            lua_geti(L, -1, i);     // Get searchers[i]
            lua_seti(L, -2, i + 1); // Set searchers[i+1] = searchers[i]
        }
    
        // Push the LuaMod instance as a light userdata (our upvalue)
        lua_pushlightuserdata(L, this);
    
        // Create the C closure with one upvalue
        lua_pushcclosure(L, custom_module_searcher, 1);
    
        // Set searchers[1] = our new closure
        lua_seti(L, -2, 1);
    
        lua_pop(L, 2); // Clean up stack: searchers, package
    }

    // Static C function for the module searcher
    int LuaMod::custom_module_searcher(lua_State* L)
    {
        const char* module_name = luaL_checkstring(L, 1);
        if (!module_name)
        {
            lua_pushstring(L, "module name is required");
            return 1;
        }
        
        // Get the LuaMod* from the upvalue at index 1
        auto* lua_mod = static_cast<LuaMod*>(lua_touserdata(L, lua_upvalueindex(1)));
        if (!lua_mod)
        {
            lua_pushstring(L, "custom searcher is missing its C++ context");
            return 1;
        }
        
        // Get paths directly from the C++ object and convert to UTF-8 strings for Lua
        const auto& mods_dir = lua_mod->get_program().get_mods_directory();
        const auto& mod_name = lua_mod->get_name();
        const auto& scripts_path = lua_mod->get_scripts_path();
        
        std::string mods_path_str = normalize_path_for_lua(mods_dir);
        std::string mod_name_str = to_utf8_string(mod_name);
        std::string scripts_path_str = normalize_path_for_lua(scripts_path);
        
        // Try different path combinations
        std::vector<std::string> paths_to_try = {
            scripts_path_str + "/" + module_name + ".lua",
            mods_path_str + "/shared/" + module_name + ".lua",
            mods_path_str + "/shared/" + module_name + "/" + module_name + ".lua"
        };
        
        // Try each path
        std::string attempted_paths_str;
        for (const auto& path : paths_to_try)
        {
            // Convert to wide string for Windows filesystem operations
            std::wstring wide_path;
            try
            {
                wide_path = utf8_to_wpath(path);
            }
            catch (const std::exception&)
            {
                attempted_paths_str += "\n\t" + path + " (encoding error)";
                continue;
            }
            
            if (!std::filesystem::exists(wide_path))
            {
                attempted_paths_str += "\n\t" + path;
                continue;
            }
            
            // Check if already loaded
            lua_getglobal(L, "ue4ss_loaded_modules");
            lua_pushstring(L, path.c_str());
            lua_gettable(L, -2);
            
            if (!lua_isnil(L, -1))
            {
                // Already loaded, return the cached function
                lua_remove(L, -2); // Remove ue4ss_loaded_modules table
                return 1;
            }
            
            lua_pop(L, 2); // Pop nil and ue4ss_loaded_modules
            
            // Try to load the file
            std::ifstream file(wide_path, std::ios::binary);
            if (!file.is_open())
            {
                attempted_paths_str += "\n\t" + path + " (cannot open)";
                continue;
            }
            
            // Get file size and read content
            file.seekg(0, std::ios::end);
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            
            std::vector<char> buffer(size);
            if (!file.read(buffer.data(), size))
            {
                attempted_paths_str += "\n\t" + path + " (read error)";
                continue;
            }
            file.close();
            
            // Create chunk name for debugging
            std::string chunk_name = "@" + path;
            
            // Load the script as a function that returns the module
            std::string module_wrapper = "return function()\n" + std::string(buffer.data(), buffer.size()) + "\nend";
            
            if (luaL_loadbuffer(L, module_wrapper.c_str(), module_wrapper.size(), chunk_name.c_str()) != LUA_OK)
            {
                attempted_paths_str += "\n\t" + path + " (syntax error: " + lua_tostring(L, -1) + ")";
                lua_pop(L, 1); // Pop error message
                continue;
            }
            
            // Execute to get the loader function
            if (lua_pcall(L, 0, 1, 0) != LUA_OK)
            {
                attempted_paths_str += "\n\t" + path + " (execution error: " + lua_tostring(L, -1) + ")";
                lua_pop(L, 1); // Pop error message
                continue;
            }
            
            // Cache the loaded module
            lua_getglobal(L, "ue4ss_loaded_modules");
            lua_pushstring(L, path.c_str());
            lua_pushvalue(L, -3); // Copy the function
            lua_settable(L, -3);
            lua_pop(L, 1); // Pop ue4ss_loaded_modules
            
            // Return the loader function
            return 1;
        }
        
        // Module not found
        std::string error_msg = "module '" + std::string(module_name) + "' not found:" + attempted_paths_str;
        lua_pushstring(L, error_msg.c_str());
        return 1;
    }

    auto LuaMod::setup_lua_require_paths(const LuaMadeSimple::Lua& lua) const -> void
    {
        try
        {
            auto* lua_state = m_lua.get_lua_state();
            lua_getglobal(lua_state, "package");

            // Get current paths
            lua_getfield(lua_state, -1, "path");
            std::string current_paths = lua_tostring(lua_state, -1);
            lua_pop(lua_state, 1);

            auto mods_dir = m_program.get_mods_directory();
            auto mod_name = get_name();

            // Create normalized UTF-8 paths with forward slashes
            std::string mods_dir_utf8 = normalize_path_for_lua(mods_dir);
            std::string mod_name_utf8 = to_utf8_string(mod_name);
            std::string scripts_path_utf8 = normalize_path_for_lua(m_scripts_path);

            // Create path strings with forward slashes for Lua
            std::string script_path = fmt::format(";{}/?.lua", scripts_path_utf8);
            std::string shared_path = fmt::format(";{}/shared/?.lua", mods_dir_utf8);
            std::string shared_nested_path = fmt::format(";{}/shared/?/?.lua", mods_dir_utf8);

            current_paths.append(script_path);
            current_paths.append(shared_path);
            current_paths.append(shared_nested_path);

            lua_pushstring(lua_state, current_paths.c_str());
            lua_setfield(lua_state, -2, "path");

            // Now set up cpath similarly
            lua_getfield(lua_state, -1, "cpath");
            std::string current_cpaths = lua_tostring(lua_state, -1);
            lua_pop(lua_state, 1);

            // Create cpath strings
            std::string script_dll_path = fmt::format(";{}/?.dll", scripts_path_utf8);
            std::string mod_dll_path = fmt::format(";{}/{}/?/?.dll", mods_dir_utf8, mod_name_utf8);

            current_cpaths.append(script_dll_path);
            current_cpaths.append(mod_dll_path);

            lua_pushstring(lua_state, current_cpaths.c_str());
            lua_setfield(lua_state, -2, "cpath");

            lua_pop(lua_state, 1); // Pop package table
        }
        catch (const std::exception& e)
        {
            Output::send<LogLevel::Error>(STR("Exception in setup_lua_require_paths: {}\n"), ensure_str(e.what()));
            throw;
        }
    }

    auto LuaMod::get_object_names(const Unreal::UObject* object) -> std::vector<Unreal::FName>
    {
        std::vector<Unreal::FName> names{};
        for (auto ptr = object; ptr; ptr = ptr->GetOuterPrivate())
        {
            names.emplace_back(ptr->GetNamePrivate());
        }
        return names;
    }

    auto LuaMod::find_function_hook_data(std::vector<FunctionHookData>& container, Unreal::FName in_name) -> FunctionHookData*
    {
        for (auto it = container.begin(); it != container.end(); ++it)
        {
            if (it->names.size() >= 1 && in_name.Equals(it->names[0]))
            {
                return &*it;
            }
        }
        return nullptr;
    }

    auto LuaMod::find_function_hook_data(std::vector<FunctionHookData>& container, const Unreal::UObject* object) -> FunctionHookData*
    {
        return find_function_hook_data(container, get_object_names(object));
    }

    auto LuaMod::find_function_hook_data(std::vector<FunctionHookData>& container, const std::vector<Unreal::FName>& in_name) -> FunctionHookData*
    {
        for (auto it = container.begin(); it != container.end(); ++it)
        {
            auto found = true;
            if (in_name.size() != it->names.size())
            {
                continue;
            }
            for (const auto& [index, name] : std::ranges::enumerate_view(it->names))
            {
                if (!name.Equals(in_name[index]))
                {
                    found = false;
                }
            }
            if (found)
            {
                return &*it;
            }
        }
        return nullptr;
    }

    auto LuaMod::remove_function_hook_data(std::vector<FunctionHookData>& container, StringViewType in_name) -> void
    {
        remove_function_hook_data(container, Unreal::FName(in_name, Unreal::FNAME_Add));
    }

    auto LuaMod::remove_function_hook_data(std::vector<FunctionHookData>& container, Unreal::FName in_name) -> void
    {
        for (auto it = container.begin(); it != container.end(); ++it)
        {
            if (it->names.size() >= 1 && it->names[0] == in_name)
            {
                container.erase(it);
                break;
            }
        }
    }

    auto LuaMod::remove_function_hook_data(std::vector<FunctionHookData>& container, const Unreal::UObject* object) -> void
    {
        remove_function_hook_data(container, get_object_names(object));
    }

    auto LuaMod::remove_function_hook_data(std::vector<FunctionHookData>& container, const std::vector<Unreal::FName>& in_name) -> void
    {
        for (auto it = container.begin(); it != container.end(); ++it)
        {
            auto found = true;
            if (in_name.size() != it->names.size())
            {
                continue;
            }
            for (const auto& [index, name] : std::ranges::enumerate_view(it->names))
            {
                if (name != in_name[index])
                {
                    found = false;
                }
            }
            if (found)
            {
                container.erase(it);
            }
        }
    }

    auto static setup_lua_global_functions_internal(const LuaMadeSimple::Lua& lua, Mod::IsTrueMod is_true_mod) -> void
    {
        lua.register_function("print", LuaLibrary::global_print);
        lua.register_function("LoadExport", LuaLibrary::load_export);

        lua.register_function("CreateInvalidObject", [](const LuaMadeSimple::Lua& lua) -> int {
            LuaType::auto_construct_object(lua, nullptr);
            return 1;
        });

        lua.register_function("StaticFindObject", [](const LuaMadeSimple::Lua& lua) -> int {
            // Stack size @ the start of the function is the same as the number of params
            int32_t stack_size = lua.get_stack_size();

            if (stack_size <= 0)
            {
                lua.throw_error("Function 'StaticFindObject' cannot be called with 0 parameters.");
            }

            std::string error_overload_not_found{R"(
No overload found for function 'StaticFindObject'.
Overloads:
#1: StaticFindObject(string name)
#2: StaticFindObject(UClass* Class, UObject* InOuter, string name, bool ExactClass = false))"};

            // Overload #1
            // P1: string name
            // Ignores any params after P1
            if (lua.is_string())
            {
                Unreal::UObject* object = Unreal::UObjectGlobals::StaticFindObject(nullptr, nullptr, ensure_str(lua.get_string()));

                // Construct a Lua object of type 'UObject'
                // Auto constructing is nullptr safe
                LuaType::auto_construct_object(lua, object);

                return 1;
            }

            // Overload #2
            // P1: UClass* Class
            // P2: UObject* InOuter
            // P3: string Name
            // P4: bool ExactClass = false
            // Full definition of StaticFindObject, including default values
            // Ignores any params after P4
            if (stack_size < 3)
            {
                // No overload found for function 'StaticFindObject'. Overloads are:
                lua.throw_error(error_overload_not_found);
            }

            Unreal::UClass* param_class{};
            Unreal::UObject* param_in_outer{};
            StringType param_name{};
            bool param_exact_class{};

            // P1 (Class), userdata
            if (lua.is_userdata())
            {
                auto& lua_object = lua.get_userdata<LuaType::UClass>();
                param_class = lua_object.get_remote_cpp_object();
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            // P2 (InOuter), userdata
            if (lua.is_userdata())
            {
                auto& lua_object = lua.get_userdata<LuaType::UObject>();
                param_in_outer = lua_object.get_remote_cpp_object();
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            // P3 (Name), string
            if (lua.is_string())
            {
                param_name = ensure_str(lua.get_string());
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            // P4 (ExactClass), bool = false
            if (lua.is_bool())
            {
                param_exact_class = lua.get_bool();
            }
            // There's no error if P4 isn't a bool, simply ignore all parameters after P3

            Unreal::UObject* object = Unreal::UObjectGlobals::StaticFindObject(param_class, param_in_outer, param_name, param_exact_class);

            // Construct a Lua object of type 'UObject'
            // Auto constructing is nullptr safe
            LuaType::auto_construct_object(lua, object);

            return 1;
        });

        lua.register_function("FindFirstOf", [](const LuaMadeSimple::Lua& lua) -> int {
            // Stack size @ the start of the function is the same as the number of params
            int32_t stack_size = lua.get_stack_size();

            if (stack_size <= 0)
            {
                lua.throw_error("Function 'FindFirstOf' cannot be called with 0 parameters.");
            }

            std::string error_overload_not_found{R"(
No overload found for function 'FindFirstOf'.
Overloads:
#1: FindFirstOf(string short_class_name))"};

            // Overload #1
            // P1: string short_name
            // Ignores any params after P1
            if (lua.is_string())
            {
                Unreal::UObject* object = Unreal::UObjectGlobals::FindFirstOf(ensure_str(lua.get_string()));

                // Construct a Lua object of type 'UObject'
                // Auto constructing is nullptr safe
                LuaType::auto_construct_object(lua, object);

                return 1;
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            return 0;
        });

        lua.register_function("FindAllOf", [](const LuaMadeSimple::Lua& lua) -> int {
            // Stack size @ the start of the function is the same as the number of params
            int32_t stack_size = lua.get_stack_size();

            if (stack_size <= 0)
            {
                lua.throw_error("Function 'FindAllOf' cannot be called with 0 parameters.");
            }

            std::string error_overload_not_found{R"(
No overload found for function 'FindAllOf'.
Overloads:
#1: FindAllOf(string short_class_name))"};

            // Overload #1
            // P1: string short_name
            // Ignores any params after P1
            if (lua.is_string())
            {
                constexpr int32_t elements_to_reserve = 40;

                std::vector<Unreal::UObject*> found_unreal_objects;

                // Reserving some space because FindAllOf is likely to find lots of objects
                found_unreal_objects.reserve(elements_to_reserve);

                Unreal::UObjectGlobals::FindAllOf(lua.get_string(), found_unreal_objects);

                if (!found_unreal_objects.empty())
                {
                    LuaMadeSimple::Lua::Table table = lua.prepare_new_table(elements_to_reserve);

                    for (size_t count{}; const auto& unreal_object : found_unreal_objects)
                    {
                        // Increasing the count first, this is to accommodate the one-index based tables of Lua
                        ++count;

                        table.add_key(count);

                        // Construct a Lua version of a UObject
                        // It will be at the top of the Lua stack and can act as the value of a key/value pair if fuse_pair() is called
                        LuaType::auto_construct_object(lua, unreal_object);
                        table.fuse_pair();
                    }

                    table.make_local();
                }
                else
                {
                    lua.set_nil();
                }

                return 1;
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            // This code isn't executed
            // Lua will error out in the else statement above
            // This is purely to shut the compiler up
            lua.set_nil();
            return 1;
        });

        if (is_true_mod == Mod::IsTrueMod::Yes)
        {
            lua.register_function("IsKeyBindRegistered", [](const LuaMadeSimple::Lua& lua) -> int {
                std::string error_overload_not_found{R"(
No overload found for function 'IsKeyBindRegistered'.
Overloads:
#1: IsKeyBindRegistered(integer key)
#2: IsKeyBindRegistered(integer key, table modifier_key_integers))"};

                if (!lua.is_integer())
                {
                    lua.throw_error(error_overload_not_found);
                }

                auto key_from_lua = lua.get_integer();
                if (key_from_lua < std::numeric_limits<uint8_t>::min() || key_from_lua > std::numeric_limits<uint8_t>::max())
                {
                    lua.throw_error("Parameter #1 for function 'IsKeyBindRegistered' must be an integer between 0 and 255");
                }
                auto key_to_check = static_cast<Input::Key>(key_from_lua);

                const auto mod = get_mod_ref(lua);

                if (lua.is_table())
                {
                    Input::Handler::ModifierKeyArray modifier_keys{};

                    uint8_t table_counter{};
                    lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
                        if (!table.value.is_integer())
                        {
                            lua.throw_error(
                                    "Lua function 'IsKeyBindRegistered', overload #2, requires a table of 1-byte large integers as the second parameter");
                        }

                        int64_t full_integer = table.value.get_integer();
                        if (full_integer < std::numeric_limits<uint8_t>::min() || full_integer > std::numeric_limits<uint8_t>::max())
                        {
                            lua.throw_error(
                                    "Lua function 'IsKeyBindRegistered', overload #2, requires a table of 1-byte large integers as the second parameter");
                        }

                        modifier_keys[table_counter++] = static_cast<Input::ModifierKey>(table.value.get_integer());

                        return false;
                    });

                    if (table_counter > 0)
                    {
                        lua.set_bool(mod->m_program.is_keydown_event_registered(key_to_check, modifier_keys));
                    }
                    else
                    {
                        lua.set_bool(mod->m_program.is_keydown_event_registered(key_to_check));
                    }
                }
                else
                {
                    lua.set_bool(mod->m_program.is_keydown_event_registered(key_to_check));
                }

                return 1;
            });

            lua.register_function("RegisterKeyBindAsync", [](const LuaMadeSimple::Lua& lua) -> int {
                std::string error_overload_not_found{R"(
No overload found for function 'RegisterKeyBindAsync'.
Overloads:
#1: RegisterKeyBindAsync(integer key)
#2: RegisterKeyBindAsync(integer key, table modifier_key_integers))"};

                Mod* mod = get_mod_ref(lua);

                if (!lua.is_integer())
                {
                    lua.throw_error(error_overload_not_found);
                }

                int64_t key_from_lua = lua.get_integer();
                if (key_from_lua < std::numeric_limits<uint8_t>::min() || key_from_lua > std::numeric_limits<uint8_t>::max())
                {
                    lua.throw_error("Parameter #1 for function 'RegisterKeyBindAsync' must be an integer between 0 and 255");
                }

                Input::Key key_to_register = static_cast<Input::Key>(key_from_lua);

                const auto lua_keybind_callback_lambda = [](const LuaMadeSimple::Lua& lua, const int callback_register_index) -> void {
                    try
                    {
                        lua.registry().get_function_ref(callback_register_index);
                        lua.call_function(0, 0);
                    }
                    catch (std::runtime_error& e)
                    {
                        Output::send<LogLevel::Error>(STR("{}\n"), ensure_str(lua.handle_error(e.what())));
                    }
                };

                if (lua.is_function())
                {
                    // Overload #1
                    // P1: Key to register
                    // P2: Callback

                    // Duplicate the Lua function to the top of the stack for luaL_ref
                    lua_pushvalue(lua.get_lua_state(), 1);

                    // Take a reference to the Lua function (it also pops it of the stack)
                    const int32_t lua_callback_registry_index = lua.registry().make_ref();

                    // Taking 'lua_callback_registry_index' by copy here to ensure its survival
                    // Using a 'custom_data' of 1 to signify that this keydown event was created by a mod
                    mod->m_program.register_keydown_event(
                            key_to_register,
                            [&lua, lua_callback_registry_index, &lua_keybind_callback_lambda]() {
                                lua_keybind_callback_lambda(lua, lua_callback_registry_index);
                            },
                            1, mod);
                }
                else if (lua.is_table())
                {
                    // Overload #2
                    // P1: Key to register
                    // P2: Table of modifier keys
                    // P3: Callback

                    Input::Handler::ModifierKeyArray modifier_keys{};

                    uint8_t table_counter{};
                    lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
                        if (!table.value.is_integer())
                        {
                            lua.throw_error(
                                    "Lua function 'RegisterKeyBindAsync', overload #2, requires a table of 1-byte large integers as the second parameter");
                        }

                        int64_t full_integer = table.value.get_integer();
                        if (full_integer < std::numeric_limits<uint8_t>::min() || full_integer > std::numeric_limits<uint8_t>::max())
                        {
                            lua.throw_error(
                                    "Lua function 'RegisterKeyBindAsync', overload #2, requires a table of 1-byte large integers as the second parameter");
                        }

                        modifier_keys[table_counter++] = static_cast<Input::ModifierKey>(table.value.get_integer());

                        return false;
                    });

                    // Duplicate the Lua function to the top of the stack for luaL_ref
                    lua_pushvalue(lua.get_lua_state(), 1);

                    // Take a reference to the Lua function (it also pops it of the stack)
                    const auto lua_callback_registry_index = lua.registry().make_ref();

                    if (table_counter > 0)
                    {
                        mod->m_program.register_keydown_event(
                                key_to_register,
                                modifier_keys,
                                [&lua, lua_callback_registry_index, &lua_keybind_callback_lambda]() {
                                    lua_keybind_callback_lambda(lua, lua_callback_registry_index);
                                },
                                1, mod);
                    }
                    else
                    {
                        mod->m_program.register_keydown_event(
                                key_to_register,
                                [&lua, lua_callback_registry_index, &lua_keybind_callback_lambda]() {
                                    lua_keybind_callback_lambda(lua, lua_callback_registry_index);
                                },
                                1, mod);
                    }
                }
                else
                {
                    lua.throw_error(error_overload_not_found);
                }

                return 0;
            });

            lua.register_function("RegisterKeyBind", [](const LuaMadeSimple::Lua& lua) -> int {
                std::string error_overload_not_found{R"(
No overload found for function 'RegisterKeyBind'.
Overloads:
#1: RegisterKeyBind(integer key)
#2: RegisterKeyBind(integer key, table modifier_key_integers))"};

                Mod* mod = get_mod_ref(lua);

                if (!lua.is_integer())
                {
                    lua.throw_error(error_overload_not_found);
                }

                int64_t key_from_lua = lua.get_integer();
                if (key_from_lua < std::numeric_limits<uint8_t>::min() || key_from_lua > std::numeric_limits<uint8_t>::max())
                {
                    lua.throw_error("Parameter #1 for function 'RegisterKeyBind' must be an integer between 0 and 255");
                }

                Input::Key key_to_register = static_cast<Input::Key>(key_from_lua);

                const auto lua_keybind_callback_lambda = [](const LuaMadeSimple::Lua& lua, const int callback_register_index) -> void {
                    std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                    try
                    {
                        lua.registry().get_function_ref(callback_register_index);
                        lua.call_function(0, 0);
                    }
                    catch (std::runtime_error& e)
                    {
                        Output::send<LogLevel::Error>(STR("{}\n"), ensure_str(lua.handle_error(e.what())));
                    }
                };

                if (lua.is_function())
                {
                    // Overload #1
                    // P1: Key to register
                    // P2: Callback

                    // Duplicate the Lua function to the top of the stack for luaL_ref
                    lua_pushvalue(lua.get_lua_state(), 1);

                    // Take a reference to the Lua function (it also pops it of the stack)
                    const int32_t lua_callback_registry_index = lua.registry().make_ref();

                    // Taking 'lua_callback_registry_index' by copy here to ensure its survival
                    // Using a 'custom_data' of 1 to signify that this keydown event was created by a mod
                    mod->m_program.register_keydown_event(
                            key_to_register,
                            [&lua, lua_callback_registry_index, &lua_keybind_callback_lambda]() {
                                lua_keybind_callback_lambda(lua, lua_callback_registry_index);
                            },
                            1, mod);
                }
                else if (lua.is_table())
                {
                    // Overload #2
                    // P1: Key to register
                    // P2: Table of modifier keys
                    // P3: Callback

                    Input::Handler::ModifierKeyArray modifier_keys{};

                    uint8_t table_counter{};
                    lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
                        if (!table.value.is_integer())
                        {
                            lua.throw_error("Lua function 'RegisterKeyBind', overload #2, requires a table of 1-byte large integers as the second parameter");
                        }

                        int64_t full_integer = table.value.get_integer();
                        if (full_integer < std::numeric_limits<uint8_t>::min() || full_integer > std::numeric_limits<uint8_t>::max())
                        {
                            lua.throw_error("Lua function 'RegisterKeyBind', overload #2, requires a table of 1-byte large integers as the second parameter");
                        }

                        modifier_keys[table_counter++] = static_cast<Input::ModifierKey>(table.value.get_integer());

                        return false;
                    });

                    // Duplicate the Lua function to the top of the stack for luaL_ref
                    lua_pushvalue(lua.get_lua_state(), 1);

                    // Take a reference to the Lua function (it also pops it of the stack)
                    const auto lua_callback_registry_index = lua.registry().make_ref();

                    if (table_counter > 0)
                    {
                        mod->m_program.register_keydown_event(
                                key_to_register,
                                modifier_keys,
                                [&lua, lua_callback_registry_index, &lua_keybind_callback_lambda]() {
                                    lua_keybind_callback_lambda(lua, lua_callback_registry_index);
                                },
                                1, mod);
                    }
                    else
                    {
                        mod->m_program.register_keydown_event(
                                key_to_register,
                                [&lua, lua_callback_registry_index, &lua_keybind_callback_lambda]() {
                                    lua_keybind_callback_lambda(lua, lua_callback_registry_index);
                                },
                                1, mod);
                    }
                }
                else
                {
                    lua.throw_error(error_overload_not_found);
                }

                return 0;
            });

            lua.register_function("UnregisterHook", [](const LuaMadeSimple::Lua& lua) -> int {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};

                std::string error_overload_not_found{R"(
No overload found for function 'UnregisterHook'.
Overloads:
#1: UnregisterHook(string UFunction_Name, integer PreCallbackId, integer PostCallbackId))"};

                if (!lua.is_string())
                {
                    lua.throw_error(error_overload_not_found);
                }

                auto function_name_no_prefix = get_function_name_without_prefix(ensure_str(lua.get_string()));

                Unreal::UFunction* unreal_function = Unreal::UObjectGlobals::StaticFindObject<Unreal::UFunction*>(nullptr, nullptr, function_name_no_prefix);
                if (!unreal_function)
                {
                    lua.throw_error(std::format("Tried to unregister a hook with Lua function 'UnregisterHook' but no UFunction with the specified name "
                                                "was found.\n>FunctionName: {}",
                                                to_string(function_name_no_prefix)));
                }

                if (!lua.is_integer())
                {
                    lua.throw_error(error_overload_not_found);
                }
                const auto pre_id = lua.get_integer();

                if (!lua.is_integer())
                {
                    lua.throw_error(error_overload_not_found);
                }
                const auto post_id = lua.get_integer();

                if (pre_id > std::numeric_limits<int32_t>::max())
                {
                    lua.throw_error(std::format("Tried to unregister a hook with Lua function 'UnregisterHook' but the PreCallbackId supplied was too "
                                                "large (>int32)\n>FunctionName: {}",
                                                to_string(function_name_no_prefix)));
                }

                if (post_id > std::numeric_limits<int32_t>::max())
                {
                    lua.throw_error(std::format("Tried to unregister a hook with Lua function 'UnregisterHook' but the PostCallbackId supplied was too "
                                                "large (>int32)\n>FunctionName: {}",
                                                to_string(function_name_no_prefix)));
                }


                auto func_ptr = unreal_function->GetFunc();
                if (func_ptr && func_ptr != Unreal::UObject::ProcessInternalInternal.get_function_address() &&
                    unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))
                {
                    const auto hook_data = std::ranges::find_if(g_hooked_script_function_data, [&](const std::unique_ptr<LuaUnrealScriptFunctionData>& elem) {
                        return elem->post_callback_id == post_id && elem->pre_callback_id == pre_id;
                    });
                    if (hook_data != g_hooked_script_function_data.end())
                    {
                        hook_data->get()->scheduled_for_removal = true;
                    }
                }
                else if (func_ptr && func_ptr == Unreal::UObject::ProcessInternalInternal.get_function_address() &&
                         !unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))
                {
                    if (auto data_ptr = LuaMod::find_function_hook_data(LuaMod::m_script_hook_callbacks, unreal_function); data_ptr)
                    {
                        Output::send<LogLevel::Verbose>(STR("Unregistering script hook with id: {}, FunctionName: {}\n"), post_id, function_name_no_prefix);
                        auto& registry_indexes = data_ptr->callback_data.registry_indexes;
                        for (auto& registry_index : registry_indexes)
                        {
                            if (post_id == registry_index.second.identifier)
                            {
                                registry_index.second.lua_index = -1;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    std::string error_message{"Was unable to unregister a hook with Lua function 'UnregisterHook', information:\n"};
                    error_message.append(fmt::format("FunctionName: {}\n", to_string(function_name_no_prefix)));
                    error_message.append(fmt::format("UFunction::Func: {}\n", std::bit_cast<void*>(func_ptr)));
                    error_message.append(fmt::format("ProcessInternal: {}\n", Unreal::UObject::ProcessInternalInternal.get_function_address()));
                    error_message.append(
                            fmt::format("FUNC_Native: {}\n", static_cast<uint32_t>(unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))));
                    lua.throw_error(error_message);
                }

                return 0;
            });

            lua.register_function("DumpAllObjects", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                const Mod* mod = get_mod_ref(lua);
                if (!mod)
                {
                    lua.throw_error("Couldn't dump objects and properties because the pointer to 'Mod' was nullptr");
                }
                UE4SSProgram::dump_all_objects_and_properties(mod->m_program.get_object_dumper_output_directory() + STR("\\") +
                                                              UE4SSProgram::m_object_dumper_file_name);
                return 0;
            });

            lua.register_function("GenerateSDK", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                const Mod* mod = get_mod_ref(lua);
                if (!mod)
                {
                    lua.throw_error("Couldn't generate SDK because the pointer to 'Mod' was nullptr");
                }
                File::StringType working_dir{mod->m_program.get_working_directory()};
                mod->m_program.generate_cxx_headers(working_dir + STR("\\CXXHeaderDump"));
                return 0;
            });

            lua.register_function("GenerateLuaTypes", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                const Mod* mod = get_mod_ref(lua);
                if (!mod)
                {
                    lua.throw_error("Couldn't generate lua types because the pointer to 'Mod' was nullptr");
                }
                File::StringType working_dir{mod->m_program.get_working_directory()};
                UE4SSProgram::get_program().generate_lua_types(working_dir + STR("\\Mods\\shared\\types"));
                return 0;
            });

            lua.register_function("GenerateUHTCompatibleHeaders", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                const Mod* mod = get_mod_ref(lua);
                mod->m_program.generate_uht_compatible_headers();
                return 0;
            });

            lua.register_function("DumpStaticMeshes", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                GUI::Dumpers::call_generate_static_mesh_file();
                return 0;
            });

            lua.register_function("DumpAllActors", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                GUI::Dumpers::call_generate_all_actor_file();
                return 0;
            });

            lua.register_function("DumpUSMAP", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                OutTheShade::generate_usmap();
                return 0;
            });
        }

        lua.register_function("StaticConstructObject", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'StaticConstructObject'.
Overloads:
#1: StaticConstructObject(
                            UClass Class,
                            UObject Outer,
                            FName Name, #Optional
                            EObjectFlags Flags, #Optional
                            EInternalObjectFlags InternalSetFlags, #Optional
                            bool CopyTransientsFromClassDefaults, #Optional
                            bool AssumeTemplateIsArchetype, #Optional
                            UObject Template, #Optional
                            FObjectInstancingGraph InstanceGraph, #Optional
                            UPackage ExternalPackage, #Optional
                            void SubobjectOverrides #Optional

))"};

            // For now, we're assuming that if there's userdata, that userdata is of the correct underlying type
            if (!lua.is_userdata())
            {
                lua.throw_error(error_overload_not_found);
            }
            Unreal::UClass* param_class = lua.get_userdata<LuaType::UClass>().get_remote_cpp_object();

            if (!lua.is_userdata())
            {
                lua.throw_error(error_overload_not_found);
            }
            Unreal::UObject* param_outer = lua.get_userdata<LuaType::UObject>().get_remote_cpp_object();

            Unreal::FName param_name;
            if (lua.is_userdata())
            {
                param_name = lua.get_userdata<LuaType::FName>().get_local_cpp_object();
            }
            else if (lua.is_integer())
            {
                param_name = Unreal::FName(lua.get_integer());
            }
            else
            {
                param_name = Unreal::FName(static_cast<int64_t>(0));
            }

            Unreal::EObjectFlags param_set_flags{};
            if (lua.is_integer())
            {
                param_set_flags = static_cast<Unreal::EObjectFlags>(lua.get_integer());
            }

            Unreal::EInternalObjectFlags param_internal_set_flags{};
            if (lua.is_integer())
            {
                param_internal_set_flags = static_cast<Unreal::EInternalObjectFlags>(lua.get_integer());
            }

            // The rest are all optional parameters
            bool param_copy_transients_from_class_defaults{};
            if (lua.is_bool())
            {
                param_copy_transients_from_class_defaults = lua.get_bool();
            }

            bool param_assume_template_is_archetype{};
            if (lua.is_bool())
            {
                param_assume_template_is_archetype = lua.get_bool();
            }

            Unreal::UObject* param_template{};
            if (lua.is_userdata())
            {
                param_template = lua.get_userdata<LuaType::UObject>().get_remote_cpp_object();
            }

            // Change this to userdata if support for 'FObjectInstancingGraph' is ever added
            void* param_instance_graph{};
            if (lua.is_integer())
            {
                param_instance_graph = reinterpret_cast<void*>(static_cast<uintptr_t>(lua.get_integer()));
            }

            // Change this to userdata if support for 'UPackage' is ever added
            void* param_external_package{};
            if (lua.is_integer())
            {
                param_external_package = reinterpret_cast<void*>(static_cast<uintptr_t>(lua.get_integer()));
            }

            // void* param_subobject_overrides{};
            // if (lua.is_integer()) { param_subobject_overrides = reinterpret_cast<void*>(static_cast<uintptr_t>(lua.get_integer())); }

            Unreal::FStaticConstructObjectParameters params{param_class, param_outer};
            params.Name = param_name;
            params.SetFlags = param_set_flags;
            params.InternalSetFlags = param_internal_set_flags;
            params.bCopyTransientsFromClassDefaults = param_copy_transients_from_class_defaults;
            params.bAssumeTemplateIsArchetype = param_assume_template_is_archetype;
            params.Template = param_template;
            params.InstanceGraph = static_cast<struct RC::Unreal::FObjectInstancingGraph*>(param_instance_graph);
            params.ExternalPackage = static_cast<class RC::Unreal::UPackage*>(param_external_package);
            Unreal::UObject* created_object = Unreal::UObjectGlobals::StaticConstructObject(params);

            LuaType::UObject::construct(lua, created_object);

            return 1;
        });

        lua.register_function("RegisterCustomProperty", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterCustomProperty'.
Overloads:
#1: RegisterCustomProperty(table PropertyInfo))"};

            if (!lua.is_table())
            {
                lua.throw_error(error_overload_not_found);
            }

            struct PropertyTypeInfo
            {
                std::string_view name{};
                int32_t size{-1};
                void* ffieldclass_pointer{};
                void* static_pointer{};

                auto is_valid() -> bool
                {
                    if (size < 0)
                    {
                        return false;
                    }
                    if (!ffieldclass_pointer)
                    {
                        return false;
                    }
                    // if (!static_pointer) { return false; }

                    return true;
                }
            };

            struct PropertyInfo
            {
                StringType name{};
                PropertyTypeInfo type{}; // Figure out what to do here, it shouldn't be just a string
                StringType belongs_to_class{};
                int32_t offset_internal{-1};
                int32_t element_size{-1}; // Is this required for trivial types like integers and floats ?

                // ArrayProperty
                PropertyTypeInfo array_inner{};

                bool offset_internal_is_table{};

                // Only one of these booleans can be true
                bool is_array_property{};

                auto set_is_array_property() -> void
                {
                    // Check here if any incompatible booleans have been set, and throw error if so
                    is_array_property = true;
                }

                auto is_missing_values() -> bool
                {
                    if (name.empty())
                    {
                        return true;
                    }
                    if (!type.is_valid())
                    {
                        return true;
                    }
                    if (belongs_to_class.empty())
                    {
                        return true;
                    }
                    if (!offset_internal_is_table && offset_internal < 0)
                    {
                        return true;
                    }
                    // if (element_size < 0) { return true; }

                    if (is_array_property && !array_inner.is_valid())
                    {
                        return true;
                    }

                    return false;
                }
            };

            PropertyInfo property_info{};

            auto lua_table = lua.get_table();

            auto verify_and_convert_int64_to_int32 = [&](std::string_view field_name,
                                                         std::string_view second_field_name = {},
                                                         std::string_view third_field_name = {},
                                                         bool* has_error = nullptr) -> int32_t {
                int64_t integer;

                if (second_field_name.empty())
                {
                    // Ignore the third field name if the second one isn't set
                    integer = lua_table.get_int_field(field_name, has_error);
                }
                else if (third_field_name.empty())
                {
                    // If the second field name is set but the third isn't, then we have two layers to the table
                    integer = lua_table.get_table_field(field_name, has_error).get_int_field(second_field_name, has_error);
                }
                else
                {
                    // If both the second field name and the third field name is set, then we have three layers to the table
                    integer = lua_table.get_table_field(field_name, has_error).get_table_field(second_field_name, has_error).get_int_field(third_field_name, has_error);
                }

                if (integer < std::numeric_limits<int32_t>::min() || integer > std::numeric_limits<int32_t>::max())
                {
                    std::string error_field_names;

                    if (second_field_name.empty())
                    {
                        error_field_names = fmt::format("{}", field_name);
                    }
                    else if (third_field_name.empty())
                    {
                        error_field_names = fmt::format("{}.{}", field_name, second_field_name);
                    }
                    else
                    {
                        error_field_names = fmt::format("{}.{}.{}", field_name, second_field_name, third_field_name);
                    }

                    lua.throw_error(fmt::format(
                            "Parameter #1 for function 'RegisterCustomProperty'. The table value for key '{}' is outside the range of a 32-bit integer",
                            error_field_names));
                }

                return static_cast<int32_t>(integer);
            };

            // Always required, for all property types
            property_info.name = ensure_str(lua_table.get_string_field("Name"));
            property_info.type.name = lua_table.get_table_field("Type").get_string_field("Name");
            property_info.type.size = verify_and_convert_int64_to_int32("Type", "Size");
            property_info.type.ffieldclass_pointer = reinterpret_cast<void*>(lua_table.get_table_field("Type").get_int_field("FFieldClassPointer"));
            property_info.type.static_pointer = reinterpret_cast<void*>(lua_table.get_table_field("Type").get_int_field("StaticPointer"));
            property_info.belongs_to_class = ensure_str(lua_table.get_string_field("BelongsToClass"));

            std::string oi_property_name;
            int32_t oi_relative_offset{};

            bool error_while_getting_offset_internal{};
            property_info.offset_internal = verify_and_convert_int64_to_int32("OffsetInternal", "", "", &error_while_getting_offset_internal);

            if (error_while_getting_offset_internal)
            {
                // Failed to get integer from table
                // This means that we may have a table instead of an integer

                oi_property_name = lua_table.get_table_field("OffsetInternal").get_string_field("Property");
                oi_relative_offset = verify_and_convert_int64_to_int32("OffsetInternal", "RelativeOffset");

                property_info.offset_internal_is_table = true;
            }

            // Only required for ArrayProperty
            if (property_info.type.name == "ArrayProperty")
            {
                if (!lua_table.does_field_exist("ArrayProperty"))
                {
                    lua.throw_error("Parameter #1 for function 'RegisterCustomProperty'. The table entry 'ArrayProperty' is missing.");
                }
                else
                {
                    property_info.set_is_array_property();
                    property_info.array_inner.name = lua_table.get_table_field("ArrayProperty").get_table_field("Type").get_string_field("Name");
                    property_info.array_inner.size = verify_and_convert_int64_to_int32("ArrayProperty", "Type", "Size");
                    property_info.array_inner.ffieldclass_pointer =
                            reinterpret_cast<void*>(lua_table.get_table_field("ArrayProperty").get_table_field("Type").get_int_field("FFieldClassPointer"));
                    property_info.array_inner.static_pointer =
                            reinterpret_cast<void*>(lua_table.get_table_field("ArrayProperty").get_table_field("Type").get_int_field("StaticPointer"));
                }
            }

            if (property_info.is_missing_values())
            {
                lua.throw_error("Parameter #1 for function 'RegisterCustomProperty'. The table is missing required fields.");
            }

            Unreal::UClass* belongs_to_class = Unreal::UObjectGlobals::StaticFindObject<Unreal::UClass*>(nullptr, nullptr, property_info.belongs_to_class);
            if (!belongs_to_class)
            {
                lua.throw_error("Tried to 'RegisterCustomProperty' but 'BelongsToClass' could not be found");
            }

            if (property_info.offset_internal_is_table)
            {
                auto name = Unreal::FName(ensure_str(oi_property_name));
                Unreal::FProperty* oi_property = belongs_to_class->FindProperty(name);
                if (!oi_property)
                {
                    lua.throw_error(fmt::format("Was unable to find property '{}' in class '{}' for use for relative Offset_Internal",
                                                oi_property_name,
                                                to_string(property_info.belongs_to_class)));
                }

                property_info.offset_internal = oi_property->GetOffset_Internal() + oi_relative_offset;
            }

            if (property_info.type.size == 0)
            {
                lua.throw_error(fmt::format("The size for property '{}' was unknown. Custom sizes are unsupported but will likely be supported in the future.",
                                            property_info.type.name));
            }

            if (property_info.is_array_property && property_info.array_inner.size == 0)
            {
                lua.throw_error(
                        fmt::format("The size for inner property '{}' was unknown. Custom sizes are unsupported but will likely be supported in the future.",
                                    property_info.array_inner.name));
            }

            LuaType::LuaCustomProperty::StaticStorage::property_list.add(
                    property_info.name,
                    Unreal::CustomArrayProperty::construct(property_info.offset_internal,
                                                           belongs_to_class,
                                                           static_cast<Unreal::UClass*>(property_info.type.ffieldclass_pointer),
                                                           static_cast<Unreal::FProperty*>(property_info.array_inner.ffieldclass_pointer),
                                                           property_info.is_array_property ? property_info.array_inner.size : property_info.type.size

                                                           ));

            printf_s("Registered Custom Property\n");
            printf_s("PropertyInfo {\n");
            printf_s("\tName: %S\n", FromCharTypePtr<wchar_t>(property_info.name.c_str()));
            printf_s("\tType {\n");
            printf_s("\t\tName: %s\n", property_info.type.name.data());
            printf_s("\t\tSize: 0x%X\n", property_info.type.size);
            printf_s("\t\tFFieldClassPointer: 0x%p\n", property_info.type.ffieldclass_pointer);
            printf_s("\t\tStaticPointer: 0x%p\n", property_info.type.static_pointer);
            printf_s("\t}\n");
            printf_s("\tBelongsToClass: %S\n", FromCharTypePtr<wchar_t>(property_info.belongs_to_class.c_str()));
            printf_s("\tOffsetInternal: 0x%X\n", property_info.offset_internal);

            if (property_info.is_array_property)
            {
                printf_s("\tArrayProperty {\n");
                printf_s("\t\tType {\n");
                printf_s("\t\t\tName: %s\n", property_info.array_inner.name.data());
                printf_s("\t\t\tSize: 0x%X\n", property_info.array_inner.size);
                printf_s("\t\t\tFFieldClassPointer: %p\n", property_info.array_inner.ffieldclass_pointer);
                printf_s("\t\t\tStaticPointer: %p\n", property_info.array_inner.static_pointer);
                printf_s("\t\t}\n");
                printf_s("\t}\n");
            }

            printf_s("}\n");

            return 0;
        });

        lua.register_function("ForEachUObject", [](const LuaMadeSimple::Lua& lua) -> int {
            Unreal::UObjectGlobals::ForEachUObject([&](void* object, int32_t chunk_index, int32_t object_index) {
                // Duplicate the Lua function so that we can use it in subsequent iterations of this loop (call_function pops the function from the stack)
                lua_pushvalue(lua.get_lua_state(), 1);

                // Set the 'Object' parameter for the Lua function (P1)
                LuaType::auto_construct_object(lua, static_cast<Unreal::UObject*>(object));

                // Set the 'ChunkIndex' parameter for the Lua function (P2)
                lua.set_integer(chunk_index);

                // Set the 'ObjectIndex' parameter for the Lua function (P3)
                lua.set_integer(object_index);

                lua.call_function(3, 1);

                return LoopAction::Continue;
            });
            return 0;
        });

        lua.register_function("NotifyOnNewObject", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'NotifyOnNewObject'.
Overloads:
#1: NotifyOnNewObject(string UClassName, LuaFunction Callback))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto class_name = ensure_str(lua.get_string());

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto [hook_lua, thread_ref] = make_hook_state(mod);

            // Duplicate the Lua function to the top of the stack for lua_xmove and luaL_ref
            lua_pushvalue(lua.get_lua_state(), 1);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            const auto func_ref = hook_lua->registry().make_ref();

            Unreal::UClass* instance_of_class = Unreal::UObjectGlobals::StaticFindObject<Unreal::UClass*>(nullptr, nullptr, class_name);

            LuaMod::m_static_construct_object_lua_callbacks.emplace_back(LuaMod::LuaCancellableCallbackData{hook_lua, instance_of_class, func_ref, thread_ref});

            return 0;
        });

        lua.register_function("RegisterCustomEvent", [](const LuaMadeSimple::Lua& lua) -> int {
            std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterCustomEvent'.
Overloads:
#1: RegisterCustomEvent(string EventName, LuaFunction Callback))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto event_name = ensure_str(lua.get_string());

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();
            if (!LuaMod::find_function_hook_data(LuaMod::m_custom_event_callbacks, Unreal::FName(event_name, Unreal::FNAME_Add)))
            {
                LuaMod::m_custom_event_callbacks.emplace_back(LuaMod::FunctionHookData{
                        {Unreal::FName(event_name, Unreal::FNAME_Add)},
                        LuaMod::LuaCallbackData{
                                .lua = &lua,
                                .instance_of_class = nullptr,
                                .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, {lua_callback_registry_index}}},
                        }});
            }

            return 0;
        });

        lua.register_function("UnregisterCustomEvent", [](const LuaMadeSimple::Lua& lua) -> int {
            std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
            std::string error_overload_not_found{R"(
No overload found for function 'UnregisterCustomEvent'.
Overloads:
#1: UnregisterCustomEvent(string EventName))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }
            auto custom_event_name = ensure_str(lua.get_string());

            LuaMod::remove_function_hook_data(LuaMod::m_custom_event_callbacks, custom_event_name);

            return 0;
        });

        lua.register_function("RegisterLoadMapPreHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterLoadMapPreHook'.
Overloads:
#1: RegisterLoadMapPreHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the lua function (it also pops it off the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_load_map_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = &lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}}});

            return 0;
        });

        lua.register_function("RegisterLoadMapPostHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterLoadMapPostHook'.
Overloads:
#1: RegisterLoadMapPostHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the lua function (it also pops it off the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_load_map_post_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = &lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}}});

            return 0;
        });

        lua.register_function("RegisterInitGameStatePreHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterInitGameStatePreHook'.
Overloads:
#1: RegisterInitGameStatePreHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_init_game_state_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = &lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}},
            });

            return 0;
        });

        lua.register_function("RegisterInitGameStatePostHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterInitGameStatePostHook'.
Overloads:
#1: RegisterInitGameStatePostHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_init_game_state_post_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = &lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}},
            });

            return 0;
        });

        lua.register_function("RegisterBeginPlayPreHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterBeginPlayPreHook'.
Overloads:
#1: RegisterBeginPlayPreHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_begin_play_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = &lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}},
            });

            return 0;
        });

        lua.register_function("RegisterBeginPlayPostHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterBeginPlayPostHook'.
Overloads:
#1: RegisterBeginPlayPostHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_begin_play_post_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = &lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}},
            });

            return 0;
        });

        lua.register_function("RegisterEndPlayPreHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterEndPlayPreHook'.
Overloads:
#1: RegisterEndPlayPreHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_end_play_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = &lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}},
            });

            return 0;
        });

        lua.register_function("RegisterEndPlayPostHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterEndPlayPostHook'.
Overloads:
#1: RegisterEndPlayPostHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_end_play_post_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = &lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}},
            });

            return 0;
        });

        lua.register_function("IterateGameDirectories", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IterateGameDirectories'.
Overloads:
#1: IterateGameDirectories())"};

            std::filesystem::path game_executable_directory = UE4SSProgram::get_program().get_game_executable_directory();
            auto game_content_dir = game_executable_directory.parent_path().parent_path() / "Content";
            if (!std::filesystem::exists(game_content_dir))
            {
                Output::send<LogLevel::Warning>(STR("IterateGameDirectories: Could not locate the root directory because the directory structure is unknown "
                                                    "(not <RootGamePath>/Game/Binaries/Win64)\n"));
                lua.set_nil();
                return 1;
            }

            auto game_name = game_executable_directory.parent_path().parent_path().filename();
            auto game_root_directory = game_executable_directory.parent_path().parent_path().parent_path();
            auto directories_table = lua.prepare_new_table();

            std::function<void(const std::filesystem::path&, LuaMadeSimple::Lua::Table&)> iterate_directory =
                    [&](const std::filesystem::path& directory, LuaMadeSimple::Lua::Table& current_directory_table) {
                        try
                        {
                            std::error_code ec;
                            for (const auto& item : std::filesystem::directory_iterator(directory, ec))
                            {
                                try
                                {
                                    if (!item.is_directory())
                                    {
                                        continue;
                                    }

                                    auto path = item.path().filename();

                                    // Set key to "Game" if this is the game directory, otherwise use the actual name
                                    std::string table_key;
                                    if (path == game_name)
                                    {
                                        table_key = "Game";
                                    }
                                    else
                                    {
                                        // TODO: When UE5 String conversion is implemented, replace with StringCast<ANSICHAR>
                                        table_key = to_utf8_string(path);
                                    }

                                    current_directory_table.add_key(table_key.c_str());
                                    auto next_directory_table = lua.prepare_new_table();

                                    // Recursively iterate the subdirectory
                                    iterate_directory(item.path(), next_directory_table);
                                    current_directory_table.fuse_pair();
                                }
                                catch (const std::exception& e)
                                {
                                    Output::send<LogLevel::Error>(STR("Error processing directory entry: {}\n"), to_wstring(e.what()));
                                }
                            }

                            if (ec)
                            {
                                Output::send<LogLevel::Error>(STR("Error iterating directory {}: {}\n"), directory.wstring(), to_wstring(ec.message()));
                            }

                            auto meta_table = lua.prepare_new_table();

                            lua_pushcfunction(lua.get_lua_state(), [](lua_State* lua_state) -> int {
                                return TRY([&] {
                                    const auto& lua = LuaMadeSimple::Lua(lua_state);
                                    std::string name{};
                                    if (!lua.is_string(2))
                                    {
                                        return 0;
                                    }
                                    name = lua.get_string(2);

                                    if (name == "__name")
                                    {
                                        lua_getmetatable(lua_state, 1);
                                        lua_pushliteral(lua_state, "__name");
                                        lua_rawget(lua_state, 2);
                                        lua.discard_value();
                                        lua.discard_value();
                                        if (!lua.is_string())
                                        {
                                            throw std::runtime_error{"Couldn't find '__name' for directory entry."};
                                        }
                                        return 1;
                                    }
                                    else if (name == "__absolute_path")
                                    {
                                        lua_getmetatable(lua_state, 1);
                                        lua_pushliteral(lua_state, "__absolute_path");
                                        lua_rawget(lua_state, 2);
                                        lua.discard_value();
                                        lua.discard_value();
                                        if (!lua.is_string())
                                        {
                                            throw std::runtime_error{"Couldn't find '__absolute_path' for directory entry."};
                                        }
                                        return 1;
                                    }
                                    else if (name == "__files")
                                    {
                                        lua_getmetatable(lua_state, 1);
                                        lua_pushliteral(lua_state, "__absolute_path");
                                        lua_rawget(lua_state, 2);
                                        lua.discard_value();
                                        lua.discard_value();
                                        if (!lua.is_string())
                                        {
                                            throw std::runtime_error{"Couldn't find '__absolute_path' for directory entry."};
                                        }

                                        const auto path_str = lua.get_string();
                                        std::wstring path_wstr;

                                        // Try to convert the path string to wstring for filesystem operations
                                        try
                                        {
                                            path_wstr = RC::to_wstring(path_str);
                                        }
                                        catch (const std::exception&)
                                        {
                                            // If conversion fails, reconstruct path as basic conversion
                                            path_wstr.reserve(path_str.size());
                                            for (char c : path_str)
                                            {
                                                path_wstr.push_back(static_cast<wchar_t>(c));
                                            }

                                            // Check if the path exists first
                                            std::error_code path_ec;
                                            if (!std::filesystem::exists(path_wstr, path_ec))
                                            {
                                                // Path doesn't exist, try to reconstruct it
                                                if (path_str.find("LogicMods") != std::string::npos)
                                                {
                                                    // Get the game executable directory
                                                    std::filesystem::path game_exec_dir = UE4SSProgram::get_program().get_game_executable_directory();

                                                    // Navigate to content directory
                                                    std::filesystem::path content_dir = game_exec_dir;
                                                    content_dir = content_dir.parent_path(); // Up to Binaries
                                                    content_dir = content_dir.parent_path(); // Up to Game
                                                    content_dir /= "Content";

                                                    if (std::filesystem::exists(content_dir))
                                                    {
                                                        auto logic_mods_dir = content_dir / "Paks/LogicMods";

                                                        // Check if it exists or try to create it
                                                        if (!std::filesystem::exists(logic_mods_dir))
                                                        {
                                                            std::error_code dir_ec;
                                                            auto paks_dir = content_dir / "Paks";
                                                            if (!std::filesystem::exists(paks_dir))
                                                            {
                                                                std::filesystem::create_directory(paks_dir, dir_ec);
                                                            }
                                                            std::filesystem::create_directory(logic_mods_dir, dir_ec);
                                                        }

                                                        if (std::filesystem::exists(logic_mods_dir))
                                                        {
                                                            path_wstr = logic_mods_dir.wstring();
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        auto files_table = lua.prepare_new_table();
                                        auto index = 1;

                                        try
                                        {
                                            std::error_code ec;
                                            if (std::filesystem::exists(path_wstr, ec))
                                            {
                                                for (const auto& item : std::filesystem::directory_iterator(path_wstr, ec))
                                                {
                                                    try
                                                    {
                                                        if (!item.is_directory())
                                                        {
                                                            files_table.add_key(index);
                                                            auto file_table = lua.prepare_new_table();

                                                            // Create safe strings for filenames and paths
                                                            // TODO: When UE5 String conversion is implemented, replace with StringCast<ANSICHAR>
                                                            std::string safe_filename = to_utf8_string(item.path().filename());
                                                            std::string safe_path = to_utf8_string(item.path());

                                                            file_table.add_pair("__name", safe_filename.c_str());
                                                            file_table.add_pair("__absolute_path", safe_path.c_str());
                                                            files_table.fuse_pair();
                                                        }
                                                        ++index;
                                                    }
                                                    catch (const std::exception& e)
                                                    {
                                                        Output::send<LogLevel::Error>(STR("Error processing file: {}\n"), to_wstring(e.what()));
                                                    }
                                                }
                                            }

                                            if (ec)
                                            {
                                                Output::send<LogLevel::Error>(STR("Error iterating files in {}: {}\n"), path_wstr, to_wstring(ec.message()));
                                            }
                                        }
                                        catch (const std::exception& e)
                                        {
                                            Output::send<LogLevel::Error>(STR("Error iterating files: {}\n"), to_wstring(e.what()));
                                        }

                                        return 1;
                                    }
                                    else
                                    {
                                        lua.set_nil();
                                        return 1;
                                    }
                                });
                            });

                            lua_setfield(lua.get_lua_state(), -2, "__index");

                            // Set metadata using safe string conversion
                            // TODO: When UE5 String conversion is implemented, replace with StringCast<ANSICHAR>
                            std::string safe_filename = to_utf8_string(directory.filename());
                            std::string safe_path = to_utf8_string(directory);

                            meta_table.add_pair("__name", safe_filename.c_str());
                            meta_table.add_pair("__absolute_path", safe_path.c_str());
                            lua_setmetatable(lua.get_lua_state(), -2);
                        }
                        catch (const std::exception& e)
                        {
                            Output::send<LogLevel::Error>(STR("Exception in iterate_directory: {}\n"), to_wstring(e.what()));
                        }
                    };

            try
            {
                iterate_directory(game_root_directory, directories_table);
            }
            catch (const std::exception& e)
            {
                Output::send<LogLevel::Error>(STR("Exception in IterateGameDirectories: {}\n"), to_wstring(e.what()));
                lua.set_nil();
                return 1;
            }

            return 1;
        });

        lua.register_function("CreateLogicModsDirectory", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'CreateLogicModsDirectory'.
Overloads:
#1: CreateLogicModsDirectory())"};
            try
            {
                std::filesystem::path game_executable_directory = UE4SSProgram::get_program().get_game_executable_directory();
                auto game_content_dir = game_executable_directory.parent_path().parent_path() / "Content";
                if (!std::filesystem::exists(game_content_dir))
                {
                    lua.throw_error("CreateLogicModsDirectory: Could not locate the \"Content\" directory because the directory structure is unknown (not "
                                    "<RootGamePath>/Game/Content)\n");
                }

                auto logic_mods_dir = game_content_dir / "Paks/LogicMods";

                std::error_code ec;
                if (std::filesystem::exists(logic_mods_dir, ec))
                {
                    Output::send<LogLevel::Warning>(
                            STR("CreateLogicModsDirectory: \"LogicMods\" directory already exists. Cancelling creation of new directory.\n"));
                    lua.set_bool(true);
                    return 1;
                }

                // Try to create the Paks directory first if it doesn't exist
                auto paks_dir = game_content_dir / "Paks";
                if (!std::filesystem::exists(paks_dir, ec))
                {
                    ec.clear();
                    bool paks_created = std::filesystem::create_directory(paks_dir, ec);
                    if (!paks_created || ec)
                    {
                        Output::send<LogLevel::Error>(STR("CreateLogicModsDirectory: Failed to create Paks directory: {}\n"), to_wstring(ec.message()));
                        // Try to continue anyway
                    }
                }

                // Now create the LogicMods directory
                ec.clear();
                bool created = std::filesystem::create_directory(logic_mods_dir, ec);

                if (!created || ec)
                {
                    Output::send<LogLevel::Error>(STR("CreateLogicModsDirectory: Error creating directory: {}\n"), to_wstring(ec.message()));

                    // Check if the directory exists despite the error (might happen with Unicode paths)
                    ec.clear();
                    if (std::filesystem::exists(logic_mods_dir, ec))
                    {
                        lua.set_bool(true);
                        return 1;
                    }

                    lua.throw_error("CreateLogicModsDirectory: Unable to create \"LogicMods\" directory. Try creating manually.\n");
                }

                Output::send<LogLevel::Warning>(STR("CreateLogicModsDirectory: LogicMods directory created.\n"));

                lua.set_bool(true);
                return 1;
            }
            catch (const std::exception& e)
            {
                Output::send<LogLevel::Error>(STR("Exception in CreateLogicModsDirectory: {}\n"), to_wstring(e.what()));
                lua.throw_error(e.what());
                return 0;
            }
        });

        lua.register_function("ExecuteAsync", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ExecuteAsync'.
Overloads:
#1: ExecuteAsync(LuaFunction Callback))"};

            auto mod = get_mod_ref(lua);

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            const int32_t lua_function_ref = lua.registry().make_ref();

            mod->actions_lock();
            mod->m_pending_actions.emplace_back(LuaMod::AsyncAction{lua_function_ref, LuaMod::ActionType::Immediate});
            mod->actions_unlock();

            return 0;
        });

        lua.register_function("ExecuteWithDelay", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ExecuteWithDelay'.
Overloads:
#1: ExecuteWithDelay(integer DelayInMilliseconds, LuaFunction Callback))"};

            if (!lua.is_integer())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            int64_t delay = lua.get_integer();

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            const int32_t lua_function_ref = lua.registry().make_ref();

            auto mod = get_mod_ref(lua);

            mod->actions_lock();
            mod->m_pending_actions.emplace_back(LuaMod::AsyncAction{
                    lua_function_ref,
                    LuaMod::ActionType::Delayed,
                    std::chrono::steady_clock::now(),
                    delay,
            });
            mod->actions_unlock();
            return 0;
        });

        lua.register_function("LoopAsync", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'LoopAsync'.
Overloads:
#1: LoopAsync(integer DelayInMilliseconds, LuaFunction Callback))"};

            if (!lua.is_integer())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            int64_t delay = lua.get_integer();

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            const int32_t lua_function_ref = lua.registry().make_ref();

            auto mod = get_mod_ref(lua);

            mod->actions_lock();
            mod->m_pending_actions.emplace_back(LuaMod::AsyncAction{
                    lua_function_ref,
                    LuaMod::ActionType::Loop,
                    std::chrono::steady_clock::now(),
                    delay,
            });
            mod->actions_unlock();

            return 0;
        });

        lua.register_function("RegisterProcessConsoleExecPreHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterProcessConsoleExecPreHook'.
Overloads:
#1: RegisterProcessConsoleExecPreHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);
            callback = &LuaMod::m_process_console_exec_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua->get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua->registry().make_ref();
            callback->registry_indexes.emplace_back(hook_lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("RegisterProcessConsoleExecPostHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterProcessConsoleExecPostHook'.
Overloads:
#1: RegisterProcessConsoleExecPostHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);
            callback = &LuaMod::m_process_console_exec_post_callbacks.emplace_back(LuaMod::LuaCallbackData{hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua->get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua->registry().make_ref();
            callback->registry_indexes.emplace_back(hook_lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("RegisterCallFunctionByNameWithArgumentsPreHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterCallFunctionByNameWithArgumentsPreHook'.
Overloads:
#1: RegisterCallFunctionByNameWithArgumentsPreHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);
            callback = &LuaMod::m_call_function_by_name_with_arguments_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua->get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua->registry().make_ref();
            callback->registry_indexes.emplace_back(hook_lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("RegisterCallFunctionByNameWithArgumentsPostHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterCallFunctionByNameWithArgumentsPostHook'.
Overloads:
#1: RegisterCallFunctionByNameWithArgumentsPostHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);
            callback = &LuaMod::m_call_function_by_name_with_arguments_post_callbacks.emplace_back(LuaMod::LuaCallbackData{hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua->get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua->registry().make_ref();
            callback->registry_indexes.emplace_back(hook_lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("RegisterULocalPlayerExecPreHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterULocalPlayerExecPreHook'.
Overloads:
#1: RegisterULocalPlayerExecPreHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);
            callback = &LuaMod::m_local_player_exec_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua->get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua->registry().make_ref();
            callback->registry_indexes.emplace_back(hook_lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("RegisterULocalPlayerExecPostHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterULocalPlayerExecPostHook'.
Overloads:
#1: RegisterULocalPlayerExecPostHook(LuaFunction Callback))"};

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto mod = get_mod_ref(lua);
            auto hook_lua = get_hook_lua(mod);
            callback = &LuaMod::m_local_player_exec_post_callbacks.emplace_back(LuaMod::LuaCallbackData{hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua->get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua->registry().make_ref();
            callback->registry_indexes.emplace_back(hook_lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("RegisterConsoleCommandGlobalHandler", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterConsoleCommandGlobalHandler'.
Overloads:
#1: RegisterConsoleCommandGlobalHandler(string CommandName, LuaFunction Callback))"};

            if (!lua.is_string())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            auto command_name = ensure_str(lua.get_string());

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto iter = LuaMod::m_global_command_lua_callbacks.find(command_name);
            if (iter == LuaMod::m_global_command_lua_callbacks.end())
            {
                auto mod = get_mod_ref(lua);
                auto hook_lua = get_hook_lua(mod);
                callback = &LuaMod::m_global_command_lua_callbacks.emplace(command_name, LuaMod::LuaCallbackData{hook_lua, nullptr, {}}).first->second;
            }
            else
            {
                callback = &iter->second;
            }
            lua_xmove(lua.get_lua_state(), callback->lua->get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua->registry().make_ref();
            callback->registry_indexes.emplace_back(&lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("RegisterConsoleCommandHandler", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterConsoleCommandHandler'.
Overloads:
#1: RegisterConsoleCommandHandler(string CommandName, LuaFunction Callback))"};

            if (!lua.is_string())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            auto command_name = ensure_str(lua.get_string());

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto iter = LuaMod::m_custom_command_lua_pre_callbacks.find(command_name);
            if (iter == LuaMod::m_custom_command_lua_pre_callbacks.end())
            {
                auto mod = get_mod_ref(lua);
                auto hook_lua = get_hook_lua(mod);
                callback = &LuaMod::m_custom_command_lua_pre_callbacks.emplace(command_name, LuaMod::LuaCallbackData{hook_lua, nullptr, {}}).first->second;
            }
            else
            {
                callback = &iter->second;
            }
            lua_xmove(lua.get_lua_state(), callback->lua->get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua->registry().make_ref();
            callback->registry_indexes.emplace_back(&lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("LoadAsset", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'LoadAsset'.
Overloads:
#1: LoadAsset(string AssetPathAndName))"};

            if (!Unreal::IsInGameThread())
            {
                throw std::runtime_error{"Function 'LoadAsset' can only be called from within the game thread"};
            }

            if (!lua.is_string())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            auto asset_path_and_name = Unreal::FName(ensure_str(lua.get_string()), Unreal::FNAME_Add);

            auto* asset_registry = static_cast<Unreal::UAssetRegistry*>(Unreal::UAssetRegistryHelpers::GetAssetRegistry().ObjectPointer);
            if (!asset_registry)
            {
                throw std::runtime_error{"Did not load assets because asset_registry was nullptr\n"};
            }

            Unreal::UObject* loaded_asset{};
            bool was_asset_found{};
            bool did_asset_load{};
            Unreal::FAssetData asset_data = asset_registry->GetAssetByObjectPath(asset_path_and_name);
            if ((Unreal::Version::IsAtMost(5, 0) && asset_data.ObjectPath().GetComparisonIndex()) || asset_data.PackageName().GetComparisonIndex())
            {
                was_asset_found = true;
                loaded_asset = Unreal::UAssetRegistryHelpers::GetAsset(asset_data);
                if (loaded_asset)
                {
                    did_asset_load = true;
                    Output::send(STR("Asset loaded\n"));
                }
                else
                {
                    Output::send(STR("Asset was found but not loaded, could be a package\n"));
                }
            }

            LuaType::auto_construct_object(lua, loaded_asset);
            lua.set_bool(was_asset_found);
            lua.set_bool(did_asset_load);
            return 3;
        });

        lua.register_function("FindObject", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'FindObject'.
Overloads:
#1: FindObject(UClass InClass, UObject|UClass InOuter, string Name, bool ExactClass)
#2: FindObject(string|FName|nil ClassName, string|FName|nil ObjectShortName, EObjectFlags RequiredFlags, EObjectFlags BannedFlags)
#3: FindObject(UClass|nil Class, string|FName|nil ObjectShortName, EObjectFlags RequiredFlags, EObjectFlags BannedFlags))"};

            if (!lua.is_string() && !lua.is_userdata() && !lua.is_nil())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            Unreal::FName object_class_name{};
            Unreal::UClass* in_class{};
            bool could_be_in_class{};
            if (lua.is_string())
            {
                object_class_name = Unreal::FName(ensure_str(lua.get_string()), Unreal::FNAME_Add);
            }
            else if (lua.is_userdata())
            {
                // The API is a bit awkward, we have to tell it to preserve the stack
                // That way, when we call 'get_userdata' again with a more specific type, there's still something to actually get
                auto& userdata = lua.get_userdata<LuaType::UE4SSBaseObject>(1, true);
                if (std::string_view{userdata.get_object_name()} == "UClass")
                {
                    in_class = lua.get_userdata<LuaType::UClass>().get_remote_cpp_object();
                    could_be_in_class = true;
                    object_class_name = in_class->GetNamePrivate();
                }
                else if (std::string_view{userdata.get_object_name()} == "FName")
                {
                    object_class_name = lua.get_userdata<LuaType::FName>().get_local_cpp_object();
                }
                else
                {
                    throw std::runtime_error{error_overload_not_found};
                }
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
                could_be_in_class = true;
            }
            else
            {
                throw std::runtime_error{error_overload_not_found};
            }

            if (!lua.is_string() && !lua.is_userdata() && !lua.is_integer() && !lua.is_nil())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            Unreal::FName object_short_name{};
            Unreal::UObject* in_outer{};
            bool could_be_in_outer{};
            bool could_be_object_short_name{};
            if (lua.is_string())
            {
                object_short_name = Unreal::FName(ensure_str(lua.get_string()), Unreal::FNAME_Add);
                could_be_object_short_name = true;
            }
            else if (lua.is_userdata())
            {
                // The API is a bit awkward, we have to tell it to preserve the stack
                // That way, when we call 'get_userdata' again with a more specific type, there's still something to actually get
                auto& userdata = lua.get_userdata<LuaType::UE4SSBaseObject>(1, true);
                std::string_view lua_object_name = userdata.get_object_name();
                // TODO: Redo when there's a bette way of checking whether a lua object is derived from UObject
                if (lua_object_name == "UObject" || lua_object_name == "UWorld" || lua_object_name == "AActor" || lua_object_name == "UClass")
                {
                    if (lua_object_name == "UClass")
                    {
                        in_outer = lua.get_userdata<LuaType::UClass>().get_remote_cpp_object();
                    }
                    else
                    {
                        in_outer = lua.get_userdata<LuaType::UObject>().get_remote_cpp_object();
                    }
                    could_be_in_outer = true;
                }
                else if (lua_object_name == "FName")
                {
                    object_short_name = lua.get_userdata<LuaType::FName>().get_local_cpp_object();
                    could_be_object_short_name = true;
                }
                else
                {
                    throw std::runtime_error{error_overload_not_found};
                }
            }
            else if (lua.is_integer())
            {
                if (lua.get_integer() == -1)
                {
                    in_outer = Unreal::UObjectGlobals::ANY_PACKAGE;
                    could_be_in_outer = true;
                }
                else
                {
                    throw std::runtime_error{error_overload_not_found};
                }
            }
            else if (lua.is_nil())
            {
                could_be_in_outer = true;
                could_be_object_short_name = true;
                lua.discard_value();
            }
            else
            {
                throw std::runtime_error{error_overload_not_found};
            }

            int32_t required_flags{Unreal::EObjectFlags::RF_NoFlags};
            std::string in_name{};
            bool could_be_in_name{};
            if (lua.is_string())
            {
                in_name = lua.get_string();
                could_be_in_name = true;
            }
            else if (lua.is_integer())
            {
                required_flags = static_cast<int32_t>(lua.get_integer());
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }

            int32_t banned_flags{Unreal::EObjectFlags::RF_NoFlags};
            bool exact_class{};
            bool could_be_exact_class{};
            if (lua.is_bool())
            {
                exact_class = lua.get_bool();
                could_be_exact_class = true;
            }
            else if (lua.is_integer())
            {
                banned_flags = static_cast<int32_t>(lua.get_integer());
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }

            if (could_be_in_class && could_be_in_outer && could_be_in_name)
            {
                LuaType::auto_construct_object(lua, Unreal::UObjectGlobals::FindObject(in_class, in_outer, ensure_str(in_name), exact_class));
            }
            else
            {
                if (could_be_exact_class || !could_be_object_short_name)
                {
                    throw std::runtime_error{error_overload_not_found};
                }
                LuaType::auto_construct_object(lua, Unreal::UObjectGlobals::FindObject(object_class_name, object_short_name, required_flags, banned_flags));
            }
            return 1;
        });

        lua.register_function("FindObjects", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'FindObjects'.
Overloads:
#1: FindObjects(integer NumObjectsToFind, string|FName|nil ClassName, string|FName|nil ObjectShortName, EObjectFlags RequiredFlags, EObjectFlags BannedFlags, bool bExactClass)
#2: FindObjects(integer NumObjectsToFind, UClass|nil Class, string|FName|nil ObjectShortName, EObjectFlags RequiredFlags, EObjectFlags BannedFlags, bool bExactClass))"};

            int32_t num_objects_to_find{};
            if (lua.is_integer())
            {
                if (num_objects_to_find < 0)
                {
                    throw std::runtime_error{error_overload_not_found};
                }
                num_objects_to_find = static_cast<int32_t>(lua.get_integer());
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }
            else
            {
                throw std::runtime_error{error_overload_not_found};
            }

            if (!lua.is_string() && !lua.is_userdata() && !lua.is_nil())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            Unreal::FName object_class_name{};
            bool object_class_name_supplied{true};
            if (lua.is_string())
            {
                object_class_name = Unreal::FName(ensure_str(lua.get_string()), Unreal::FNAME_Add);
            }
            else if (lua.is_userdata())
            {
                // The API is a bit awkward, we have to tell it to preserve the stack
                // That way, when we call 'get_userdata' again with a more specific type, there's still something to actually get
                auto& userdata = lua.get_userdata<LuaType::UE4SSBaseObject>(1, true);
                if (std::string_view{userdata.get_object_name()} == "UClass")
                {
                    object_class_name = lua.get_userdata<LuaType::UClass>().get_remote_cpp_object()->GetNamePrivate();
                }
                else if (std::string_view{userdata.get_object_name()} == "FName")
                {
                    object_class_name = lua.get_userdata<LuaType::FName>().get_local_cpp_object();
                }
                else
                {
                    throw std::runtime_error{error_overload_not_found};
                }
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
                object_class_name_supplied = false;
            }
            else
            {
                throw std::runtime_error{error_overload_not_found};
            }

            if (!lua.is_string() && !lua.is_userdata() && !lua.is_nil())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            Unreal::FName object_short_name{};
            if (lua.is_string())
            {
                object_short_name = Unreal::FName(ensure_str(lua.get_string()), Unreal::FNAME_Add);
            }
            else if (lua.is_userdata())
            {
                // The API is a bit awkward, we have to tell it to preserve the stack
                // That way, when we call 'get_userdata' again with a more specific type, there's still something to actually get
                auto& userdata = lua.get_userdata<LuaType::UE4SSBaseObject>(1, true);
                if (std::string_view{userdata.get_object_name()} == "FName")
                {
                    object_short_name = lua.get_userdata<LuaType::FName>().get_local_cpp_object();
                }
                else
                {
                    throw std::runtime_error{error_overload_not_found};
                }
            }
            else if (lua.is_nil())
            {
                if (!object_class_name_supplied)
                {
                    error_overload_not_found.append("\nBoth param #1 and param #2 cannot be nil");
                    throw std::runtime_error{error_overload_not_found};
                }
                else
                {
                    lua.discard_value();
                }
            }
            else
            {
                throw std::runtime_error{error_overload_not_found};
            }

            int32_t required_flags{Unreal::EObjectFlags::RF_NoFlags};
            if (lua.is_integer())
            {
                required_flags = static_cast<int32_t>(lua.get_integer());
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }

            int32_t banned_flags{Unreal::EObjectFlags::RF_NoFlags};
            if (lua.is_integer())
            {
                banned_flags = static_cast<int32_t>(lua.get_integer());
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }

            bool exact_class{true};
            if (lua.is_integer())
            {
                exact_class = lua.get_integer();
            }
            else if (lua.is_bool())
            {
                exact_class = lua.get_bool();
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }

            std::vector<Unreal::UObject*> objects_found{};
            Unreal::UObjectGlobals::FindObjects(static_cast<size_t>(num_objects_to_find),
                                                object_class_name,
                                                object_short_name,
                                                objects_found,
                                                required_flags,
                                                banned_flags,
                                                exact_class);

            auto table = lua.prepare_new_table(static_cast<int32_t>(objects_found.size()));
            for (size_t i = 0; i < objects_found.size(); ++i)
            {
                table.add_key(i + 1);
                LuaType::auto_construct_object(lua, objects_found[i]);
                table.fuse_pair();
            }

            return 1;
        });

        lua.register_function("GetCurrentThreadId", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'GetCurrentThreadId'.
Overloads:
#1: GetCurrentThreadId())"};

            LuaType::ThreadId::construct(lua, std::this_thread::get_id());

            return 1;
        });

        lua.register_function("GetMainModThreadId", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'GetMainModThreadId'.
Overloads:
#1: GetMainModThreadId())"};

            const auto mod = get_mod_ref(lua);
            LuaType::ThreadId::construct(lua, mod->get_main_thread_id());

            return 1;
        });

        lua.register_function("GetAsyncThreadId", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'GetAsyncThreadId'.
Overloads:
#1: GetAsyncThreadId())"};

            const auto mod = get_mod_ref(lua);
            LuaType::ThreadId::construct(lua, mod->get_async_thread_id());

            return 1;
        });

        lua.register_function("GetGameThreadId", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'GetGameThreadId'.
Overloads:
#1: GetGameThreadId())"};

            LuaType::ThreadId::construct(lua, Unreal::GetGameThreadId());

            return 1;
        });

        lua.register_function("IsInMainModThread", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IsInMainModThread'.
Overloads:
#1: IsInMainModThread())"};

            const auto mod = get_mod_ref(lua);
            lua.set_bool(std::this_thread::get_id() == mod->get_main_thread_id());

            return 1;
        });

        lua.register_function("IsInAsyncThread", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IsInAsyncThread'.
Overloads:
#1: IsInAsyncThread())"};

            const auto mod = get_mod_ref(lua);
            lua.set_bool(std::this_thread::get_id() == mod->get_async_thread_id());

            return 1;
        });

        lua.register_function("IsInGameThread", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IsInGameThread'.
Overloads:
#1: IsInGameThread())"};

            lua.set_bool(std::this_thread::get_id() == Unreal::GetGameThreadId());

            return 1;
        });
    }

    auto LuaMod::setup_lua_global_functions(const LuaMadeSimple::Lua& lua) const -> void
    {
        setup_lua_global_functions_internal(lua, IsTrueMod::Yes);
    }

    static auto process_simple_actions(std::vector<LuaMod::SimpleLuaAction>& actions) -> void
    {
        std::erase_if(actions, [&](const LuaMod::SimpleLuaAction& lua_data) -> bool {
            if (LuaMod::m_is_currently_executing_game_action)
            {
                return false;
            }

            LuaMod::m_is_currently_executing_game_action = true;

            lua_data.lua->registry().get_function_ref(lua_data.lua_action_function_ref);

            TRY([&]() {
                lua_data.lua->call_function(0, 0);
            });

            luaL_unref(lua_data.lua->get_lua_state(), LUA_REGISTRYINDEX, lua_data.lua_action_function_ref);

            LuaMod::m_is_currently_executing_game_action = false;
            return true;
        });
    }

    template <GameThreadExecutionMethod Executor>
    static auto process_delayed_actions(std::vector<LuaMod::DelayedGameThreadAction>& actions) -> void
    {
        const auto now = std::chrono::steady_clock::now();
        std::erase_if(actions, [&](LuaMod::DelayedGameThreadAction& action) -> bool {
            // Check if pending removal first - any executor can clean up removed actions
            // regardless of method, to avoid orphaned actions that never get cleaned up
            if (action.status == LuaMod::DelayedActionStatus::PendingRemoval)
            {
                // Unref the function, but NOT the thread - the thread is shared across all actions
                // and is anchored in the registry by ensure_hook_thread_exists
                luaL_unref(action.lua->get_lua_state(), LUA_REGISTRYINDEX, action.lua_action_function_ref);
                return true;
            }

            // Only handle actions matching the executor method
            if constexpr (Executor == GameThreadExecutionMethod::ProcessEvent)
            {
                if (action.method != GameThreadExecutionMethod::ProcessEvent)
                {
                    return false;
                }
            }
            else if constexpr (Executor == GameThreadExecutionMethod::EngineTick)
            {
                if (action.method != GameThreadExecutionMethod::EngineTick)
                {
                    return false;
                }
            }

            // Skip paused actions
            if (action.status == LuaMod::DelayedActionStatus::Paused)
            {
                return false;
            }

            // Check if ready to execute
            bool ready = false;
            if (action.method == GameThreadExecutionMethod::EngineTick && action.delay_frames > 0)
            {
                // Frame-based delay
                action.frames_remaining--;
                ready = action.frames_remaining <= 0;
            }
            else if (action.method != GameThreadExecutionMethod::EngineTick && action.delay_frames > 0)
            {
                // Skip frame-based delays - they can only be processed by EngineTick
                // This should never happen since frame-based functions error if EngineTick unavailable
                if (action.delay_frames > 0)
                {
                    Output::send<LogLevel::Warning>(STR("ProcessEvent hook received frame-based delayed action - this should not happen\n"));
                    return false;
                }
            }
            else
            {
                // Time-based delay
                ready = now >= action.execute_at;
            }

            if (!ready)
            {
                return false;
            }

            if (LuaMod::m_is_currently_executing_game_action)
            {
                return false;
            }

            LuaMod::m_is_currently_executing_game_action = true;

            action.lua->registry().get_function_ref(action.lua_action_function_ref);

            TRY([&]() {
                action.lua->call_function(0, 0);
            });

            LuaMod::m_is_currently_executing_game_action = false;

            // Handle looping
            if (action.is_looping && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
            {
                // Reset the timer/frame counter
                if (action.method == GameThreadExecutionMethod::EngineTick && action.delay_frames > 0)
                {
                    action.frames_remaining = action.delay_frames;
                }
                else
                {
                    action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(action.delay_ms);
                }
                return false; // Keep in list
            }

            // Unref the function, but NOT the thread - the thread is shared across all actions
            // and is anchored in the registry by ensure_hook_thread_exists
            luaL_unref(action.lua->get_lua_state(), LUA_REGISTRYINDEX, action.lua_action_function_ref);
            return true;
        });
    }

    auto static process_event_hook([[maybe_unused]] Unreal::UObject* Context,
                                   [[maybe_unused]] Unreal::UFunction* Function,
                                   [[maybe_unused]] void* Parms) -> void
    {
        std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};

        process_simple_actions(LuaMod::m_game_thread_actions);
        process_delayed_actions<GameThreadExecutionMethod::ProcessEvent>(LuaMod::m_delayed_game_thread_actions);
    }

    auto static engine_tick_hook([[maybe_unused]] Unreal::UEngine* Context, [[maybe_unused]] float DeltaSeconds) -> void
    {
        std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};

        process_simple_actions(LuaMod::m_engine_tick_actions);
        process_delayed_actions<GameThreadExecutionMethod::EngineTick>(LuaMod::m_delayed_game_thread_actions);
    }

    // Local convenience wrappers for Capabilities functions
    static auto is_engine_tick_hook_available() -> bool
    {
        return UE4SSRuntime::IsEngineTickAvailable();
    }

    static auto is_process_event_hook_available() -> bool
    {
        return UE4SSRuntime::IsProcessEventAvailable();
    }

    // Helper to ensure engine tick hook is registered
    auto LuaMod::ensure_engine_tick_hooked() -> void
    {
        if (!m_is_engine_tick_hooked)
        {
            Unreal::Hook::RegisterEngineTickPreCallback(&engine_tick_hook);
            m_is_engine_tick_hooked = true;
        }
    }

    // Helper to ensure process event hook is registered
    auto LuaMod::ensure_process_event_hooked(LuaMod* mod) -> void
    {
        if (!mod->m_is_process_event_hooked)
        {
            Unreal::Hook::RegisterProcessEventPreCallback(&process_event_hook);
            mod->m_is_process_event_hooked = true;
        }
    }

    auto LuaMod::setup_lua_global_functions_main_state_only() const -> void
    {
        m_lua.register_function("RegisterHook", [](const LuaMadeSimple::Lua& lua) -> int {
            std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};

            std::string error_overload_not_found{R"(
No overload found for function 'RegisterHook'.
Overloads:
#1: RegisterHook(string UFunction_Name, LuaFunction Callback, LuaFunction PostCallback))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto function_name_no_prefix = get_function_name_without_prefix(ensure_str(lua.get_string()));

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto [hook_lua, lua_thread_registry_index] = make_hook_state(mod); // operates on LuaMod::m_lua incrementing its stack via lua_newthread

            // Duplicate the Lua function to the top of the stack for lua_xmove and luaL_ref
            lua_pushvalue(lua.get_lua_state(), 1); // operates on LuaMadeSimple::Lua::m_lua_state
            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const auto lua_callback_registry_index = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);

            bool has_post_callback{};
            int lua_post_callback_registry_index = -1;
            if (lua.is_function())
            {
                lua.discard_value();
                // Duplicate the second Lua function to the top of the stack for lua_xmove and luaL_ref
                lua_pushvalue(lua.get_lua_state(), 1); // operates on LuaMadeSimple::Lua::m_lua_state
                lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);
                lua_post_callback_registry_index = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);
                has_post_callback = true;
            }

            Unreal::UFunction* unreal_function = Unreal::UObjectGlobals::StaticFindObject<Unreal::UFunction*>(nullptr, nullptr, function_name_no_prefix);
            if (!unreal_function)
            {
                lua.throw_error(std::format(
                        "Tried to register a hook with Lua function 'RegisterHook' but no UFunction with the specified name was found.\nFunction Name: {}",
                        to_string(function_name_no_prefix)));
            }

            int32_t pre_id{};
            int32_t post_id{};

            auto func_ptr = unreal_function->GetFunc();
            if (func_ptr && func_ptr != Unreal::UObject::ProcessInternalInternal.get_function_address() &&
                unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))
            {
                auto& custom_data = g_hooked_script_function_data.emplace_back(std::make_unique<LuaUnrealScriptFunctionData>(
                        0, 0, unreal_function, mod, *hook_lua, lua_callback_registry_index, lua_post_callback_registry_index, lua_thread_registry_index));
                pre_id = unreal_function->RegisterPreHook(&lua_unreal_script_function_hook_pre, custom_data.get());
                post_id = unreal_function->RegisterPostHook(&lua_unreal_script_function_hook_post, custom_data.get());
                custom_data->pre_callback_id = pre_id;
                custom_data->post_callback_id = post_id;
                Output::send<LogLevel::Verbose>(STR("[RegisterHook] Registered native hook ({}, {}) for {}\n"),
                                                custom_data->pre_callback_id,
                                                custom_data->post_callback_id,
                                                unreal_function->GetFullName());
            }
            else if (func_ptr && func_ptr == Unreal::UObject::ProcessInternalInternal.get_function_address() &&
                     !unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))
            {
                auto function_data = find_function_hook_data(m_script_hook_callbacks, unreal_function);
                if (!function_data)
                {
                    function_data = &m_script_hook_callbacks.emplace_back(get_object_names(unreal_function), LuaCallbackData{hook_lua, nullptr, {}});
                }
                auto& callback_data = function_data->callback_data;
                // Note that non-native hooks don't have a different id for the post-callback.
                pre_id = Unreal::UnrealScriptFunctionData::MakeNewId();
                post_id = pre_id;
                callback_data.registry_indexes.emplace_back(hook_lua, LuaCallbackData::RegistryIndex{lua_callback_registry_index, pre_id});
                Output::send<LogLevel::Verbose>(STR("[RegisterHook] Registered script hook ({}, {}) for {}\n"),
                                                pre_id,
                                                post_id,
                                                unreal_function->GetFullName());
            }
            else
            {
                std::string error_message{"Was unable to register a hook with Lua function 'RegisterHook', information:\n"};
                error_message.append(fmt::format("FunctionName: {}\n", to_string(function_name_no_prefix)));
                error_message.append(fmt::format("UFunction::Func: {}\n", std::bit_cast<void*>(func_ptr)));
                error_message.append(fmt::format("ProcessInternal: {}\n", Unreal::UObject::ProcessInternalInternal.get_function_address()));
                error_message.append(
                        fmt::format("FUNC_Native: {}\n", static_cast<uint32_t>(unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))));
                lua.throw_error(error_message);
            }

            lua.set_integer(pre_id);
            lua.set_integer(post_id);

            return 2;
        });


        // Register EGameThreadMethod enum table
        {
            lua_State* L = m_lua.get_lua_state();
            lua_newtable(L);
            lua_pushinteger(L, static_cast<int>(GameThreadExecutionMethod::EngineTick));
            lua_setfield(L, -2, "EngineTick");
            lua_pushinteger(L, static_cast<int>(GameThreadExecutionMethod::ProcessEvent));
            lua_setfield(L, -2, "ProcessEvent");
            lua_setglobal(L, "EGameThreadMethod");
        }

        // Register capability globals
        // These indicate whether certain hooks are available (scan succeeded)
        {
            lua_State* L = m_lua.get_lua_state();
            lua_pushboolean(L, UE4SSRuntime::IsEngineTickAvailable());
            lua_setglobal(L, "EngineTickAvailable");
            lua_pushboolean(L, UE4SSRuntime::IsProcessEventAvailable());
            lua_setglobal(L, "ProcessEventAvailable");
        }

        m_lua.register_function("ExecuteInGameThread", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ExecuteInGameThread'.
Overloads:
#1: ExecuteInGameThread(LuaFunction callback)
#2: ExecuteInGameThread(LuaFunction callback, EGameThreadMethod method)
    method: EGameThreadMethod.EngineTick or EGameThreadMethod.ProcessEvent)"};

            lua_State* L = lua.get_lua_state();
            GameThreadExecutionMethod method = LuaMod::m_default_game_thread_method;
            int callback_idx = 1;

            if (lua_isfunction(L, 1) && lua_isinteger(L, 2))
            {
                // Overload #2: callback, method
                method = static_cast<GameThreadExecutionMethod>(lua_tointeger(L, 2));
            }
            else if (!lua_isfunction(L, 1))
            {
                lua.throw_error(error_overload_not_found);
            }

            const auto mod = get_mod_ref(lua);

            // Check hook availability before registering
            if (method == GameThreadExecutionMethod::EngineTick)
            {
                if (!is_engine_tick_hook_available())
                {
                    lua.throw_error("ExecuteInGameThread: EngineTick method requested but EngineTick hook is not available (AOB scan failed)");
                }
                LuaMod::ensure_engine_tick_hooked();
            }
            else if (method == GameThreadExecutionMethod::ProcessEvent)
            {
                if (!is_process_event_hook_available())
                {
                    lua.throw_error("ExecuteInGameThread: ProcessEvent method requested but ProcessEvent hook is not available (AOB scan failed)");
                }
                LuaMod::ensure_process_event_hooked(mod);
            }

            auto [hook_lua, lua_thread_registry_index] = make_hook_state(mod);

            lua_pushvalue(L, callback_idx);
            lua_xmove(L, hook_lua->get_lua_state(), 1);
            const auto func_ref = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);

            SimpleLuaAction simpleAction{hook_lua, func_ref, lua_thread_registry_index};
            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                if (method == GameThreadExecutionMethod::EngineTick)
                {
                    LuaMod::m_engine_tick_actions.emplace_back(simpleAction);
                }
                else
                {
                    mod->m_game_thread_actions.emplace_back(simpleAction);
                }
            }

            return 0;
        });

        // ExecuteInGameThreadWithDelay - executes callback after a time delay
        // Uses default method from config, falls back to the other if unavailable
        m_lua.register_function("ExecuteInGameThreadWithDelay", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ExecuteInGameThreadWithDelay'.
Overloads:
#1: ExecuteInGameThreadWithDelay(integer delayMs, LuaFunction callback) -> integer handle
#2: ExecuteInGameThreadWithDelay(integer handle, integer delayMs, LuaFunction callback) -> nil (only creates if handle doesn't exist))"};

            lua_State* L = lua.get_lua_state();

            // Determine which overload based on argument count
            int num_args = lua_gettop(L);
            bool has_handle = (num_args >= 3 && lua_isinteger(L, 1) && lua_isinteger(L, 2) && lua_isfunction(L, 3));
            bool no_handle = (num_args >= 2 && lua_isinteger(L, 1) && lua_isfunction(L, 2));

            if (!has_handle && !no_handle)
            {
                lua.throw_error(error_overload_not_found);
            }

            const auto mod = get_mod_ref(lua);

            // Use default method from config, fall back to the other if unavailable
            GameThreadExecutionMethod method = LuaMod::m_default_game_thread_method;
            if (method == GameThreadExecutionMethod::EngineTick)
            {
                LuaMod::ensure_engine_tick_hooked();
                if (!is_engine_tick_hook_available())
                {
                    LuaMod::ensure_process_event_hooked(mod);
                    if (!is_process_event_hook_available())
                    {
                        lua.throw_error("ExecuteInGameThreadWithDelay: Neither EngineTick nor ProcessEvent hooks are available (AOB scans failed)");
                    }
                    method = GameThreadExecutionMethod::ProcessEvent;
                }
            }
            else if (method == GameThreadExecutionMethod::ProcessEvent)
            {
                LuaMod::ensure_process_event_hooked(mod);
                if (!is_process_event_hook_available())
                {
                    LuaMod::ensure_engine_tick_hooked();
                    if (!is_engine_tick_hook_available())
                    {
                        lua.throw_error("ExecuteInGameThreadWithDelay: Neither EngineTick nor ProcessEvent hooks are available (AOB scans failed)");
                    }
                    method = GameThreadExecutionMethod::EngineTick;
                }
            }

            if (has_handle)
            {
                // Overload #2: ExecuteInGameThreadWithDelay(handle, delayMs, callback)
                // Like UE's Delay - only creates if handle doesn't already exist
                auto handle = lua_tointeger(L, 1);
                auto delay_ms = lua_tointeger(L, 2);

                // Check if handle already exists
                {
                    std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                    for (const auto& action : LuaMod::m_delayed_game_thread_actions)
                    {
                        if (action.handle == handle && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                        {
                            // Handle exists, do nothing (like UE's Delay node)
                            return 0;
                        }
                    }
                }

                // Handle doesn't exist, create new action
                auto [hook_lua, lua_thread_registry_index] = make_hook_state(mod);

                lua_pushvalue(L, 3);
                lua_xmove(L, hook_lua->get_lua_state(), 1);
                const auto func_ref = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);

                DelayedGameThreadAction action{};
                action.lua = hook_lua;
                action.lua_action_function_ref = func_ref;
                action.lua_action_thread_ref = lua_thread_registry_index;
                action.method = method;
                action.delay_ms = delay_ms;
                action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
                action.handle = handle;

                {
                    std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                    LuaMod::m_delayed_game_thread_actions.emplace_back(action);
                }

                return 0;
            }
            else
            {
                // Overload #1: ExecuteInGameThreadWithDelay(delayMs, callback) -> handle
                auto delay_ms = lua_tointeger(L, 1);
                auto [hook_lua, lua_thread_registry_index] = make_hook_state(mod);

                lua_pushvalue(L, 2);
                lua_xmove(L, hook_lua->get_lua_state(), 1);
                const auto func_ref = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);

                DelayedGameThreadAction action{};
                action.lua = hook_lua;
                action.lua_action_function_ref = func_ref;
                action.lua_action_thread_ref = lua_thread_registry_index;
                action.method = method;
                action.delay_ms = delay_ms;
                action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
                action.handle = LuaMod::m_next_delayed_action_handle++;

                {
                    std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                    LuaMod::m_delayed_game_thread_actions.emplace_back(action);
                }

                lua.set_integer(action.handle);
                return 1;
            }
        });

        // RetriggerableExecuteInGameThreadWithDelay - executes callback after a time delay, resets timer if called again with same handle
        // Uses default method from config, falls back to the other if unavailable
        m_lua.register_function("RetriggerableExecuteInGameThreadWithDelay", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RetriggerableExecuteInGameThreadWithDelay'.
Overloads:
#1: RetriggerableExecuteInGameThreadWithDelay(integer handle, integer delayMs, LuaFunction callback))"};

            lua_State* L = lua.get_lua_state();
            if (!lua_isinteger(L, 1) || !lua_isinteger(L, 2) || !lua_isfunction(L, 3))
            {
                lua.throw_error(error_overload_not_found);
            }

            const auto mod = get_mod_ref(lua);

            // Use default method from config, fall back to the other if unavailable
            GameThreadExecutionMethod method = LuaMod::m_default_game_thread_method;
            if (method == GameThreadExecutionMethod::EngineTick)
            {
                LuaMod::ensure_engine_tick_hooked();
                if (!is_engine_tick_hook_available())
                {
                    LuaMod::ensure_process_event_hooked(mod);
                    if (!is_process_event_hook_available())
                    {
                        lua.throw_error("RetriggerableExecuteInGameThreadWithDelay: Neither EngineTick nor ProcessEvent hooks are available (AOB scans failed)");
                    }
                    method = GameThreadExecutionMethod::ProcessEvent;
                }
            }
            else if (method == GameThreadExecutionMethod::ProcessEvent)
            {
                LuaMod::ensure_process_event_hooked(mod);
                if (!is_process_event_hook_available())
                {
                    LuaMod::ensure_engine_tick_hooked();
                    if (!is_engine_tick_hook_available())
                    {
                        lua.throw_error("RetriggerableExecuteInGameThreadWithDelay: Neither EngineTick nor ProcessEvent hooks are available (AOB scans failed)");
                    }
                    method = GameThreadExecutionMethod::EngineTick;
                }
            }

            auto handle = lua_tointeger(L, 1);
            auto delay_ms = lua_tointeger(L, 2);

            // Check if an action with this handle already exists
            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    if (action.handle == handle && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        // Reset the timer for the existing action
                        action.delay_ms = delay_ms;
                        action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
                        action.status = LuaMod::DelayedActionStatus::Active;  // Unpause if paused
                        lua.set_integer(handle);
                        return 1;
                    }
                }
            }

            // No existing action, create a new one
            auto [hook_lua, lua_thread_registry_index] = make_hook_state(mod);

            lua_pushvalue(L, 3);
            lua_xmove(L, hook_lua->get_lua_state(), 1);
            const auto func_ref = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);

            DelayedGameThreadAction action{};
            action.lua = hook_lua;
            action.lua_action_function_ref = func_ref;
            action.lua_action_thread_ref = lua_thread_registry_index;
            action.method = method;
            action.delay_ms = delay_ms;
            action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
            action.is_retriggerable = true;
            action.handle = handle;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                LuaMod::m_delayed_game_thread_actions.emplace_back(action);
            }

            return 0;
        });

        // ExecuteInGameThreadAfterFrames - executes callback after a frame delay
        // Requires EngineTick hook - cannot fall back to ProcessEvent since frames cannot be counted there
        m_lua.register_function("ExecuteInGameThreadAfterFrames", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ExecuteInGameThreadAfterFrames'.
Overloads:
#1: ExecuteInGameThreadAfterFrames(integer frames, LuaFunction callback) -> integer handle)"};

            lua_State* L = lua.get_lua_state();
            if (!lua.is_integer() || !lua_isfunction(L, 2))
            {
                lua.throw_error(error_overload_not_found);
            }

            // Frame-based delays require EngineTick - cannot use ProcessEvent
            if (!is_engine_tick_hook_available())
            {
                lua.throw_error("ExecuteInGameThreadAfterFrames: EngineTick hook is not available (AOB scan failed). Frame-based delays require EngineTick.");
            }

            auto frames = lua.get_integer();
            auto mod = get_mod_ref(lua);
            auto [hook_lua, lua_thread_registry_index] = make_hook_state(mod);

            lua_pushvalue(L, 1);
            lua_xmove(L, hook_lua->get_lua_state(), 1);
            const auto func_ref = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);

            DelayedGameThreadAction action{};
            action.lua = hook_lua;
            action.lua_action_function_ref = func_ref;
            action.lua_action_thread_ref = lua_thread_registry_index;
            action.delay_frames = frames;
            action.frames_remaining = frames;
            action.handle = LuaMod::m_next_delayed_action_handle++;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                LuaMod::m_delayed_game_thread_actions.emplace_back(action);
                LuaMod::ensure_engine_tick_hooked();
            }

            lua.set_integer(action.handle);
            return 1;
        });

        // LoopInGameThreadWithDelay - executes callback repeatedly with a time delay
        // Uses default method from config, falls back to the other if unavailable
        m_lua.register_function("LoopInGameThreadWithDelay", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'LoopInGameThreadWithDelay'.
Overloads:
#1: LoopInGameThreadWithDelay(integer delayMs, LuaFunction callback) -> integer handle)"};

            lua_State* L = lua.get_lua_state();
            if (!lua.is_integer() || !lua_isfunction(L, 2))
            {
                lua.throw_error(error_overload_not_found);
            }

            const auto mod = get_mod_ref(lua);

            // Use default method from config, fall back to the other if unavailable
            GameThreadExecutionMethod method = LuaMod::m_default_game_thread_method;
            if (method == GameThreadExecutionMethod::EngineTick)
            {
                LuaMod::ensure_engine_tick_hooked();
                if (!is_engine_tick_hook_available())
                {
                    LuaMod::ensure_process_event_hooked(mod);
                    if (!is_process_event_hook_available())
                    {
                        lua.throw_error("LoopInGameThreadWithDelay: Neither EngineTick nor ProcessEvent hooks are available (AOB scans failed)");
                    }
                    method = GameThreadExecutionMethod::ProcessEvent;
                }
            }
            else if (method == GameThreadExecutionMethod::ProcessEvent)
            {
                LuaMod::ensure_process_event_hooked(mod);
                if (!is_process_event_hook_available())
                {
                    LuaMod::ensure_engine_tick_hooked();
                    if (!is_engine_tick_hook_available())
                    {
                        lua.throw_error("LoopInGameThreadWithDelay: Neither EngineTick nor ProcessEvent hooks are available (AOB scans failed)");
                    }
                    method = GameThreadExecutionMethod::EngineTick;
                }
            }

            auto delay_ms = lua.get_integer();
            auto [hook_lua, lua_thread_registry_index] = make_hook_state(mod);

            lua_pushvalue(L, 1);
            lua_xmove(L, hook_lua->get_lua_state(), 1);
            const auto func_ref = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);

            DelayedGameThreadAction action{};
            action.lua = hook_lua;
            action.lua_action_function_ref = func_ref;
            action.lua_action_thread_ref = lua_thread_registry_index;
            action.method = method;
            action.delay_ms = delay_ms;
            action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
            action.is_looping = true;
            action.handle = LuaMod::m_next_delayed_action_handle++;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                LuaMod::m_delayed_game_thread_actions.emplace_back(action);
            }

            lua.set_integer(action.handle);
            return 1;
        });

        // LoopInGameThreadAfterFrames - executes callback repeatedly with a frame delay
        // Requires EngineTick hook - cannot fall back to ProcessEvent since frames cannot be counted there
        m_lua.register_function("LoopInGameThreadAfterFrames", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'LoopInGameThreadAfterFrames'.
Overloads:
#1: LoopInGameThreadAfterFrames(integer frames, LuaFunction callback) -> integer handle)"};

            lua_State* L = lua.get_lua_state();
            if (!lua.is_integer() || !lua_isfunction(L, 2))
            {
                lua.throw_error(error_overload_not_found);
            }

            // Frame-based delays require EngineTick - cannot use ProcessEvent
            if (!is_engine_tick_hook_available())
            {
                lua.throw_error("LoopInGameThreadAfterFrames: EngineTick hook is not available (AOB scan failed). Frame-based delays require EngineTick.");
            }

            auto frames = lua.get_integer();
            auto mod = get_mod_ref(lua);
            auto [hook_lua, lua_thread_registry_index] = make_hook_state(mod);

            // After get_integer() pops the first arg, the function is now at index 1
            lua_pushvalue(L, 1);
            lua_xmove(L, hook_lua->get_lua_state(), 1);
            const auto func_ref = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);

            DelayedGameThreadAction action{};
            action.lua = hook_lua;
            action.lua_action_function_ref = func_ref;
            action.lua_action_thread_ref = lua_thread_registry_index;
            action.delay_frames = frames;
            action.frames_remaining = frames;
            action.is_looping = true;
            action.handle = LuaMod::m_next_delayed_action_handle++;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                LuaMod::m_delayed_game_thread_actions.emplace_back(action);
                LuaMod::ensure_engine_tick_hooked();
            }

            lua.set_integer(action.handle);
            return 1;
        });

        // ResetDelayedActionTimer - resets the timer for any delayed action using the original delay (only if owned by calling mod)
        m_lua.register_function("ResetDelayedActionTimer", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ResetDelayedActionTimer'.
Overloads:
#1: ResetDelayedActionTimer(integer handle) -> boolean success)"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            const auto mod = get_mod_ref(lua);
            const LuaMadeSimple::Lua* mod_hook_lua = mod->m_hook_lua;
            bool found = false;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    // Only allow resetting actions owned by the calling mod
                    if (action.handle == handle && action.lua == mod_hook_lua && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        // Reset the timer based on whether it's time-based or frame-based
                        if (action.delay_frames > 0)
                        {
                            action.frames_remaining = action.delay_frames;
                        }
                        else
                        {
                            action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(action.delay_ms);
                        }
                        action.status = LuaMod::DelayedActionStatus::Active;  // Unpause if paused
                        found = true;
                        break;
                    }
                }
            }

            lua.set_bool(found);
            return 1;
        });

        // SetDelayedActionTimer - sets a new delay for a delayed action and restarts the timer (only if owned by calling mod)
        m_lua.register_function("SetDelayedActionTimer", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'SetDelayedActionTimer'.
Overloads:
#1: SetDelayedActionTimer(integer handle, integer newDelay) -> boolean success)"};

            lua_State* L = lua.get_lua_state();
            if (!lua.is_integer() || !lua_isinteger(L, 2))
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            auto new_delay = lua_tointeger(L, 2);
            const auto mod = get_mod_ref(lua);
            const LuaMadeSimple::Lua* mod_hook_lua = mod->m_hook_lua;
            bool found = false;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    // Only allow modifying actions owned by the calling mod
                    if (action.handle == handle && action.lua == mod_hook_lua && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        // Set new delay and reset the timer
                        if (action.delay_frames > 0)
                        {
                            action.delay_frames = new_delay;
                            action.frames_remaining = new_delay;
                        }
                        else
                        {
                            action.delay_ms = new_delay;
                            action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(new_delay);
                        }
                        action.status = LuaMod::DelayedActionStatus::Active;  // Unpause if paused
                        found = true;
                        break;
                    }
                }
            }

            lua.set_bool(found);
            return 1;
        });

        // PauseDelayedAction - pauses a delayed action timer (only if owned by calling mod)
        m_lua.register_function("PauseDelayedAction", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'PauseDelayedAction'.
Overloads:
#1: PauseDelayedAction(integer handle) -> boolean success)"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            const auto mod = get_mod_ref(lua);
            const LuaMadeSimple::Lua* mod_hook_lua = mod->m_hook_lua;
            bool found = false;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    // Only allow pausing actions owned by the calling mod
                    if (action.handle == handle && action.lua == mod_hook_lua && action.status == LuaMod::DelayedActionStatus::Active)
                    {
                        // Store remaining time before pausing
                        auto now = std::chrono::steady_clock::now();
                        if (action.execute_at > now)
                        {
                            action.time_remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(action.execute_at - now).count();
                        }
                        else
                        {
                            action.time_remaining_ms = 0;
                        }
                        action.status = LuaMod::DelayedActionStatus::Paused;
                        found = true;
                        break;
                    }
                }
            }

            lua.set_bool(found);
            return 1;
        });

        // UnpauseDelayedAction - resumes a paused delayed action timer (only if owned by calling mod)
        m_lua.register_function("UnpauseDelayedAction", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UnpauseDelayedAction'.
Overloads:
#1: UnpauseDelayedAction(integer handle) -> boolean success)"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            const auto mod = get_mod_ref(lua);
            const LuaMadeSimple::Lua* mod_hook_lua = mod->m_hook_lua;
            bool found = false;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    // Only allow unpausing actions owned by the calling mod
                    if (action.handle == handle && action.lua == mod_hook_lua && action.status == LuaMod::DelayedActionStatus::Paused)
                    {
                        // Restore execute_at from remaining time
                        action.execute_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(action.time_remaining_ms);
                        action.status = LuaMod::DelayedActionStatus::Active;
                        found = true;
                        break;
                    }
                }
            }

            lua.set_bool(found);
            return 1;
        });

        // CancelDelayedAction - cancels a delayed action (only if owned by calling mod)
        m_lua.register_function("CancelDelayedAction", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'CancelDelayedAction'.
Overloads:
#1: CancelDelayedAction(integer handle) -> boolean success)"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            const auto mod = get_mod_ref(lua);
            const LuaMadeSimple::Lua* mod_hook_lua = mod->m_hook_lua;
            bool found = false;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    // Only allow cancelling actions owned by the calling mod
                    if (action.handle == handle && action.lua == mod_hook_lua && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        action.status = LuaMod::DelayedActionStatus::PendingRemoval;
                        found = true;
                        break;
                    }
                }
            }

            lua.set_bool(found);
            return 1;
        });

        // IsValidDelayedActionHandle - checks if a handle refers to an existing, non-cancelled action
        m_lua.register_function("IsValidDelayedActionHandle", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IsValidDelayedActionHandle'.
Overloads:
#1: IsValidDelayedActionHandle(integer handle) -> boolean valid)"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            bool valid = false;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (const auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    if (action.handle == handle && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        valid = true;
                        break;
                    }
                }
            }

            lua.set_bool(valid);
            return 1;
        });

        // IsDelayedActionActive - checks if a delayed action is active (not paused or cancelled)
        m_lua.register_function("IsDelayedActionActive", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IsDelayedActionActive'.
Overloads:
#1: IsDelayedActionActive(integer handle) -> boolean active)"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            bool active = false;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (const auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    if (action.handle == handle && action.status == LuaMod::DelayedActionStatus::Active)
                    {
                        active = true;
                        break;
                    }
                }
            }

            lua.set_bool(active);
            return 1;
        });

        // IsDelayedActionPaused - checks if a delayed action is paused
        m_lua.register_function("IsDelayedActionPaused", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IsDelayedActionPaused'.
Overloads:
#1: IsDelayedActionPaused(integer handle) -> boolean paused)"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            bool paused = false;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (const auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    if (action.handle == handle && action.status == LuaMod::DelayedActionStatus::Paused)
                    {
                        paused = true;
                        break;
                    }
                }
            }

            lua.set_bool(paused);
            return 1;
        });

        // GetDelayedActionTimeRemaining - returns remaining time in milliseconds (or frames for frame-based)
        m_lua.register_function("GetDelayedActionTimeRemaining", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'GetDelayedActionTimeRemaining'.
Overloads:
#1: GetDelayedActionTimeRemaining(integer handle) -> integer remainingMs (or -1 if not found))"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            int64_t remaining = -1;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (const auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    if (action.handle == handle && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        if (action.delay_frames > 0)
                        {
                            // Frame-based: return frames remaining
                            remaining = action.frames_remaining;
                        }
                        else if (action.status == LuaMod::DelayedActionStatus::Paused)
                        {
                            // Paused: return stored remaining time
                            remaining = action.time_remaining_ms;
                        }
                        else
                        {
                            // Active: calculate remaining time
                            auto now = std::chrono::steady_clock::now();
                            if (action.execute_at > now)
                            {
                                remaining = std::chrono::duration_cast<std::chrono::milliseconds>(action.execute_at - now).count();
                            }
                            else
                            {
                                remaining = 0;
                            }
                        }
                        break;
                    }
                }
            }

            lua.set_integer(remaining);
            return 1;
        });

        // GetDelayedActionTimeElapsed - returns elapsed time in milliseconds (or frames for frame-based)
        m_lua.register_function("GetDelayedActionTimeElapsed", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'GetDelayedActionTimeElapsed'.
Overloads:
#1: GetDelayedActionTimeElapsed(integer handle) -> integer elapsedMs (or -1 if not found))"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            int64_t elapsed = -1;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (const auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    if (action.handle == handle && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        if (action.delay_frames > 0)
                        {
                            // Frame-based: return frames elapsed
                            elapsed = action.delay_frames - action.frames_remaining;
                        }
                        else if (action.status == LuaMod::DelayedActionStatus::Paused)
                        {
                            // Paused: calculate from stored remaining time
                            elapsed = action.delay_ms - action.time_remaining_ms;
                        }
                        else
                        {
                            // Active: calculate elapsed time
                            auto now = std::chrono::steady_clock::now();
                            if (action.execute_at > now)
                            {
                                auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(action.execute_at - now).count();
                                elapsed = action.delay_ms - remaining;
                            }
                            else
                            {
                                elapsed = action.delay_ms;
                            }
                        }
                        break;
                    }
                }
            }

            lua.set_integer(elapsed);
            return 1;
        });

        // GetDelayedActionRate - returns the configured delay rate (not remaining time)
        m_lua.register_function("GetDelayedActionRate", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'GetDelayedActionRate'.
Overloads:
#1: GetDelayedActionRate(integer handle) -> integer rateMs (or -1 if not found))"};

            if (!lua.is_integer())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto handle = lua.get_integer();
            int64_t rate = -1;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (const auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    if (action.handle == handle && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        if (action.delay_frames > 0)
                        {
                            // Frame-based: return frames
                            rate = action.delay_frames;
                        }
                        else
                        {
                            // Time-based: return ms
                            rate = action.delay_ms;
                        }
                        break;
                    }
                }
            }

            lua.set_integer(rate);
            return 1;
        });

        // ClearAllDelayedActions - cancels all delayed actions for the current mod
        m_lua.register_function("ClearAllDelayedActions", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ClearAllDelayedActions'.
Overloads:
#1: ClearAllDelayedActions() -> integer count)"};

            const auto mod = get_mod_ref(lua);
            const LuaMadeSimple::Lua* mod_hook_lua = mod->m_hook_lua;
            int64_t count = 0;

            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                for (auto& action : LuaMod::m_delayed_game_thread_actions)
                {
                    // Check if this action belongs to the current mod by comparing hook lua states
                    if (action.lua == mod_hook_lua && action.status != LuaMod::DelayedActionStatus::PendingRemoval)
                    {
                        // Mark for removal
                        action.status = LuaMod::DelayedActionStatus::PendingRemoval;
                        count++;
                    }
                }
            }

            lua.set_integer(count);
            return 1;
        });

        // MakeHandle - returns a unique action handle
        m_lua.register_function("MakeActionHandle", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'MakeActionHandle'.
Overloads:
#1: MakeActionHandle() -> integer Handle)"};

            lua.set_integer(m_next_delayed_action_handle++);

            return 1;
        });

        m_lua.register_function("RestartCurrentMod", [](const LuaMadeSimple::Lua& lua) -> int {
            auto mod = get_mod_ref(lua);
            if (!mod)
            {
                lua.throw_error("RestartCurrentMod: Could not get mod reference");
            }

            // Use mod ID for safe cross-thread reference
            ModId mod_id = mod->get_id();
            UE4SSProgram::get_program().queue_reinstall_mod(mod_id);

            return 0;
        });

        m_lua.register_function("UninstallCurrentMod", [](const LuaMadeSimple::Lua& lua) -> int {
            auto mod = get_mod_ref(lua);
            if (!mod)
            {
                lua.throw_error("UninstallCurrentMod: Could not get mod reference");
            }

            // Use mod ID for safe cross-thread reference
            ModId mod_id = mod->get_id();
            UE4SSProgram::get_program().queue_uninstall_mod(mod_id);

            return 0;
        });

        // P1: string mod_name - Name of the mod to restart
        m_lua.register_function("RestartMod", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RestartMod'.
Overloads:
#1: RestartMod(string mod_name))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }
                        
            UE4SSProgram::get_program().queue_reinstall_mod_by_name(lua.get_string());

            return 0;
        });

        // P1: string mod_name - Name of the mod to uninstall
        m_lua.register_function("UninstallMod", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UninstallMod'.
Overloads:
#1: UninstallMod(string mod_name))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }
            
            UE4SSProgram::get_program().queue_uninstall_mod_by_name(lua.get_string());

            return 0;
        });
    }

    auto static is_unreal_version_out_of_bounds_from_64bit(int64_t major_version, int64_t minor_version) -> bool
    {
        if (major_version < std::numeric_limits<uint32_t>::min() || major_version > std::numeric_limits<uint32_t>::max() ||
            minor_version < std::numeric_limits<uint32_t>::min() || minor_version > std::numeric_limits<uint32_t>::max())
        {
            return false;
        }
        else
        {
            return true;
        }
    };

    using UnrealVersionCheckFunctionPtr = bool (*)(int32_t, int32_t);
    auto static unreal_version_check(const LuaMadeSimple::Lua& lua, UnrealVersionCheckFunctionPtr check_function, const std::string& error_overload_not_found) -> void
    {
        // Removing the table from the stack as we don't need it
        // This is required in order to align the parameters (or manually provide the stack index for the params)
        if (lua.is_table())
        {
            lua.discard_value();
        }

        // Checking the first and second param, without retrieving either
        // Makes for less code
        if (!lua.is_integer() || !lua.is_integer(2))
        {
            lua.throw_error(error_overload_not_found);
        }

        int64_t major_version = lua.get_integer();
        int64_t minor_version = lua.get_integer();

        if (!is_unreal_version_out_of_bounds_from_64bit(major_version, minor_version))
        {
            lua.throw_error("[UnrealVersion::unreal_version_check] Major/minor version numbers must be within the range of uint32");
        }

        lua.set_bool(check_function(static_cast<int32_t>(major_version), static_cast<int32_t>(minor_version)));
    }

    auto static setup_lua_classes_internal(const LuaMadeSimple::Lua& lua) -> void
    {
        // UE4SS Class -> START
        auto mod_class = lua.prepare_new_table();
        mod_class.set_has_userdata(false);

        mod_class.add_pair("GetVersion", [](const LuaMadeSimple::Lua& lua) -> int {
            lua.set_integer(UE4SS_LIB_VERSION_MAJOR);
            lua.set_integer(UE4SS_LIB_VERSION_MINOR);
            lua.set_integer(UE4SS_LIB_VERSION_HOTFIX);
            return 3;
        });
        mod_class.make_global("UE4SS");
        // UE4SS Class -> END

        // UnrealVersion Class -> START
        auto unreal_version_class = lua.prepare_new_table();
        unreal_version_class.set_has_userdata(false);

        unreal_version_class.add_pair("GetMajor", [](const LuaMadeSimple::Lua& lua) -> int {
            lua.set_integer(Unreal::Version::Major);
            return 1;
        });

        unreal_version_class.add_pair("GetMinor", [](const LuaMadeSimple::Lua& lua) -> int {
            lua.set_integer(Unreal::Version::Minor);
            return 1;
        });

        unreal_version_class.add_pair("IsEqual", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UnrealVersion:IsEqual'.
Overloads:
#1: IsEqual(number MajorVersion, number MinorVersion))"};

            unreal_version_check(lua, &Unreal::Version::IsEqual, error_overload_not_found);

            return 1;
        });

        unreal_version_class.add_pair("IsAtLeast", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UnrealVersion:IsAtLeast'.
Overloads:
#1: IsAtLeast(number MajorVersion, number MinorVersion))"};

            unreal_version_check(lua, &Unreal::Version::IsAtLeast, error_overload_not_found);

            return 1;
        });

        unreal_version_class.add_pair("IsAtMost", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UnrealVersion:IsAtMost'.
Overloads:
#1: IsAtMost(number MajorVersion, number MinorVersion))"};

            unreal_version_check(lua, &Unreal::Version::IsAtMost, error_overload_not_found);

            return 1;
        });

        unreal_version_class.add_pair("IsBelow", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UnrealVersion:IsBelow'.
Overloads:
#1: IsBelow(number MajorVersion, number MinorVersion))"};

            unreal_version_check(lua, &Unreal::Version::IsBelow, error_overload_not_found);

            return 1;
        });

        unreal_version_class.add_pair("IsAbove", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UnrealVersion:IsAbove'.
Overloads:
#1: IsAbove(number MajorVersion, number MinorVersion))"};

            unreal_version_check(lua, &Unreal::Version::IsAbove, error_overload_not_found);

            return 1;
        });
        unreal_version_class.make_global("UnrealVersion");
        // UnrealVersion Class -> END

        // FName Class -> START
        // Pre-load the global FName table
        // Without this, the metatable won't be created until an FName is constructed by another part of UE4SS
        LuaType::FName::construct(lua, Unreal::FName(static_cast<int64_t>(0)));
        lua_setglobal(lua.get_lua_state(), "FName");
        LuaType::FName::construct(lua, Unreal::NAME_None);
        lua_setglobal(lua.get_lua_state(), "NAME_None");
        // FName Class -> END

        // FText Class -> START
        // Pre-load the global FText table
        // Without this, the metatable won't be created until an FText is constructed by another part of UE4SS
        LuaType::FText::construct(lua, Unreal::FText());
        lua_setglobal(lua.get_lua_state(), "FText");
        // FText Class -> END

        // FString Class -> START
        // Pre-load the global FString constructor
        lua.register_function("FString",
                              [](const LuaMadeSimple::Lua& lua) -> int {
                                  if (lua.get_stack_size() < 1 || !lua.is_string())
                                  {
                                      lua.throw_error("FString constructor requires a string argument");
                                  }
                                  std::string_view str = lua.get_string();
                                  auto fstring = Unreal::FString(ensure_str(std::string(str)).c_str());
                                  LuaType::FString::construct(lua, &fstring);
                                  return 1;
                              });
        // FString Class -> END

        // FUtf8String Class -> START
        // Pre-load the global FUtf8String constructor
        lua.register_function("FUtf8String",
                              [](const LuaMadeSimple::Lua& lua) -> int {
                                  if (lua.get_stack_size() < 1 || !lua.is_string())
                                  {
                                      lua.throw_error("FUtf8String constructor requires a string argument");
                                  }
                                  std::string_view str = lua.get_string();
                                  auto utf8string = Unreal::FUtf8String(reinterpret_cast<const Unreal::UTF8CHAR*>(str.data()));
                                  LuaType::FUtf8String::construct(lua, &utf8string);
                                  return 1;
                              });
        // FUtf8String Class -> END

        // FAnsiString Class -> START
        // Pre-load the global FAnsiString constructor
        lua.register_function("FAnsiString",
                              [](const LuaMadeSimple::Lua& lua) -> int {
                                  if (lua.get_stack_size() < 1 || !lua.is_string())
                                  {
                                      lua.throw_error("FAnsiString constructor requires a string argument");
                                  }
                                  std::string_view str = lua.get_string();
                                  auto ansistring = Unreal::FAnsiString(str.data());
                                  LuaType::FAnsiString::construct(lua, &ansistring);
                                  return 1;
                              });
        // FAnsiString Class -> END

        // FPackageName -> START
        auto package_name = lua.prepare_new_table();
        package_name.set_has_userdata(false);

        package_name.add_pair("IsShortPackageName", [](const LuaMadeSimple::Lua& lua) -> int {
            static std::string error_overload_not_found{R"(
No overload found for function 'FPackageName:IsShortPackageName'.
Overloads:
#1: IsShortPackageName(string PossiblyLongName))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }

            File::StringType PossiblyLongName = ensure_str(lua.get_string());
            lua.set_bool(Unreal::FPackageName::IsShortPackageName(PossiblyLongName));

            return 1;
        });

        package_name.add_pair("IsValidLongPackageName", [](const LuaMadeSimple::Lua& lua) -> int {
            static std::string error_overload_not_found{R"(
No overload found for function 'FPackageName:IsValidLongPackageName'.
Overloads:
#1: IsValidLongPackageName(string InLongPackageName))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }

            File::StringType InLongPackageName = ensure_str(lua.get_string());
            lua.set_bool(Unreal::FPackageName::IsValidLongPackageName(InLongPackageName));

            return 1;
        });

        package_name.make_global("FPackageName");
        // FPackageName -> END
    }

    auto LuaMod::setup_lua_classes(const LuaMadeSimple::Lua& lua) const -> void
    {
        setup_lua_classes_internal(lua);
    }

    auto LuaMod::prepare_mod(const LuaMadeSimple::Lua& lua) -> void
    {
        try
        {
            lua.open_all_libs();
            setup_lua_require_paths(lua);
            setup_lua_global_functions(lua);
            setup_lua_classes(lua);

            // Setup a global reference for this mod
            // It can be accessed later when you otherwise don't have access to the 'Mod' instance
            LuaType::LuaModRef::construct(lua, this);
            lua_setglobal(lua.get_lua_state(), "ModRef");

            // Setup all the input related globals (keys & modifier keys)
            register_input_globals(lua);

            register_all_property_types(lua);
            register_object_flags(lua);
            register_efindname(lua);

            lua.set_nil();
            lua_setglobal(lua.get_lua_state(), "__OriginalReturnValue");
        }
        catch (std::exception& e)
        {
            Output::send<LogLevel::Error>(STR("Exception during mod preparation: {}\n"), ensure_str(e.what()));
            throw; // Re-throw to allow proper error handling upstream
        }
        catch (...)
        {
            Output::send<LogLevel::Error>(STR("Unknown exception during mod preparation\n"));
            throw; // Re-throw to allow proper error handling upstream
        }
    }

    auto LuaMod::fire_on_lua_start_for_cpp_mods() -> void
    {
        if (!is_started())
        {
            return;
        }

        for (const auto& mod : UE4SSProgram::get_program().m_mods)
        {
            if (auto cpp_mod = dynamic_cast<CppMod*>(mod.get()); cpp_mod && mod->is_started())
            {
                if (mod->get_name() == get_name())
                {
                    cpp_mod->fire_on_lua_start(m_lua, *m_main_lua, *m_async_lua, m_hook_lua);
                }
                cpp_mod->fire_on_lua_start(get_name(), m_lua, *m_main_lua, *m_async_lua, m_hook_lua);
            }
        }
    }

    auto LuaMod::fire_on_lua_stop_for_cpp_mods() -> void
    {
        if (!is_started())
        {
            return;
        }

        for (const auto& mod : UE4SSProgram::get_program().m_mods)
        {
            if (auto cpp_mod = dynamic_cast<CppMod*>(mod.get()); cpp_mod && mod->is_started())
            {
                if (mod->get_name() == get_name())
                {
                    cpp_mod->fire_on_lua_stop(m_lua, *m_main_lua, *m_async_lua, m_hook_lua);
                }
                cpp_mod->fire_on_lua_stop(get_name(), m_lua, *m_main_lua, *m_async_lua, m_hook_lua);
            }
        }
    }

    auto LuaMod::load_and_execute_script(const std::filesystem::path& script_path) -> bool
    {
        try
        {
            if (!std::filesystem::exists(script_path))
            {
                return false;
            }

            // Read the file content
            std::ifstream file(script_path, std::ios::binary);
            if (!file.is_open())
            {
                throw std::runtime_error(fmt::format("Failed to open script file: {}", to_utf8_string(script_path)));
            }

            // Get file size and read content
            file.seekg(0, std::ios::end);
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<char> buffer(size);
            if (!file.read(buffer.data(), size))
            {
                throw std::runtime_error(fmt::format("Failed to read script file: {}", to_utf8_string(script_path)));
            }
            file.close();

            // Generate a UTF-8 chunk name for better error messages
            std::string chunk_name = "@" + to_utf8_string(script_path);

            lua_State* L = main_lua()->get_lua_state();

            // Push error handler first so we capture the stack before it unwinds
            int err_handler_idx = LuaMadeSimple::push_pcall_error_handler(L);

            // Load the buffer
            if (int status = luaL_loadbuffer(L, buffer.data(), buffer.size(), chunk_name.c_str()); status != LUA_OK)
            {
                std::string error_msg = lua_tostring(L, -1);
                Output::send<LogLevel::Error>(STR("Error loading script: {}\n"), ensure_str(error_msg));
                lua_pop(L, 1);
                lua_remove(L, err_handler_idx); // Clean up error handler
                return false;
            }

            // Execute the chunk with our error handler
            if (int status = lua_pcall(L, 0, 0, err_handler_idx); status != LUA_OK)
            {
                // Error handler already captured the stack and notified debugger
                std::string error_msg = lua_tostring(L, -1);
                Output::send<LogLevel::Error>(STR("Error executing script: {}\n"), ensure_str(error_msg));
                lua_pop(L, 1);
                lua_remove(L, err_handler_idx); // Clean up error handler
                return false;
            }

            lua_remove(L, err_handler_idx); // Clean up error handler
            return true;
        }
        catch (const std::exception& e)
        {
            Output::send<LogLevel::Error>(STR("Exception in load_and_execute_script: {}\n"), ensure_str(e.what()));
            return false;
        }
    }

    auto LuaMod::start_mod() -> void
    {
        try
        {
            m_main_thread_id = std::this_thread::get_id();

            prepare_mod(lua());
            make_main_state(this, lua());
            setup_lua_global_functions_main_state_only();
            make_async_state(this, lua());
            start_async_thread();

            m_is_started = true;
            fire_on_lua_start_for_cpp_mods();

            // Set up the custom module loader for handling UTF-8 paths
            setup_custom_module_loader(main_lua());

            // Use the scripts path that was already determined in the constructor
            std::filesystem::path main_script_path = m_scripts_path / STR("main.lua");

            if (std::filesystem::exists(main_script_path))
            {
                if (!load_and_execute_script(main_script_path))
                {
                    Output::send<LogLevel::Error>(STR("Failed to execute main script: {}\n"), ensure_str(main_script_path));
                }
            }
            else
            {
                // This case implies m_scripts_path itself is valid, but main.lua is missing
                Output::send<LogLevel::Error>(
                        STR("Main script 'main.lua' not found in scripts directory: {} -- Ensure your script file uses the correct casing.\n"),
                        ensure_str(m_scripts_path));
            }
        }
        catch (const std::exception& e)
        {
            Output::send<LogLevel::Error>(STR("Exception during mod startup: {}\n"), ensure_str(e.what()));
        }
    }

    template <typename T>
    concept IsPair = requires(T t) {
        { t.second };
    };
    template <typename T>
    concept IsDataIndirect = requires(T t) {
        { t.callback_data };
    };

    static auto erase_from_container(LuaMod* mod, auto& container) -> void
    {
        for (auto it = container.begin(); it != container.end();)
        {
            const auto& data = [&] {
                if constexpr (IsPair<decltype(*it)>)
                {
                    return it->second;
                }
                else if constexpr (IsDataIndirect<decltype(*it)>)
                {
                    return it->callback_data;
                }
                else
                {
                    return *it;
                }
            }();
            if (get_mod_ref(*data.lua) == mod)
            {
                it = container.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    auto LuaMod::uninstall() -> void
    {
        // ProcessEvent hook may try to run, and the lua state will not be valid
        std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};

        Output::send(STR("Stopping mod '{}' for uninstall\n"), m_mod_name);

        fire_on_lua_stop_for_cpp_mods();

        if (m_async_thread.joinable())
        {
            m_async_thread.request_stop();
            m_async_thread.join();
        }

        erase_from_container(this, m_static_construct_object_lua_callbacks);
        erase_from_container(this, m_process_console_exec_pre_callbacks);
        erase_from_container(this, m_process_console_exec_post_callbacks);
        erase_from_container(this, m_global_command_lua_callbacks);
        erase_from_container(this, m_custom_command_lua_pre_callbacks);
        erase_from_container(this, m_custom_event_callbacks);
        erase_from_container(this, m_load_map_pre_callbacks);
        erase_from_container(this, m_load_map_post_callbacks);
        erase_from_container(this, m_init_game_state_pre_callbacks);
        erase_from_container(this, m_init_game_state_post_callbacks);
        erase_from_container(this, m_begin_play_pre_callbacks);
        erase_from_container(this, m_begin_play_post_callbacks);
        erase_from_container(this, m_call_function_by_name_with_arguments_pre_callbacks);
        erase_from_container(this, m_call_function_by_name_with_arguments_post_callbacks);
        erase_from_container(this, m_local_player_exec_pre_callbacks);
        erase_from_container(this, m_local_player_exec_post_callbacks);
        erase_from_container(this, m_script_hook_callbacks);

        UE4SSProgram::get_program().get_all_input_events([&](auto& key_set) {
            std::erase_if(key_set.key_data,
                          [&](auto& item) -> bool {
                              auto& [_, key_data] = item;
                              std::erase_if(key_data,
                                            [&](Input::KeyData& key_data) -> bool {
                                                // custom_data == 1: Bind came from Lua, and custom_data2 is a pointer to LuaMod.
                                                // custom_data == 2: Bind came from C++, and custom_data2 is a pointer to KeyDownEventData. Must free it.
                                                if (key_data.custom_data == 1)
                                                {
                                                    return key_data.custom_data2 == this;
                                                }
                                                else
                                                {
                                                    return false;
                                                }
                                            });

                              return key_data.empty();
                          });
        });


        // Mark all hooks for this mod as scheduled_for_removal BEFORE closing Lua state
        // This prevents hooks from firing with an invalid Lua state during the window between
        // lua_close and the actual hook unregistration
        for (auto& item : g_hooked_script_function_data)
        {
            if (item->mod == this)
            {
                item->scheduled_for_removal = true;
            }
        }

        // Remove any pending game thread actions for this mod BEFORE closing Lua state
        // Otherwise process_event_hook may try to execute actions with an invalid Lua state
        // Note: action.lua points to m_hook_lua (a thread), so compare against that
        // Must be done BEFORE m_hook_lua is set to nullptr
        std::erase_if(m_game_thread_actions, [&](const SimpleLuaAction& action) {
            return action.lua == m_hook_lua;
        });

        // Remove any pending engine tick actions for this mod
        std::erase_if(m_engine_tick_actions, [&](const SimpleLuaAction& action) {
            return action.lua == m_hook_lua;
        });

        // Remove any delayed game thread actions for this mod
        std::erase_if(m_delayed_game_thread_actions, [&](const DelayedGameThreadAction& action) {
            return action.lua == m_hook_lua;
        });

        if (m_hook_lua != nullptr)
        {
            m_hook_lua = nullptr; // lua_newthread results are handled by lua GC
        }

        if (m_async_lua && m_async_lua->get_lua_state())
        {
            lua_resetthread(m_async_lua->get_lua_state());
        }

        if (m_main_lua && m_main_lua->get_lua_state())
        {
            lua_resetthread(m_main_lua->get_lua_state());
        }

        lua_close(lua().get_lua_state());

        // Unhook all UFunctions for this mod & remove from the map that keeps track of which UFunctions have been hooked
        std::erase_if(g_hooked_script_function_data, [&](std::unique_ptr<LuaUnrealScriptFunctionData>& item) -> bool {
            if (item->mod == this)
            {
                Output::send(STR("\tUnregistering hook by id '{}#{}' for mod {}\n"), item->unreal_function->GetName(), item->pre_callback_id, item->mod->get_name());
                Output::send(STR("\tUnregistering hook by id '{}#{}' for mod {}\n"), item->unreal_function->GetName(), item->post_callback_id, item->mod->get_name());
                item->unreal_function->UnregisterHook(item->pre_callback_id);
                item->unreal_function->UnregisterHook(item->post_callback_id);
                return true;
            }

            return false;
        });

        clear_delayed_actions();
    }

    auto LuaMod::lua() const -> const LuaMadeSimple::Lua&
    {
        return m_lua;
    }

    auto LuaMod::main_lua() const -> const LuaMadeSimple::Lua*
    {
        return m_main_lua;
    }

    auto LuaMod::async_lua() const -> const LuaMadeSimple::Lua*
    {
        return m_async_lua;
    }

    auto LuaMod::get_lua_state() const -> lua_State*
    {
        return lua().get_lua_state();
    }

    auto static start_console_lua_executor() -> void
    {
        LuaStatics::console_executor = &LuaMadeSimple::new_state();
        LuaStatics::console_executor->open_all_libs();
        setup_lua_global_functions_internal(*LuaStatics::console_executor, LuaMod::IsTrueMod::No);
        setup_lua_classes_internal(*LuaStatics::console_executor);
        register_input_globals(*LuaStatics::console_executor);
        register_all_property_types(*LuaStatics::console_executor);
        register_object_flags(*LuaStatics::console_executor);
        LuaStatics::console_executor_enabled = true;
    };

    auto static stop_console_lua_executor() -> void
    {
        lua_close(LuaStatics::console_executor->get_lua_state());

        LuaStatics::console_executor = nullptr;
        LuaStatics::console_executor_enabled = false;
    }

    static auto script_hook([[maybe_unused]] Unreal::UObject* Context, Unreal::FFrame& Stack, [[maybe_unused]] void* RESULT_DECL) -> void
    {
        std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};

        auto execute_hook = [&](std::vector<LuaMod::FunctionHookData>& callback_container, bool precise_name_match) {
            if (callback_container.empty())
            {
                return;
            }
            auto data = precise_name_match ? LuaMod::find_function_hook_data(callback_container, Stack.Node())
                                           : LuaMod::find_function_hook_data(callback_container, Stack.Node()->GetNamePrivate());
            if (data)
            {
                const auto& callback_data = data->callback_data;
                for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                {
                    const auto& lua = *lua_ptr;

                    // -1 is a special value that signifies that the hook has been unregistered.
                    if (registry_index.lua_index == -1)
                    {
                        continue;
                    }

                    lua.registry().get_function_ref(registry_index.lua_index);

                    static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                    LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);

                    auto node = Stack.Node();
                    auto return_value_offset = node->GetReturnValueOffset();
                    auto has_return_value = return_value_offset != 0xFFFF;
                    auto num_unreal_params = node->GetNumParms();
                    if (has_return_value && num_unreal_params > 0)
                    {
                        --num_unreal_params;
                    }

                    if (has_return_value || num_unreal_params > 0)
                    {
                        for (Unreal::FProperty* param : Unreal::TFieldRange<Unreal::FProperty>(node, Unreal::EFieldIterationFlags::IncludeDeprecated))
                        {
                            if (!param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_Parm))
                            {
                                continue;
                            }
                            if (has_return_value && param->GetOffset_Internal() == return_value_offset)
                            {
                                continue;
                            }

                            auto param_type = param->GetClass().GetFName();
                            auto param_type_comparison_index = param_type.GetComparisonIndex();
                            if (auto it = LuaType::StaticState::m_property_value_pushers.find(param_type_comparison_index);
                                it != LuaType::StaticState::m_property_value_pushers.end())
                            {
                                void* data{};
                                if (param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_OutParm))
                                {
                                    data = Unreal::FindOutParamValueAddress(Stack, param);
                                }
                                else
                                {
                                    data = param->ContainerPtrToValuePtr<void>(Stack.Locals());
                                }
                                const LuaType::PusherParams pusher_param{
                                        .operation = LuaType::Operation::GetParam,
                                        .lua = lua,
                                        .base = nullptr,
                                        .data = data,
                                        .property = param,
                                };
                                auto& type_handler = it->second;
                                type_handler(pusher_param);
                            }
                            else
                            {
                                lua.throw_error(fmt::format(
                                        "[script_hook] Tried accessing unreal property without a registered handler. Property type '{}' not supported.",
                                        to_string(param_type.ToString())));
                            }
                        };
                    }

                    lua.call_function(num_unreal_params + 1, 1);

                    bool return_value_handled{};
                    if (has_return_value && RESULT_DECL && lua.get_stack_size() > 0 && !lua.is_nil())
                    {
                        auto return_property = node->GetReturnProperty();
                        if (return_property)
                        {
                            auto return_property_type = return_property->GetClass().GetFName();
                            auto return_property_type_comparison_index = return_property_type.GetComparisonIndex();

                            if (auto it = LuaType::StaticState::m_property_value_pushers.find(return_property_type_comparison_index);
                                it != LuaType::StaticState::m_property_value_pushers.end())
                            {
                                const LuaType::PusherParams pusher_params{.operation = LuaType::Operation::Set,
                                                                          .lua = lua,
                                                                          .base = static_cast<Unreal::UObject*>(RESULT_DECL),
                                                                          .data = RESULT_DECL,
                                                                          .property = return_property};
                                auto& type_handler = it->second;
                                type_handler(pusher_params);
                                return_value_handled = true;
                            }
                            else
                            {
                                auto return_property_type_name = return_property_type.ToString();
                                auto return_property_name = return_property->GetName();

                                Output::send(STR("Tried altering return value of a custom BP function without a registered handler for return type Return "
                                                 "property '{}' of type '{}' not supported."),
                                             return_property_name,
                                             return_property_type_name);
                            }
                        }
                    }

                    if (!return_value_handled)
                    {
                        lua.discard_value();
                    }
                }
            }
        };

        TRY([&] {
            execute_hook(LuaMod::m_custom_event_callbacks, false);
            execute_hook(LuaMod::m_script_hook_callbacks, true);
        });
    }

    auto LuaMod::on_program_start() -> void
    {
        Unreal::UObjectArray::AddUObjectDeleteListener(&LuaType::FLuaObjectDeleteListener::s_lua_object_delete_listener);

        Unreal::Hook::RegisterLoadMapPreCallback(
                [](Unreal::UEngine* Engine, Unreal::FWorldContext& WorldContext, Unreal::FURL URL, Unreal::UPendingNetGame* PendingGame, Unreal::FString& Error)
                        -> std::pair<bool, bool> {
                    return TRY([&] {
                        for (const auto& callback_data : m_load_map_pre_callbacks)
                        {
                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                            {
                                const auto& lua = *lua_ptr;

                                lua.registry().get_function_ref(registry_index.lua_index);
                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                                LuaType::RemoteUnrealParam::construct(lua, &Engine, s_object_property_name);
                                LuaType::RemoteUnrealParam::construct(lua, &WorldContext.GetThisCurrentWorld(), s_object_property_name);
                                LuaType::FURL::construct(lua, URL);
                                LuaType::RemoteUnrealParam::construct(lua, &PendingGame, s_object_property_name);
                                callback_data.lua->set_string(to_string(*Error));
                                lua.call_function(5, 1);

                                if (callback_data.lua->is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua->discard_value();
                                }
                                else if (!callback_data.lua->is_bool())
                                {
                                    throw std::runtime_error{"A callback for 'LoadMap' must return bool or nil"};
                                }
                                else
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua->get_bool();
                                }
                            }

                            return return_value;
                        }
                        return std::pair<bool, bool>{false, false};
                    });
                });

        Unreal::Hook::RegisterLoadMapPostCallback(
                [](Unreal::UEngine* Engine, Unreal::FWorldContext& WorldContext, Unreal::FURL URL, Unreal::UPendingNetGame* PendingGame, Unreal::FString& Error)
                        -> std::pair<bool, bool> {
                    return TRY([&] {
                        for (const auto& callback_data : m_load_map_post_callbacks)
                        {
                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                            {
                                const auto& lua = *lua_ptr;

                                lua.registry().get_function_ref(registry_index.lua_index);
                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                                LuaType::RemoteUnrealParam::construct(lua, &Engine, s_object_property_name);
                                LuaType::RemoteUnrealParam::construct(lua, &WorldContext.GetThisCurrentWorld(), s_object_property_name);
                                LuaType::FURL::construct(lua, URL);
                                LuaType::RemoteUnrealParam::construct(lua, &PendingGame, s_object_property_name);
                                callback_data.lua->set_string(to_string(*Error));
                                lua.call_function(5, 1);

                                if (callback_data.lua->is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua->discard_value();
                                }
                                else if (!callback_data.lua->is_bool())
                                {
                                    throw std::runtime_error{"A callback for 'LoadMap' must return bool or nil"};
                                }
                                else
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua->get_bool();
                                }
                            }

                            return return_value;
                        }
                        return std::pair<bool, bool>{false, false};
                    });
                });

        Unreal::Hook::RegisterInitGameStatePreCallback([]([[maybe_unused]] Unreal::AGameModeBase* Context) {
            TRY([&] {
                for (const auto& callback_data : m_init_game_state_pre_callbacks)
                {
                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        lua.call_function(1, 0);
                    }
                }
            });
        });

        Unreal::Hook::RegisterInitGameStatePostCallback([]([[maybe_unused]] Unreal::AGameModeBase* Context) {
            TRY([&] {
                for (const auto& callback_data : m_init_game_state_post_callbacks)
                {
                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        lua.call_function(1, 0);
                    }
                }
            });
        });

        Unreal::Hook::RegisterBeginPlayPreCallback([]([[maybe_unused]] Unreal::AActor* Context) {
            TRY([&] {
                for (const auto& callback_data : m_begin_play_pre_callbacks)
                {
                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        lua.call_function(1, 0);
                    }
                }
            });
        });

        Unreal::Hook::RegisterBeginPlayPostCallback([]([[maybe_unused]] Unreal::AActor* Context) {
            TRY([&] {
                for (const auto& callback_data : m_begin_play_post_callbacks)
                {
                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        lua.call_function(1, 0);
                    }
                }
            });
        });

        Unreal::Hook::RegisterEndPlayPreCallback([]([[maybe_unused]] Unreal::AActor* Context, Unreal::EEndPlayReason EndPlayReason) {
            TRY([&] {
                for (const auto& callback_data : m_end_play_pre_callbacks)
                {
                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        static auto s_int_property_name = Unreal::FName(STR("IntProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(lua, &EndPlayReason, s_int_property_name);
                        lua.call_function(2, 0);
                    }
                }
            });
        });

        Unreal::Hook::RegisterEndPlayPostCallback([]([[maybe_unused]] Unreal::AActor* Context, Unreal::EEndPlayReason EndPlayReason) {
            TRY([&] {
                for (const auto& callback_data : m_end_play_post_callbacks)
                {
                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        static auto s_int_property_name = Unreal::FName(STR("IntProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(lua, &EndPlayReason, s_int_property_name);
                        lua.call_function(2, 0);
                    }
                }
            });
        });

        Unreal::Hook::RegisterStaticConstructObjectPostCallback([](const Unreal::FStaticConstructObjectParameters&, Unreal::UObject* constructed_object) {
            return TRY([&] {
                Unreal::UStruct* object_class = constructed_object->GetClassPrivate();
                while (object_class)
                {
                    std::erase_if(m_static_construct_object_lua_callbacks, [&](auto& callback_data) -> bool {
                        bool cancel = false;
                        if (callback_data.instance_of_class == object_class)
                        {
                            callback_data.lua->registry().get_function_ref(callback_data.lua_callback_function_ref);
                            LuaType::auto_construct_object(*callback_data.lua, constructed_object);
                            callback_data.lua->call_function(1, 1);

                            cancel = callback_data.lua->is_bool(-1) && callback_data.lua->get_bool(-1);

                            if (cancel)
                            {
                                // Release the thread_ref to GC.
                                luaL_unref(callback_data.lua->get_lua_state(), LUA_REGISTRYINDEX, callback_data.lua_callback_thread_ref);
                            }
                        }

                        return cancel;
                    });

                    object_class = object_class->GetSuperStruct();
                }

                return constructed_object;
            });
        });

        Unreal::Hook::RegisterULocalPlayerExecPreCallback([](Unreal::ULocalPlayer* context, Unreal::UWorld* in_world, const TCHAR* cmd, Unreal::FOutputDevice& ar)
                                                                  -> Unreal::Hook::ULocalPlayerExecCallbackReturnValue {
            return TRY([&] {
                for (const auto& callback_data : m_local_player_exec_pre_callbacks)
                {
                    Unreal::Hook::ULocalPlayerExecCallbackReturnValue return_value{};

                    for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                    {
                        callback_data.lua->registry().get_function_ref(registry_index.lua_index);

                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(*callback_data.lua, &context, s_object_property_name);
                        LuaType::RemoteUnrealParam::construct(*callback_data.lua, &in_world, s_object_property_name);
                        callback_data.lua->set_string(to_string(cmd));
                        LuaType::FOutputDevice::construct(*callback_data.lua, &ar);

                        callback_data.lua->call_function(4, 2);

                        if (callback_data.lua->is_nil())
                        {
                            return_value.UseOriginalReturnValue = true;
                            callback_data.lua->discard_value();
                        }
                        else if (!callback_data.lua->is_bool())
                        {
                            throw std::runtime_error{"The first return value for 'RegisterULocalPlayerExecPreHook' must return bool or nil"};
                        }
                        else
                        {
                            return_value.UseOriginalReturnValue = false;
                            return_value.NewReturnValue = callback_data.lua->get_bool();
                        }

                        if (callback_data.lua->is_nil())
                        {
                            return_value.ExecuteOriginalFunction = true;
                            callback_data.lua->discard_value();
                        }
                        else if (!callback_data.lua->is_bool())
                        {
                            throw std::runtime_error{"The second return value for callback 'RegisterULocalPlayerExecPreHook' must return bool or nil"};
                        }
                        else
                        {
                            return_value.ExecuteOriginalFunction = callback_data.lua->get_bool();
                        }
                    }

                    return return_value;
                }

                return Unreal::Hook::ULocalPlayerExecCallbackReturnValue{};
            });
        });

        Unreal::Hook::RegisterULocalPlayerExecPostCallback([](Unreal::ULocalPlayer* context, Unreal::UWorld* in_world, const TCHAR* cmd, Unreal::FOutputDevice& ar)
                                                                   -> Unreal::Hook::ULocalPlayerExecCallbackReturnValue {
            return TRY([&] {
                for (const auto& callback_data : m_local_player_exec_post_callbacks)
                {
                    Unreal::Hook::ULocalPlayerExecCallbackReturnValue return_value{};

                    for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                    {
                        callback_data.lua->registry().get_function_ref(registry_index.lua_index);

                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                        LuaType::RemoteUnrealParam::construct(*callback_data.lua, &context, s_object_property_name);
                        LuaType::RemoteUnrealParam::construct(*callback_data.lua, &in_world, s_object_property_name);
                        callback_data.lua->set_string(to_string(cmd));
                        LuaType::FOutputDevice::construct(*callback_data.lua, &ar);

                        callback_data.lua->call_function(4, 2);

                        if (callback_data.lua->is_nil())
                        {
                            return_value.UseOriginalReturnValue = true;
                            callback_data.lua->discard_value();
                        }
                        else if (!callback_data.lua->is_bool())
                        {
                            throw std::runtime_error{"A callback for 'RegisterULocalPlayerExecPreHook' must return bool or nil"};
                        }
                        else
                        {
                            return_value.UseOriginalReturnValue = false;
                            return_value.NewReturnValue = callback_data.lua->get_bool();
                        }

                        if (callback_data.lua->is_nil())
                        {
                            return_value.ExecuteOriginalFunction = true;
                            callback_data.lua->discard_value();
                        }
                        else if (!callback_data.lua->is_bool())
                        {
                            throw std::runtime_error{"The second return value for callback 'RegisterULocalPlayerExecPreHook' must return bool or nil"};
                        }
                        else
                        {
                            return_value.ExecuteOriginalFunction = callback_data.lua->get_bool();
                        }
                    }

                    return return_value;
                }

                return Unreal::Hook::ULocalPlayerExecCallbackReturnValue{};
            });
        });

        Unreal::Hook::RegisterCallFunctionByNameWithArgumentsPreCallback(
                [](Unreal::UObject* context, const TCHAR* str, Unreal::FOutputDevice& ar, Unreal::UObject* executor, bool b_force_call_with_non_exec)
                        -> std::pair<bool, bool> {
                    return TRY([&] {
                        for (const auto& callback_data : m_call_function_by_name_with_arguments_pre_callbacks)
                        {
                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                            {
                                callback_data.lua->registry().get_function_ref(registry_index.lua_index);

                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                                LuaType::RemoteUnrealParam::construct(*callback_data.lua, &context, s_object_property_name);
                                callback_data.lua->set_string(to_string(str));
                                LuaType::FOutputDevice::construct(*callback_data.lua, &ar);
                                LuaType::RemoteUnrealParam::construct(*callback_data.lua, &executor, s_object_property_name);
                                callback_data.lua->set_bool(b_force_call_with_non_exec);

                                callback_data.lua->call_function(5, 1);

                                if (callback_data.lua->is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua->discard_value();
                                }
                                else if (!callback_data.lua->is_bool())
                                {
                                    throw std::runtime_error{"A callback for 'RegisterCallFunctionByNameWithArgumentsPreHook' must return bool or nil"};
                                }
                                else
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua->get_bool();
                                }
                            }

                            return return_value;
                        }

                        return std::pair{false, false};
                    });
                });

        Unreal::Hook::RegisterCallFunctionByNameWithArgumentsPostCallback(
                [](Unreal::UObject* context, const TCHAR* str, Unreal::FOutputDevice& ar, Unreal::UObject* executor, bool b_force_call_with_non_exec)
                        -> std::pair<bool, bool> {
                    return TRY([&] {
                        for (const auto& callback_data : m_call_function_by_name_with_arguments_post_callbacks)
                        {
                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                            {
                                callback_data.lua->registry().get_function_ref(registry_index.lua_index);

                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                                LuaType::RemoteUnrealParam::construct(*callback_data.lua, &context, s_object_property_name);
                                callback_data.lua->set_string(to_string(str));
                                LuaType::FOutputDevice::construct(*callback_data.lua, &ar);
                                LuaType::RemoteUnrealParam::construct(*callback_data.lua, &executor, s_object_property_name);
                                callback_data.lua->set_bool(b_force_call_with_non_exec);

                                callback_data.lua->call_function(5, 1);

                                if (callback_data.lua->is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua->discard_value();
                                }
                                else if (!callback_data.lua->is_bool())
                                {
                                    throw std::runtime_error{"A callback for 'RegisterCallFunctionByNameWithArgumentsPreHook' must return bool or nil"};
                                }
                                else
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua->get_bool();
                                }
                            }

                            return return_value;
                        }

                        return std::pair{false, false};
                    });
                });

        // Lua from the in-game console.
        Unreal::Hook::RegisterProcessConsoleExecCallback([](Unreal::UObject* context, const TCHAR* cmd, Unreal::FOutputDevice& ar, Unreal::UObject* executor) -> bool {
            auto logln = [&ar](const File::StringType& log_message) {
                Output::send(fmt::format(STR("{}\n"), log_message));
                ar.Log(FromCharTypePtr<TCHAR>(log_message.c_str()));
            };

            if (!LuaStatics::console_executor_enabled && String::iequal(File::StringViewType{ToCharTypePtr(cmd)}, File::StringViewType{STR("luastart")}))
            {
                start_console_lua_executor();
                logln(STR("Console Lua executor started"));
                return true;
            }
            else if (LuaStatics::console_executor_enabled && String::iequal(File::StringViewType{ToCharTypePtr(cmd)}, File::StringViewType{STR("luastop")}))
            {
                stop_console_lua_executor();
                logln(STR("Console Lua executor stopped"));
                return true;
            }
            else if (LuaStatics::console_executor_enabled && String::iequal(File::StringViewType{ToCharTypePtr(cmd)}, File::StringViewType{STR("luarestart")}))
            {
                stop_console_lua_executor();
                start_console_lua_executor();
                logln(STR("Console Lua executor restarted"));
                return true;
            }
            else if (String::iequal(File::StringViewType{ToCharTypePtr(cmd)}, File::StringViewType{STR("clear")}))
            {
                // TODO: Replace with proper implementation when we have UGameViewportClient and UConsole.
                //       This should be fairly cross-game & cross-engine-version compatible even without the proper implementation.
                //       This is because I don't think they've changed the layout here and we have a reflected property right before the unreflected one that we're looking for.
                Unreal::UObject** console = static_cast<Unreal::UObject**>(context->GetValuePtrByPropertyName(FromCharTypePtr<TCHAR>(STR("ViewportConsole"))));
                auto* default_texture_white = std::bit_cast<Unreal::TArray<Unreal::FString>*>(
                        static_cast<uint8_t*>((*console)->GetValuePtrByPropertyNameInChain(FromCharTypePtr<TCHAR>(STR("DefaultTexture_White")))) + 0x8);
                auto* scrollback = std::bit_cast<int32_t*>(std::bit_cast<uint8_t*>(default_texture_white) + 0x10);
                default_texture_white->SetNum(0);
                default_texture_white->SetMax(0);
                *scrollback = 0;
                return true;
            }
            else if (LuaStatics::console_executor_enabled)
            {
                if (!LuaStatics::console_executor)
                {
                    logln(STR("Console Lua executor is enabled but the Lua instance is nullptr. Please try run RC_LUA_START again."));
                    return true;
                }

                LuaLibrary::set_outputdevice_ref(*LuaStatics::console_executor, &ar);

                // logln(fmt::format(STR("Executing '{}' as Lua"), cmd));

                try
                {
                    lua_State* L = LuaStatics::console_executor->get_lua_state();

                    // Push error handler to capture stack before it unwinds
                    int err_handler_idx = LuaMadeSimple::push_pcall_error_handler(L);

                    if (int status = luaL_loadstring(L, to_string(cmd).c_str()); status != LUA_OK)
                    {
                        lua_remove(L, err_handler_idx);
                        LuaStatics::console_executor->throw_error(
                                fmt::format("luaL_loadstring returned {}", LuaStatics::console_executor->resolve_status_message(status, true)));
                    }

                    if (int status = lua_pcall(L, 0, LUA_MULTRET, err_handler_idx); status != LUA_OK)
                    {
                        lua_pop(L, 1); // Pop error message
                        lua_remove(L, err_handler_idx);
                        LuaStatics::console_executor->throw_error(
                                fmt::format("lua_pcall returned {}", LuaStatics::console_executor->resolve_status_message(status, true)));
                    }

                    lua_remove(L, err_handler_idx);
                }
                catch (std::runtime_error& e)
                {
                    logln(ensure_str(e.what()));
                }

                // We always return true when the console Lua executor is enabled in order to suppress other handlers
                return true;
            }
            else
            {
                return false;
            }
        });

        // RegisterProcessConsoleExecPreHook
        Unreal::Hook::RegisterProcessConsoleExecGlobalPreCallback(
                [](Unreal::UObject* context, const TCHAR* cmd, Unreal::FOutputDevice& ar, Unreal::UObject* executor) -> std::pair<bool, bool> {
                    return TRY([&] {
                        auto command = File::StringType{ToCharTypePtr(cmd)};
                        auto command_parts = explode_by_occurrence_with_quotes(command, STR(' '));

                        for (const auto& callback_data : m_process_console_exec_pre_callbacks)
                        {
                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                            {
                                callback_data.lua->registry().get_function_ref(registry_index.lua_index);

                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                                LuaType::RemoteUnrealParam::construct(*callback_data.lua, &context, s_object_property_name);
                                callback_data.lua->set_string(to_string(command));
                                auto params_table = callback_data.lua->prepare_new_table();
                                for (size_t i = 0; i < command_parts.size(); ++i)
                                {
                                    const auto& command_part = command_parts[i];
                                    params_table.add_pair(i + 1, to_string(command_part).c_str());
                                }
                                LuaType::FOutputDevice::construct(*callback_data.lua, &ar);
                                LuaType::RemoteUnrealParam::construct(*callback_data.lua, &executor, s_object_property_name);

                                callback_data.lua->call_function(5, 1);

                                if (callback_data.lua->is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua->discard_value();
                                }
                                else if (callback_data.lua->is_bool())
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua->get_bool();
                                }
                                else
                                {
                                    throw std::runtime_error{"A callback for 'RegisterProcessConsoleExecHook' must return bool or nil"};
                                }
                            }

                            return return_value;
                        }

                        return std::pair{false, false};
                    });
                });

        // RegisterProcessConsoleExecPostHook
        Unreal::Hook::RegisterProcessConsoleExecGlobalPostCallback(
                [](Unreal::UObject* context, const TCHAR* cmd, Unreal::FOutputDevice& ar, Unreal::UObject* executor) -> std::pair<bool, bool> {
                    return TRY([&] {
                        auto command = File::StringType{ToCharTypePtr(cmd)};
                        auto command_parts = explode_by_occurrence_with_quotes(command, STR(' '));

                        for (const auto& callback_data : m_process_console_exec_post_callbacks)
                        {
                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                            {
                                callback_data.lua->registry().get_function_ref(registry_index.lua_index);

                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"), Unreal::FNAME_Find);
                                LuaType::RemoteUnrealParam::construct(*callback_data.lua, &context, s_object_property_name);
                                callback_data.lua->set_string(to_string(command));
                                auto params_table = callback_data.lua->prepare_new_table();
                                for (size_t i = 0; i < command_parts.size(); ++i)
                                {
                                    const auto& command_part = command_parts[i];
                                    params_table.add_pair(i + 1, to_string(command_part).c_str());
                                }
                                LuaType::FOutputDevice::construct(*callback_data.lua, &ar);
                                LuaType::RemoteUnrealParam::construct(*callback_data.lua, &executor, s_object_property_name);

                                callback_data.lua->call_function(5, 1);

                                if (callback_data.lua->is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua->discard_value();
                                }
                                else if (callback_data.lua->is_bool())
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua->get_bool();
                                }
                                else
                                {
                                    throw std::runtime_error{"A callback for 'RegisterProcessConsoleExecHook' must return bool or nil"};
                                }
                            }

                            return return_value;
                        }

                        return std::pair{false, false};
                    });
                });

        // RegisterConsoleCommandHandler
        Unreal::Hook::RegisterProcessConsoleExecCallback([](Unreal::UObject* context, const TCHAR* cmd, Unreal::FOutputDevice& ar, Unreal::UObject* executor) -> bool {
            (void)executor;

            if (!Unreal::Cast<Unreal::UGameViewportClient>(context))
            {
                return false;
            }

            return TRY([&] {
                auto command = File::StringType{ToCharTypePtr(cmd)};
                auto command_parts = explode_by_occurrence_with_quotes(command, STR(' '));
                File::StringType command_name = command;
                if (command_parts.size() > 1)
                {
                    command_name = command_parts[0];
                }

                if (auto it = m_custom_command_lua_pre_callbacks.find(command_name); it != m_custom_command_lua_pre_callbacks.end())
                {
                    const auto& callback_data = it->second;

                    bool return_value{};

                    for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                    {
                        callback_data.lua->registry().get_function_ref(registry_index.lua_index);
                        callback_data.lua->set_string(to_string(command));

                        auto params_table = callback_data.lua->prepare_new_table();
                        for (size_t i = 1; i < command_parts.size(); ++i)
                        {
                            const auto& command_part = command_parts[i];
                            params_table.add_pair(i, to_string(command_part).c_str());
                        }

                        LuaType::FOutputDevice::construct(*callback_data.lua, &ar);

                        callback_data.lua->call_function(3, 1);

                        if (!callback_data.lua->is_bool())
                        {
                            throw std::runtime_error{"A custom console command handle must return true or false"};
                        }

                        return_value = callback_data.lua->get_bool();
                    }

                    return return_value;
                }

                return false;
            });
        });

        // RegisterConsoleCommandGlobalHandler
        Unreal::Hook::RegisterProcessConsoleExecCallback([](Unreal::UObject* context, const TCHAR* cmd, Unreal::FOutputDevice& ar, Unreal::UObject* executor) -> bool {
            (void)context;
            (void)executor;

            return TRY([&] {
                auto command = File::StringType{ToCharTypePtr(cmd)};
                auto command_parts = explode_by_occurrence_with_quotes(command, STR(' '));
                File::StringType command_name = command;
                if (command_parts.size() > 1)
                {
                    command_name = command_parts[0];
                }

                if (auto it = m_global_command_lua_callbacks.find(command_name); it != m_global_command_lua_callbacks.end())
                {
                    const auto& callback_data = it->second;

                    bool return_value{};

                    for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                    {
                        callback_data.lua->registry().get_function_ref(registry_index.lua_index);
                        callback_data.lua->set_string(to_string(command));

                        auto params_table = callback_data.lua->prepare_new_table();
                        for (size_t i = 1; i < command_parts.size(); ++i)
                        {
                            const auto& command_part = command_parts[i];
                            params_table.add_pair(i, to_string(command_part).c_str());
                        }

                        LuaType::FOutputDevice::construct(*callback_data.lua, &ar);

                        callback_data.lua->call_function(3, 1);

                        if (!callback_data.lua->is_bool())
                        {
                            throw std::runtime_error{"A custom console command handle must return true or false"};
                        }

                        return_value = callback_data.lua->get_bool();
                    }

                    return return_value;
                }

                return false;
            });
        });

        if (Unreal::UObject::ProcessLocalScriptFunctionInternal.is_ready() && Unreal::Version::IsAtLeast(4, 22))
        {
            Output::send(STR("Enabling custom events\n"));
            Unreal::Hook::RegisterProcessLocalScriptFunctionPostCallback(script_hook);
        }
        else if (Unreal::UObject::ProcessInternalInternal.is_ready() && Unreal::Version::IsBelow(4, 22))
        {
            Output::send(STR("Enabling custom events\n"));
            Unreal::Hook::RegisterProcessInternalPostCallback(script_hook);
        }
    }

    auto LuaMod::update_async() -> void
    {
        for (m_processing_events = true; m_processing_events && !m_async_thread.get_stop_token().stop_requested();)
        {
            if (m_pause_events_processing)
            {
                continue;
            }

            process_delayed_actions();

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    auto LuaMod::process_delayed_actions() -> void
    {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

        actions_lock();

        m_delayed_actions.insert(m_delayed_actions.end(), std::make_move_iterator(m_pending_actions.begin()), std::make_move_iterator(m_pending_actions.end()));
        m_pending_actions.clear();

        actions_unlock();

        m_delayed_actions.erase(std::remove_if(m_delayed_actions.begin(),
                                               m_delayed_actions.end(),
                                               [&](AsyncAction& action) -> bool {
                                                   auto passed =
                                                           now -
                                                           std::chrono::duration_cast<std::chrono::milliseconds>(action.created_at.time_since_epoch()).count();
                                                   auto duration_since_creation = (action.type == LuaMod::ActionType::Immediate || passed >= action.delay);
                                                   if (duration_since_creation)
                                                   {
                                                       bool result = true;
                                                       try
                                                       {
                                                           async_lua()->registry().get_function_ref(action.lua_action_function_ref);
                                                           if (action.type == LuaMod::ActionType::Loop)
                                                           {
                                                               async_lua()->call_function(0, 1);
                                                               result = async_lua()->is_bool() && async_lua()->get_bool();
                                                               action.created_at = std::chrono::steady_clock::now();
                                                           }
                                                           else
                                                           {
                                                               async_lua()->call_function(0, 0);
                                                           }
                                                       }
                                                       catch (std::runtime_error& e)
                                                       {
                                                           Output::send(STR("[{}] {}\n"),
                                                                        ensure_str(action.type == LuaMod::ActionType::Loop ? "LoopAsync" : "DelayedAction"),
                                                                        ensure_str(e.what()));
                                                       }

                                                       return result;
                                                   }
                                                   else
                                                   {
                                                       return false;
                                                   }
                                               }),
                                m_delayed_actions.end());
    }

    auto LuaMod::clear_delayed_actions() -> void
    {
        actions_lock();
        m_pending_actions.clear();
        m_delayed_actions.clear();
        actions_unlock();
    }
} // namespace RC

#if PLATFORM_WINDOWS
#include <Unreal/Core/Windows/HideWindowsPlatformTypes.hpp>
#endif