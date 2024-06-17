#define NOMINMAX

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
#include <LuaType/LuaFOutputDevice.hpp>
#include <LuaType/LuaModRef.hpp>
#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaFURL.hpp>
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
#include <Unreal/FProperty.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/PackageName.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/Property/FWeakObjectProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/TypeChecker.hpp>
#include <Unreal/UAssetRegistry.hpp>
#include <Unreal/UAssetRegistryHelpers.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UGameViewportClient.hpp>
#include <Unreal/UKismetSystemLibrary.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UnrealVersion.hpp>
#include <UnrealCustom/CustomProperty.hpp>
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

    static auto set_is_in_game_thread(const LuaMadeSimple::Lua& lua, bool new_value)
    {
        lua.set_bool(new_value);
        lua_setfield(lua.get_lua_state(), LUA_REGISTRYINDEX, "IsInGameThread");
    }

    static auto is_in_game_thread(const LuaMadeSimple::Lua& lua) -> bool
    {
        lua_getfield(lua.get_lua_state(), LUA_REGISTRYINDEX, "IsInGameThread");
        return lua.get_bool(-1);
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
    };
    static std::vector<std::unique_ptr<LuaUnrealScriptFunctionData>> g_hooked_script_function_data{};

    static auto lua_unreal_script_function_hook_pre(Unreal::UnrealScriptFunctionCallableContext context, void* custom_data) -> void
    {

        // Fetch the data corresponding to this UFunction
        auto& lua_data = *static_cast<LuaUnrealScriptFunctionData*>(custom_data);

        // This is a promise that we're in the game thread, used by other functions to ensure that we don't execute when unsafe
        set_is_in_game_thread(lua_data.lua, true);

        // Use the stored registry index to put a Lua function on the Lua stack
        // This is the function that was provided by the Lua call to "RegisterHook"
        lua_data.lua.registry().get_function_ref(lua_data.lua_callback_ref);

        // Set up the first param (context / this-ptr)
        // TODO: Check what happens if a static UFunction is hooked since they don't have any context
        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
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

            for (Unreal::FProperty* func_prop : FunctionBeingExecuted->ForEachProperty())
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

        // No longer promising to be in the game thread
        set_is_in_game_thread(lua_data.lua, false);
    }

    static auto lua_unreal_script_function_hook_post(Unreal::UnrealScriptFunctionCallableContext context, void* custom_data) -> void
    {
        // Fetch the data corresponding to this UFunction
        auto& lua_data = *static_cast<LuaUnrealScriptFunctionData*>(custom_data);

        // This is a promise that we're in the game thread, used by other functions to ensure that we don't execute when unsafe
        set_is_in_game_thread(lua_data.lua, true);

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

                    std::wstring parameter_type_name = property_type_name.ToString();
                    std::wstring parameter_name = lua_data.return_property->GetName();

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
            static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
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
                for (Unreal::FProperty* func_prop : FunctionBeingExecuted->ForEachProperty())
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
                        // void* data = &context.TheStack.Locals[current_param_offset];
                        void* data = func_prop->ContainerPtrToValuePtr<void>(context.TheStack.Locals());

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

        // No longer promising to be in the game thread
        set_is_in_game_thread(lua_data.lua, false);
    }

    static auto register_input_globals(const LuaMadeSimple::Lua& lua) -> void
    {
        LuaMadeSimple::Lua::Table key_table = lua.prepare_new_table();
        key_table.add_pair("LEFT_MOUSE_BUTTON", 0x1);
        key_table.add_pair("RIGHT_MOUSE_BUTTON", 0x2);
        key_table.add_pair("CANCEL", 0x3);
        key_table.add_pair("MIDDLE_MOUSE_BUTTON", 0x4);
        key_table.add_pair("XBUTTON_ONE", 0x5);
        key_table.add_pair("XBUTTON_TWO", 0x6);
        key_table.add_pair("BACKSPACE", 0x8);
        key_table.add_pair("TAB", 0x9);
        key_table.add_pair("CLEAR", 0x0C);
        key_table.add_pair("RETURN", 0x0D);
        key_table.add_pair("PAUSE", 0x13);
        key_table.add_pair("CAPS_LOCK", 0x14);
        key_table.add_pair("IME_KANA", 0x15);
        key_table.add_pair("IME_HANGUEL", 0x15);
        key_table.add_pair("IME_HANGUL", 0x15);
        key_table.add_pair("IME_ON", 0x16);
        key_table.add_pair("IME_JUNJA", 0x17);
        key_table.add_pair("IME_FINAL", 0x18);
        key_table.add_pair("IME_HANJA", 0x19);
        key_table.add_pair("IME_KANJI", 0x19);
        key_table.add_pair("IME_OFF", 0x1A);
        key_table.add_pair("ESCAPE", 0x1B);
        key_table.add_pair("IME_CONVERT", 0x1C);
        key_table.add_pair("IME_NONCONVERT", 0x1D);
        key_table.add_pair("IME_ACCEPT", 0x1E);
        key_table.add_pair("IME_MODECHANGE", 0x1F);
        key_table.add_pair("SPACE", 0x20);
        key_table.add_pair("PAGE_UP", 0x21);
        key_table.add_pair("PAGE_DOWN", 0x22);
        key_table.add_pair("END", 0x23);
        key_table.add_pair("HOME", 0x24);
        key_table.add_pair("LEFT_ARROW", 0x25);
        key_table.add_pair("UP_ARROW", 0x26);
        key_table.add_pair("RIGHT_ARROW", 0x27);
        key_table.add_pair("DOWN_ARROW", 0x28);
        key_table.add_pair("SELECT", 0x29);
        key_table.add_pair("PRINT", 0x2A);
        key_table.add_pair("EXECUTE", 0x2B);
        key_table.add_pair("PRINT_SCREEN", 0x2C);
        key_table.add_pair("INS", 0x2D);
        key_table.add_pair("DEL", 0x2E);
        key_table.add_pair("HELP", 0x2F);
        key_table.add_pair("ZERO", 0x30);
        key_table.add_pair("ONE", 0x31);
        key_table.add_pair("TWO", 0x32);
        key_table.add_pair("THREE", 0x33);
        key_table.add_pair("FOUR", 0x34);
        key_table.add_pair("FIVE", 0x35);
        key_table.add_pair("SIX", 0x36);
        key_table.add_pair("SEVEN", 0x37);
        key_table.add_pair("EIGHT", 0x38);
        key_table.add_pair("NINE", 0x39);
        key_table.add_pair("A", 0x41);
        key_table.add_pair("B", 0x42);
        key_table.add_pair("C", 0x43);
        key_table.add_pair("D", 0x44);
        key_table.add_pair("E", 0x45);
        key_table.add_pair("F", 0x46);
        key_table.add_pair("G", 0x47);
        key_table.add_pair("H", 0x48);
        key_table.add_pair("I", 0x49);
        key_table.add_pair("J", 0x4A);
        key_table.add_pair("K", 0x4B);
        key_table.add_pair("L", 0x4C);
        key_table.add_pair("M", 0x4D);
        key_table.add_pair("N", 0x4E);
        key_table.add_pair("O", 0x4F);
        key_table.add_pair("P", 0x50);
        key_table.add_pair("Q", 0x51);
        key_table.add_pair("R", 0x52);
        key_table.add_pair("S", 0x53);
        key_table.add_pair("T", 0x54);
        key_table.add_pair("U", 0x55);
        key_table.add_pair("V", 0x56);
        key_table.add_pair("W", 0x57);
        key_table.add_pair("X", 0x58);
        key_table.add_pair("Y", 0x59);
        key_table.add_pair("Z", 0x5A);
        key_table.add_pair("LEFT_WIN", 0x5B);
        key_table.add_pair("RIGHT_WIN", 0x5C);
        key_table.add_pair("APPS", 0x5D);
        key_table.add_pair("SLEEP", 0x5F);
        key_table.add_pair("NUM_ZERO", 0x69);
        key_table.add_pair("NUM_ONE", 0x61);
        key_table.add_pair("NUM_TWO", 0x62);
        key_table.add_pair("NUM_THREE", 0x63);
        key_table.add_pair("NUM_FOUR", 0x64);
        key_table.add_pair("NUM_FIVE", 0x65);
        key_table.add_pair("NUM_SIX", 0x66);
        key_table.add_pair("NUM_SEVEN", 0x67);
        key_table.add_pair("NUM_EIGHT", 0x68);
        key_table.add_pair("NUM_NINE", 0x69);
        key_table.add_pair("MULTIPLY", 0x6A);
        key_table.add_pair("ADD", 0x6B);
        key_table.add_pair("SEPARATOR", 0x6C);
        key_table.add_pair("SUBTRACT", 0x6D);
        key_table.add_pair("DECIMAL", 0x6E);
        key_table.add_pair("DIVIDE", 0x6F);
        key_table.add_pair("F1", 0x70);
        key_table.add_pair("F2", 0x71);
        key_table.add_pair("F3", 0x72);
        key_table.add_pair("F4", 0x73);
        key_table.add_pair("F5", 0x74);
        key_table.add_pair("F6", 0x75);
        key_table.add_pair("F7", 0x76);
        key_table.add_pair("F8", 0x77);
        key_table.add_pair("F9", 0x78);
        key_table.add_pair("F10", 0x79);
        key_table.add_pair("F11", 0x7A);
        key_table.add_pair("F12", 0x7B);
        key_table.add_pair("F13", 0x7C);
        key_table.add_pair("F14", 0x7D);
        key_table.add_pair("F15", 0x7E);
        key_table.add_pair("F16", 0x7F);
        key_table.add_pair("F17", 0x80);
        key_table.add_pair("F18", 0x81);
        key_table.add_pair("F19", 0x82);
        key_table.add_pair("F20", 0x83);
        key_table.add_pair("F21", 0x84);
        key_table.add_pair("F22", 0x85);
        key_table.add_pair("F23", 0x86);
        key_table.add_pair("F24", 0x87);
        key_table.add_pair("NUM_LOCK", 0x90);
        key_table.add_pair("SCROLL_LOCK", 0x91);
        key_table.add_pair("BROWSER_BACK", 0xA6);
        key_table.add_pair("BROWSER_FORWARD", 0xA7);
        key_table.add_pair("BROWSER_REFRESH", 0xA8);
        key_table.add_pair("BROWSER_STOP", 0xA9);
        key_table.add_pair("BROWSER_SEARCH", 0xAA);
        key_table.add_pair("BROWSER_FAVORITES", 0xAB);
        key_table.add_pair("BROWSER_HOME", 0xAC);
        key_table.add_pair("VOLUME_MUTE", 0xAD);
        key_table.add_pair("VOLUME_DOWN", 0xAE);
        key_table.add_pair("VOLUME_UP", 0xAF);
        key_table.add_pair("MEDIA_NEXT_TRACK", 0xB0);
        key_table.add_pair("MEDIA_PREV_TRACK", 0xB1);
        key_table.add_pair("MEDIA_STOP", 0xB2);
        key_table.add_pair("MEDIA_PLAY_PAUSE", 0xB3);
        key_table.add_pair("LAUNCH_MAIL", 0xB4);
        key_table.add_pair("LAUNCH_MEDIA_SELECT", 0xB5);
        key_table.add_pair("LAUNCH_APP1", 0xB6);
        key_table.add_pair("LAUNCH_APP2", 0xB7);
        key_table.add_pair("OEM_ONE", 0xBA);
        key_table.add_pair("OEM_PLUS", 0xBB);
        key_table.add_pair("OEM_COMMA", 0xBC);
        key_table.add_pair("OEM_MINUS", 0xBD);
        key_table.add_pair("OEM_PERIOD", 0xBE);
        key_table.add_pair("OEM_TWO", 0xBF);
        key_table.add_pair("OEM_THREE", 0xC0);
        key_table.add_pair("OEM_FOUR", 0xDB);
        key_table.add_pair("OEM_FIVE", 0xDC);
        key_table.add_pair("OEM_SIX", 0xDD);
        key_table.add_pair("OEM_SEVEN", 0xDE);
        key_table.add_pair("OEM_EIGHT", 0xDF);
        key_table.add_pair("OEM_102", 0xE2);
        key_table.add_pair("IME_PROCESS", 0xE5);
        key_table.add_pair("PACKET", 0xE7);
        key_table.add_pair("ATTN", 0xF6);
        key_table.add_pair("CRSEL", 0xF7);
        key_table.add_pair("EXSEL", 0xF8);
        key_table.add_pair("EREOF", 0xF9);
        key_table.add_pair("PLAY", 0xFA);
        key_table.add_pair("ZOOM", 0xFB);
        key_table.add_pair("PA1", 0xFD);
        key_table.add_pair("OEM_CLEAR", 0xFE);
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

    LuaMod::LuaMod(UE4SSProgram& program, std::wstring&& mod_name, std::wstring&& mod_path)
        : Mod(program, std::move(mod_name), std::move(mod_path)), m_lua(LuaMadeSimple::new_state())
    {
        // Verify that there's a 'Scripts' directory
        // Give the full path to the 'Scripts' directory to the mod container
        m_scripts_path = m_mod_path + L"\\scripts";

        // If the 'Scripts' directory doesn't exist then mark the mod as non-installable and move on to the next mod
        if (!std::filesystem::exists(m_scripts_path))
        {
            set_installable(false);
            return;
        }
    }

    auto LuaMod::global_uninstall() -> void
    {
        LuaMod::m_static_construct_object_lua_callbacks.clear();
        LuaMod::m_process_console_exec_pre_callbacks.clear();
        LuaMod::m_process_console_exec_post_callbacks.clear();
        LuaMod::m_global_command_lua_callbacks.clear();
        LuaMod::m_custom_command_lua_pre_callbacks.clear();
        LuaMod::m_custom_event_callbacks.clear();
        LuaMod::m_load_map_pre_callbacks.clear();
        LuaMod::m_load_map_post_callbacks.clear();
        LuaMod::m_init_game_state_pre_callbacks.clear();
        LuaMod::m_init_game_state_post_callbacks.clear();
        LuaMod::m_begin_play_pre_callbacks.clear();
        LuaMod::m_begin_play_post_callbacks.clear();
        LuaMod::m_call_function_by_name_with_arguments_pre_callbacks.clear();
        LuaMod::m_call_function_by_name_with_arguments_post_callbacks.clear();
        LuaMod::m_local_player_exec_pre_callbacks.clear();
        LuaMod::m_local_player_exec_post_callbacks.clear();
        LuaMod::m_script_hook_callbacks.clear();
        LuaMod::m_generic_hook_id_to_native_hook_id.clear();
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

    // auto static make_hook_state(Mod* mod, const LuaMadeSimple::Lua& lua)->std::shared_ptr<LuaMadeSimple::Lua>
    auto static make_hook_state(LuaMod* mod) -> LuaMadeSimple::Lua*
    {
        // if (!mod->m_hook_lua)
        //{
        return mod->m_hook_lua.emplace_back(&mod->lua().new_thread());

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

        property_types_table.make_global("PropertyTypes");
    }

    auto LuaMod::setup_lua_require_paths(const LuaMadeSimple::Lua& lua) const -> void
    {
        auto* lua_state = m_lua.get_lua_state();
        lua_getglobal(lua_state, "package");

        lua_getfield(lua_state, -1, "path");
        std::string current_paths = lua_tostring(lua_state, -1);
        current_paths.append(fmt::format(";{}\\{}\\Scripts\\?.lua", to_string(m_program.get_mods_directory()).c_str(), to_string(get_name())));
        current_paths.append(fmt::format(";{}\\shared\\?.lua", to_string(m_program.get_mods_directory()).c_str()));
        current_paths.append(fmt::format(";{}\\shared\\?\\?.lua", to_string(m_program.get_mods_directory()).c_str()));
        lua_pop(lua_state, 1);
        lua_pushstring(lua_state, current_paths.c_str());
        lua_setfield(lua_state, -2, "path");

        lua_getfield(lua_state, -1, "cpath");
        std::string current_cpaths = lua_tostring(lua_state, -1);
        current_cpaths.append(fmt::format(";{}\\{}\\Scripts\\?.dll", to_string(m_program.get_mods_directory()).c_str(), to_string(get_name())));
        current_cpaths.append(fmt::format(";{}\\{}\\?.dll", to_string(m_program.get_mods_directory()).c_str(), to_string(get_name())));
        lua_pop(lua_state, 1);
        lua_pushstring(lua_state, current_cpaths.c_str());
        lua_setfield(lua_state, -2, "cpath");

        lua_pop(lua_state, 1);
    }

    auto static setup_lua_global_functions_internal(const LuaMadeSimple::Lua& lua, Mod::IsTrueMod is_true_mod) -> void
    {
        lua.register_function("print", LuaLibrary::global_print);

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
                Unreal::UObject* object = Unreal::UObjectGlobals::StaticFindObject(nullptr, nullptr, to_wstring(lua.get_string()));

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
            std::wstring param_name{};
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
                param_name = to_wstring(lua.get_string());
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
                Unreal::UObject* object = Unreal::UObjectGlobals::FindFirstOf(to_wstring(lua.get_string()));

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

                const Mod* mod = get_mod_ref(lua);

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
                        Output::send(STR("{}\n"), to_wstring(lua.handle_error(e.what())));
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
                            1);
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
                                1);
                    }
                    else
                    {
                        mod->m_program.register_keydown_event(
                                key_to_register,
                                [&lua, lua_callback_registry_index, &lua_keybind_callback_lambda]() {
                                    lua_keybind_callback_lambda(lua, lua_callback_registry_index);
                                },
                                1);
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

                const Mod* mod = get_mod_ref(lua);

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
                        Output::send(STR("{}\n"), to_wstring(lua.handle_error(e.what())));
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
                            1);
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
                                1);
                    }
                    else
                    {
                        mod->m_program.register_keydown_event(
                                key_to_register,
                                [&lua, lua_callback_registry_index, &lua_keybind_callback_lambda]() {
                                    lua_keybind_callback_lambda(lua, lua_callback_registry_index);
                                },
                                1);
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

                std::wstring function_name = to_wstring(lua.get_string());
                std::wstring function_name_no_prefix = function_name.substr(function_name.find_first_of(L" ") + 1, function_name.size());

                Unreal::UFunction* unreal_function = Unreal::UObjectGlobals::StaticFindObject<Unreal::UFunction*>(nullptr, nullptr, function_name_no_prefix);
                if (!unreal_function)
                {
                    lua.throw_error("Tried to unregister a hook with Lua function 'UnregisterHook' but no UFunction with the specified name was found.");
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
                    lua.throw_error("Tried to unregister a hook with Lua function 'UnregisterHook' but the PreCallbackId supplied was too large (>int32)");
                }

                if (post_id > std::numeric_limits<int32_t>::max())
                {
                    lua.throw_error("Tried to unregister a hook with Lua function 'UnregisterHook' but the PostCallbackId supplied was too large (>int32)");
                }

                // Hooks on native UFunctions will have both of these IDs.
                auto native_hook_pre_id_it = LuaMod::m_generic_hook_id_to_native_hook_id.find(static_cast<int32_t>(pre_id));
                auto native_hook_post_id_it = LuaMod::m_generic_hook_id_to_native_hook_id.find(static_cast<int32_t>(post_id));
                if (native_hook_pre_id_it != LuaMod::m_generic_hook_id_to_native_hook_id.end() &&
                    native_hook_post_id_it != LuaMod::m_generic_hook_id_to_native_hook_id.end())
                {
                    Output::send<LogLevel::Verbose>(STR("Unregistering native hook with pre-id: {}\n"), native_hook_pre_id_it->first);
                    unreal_function->UnregisterHook(static_cast<int32_t>(native_hook_pre_id_it->second));
                    Output::send<LogLevel::Verbose>(STR("Unregistering native hook with post-id: {}\n"), native_hook_post_id_it->first);
                    unreal_function->UnregisterHook(static_cast<int32_t>(native_hook_post_id_it->second));

                    // LuaUnrealScriptFunctionData contains the hook's lua registry references, captured in RegisterHook in two different lua states.
                    for (auto hook_iter = g_hooked_script_function_data.begin(); hook_iter != g_hooked_script_function_data.end(); hook_iter++)
                    {
                        RC::LuaUnrealScriptFunctionData* hook_data = (*hook_iter).get();
                        if (hook_data->post_callback_id == post_id && hook_data->pre_callback_id == pre_id)
                        {
                            auto mod = get_mod_ref(lua);
                            luaL_unref(hook_data->lua.get_lua_state(), LUA_REGISTRYINDEX, hook_data->lua_callback_ref);
                            if (hook_data->lua_post_callback_ref != -1)
                            {
                                luaL_unref(hook_data->lua.get_lua_state(), LUA_REGISTRYINDEX, hook_data->lua_post_callback_ref);
                            }
                            luaL_unref(mod->lua().get_lua_state(), LUA_REGISTRYINDEX, hook_data->lua_thread_ref);
                            g_hooked_script_function_data.erase(hook_iter);
                            break;
                        }
                    }
                }
                else
                {
                    if (auto callback_data_it = LuaMod::m_script_hook_callbacks.find(unreal_function->GetFullName());
                        callback_data_it != LuaMod::m_script_hook_callbacks.end())
                    {
                        Output::send<LogLevel::Verbose>(STR("Unregistering script hook with id: {}\n"), post_id);
                        auto& registry_indexes = callback_data_it->second.registry_indexes;
                        std::erase_if(registry_indexes, [&](const auto& pair) -> bool {
                            return post_id == pair.second.identifier;
                        });
                    }
                }

                return 0;
            });

            lua.register_function("DumpAllObjects", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                const Mod* mod = get_mod_ref(lua);
                if (!mod)
                {
                    lua.throw_error("Could not dump objects and properties because the pointer to 'Mod' was nullptr");
                }
                UE4SSProgram::dump_all_objects_and_properties(mod->m_program.get_object_dumper_output_directory() + STR("\\") +
                                                              UE4SSProgram::m_object_dumper_file_name);
                return 0;
            });

            lua.register_function("GenerateSDK", []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
                const Mod* mod = get_mod_ref(lua);
                File::StringType working_dir{mod->m_program.get_working_directory()};
                mod->m_program.generate_cxx_headers(working_dir + STR("\\CXXHeaderDump"));
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
                std::wstring name{};
                PropertyTypeInfo type{}; // Figure out what to do here, it shouldn't be just a string
                std::wstring belongs_to_class{};
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
            property_info.name = to_wstring(lua_table.get_string_field("Name"));
            property_info.type.name = lua_table.get_table_field("Type").get_string_field("Name");
            property_info.type.size = verify_and_convert_int64_to_int32("Type", "Size");
            property_info.type.ffieldclass_pointer = reinterpret_cast<void*>(lua_table.get_table_field("Type").get_int_field("FFieldClassPointer"));
            property_info.type.static_pointer = reinterpret_cast<void*>(lua_table.get_table_field("Type").get_int_field("StaticPointer"));
            property_info.belongs_to_class = to_wstring(lua_table.get_string_field("BelongsToClass"));

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
                auto name = Unreal::FName(to_wstring(oi_property_name));
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
            printf_s("\tName: %S\n", property_info.name.c_str());
            printf_s("\tType {\n");
            printf_s("\t\tName: %s\n", property_info.type.name.data());
            printf_s("\t\tSize: 0x%X\n", property_info.type.size);
            printf_s("\t\tFFieldClassPointer: 0x%p\n", property_info.type.ffieldclass_pointer);
            printf_s("\t\tStaticPointer: 0x%p\n", property_info.type.static_pointer);
            printf_s("\t}\n");
            printf_s("\tBelongsToClass: %S\n", property_info.belongs_to_class.c_str());
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

            std::wstring class_name = to_wstring(lua.get_string());

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = make_hook_state(mod);

            // Duplicate the Lua function to the top of the stack for lua_xmove and luaL_ref
            lua_pushvalue(lua.get_lua_state(), 1);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            const auto func_ref = hook_lua->registry().make_ref();
            const auto thread_ref = mod->lua().registry().make_ref();

            Unreal::UClass* instance_of_class = Unreal::UObjectGlobals::StaticFindObject<Unreal::UClass*>(nullptr, nullptr, class_name);

            LuaMod::m_static_construct_object_lua_callbacks.emplace_back(LuaMod::LuaCancellableCallbackData{hook_lua, instance_of_class, func_ref, thread_ref});

            return 0;
        });

        lua.register_function("RegisterCustomEvent", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'RegisterCustomEvent'.
Overloads:
#1: RegisterCustomEvent(string EventName, LuaFunction Callback))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }

            std::wstring event_name = to_wstring(lua.get_string());

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = make_hook_state(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_custom_event_callbacks.emplace(
                    event_name,
                    LuaMod::LuaCallbackData{
                            .lua = lua,
                            .instance_of_class = nullptr,
                            .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, {lua_callback_registry_index}}},
                    });

            return 0;
        });

        lua.register_function("UnregisterCustomEvent", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'UnregisterCustomEvent'.
Overloads:
#1: UnregisterCustomEvent(string EventName))"};

            if (!lua.is_string())
            {
                lua.throw_error(error_overload_not_found);
            }
            auto custom_event_name = to_wstring(lua.get_string());

            LuaMod::m_custom_event_callbacks.erase(custom_event_name);

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
            auto hook_lua = make_hook_state(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the lua function (it also pops it off the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_load_map_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = lua,
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
            auto hook_lua = make_hook_state(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the lua function (it also pops it off the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_load_map_post_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = lua,
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
            auto hook_lua = make_hook_state(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_init_game_state_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = lua,
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
            auto hook_lua = make_hook_state(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_init_game_state_post_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = lua,
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
            auto hook_lua = make_hook_state(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_begin_play_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = lua,
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
            auto hook_lua = make_hook_state(mod);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const int32_t lua_callback_registry_index = hook_lua->registry().make_ref();

            LuaMod::m_begin_play_post_callbacks.emplace_back(LuaMod::LuaCallbackData{
                    .lua = lua,
                    .instance_of_class = nullptr,
                    .registry_indexes = {std::pair<const LuaMadeSimple::Lua*, LuaMod::LuaCallbackData::RegistryIndex>{&lua, lua_callback_registry_index}},
            });

            return 0;
        });

        lua.register_function("IterateGameDirectories", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IterateGameDirectories'.
Overloads:
#1: IterateGameDirectories()"};

            std::filesystem::path game_executable_directory = UE4SSProgram::get_program().get_game_executable_directory();
            auto game_content_dir = game_executable_directory.parent_path().parent_path() / "Content";
            if (!std::filesystem::exists(game_content_dir))
            {
                Output::send<LogLevel::Warning>(STR("IterateGameDirectories: Could not locate the root directory because the directory structure is unknown "
                                                    "(not <RootGamePath>/Game/Binaries/Win64)\n"));
                lua.set_nil();
                return 1;
            }

            auto game_name = game_executable_directory.parent_path().parent_path().stem();
            auto game_root_directory = game_executable_directory.parent_path().parent_path().parent_path();
            auto directories_table = lua.prepare_new_table();

            std::function<void(const std::filesystem::path&, LuaMadeSimple::Lua::Table&)> iterate_directory =
                    [&](const std::filesystem::path& directory, LuaMadeSimple::Lua::Table& current_directory_table) {
                        for (const auto& item : std::filesystem::directory_iterator(directory))
                        {
                            if (!item.is_directory())
                            {
                                continue;
                            }
                            auto path = item.path().stem();
                            current_directory_table.add_key(path == game_name ? "Game" : to_string(path.wstring()).c_str());
                            auto next_directory_table = lua.prepare_new_table();
                            iterate_directory(item.path(), next_directory_table);
                            current_directory_table.fuse_pair();
                        }

                        auto meta_table = lua.prepare_new_table();
                        meta_table.add_pair("__index", [](lua_State* lua_state) -> int {
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
                                    const auto current_path = std::string{lua.get_string()};
                                    auto files_table = lua.prepare_new_table();
                                    auto index = 1;
                                    for (const auto& item : std::filesystem::directory_iterator(current_path))
                                    {
                                        if (!item.is_directory())
                                        {
                                            files_table.add_key(index);
                                            auto file_table = lua.prepare_new_table();
                                            file_table.add_pair("__name", to_string(item.path().filename().wstring()).c_str());
                                            file_table.add_pair("__absolute_path", to_string(item.path().wstring()).c_str());
                                            files_table.fuse_pair();
                                        }
                                        ++index;
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
                        meta_table.add_pair("__name", to_string(directory.stem().wstring()).c_str());
                        meta_table.add_pair("__absolute_path", to_string(directory.wstring()).c_str());
                        lua_setmetatable(lua.get_lua_state(), -2);
                    };

            iterate_directory(game_root_directory, directories_table);
            return 1;
        });

        lua.register_function("CreateLogicModsDirectory", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'CreateLogicModsDirectory'.
Overloads:
#1: CreateLogicModsDirectory()"};

            std::filesystem::path game_executable_directory = UE4SSProgram::get_program().get_game_executable_directory();
            auto game_content_dir = game_executable_directory.parent_path().parent_path() / "Content";
            if (!std::filesystem::exists(game_content_dir))
            {
                lua.throw_error("CreateLogicModsDirectory: Could not locate the \"Content\" directory because the directory structure is unknown (not "
                                "<RootGamePath>/Game/Content)\n");
            }
            auto logic_mods_dir = game_content_dir / "Paks/LogicMods";
            if (std::filesystem::exists(logic_mods_dir))
            {
                Output::send<LogLevel::Warning>(
                        STR("CreateLogicModsDirectory: \"LogicMods\" directory already exists. Cancelling creation of new directory.\n"));
                lua.set_bool(false);
                return 1;
            }

            Output::send<LogLevel::Warning>(STR("CreateLogicModsDirectory: LogicMods directory not found. Creating LogicMods directory.\n"));
            auto new_logic_mods_dir = std::filesystem::create_directory(logic_mods_dir);
            if (!new_logic_mods_dir)
            {
                lua.throw_error("CreateLogicModsDirectory: Unable to create \"LogicMods\" directory. Try creating manually.\n");
            }

            Output::send<LogLevel::Warning>(STR("CreateLogicModsDirectory: LogicMods directory created.\n"));

            lua.set_bool(true);
            return 1;
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
            auto hook_lua = make_hook_state(mod);
            callback = &LuaMod::m_process_console_exec_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{*hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua.get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua.registry().make_ref();
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
            auto hook_lua = make_hook_state(mod);
            callback = &LuaMod::m_process_console_exec_post_callbacks.emplace_back(LuaMod::LuaCallbackData{*hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua.get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua.registry().make_ref();
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
            auto hook_lua = make_hook_state(mod);
            callback = &LuaMod::m_call_function_by_name_with_arguments_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{*hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua.get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua.registry().make_ref();
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
            auto hook_lua = make_hook_state(mod);
            callback = &LuaMod::m_call_function_by_name_with_arguments_post_callbacks.emplace_back(LuaMod::LuaCallbackData{*hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua.get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua.registry().make_ref();
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
            auto hook_lua = make_hook_state(mod);
            callback = &LuaMod::m_local_player_exec_pre_callbacks.emplace_back(LuaMod::LuaCallbackData{*hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua.get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua.registry().make_ref();
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
            auto hook_lua = make_hook_state(mod);
            callback = &LuaMod::m_local_player_exec_post_callbacks.emplace_back(LuaMod::LuaCallbackData{*hook_lua, nullptr, {}});
            lua_xmove(lua.get_lua_state(), callback->lua.get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua.registry().make_ref();
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
            auto command_name = to_wstring(lua.get_string());

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto iter = LuaMod::m_global_command_lua_callbacks.find(command_name);
            if (iter == LuaMod::m_global_command_lua_callbacks.end())
            {
                auto mod = get_mod_ref(lua);
                auto hook_lua = make_hook_state(mod);
                callback = &LuaMod::m_global_command_lua_callbacks.emplace(command_name, LuaMod::LuaCallbackData{*hook_lua, nullptr, {}}).first->second;
            }
            else
            {
                callback = &iter->second;
            }
            lua_xmove(lua.get_lua_state(), callback->lua.get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua.registry().make_ref();
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
            auto command_name = to_wstring(lua.get_string());

            if (!lua.is_function())
            {
                throw std::runtime_error{error_overload_not_found};
            }

            LuaMod::LuaCallbackData* callback = nullptr;
            auto iter = LuaMod::m_custom_command_lua_pre_callbacks.find(command_name);
            if (iter == LuaMod::m_custom_command_lua_pre_callbacks.end())
            {
                auto mod = get_mod_ref(lua);
                auto hook_lua = make_hook_state(mod);
                callback = &LuaMod::m_custom_command_lua_pre_callbacks.emplace(command_name, LuaMod::LuaCallbackData{*hook_lua, nullptr, {}}).first->second;
            }
            else
            {
                callback = &iter->second;
            }
            lua_xmove(lua.get_lua_state(), callback->lua.get_lua_state(), 1);
            const int32_t lua_function_ref = callback->lua.registry().make_ref();
            callback->registry_indexes.emplace_back(&lua, LuaMod::LuaCallbackData::RegistryIndex{lua_function_ref});
            return 0;
        });

        lua.register_function("LoadAsset", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'LoadAsset'.
Overloads:
#1: LoadAsset(string AssetPathAndName))"};

            if (!is_in_game_thread(lua))
            {
                throw std::runtime_error{"Function 'LoadAsset' can only be called from within the game thread"};
            }

            if (!lua.is_string())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            auto asset_path_and_name = Unreal::FName(to_wstring(lua.get_string()), Unreal::FNAME_Add);

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
#1: FindObject(UClass InClass, UObject InOuter, string Name, bool ExactClass)
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
                object_class_name = Unreal::FName(to_wstring(lua.get_string()), Unreal::FNAME_Add);
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
                object_short_name = Unreal::FName(to_wstring(lua.get_string()), Unreal::FNAME_Add);
                could_be_object_short_name = true;
            }
            else if (lua.is_userdata())
            {
                // The API is a bit awkward, we have to tell it to preserve the stack
                // That way, when we call 'get_userdata' again with a more specific type, there's still something to actually get
                auto& userdata = lua.get_userdata<LuaType::UE4SSBaseObject>(1, true);
                std::string_view lua_object_name = userdata.get_object_name();
                // TODO: Redo when there's a bette way of checking whether a lua object is derived from UObject
                if (lua_object_name == "UObject" || lua_object_name == "UWorld" || lua_object_name == "AActor")
                {
                    in_outer = lua.get_userdata<LuaType::UObject>().get_remote_cpp_object();
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
                LuaType::auto_construct_object(lua, Unreal::UObjectGlobals::FindObject(in_class, in_outer, to_wstring(in_name), exact_class));
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
                object_class_name = Unreal::FName(to_wstring(lua.get_string()), Unreal::FNAME_Add);
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
                object_short_name = Unreal::FName(to_wstring(lua.get_string()), Unreal::FNAME_Add);
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
    }

    auto LuaMod::setup_lua_global_functions(const LuaMadeSimple::Lua& lua) const -> void
    {
        setup_lua_global_functions_internal(lua, IsTrueMod::Yes);
    }

    auto static process_event_hook([[maybe_unused]] Unreal::UObject* Context, [[maybe_unused]] Unreal::UFunction* Function, [[maybe_unused]] void* Parms) -> void
    {
        std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
        // NOTE: This will break horribly if UFunctions ever execute asynchronously.
        LuaMod::m_game_thread_actions.erase(std::remove_if(LuaMod::m_game_thread_actions.begin(),
                                                           LuaMod::m_game_thread_actions.end(),
                                                           [&](LuaMod::SimpleLuaAction& lua_data) -> bool {
                                                               if (LuaMod::m_is_currently_executing_game_action)
                                                               {
                                                                   // We can only execute one action per frame so we'll have to wait until the next frame.
                                                                   return false;
                                                               }

                                                               // This is a promise that we're in the game thread, used by other functions to ensure that we don't execute when unsafe
                                                               set_is_in_game_thread(*lua_data.lua, true);
                                                               LuaMod::m_is_currently_executing_game_action = true;

                                                               lua_data.lua->registry().get_function_ref(lua_data.lua_action_function_ref);

                                                               TRY([&]() {
                                                                   lua_data.lua->call_function(0, 0);
                                                               });

                                                               // thread_ref came from lua_newthread, we can let it GC now.
                                                               luaL_unref(lua_data.lua->get_lua_state(), LUA_REGISTRYINDEX, lua_data.lua_action_thread_ref);

                                                               // No longer promising to be in the game thread
                                                               set_is_in_game_thread(*lua_data.lua, false);
                                                               LuaMod::m_is_currently_executing_game_action = false;
                                                               return true;
                                                           }),
                                            LuaMod::m_game_thread_actions.end());
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

            std::wstring function_name = to_wstring(lua.get_string());
            std::wstring function_name_no_prefix = function_name.substr(function_name.find_first_of(L"/"), function_name.size());

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = make_hook_state(mod); // operates on LuaMod::m_lua incrementing its stack via lua_newthread

            // Duplicate the Lua function to the top of the stack for lua_xmove and luaL_ref
            lua_pushvalue(lua.get_lua_state(), 1); // operates on LuaMadeSimple::Lua::m_lua_state
            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            // Take a reference to the Lua function (it also pops it of the stack)
            const auto lua_callback_registry_index = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);
            const auto lua_thread_registry_index = luaL_ref(mod->lua().get_lua_state(), LUA_REGISTRYINDEX);

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
                lua.throw_error("Tried to register a hook with Lua function 'RegisterHook' but no UFunction with the specified name was found.");
            }

            int32_t generic_pre_id{};
            int32_t generic_post_id{};

            auto func_ptr = unreal_function->GetFunc();
            if (func_ptr && func_ptr != Unreal::UObject::ProcessInternalInternal.get_function_address() &&
                unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))
            {
                auto& custom_data = g_hooked_script_function_data.emplace_back(std::make_unique<LuaUnrealScriptFunctionData>(
                        LuaUnrealScriptFunctionData{0, 0, unreal_function, mod, *hook_lua, lua_callback_registry_index, lua_post_callback_registry_index, lua_thread_registry_index}));
                auto pre_id = unreal_function->RegisterPreHook(&lua_unreal_script_function_hook_pre, custom_data.get());
                auto post_id = unreal_function->RegisterPostHook(&lua_unreal_script_function_hook_post, custom_data.get());
                custom_data->pre_callback_id = pre_id;
                custom_data->post_callback_id = post_id;
                m_generic_hook_id_to_native_hook_id.emplace(++m_last_generic_hook_id, pre_id);
                generic_pre_id = m_last_generic_hook_id;
                m_generic_hook_id_to_native_hook_id.emplace(++m_last_generic_hook_id, post_id);
                generic_post_id = m_last_generic_hook_id;
                Output::send<LogLevel::Verbose>(STR("[RegisterHook] Registered native hook ({}, {}) for {}\n"),
                                                generic_pre_id,
                                                generic_post_id,
                                                unreal_function->GetFullName());
            }
            else if (func_ptr && func_ptr == Unreal::UObject::ProcessInternalInternal.get_function_address() &&
                     !unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))
            {
                ++m_last_generic_hook_id;
                auto [callback_data, _] = LuaMod::m_script_hook_callbacks.emplace(unreal_function->GetFullName(), LuaCallbackData{*hook_lua, nullptr, {}});
                callback_data->second.registry_indexes.emplace_back(hook_lua,
                                                                    LuaMod::LuaCallbackData::RegistryIndex{lua_callback_registry_index, m_last_generic_hook_id});
                generic_pre_id = m_last_generic_hook_id;
                generic_post_id = m_last_generic_hook_id;
                Output::send<LogLevel::Verbose>(STR("[RegisterHook] Registered script hook ({}, {}) for {}\n"),
                                                generic_pre_id,
                                                generic_post_id,
                                                unreal_function->GetFullName());
            }
            else
            {
                std::string error_message{"Was unable to register a hook with Lua function 'RegisterHook', information:\n"};
                error_message.append(fmt::format("UFunction::Func: {}\n", std::bit_cast<void*>(func_ptr)));
                error_message.append(fmt::format("ProcessInternal: {}\n", Unreal::UObject::ProcessInternalInternal.get_function_address()));
                error_message.append(
                        fmt::format("FUNC_Native: {}\n", static_cast<uint32_t>(unreal_function->HasAnyFunctionFlags(Unreal::EFunctionFlags::FUNC_Native))));
                lua.throw_error(error_message);
            }

            lua.set_integer(generic_pre_id);
            lua.set_integer(generic_post_id);

            return 2;
        });

        m_lua.register_function("ExecuteInGameThread", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ExecuteInGameThread'.
Overloads:
#1: ExecuteInGameThread(LuaFunction callback))"};

            if (!lua.is_function())
            {
                lua.throw_error(error_overload_not_found);
            }

            auto mod = get_mod_ref(lua);
            auto hook_lua = make_hook_state(mod);

            // Duplicate the Lua function to the top of the stack for lua_xmove and luaL_ref
            lua_pushvalue(lua.get_lua_state(), 1);

            lua_xmove(lua.get_lua_state(), hook_lua->get_lua_state(), 1);

            const auto func_ref = luaL_ref(hook_lua->get_lua_state(), LUA_REGISTRYINDEX);
            const auto thread_ref = luaL_ref(mod->lua().get_lua_state(), LUA_REGISTRYINDEX);
            SimpleLuaAction simpleAction{hook_lua, func_ref, thread_ref};
            {
                std::lock_guard<std::recursive_mutex> guard{LuaMod::m_thread_actions_mutex};
                mod->m_game_thread_actions.emplace_back(simpleAction);
            }

            if (!mod->m_is_process_event_hooked)
            {
                mod->m_is_process_event_hooked = true; // below can't fail, we don't want infinite processevent hooks.
                Unreal::Hook::RegisterProcessEventPreCallback(&process_event_hook);
            }

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

            File::StringType PossiblyLongName = to_wstring(lua.get_string());
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

            File::StringType InLongPackageName = to_wstring(lua.get_string());
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

    auto LuaMod::fire_on_lua_start_for_cpp_mods() -> void
    {
        if (!is_started())
        {
            return;
        }

        for (const auto& mod : UE4SSProgram::m_mods)
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

        for (const auto& mod : UE4SSProgram::m_mods)
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

    auto LuaMod::start_mod() -> void
    {
        prepare_mod(lua());
        make_main_state(this, lua());
        setup_lua_global_functions_main_state_only();

        make_async_state(this, lua());

        start_async_thread();

        m_is_started = true;

        fire_on_lua_start_for_cpp_mods();

        // Don't crash on syntax errors.
        try
        {
            main_lua()->execute_file(m_scripts_path + L"\\main.lua");
        }
        catch (std::runtime_error& e)
        {
            Output::send<LogLevel::Error>(STR("{}\n"), to_wstring(e.what()));
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

        if (m_hook_lua.size() > 0)
        {
            m_hook_lua.clear(); // lua_newthread results are handled by lua GC
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

        auto execute_hook = [&](std::unordered_map<StringType, LuaMod::LuaCallbackData>& callback_container, bool precise_name_match) {
            if (callback_container.empty())
            {
                return;
            }
            if (auto it = callback_container.find(precise_name_match ? Stack.Node()->GetFullName() : Stack.Node()->GetName()); it != callback_container.end())
            {
                const auto& callback_data = it->second;
                for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                {
                    const auto& lua = *lua_ptr;

                    set_is_in_game_thread(lua, true);

                    lua.registry().get_function_ref(registry_index.lua_index);

                    static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
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
                        for (Unreal::FProperty* param : node->ForEachProperty())
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
                    if (has_return_value && RESULT_DECL && lua.get_stack_size() > 0)
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
                                std::wstring return_property_type_name = return_property_type.ToString();
                                std::wstring return_property_name = return_property->GetName();

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

                    set_is_in_game_thread(lua, false);
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
                    for (const auto& callback_data : m_load_map_pre_callbacks)
                    {
                        std::pair<bool, bool> return_value{};

                        for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                        {
                            const auto& lua = *lua_ptr;

                            lua.registry().get_function_ref(registry_index.lua_index);
                            static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                            LuaType::RemoteUnrealParam::construct(lua, &Engine, s_object_property_name);
                            LuaType::RemoteUnrealParam::construct(lua, WorldContext.GetThisCurrentWorld().UnderlyingObjectPointer, s_object_property_name);
                            LuaType::FURL::construct(lua, URL);
                            LuaType::RemoteUnrealParam::construct(lua, PendingGame, s_object_property_name);
                            callback_data.lua.set_string(to_string(Error.GetCharArray()));
                            lua.call_function(4, 1);

                            if (callback_data.lua.is_nil())
                            {
                                return_value.first = false;
                                callback_data.lua.discard_value();
                            }
                            else if (!callback_data.lua.is_bool())
                            {
                                throw std::runtime_error{"A callback for 'LoadMap' must return bool or nil"};
                            }
                            else
                            {
                                return_value.first = true;
                                return_value.second = callback_data.lua.get_bool();
                            }
                        }

                        return return_value;
                    }
                    return std::pair<bool, bool>{false, false};
                });

        Unreal::Hook::RegisterLoadMapPostCallback(
                [](Unreal::UEngine* Engine, Unreal::FWorldContext& WorldContext, Unreal::FURL URL, Unreal::UPendingNetGame* PendingGame, Unreal::FString& Error)
                        -> std::pair<bool, bool> {
                    for (const auto& callback_data : m_load_map_post_callbacks)
                    {
                        std::pair<bool, bool> return_value{};

                        for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                        {
                            const auto& lua = *lua_ptr;

                            lua.registry().get_function_ref(registry_index.lua_index);
                            static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                            LuaType::RemoteUnrealParam::construct(lua, &Engine, s_object_property_name);
                            LuaType::RemoteUnrealParam::construct(lua, &WorldContext.GetThisCurrentWorld().UnderlyingObjectPointer, s_object_property_name);
                            LuaType::FURL::construct(lua, URL);
                            LuaType::RemoteUnrealParam::construct(lua, &PendingGame, s_object_property_name);
                            callback_data.lua.set_string(to_string(Error.GetCharArray()));
                            lua.call_function(5, 1);

                            if (callback_data.lua.is_nil())
                            {
                                return_value.first = false;
                                callback_data.lua.discard_value();
                            }
                            else if (!callback_data.lua.is_bool())
                            {
                                throw std::runtime_error{"A callback for 'LoadMap' must return bool or nil"};
                            }
                            else
                            {
                                return_value.first = true;
                                return_value.second = callback_data.lua.get_bool();
                            }
                        }

                        return return_value;
                    }
                    return std::pair<bool, bool>{false, false};
                });

        Unreal::Hook::RegisterInitGameStatePreCallback([]([[maybe_unused]] Unreal::AGameModeBase* Context) {
            TRY([&] {
                for (const auto& callback_data : m_init_game_state_pre_callbacks)
                {
                    // set_is_in_game_thread(lua, true);

                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        lua.call_function(1, 0);
                    }

                    // set_is_in_game_thread(lua, false);
                }
            });
        });

        Unreal::Hook::RegisterInitGameStatePostCallback([]([[maybe_unused]] Unreal::AGameModeBase* Context) {
            TRY([&] {
                for (const auto& callback_data : m_init_game_state_post_callbacks)
                {
                    // set_is_in_game_thread(lua, true);

                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        lua.call_function(1, 0);
                    }

                    // set_is_in_game_thread(lua, false);
                }
            });
        });

        Unreal::Hook::RegisterBeginPlayPreCallback([]([[maybe_unused]] Unreal::AActor* Context) {
            TRY([&] {
                for (const auto& callback_data : m_begin_play_pre_callbacks)
                {
                    // set_is_in_game_thread(lua, true);

                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        lua.call_function(1, 0);
                    }

                    // set_is_in_game_thread(lua, false);
                }
            });
        });

        Unreal::Hook::RegisterBeginPlayPostCallback([]([[maybe_unused]] Unreal::AActor* Context) {
            TRY([&] {
                for (const auto& callback_data : m_begin_play_post_callbacks)
                {
                    // set_is_in_game_thread(lua, true);

                    for (const auto& [lua_ptr, registry_index] : callback_data.registry_indexes)
                    {
                        const auto& lua = *lua_ptr;

                        lua.registry().get_function_ref(registry_index.lua_index);
                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                        LuaType::RemoteUnrealParam::construct(lua, &Context, s_object_property_name);
                        lua.call_function(1, 0);
                    }

                    // set_is_in_game_thread(lua, false);
                }
            });
        });

        Unreal::Hook::RegisterStaticConstructObjectPostCallback([](const Unreal::FStaticConstructObjectParameters&, Unreal::UObject* constructed_object) {
            Unreal::UStruct* object_class = constructed_object->GetClassPrivate();
            while (object_class)
            {
                std::erase_if(m_static_construct_object_lua_callbacks, [&](auto& callback_data) -> bool {
                    bool cancel = false;
                    if (callback_data.instance_of_class == object_class)
                    {
                        try
                        {
                            callback_data.lua->registry().get_function_ref(callback_data.lua_callback_function_ref);
                            LuaType::auto_construct_object(*callback_data.lua, constructed_object);
                            callback_data.lua->call_function(1, 1);

                            cancel = callback_data.lua->is_bool(-1) && callback_data.lua->get_bool(-1);
                        }
                        catch (std::runtime_error& e)
                        {
                            Output::send(STR("{}\n"), to_wstring(e.what()));
                        }

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

        Unreal::Hook::RegisterULocalPlayerExecPreCallback([](Unreal::ULocalPlayer* context, Unreal::UWorld* in_world, const TCHAR* cmd, Unreal::FOutputDevice& ar)
                                                                  -> Unreal::Hook::ULocalPlayerExecCallbackReturnValue {
            return TRY([&] {
                for (const auto& callback_data : m_local_player_exec_pre_callbacks)
                {
                    set_is_in_game_thread(callback_data.lua, true);

                    Unreal::Hook::ULocalPlayerExecCallbackReturnValue return_value{};

                    for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                    {
                        callback_data.lua.registry().get_function_ref(registry_index.lua_index);

                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                        LuaType::RemoteUnrealParam::construct(callback_data.lua, &context, s_object_property_name);
                        LuaType::RemoteUnrealParam::construct(callback_data.lua, &in_world, s_object_property_name);
                        callback_data.lua.set_string(to_string(cmd));
                        LuaType::FOutputDevice::construct(callback_data.lua, &ar);

                        callback_data.lua.call_function(4, 2);

                        if (callback_data.lua.is_nil())
                        {
                            return_value.UseOriginalReturnValue = true;
                            callback_data.lua.discard_value();
                        }
                        else if (!callback_data.lua.is_bool())
                        {
                            throw std::runtime_error{"The first return value for 'RegisterULocalPlayerExecPreHook' must return bool or nil"};
                        }
                        else
                        {
                            return_value.UseOriginalReturnValue = false;
                            return_value.NewReturnValue = callback_data.lua.get_bool();
                        }

                        if (callback_data.lua.is_nil())
                        {
                            return_value.ExecuteOriginalFunction = true;
                            callback_data.lua.discard_value();
                        }
                        else if (!callback_data.lua.is_bool())
                        {
                            throw std::runtime_error{"The second return value for callback 'RegisterULocalPlayerExecPreHook' must return bool or nil"};
                        }
                        else
                        {
                            return_value.ExecuteOriginalFunction = callback_data.lua.get_bool();
                        }
                    }

                    set_is_in_game_thread(callback_data.lua, false);

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
                    set_is_in_game_thread(callback_data.lua, true);

                    Unreal::Hook::ULocalPlayerExecCallbackReturnValue return_value{};

                    for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                    {
                        callback_data.lua.registry().get_function_ref(registry_index.lua_index);

                        static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                        LuaType::RemoteUnrealParam::construct(callback_data.lua, &context, s_object_property_name);
                        LuaType::RemoteUnrealParam::construct(callback_data.lua, &in_world, s_object_property_name);
                        callback_data.lua.set_string(to_string(cmd));
                        LuaType::FOutputDevice::construct(callback_data.lua, &ar);

                        callback_data.lua.call_function(4, 2);

                        if (callback_data.lua.is_nil())
                        {
                            return_value.UseOriginalReturnValue = true;
                            callback_data.lua.discard_value();
                        }
                        else if (!callback_data.lua.is_bool())
                        {
                            throw std::runtime_error{"A callback for 'RegisterULocalPlayerExecPreHook' must return bool or nil"};
                        }
                        else
                        {
                            return_value.UseOriginalReturnValue = false;
                            return_value.NewReturnValue = callback_data.lua.get_bool();
                        }

                        if (callback_data.lua.is_nil())
                        {
                            return_value.ExecuteOriginalFunction = true;
                            callback_data.lua.discard_value();
                        }
                        else if (!callback_data.lua.is_bool())
                        {
                            throw std::runtime_error{"The second return value for callback 'RegisterULocalPlayerExecPreHook' must return bool or nil"};
                        }
                        else
                        {
                            return_value.ExecuteOriginalFunction = callback_data.lua.get_bool();
                        }
                    }

                    set_is_in_game_thread(callback_data.lua, false);

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
                            set_is_in_game_thread(callback_data.lua, true);

                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                            {
                                callback_data.lua.registry().get_function_ref(registry_index.lua_index);

                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                                LuaType::RemoteUnrealParam::construct(callback_data.lua, &context, s_object_property_name);
                                callback_data.lua.set_string(to_string(str));
                                LuaType::FOutputDevice::construct(callback_data.lua, &ar);
                                LuaType::RemoteUnrealParam::construct(callback_data.lua, &executor, s_object_property_name);
                                callback_data.lua.set_bool(b_force_call_with_non_exec);

                                callback_data.lua.call_function(5, 1);

                                if (callback_data.lua.is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua.discard_value();
                                }
                                else if (!callback_data.lua.is_bool())
                                {
                                    throw std::runtime_error{"A callback for 'RegisterCallFunctionByNameWithArgumentsPreHook' must return bool or nil"};
                                }
                                else
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua.get_bool();
                                }
                            }

                            set_is_in_game_thread(callback_data.lua, false);

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
                            set_is_in_game_thread(callback_data.lua, true);

                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                            {
                                callback_data.lua.registry().get_function_ref(registry_index.lua_index);

                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                                LuaType::RemoteUnrealParam::construct(callback_data.lua, &context, s_object_property_name);
                                callback_data.lua.set_string(to_string(str));
                                LuaType::FOutputDevice::construct(callback_data.lua, &ar);
                                LuaType::RemoteUnrealParam::construct(callback_data.lua, &executor, s_object_property_name);
                                callback_data.lua.set_bool(b_force_call_with_non_exec);

                                callback_data.lua.call_function(5, 1);

                                if (callback_data.lua.is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua.discard_value();
                                }
                                else if (!callback_data.lua.is_bool())
                                {
                                    throw std::runtime_error{"A callback for 'RegisterCallFunctionByNameWithArgumentsPreHook' must return bool or nil"};
                                }
                                else
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua.get_bool();
                                }
                            }

                            set_is_in_game_thread(callback_data.lua, false);

                            return return_value;
                        }

                        return std::pair{false, false};
                    });
                });

        // Lua from the in-game console.
        Unreal::Hook::RegisterProcessConsoleExecCallback([](Unreal::UObject* context, const TCHAR* cmd, Unreal::FOutputDevice& ar, Unreal::UObject* executor) -> bool {
            auto logln = [&ar](const File::StringType& log_message) {
                Output::send(fmt::format(STR("{}\n"), log_message));
                ar.Log(log_message.c_str());
            };

            if (!LuaStatics::console_executor_enabled && String::iequal(File::StringViewType{cmd}, STR("luastart")))
            {
                start_console_lua_executor();
                logln(STR("Console Lua executor started"));
                return true;
            }
            else if (LuaStatics::console_executor_enabled && String::iequal(File::StringViewType{cmd}, STR("luastop")))
            {
                stop_console_lua_executor();
                logln(STR("Console Lua executor stopped"));
                return true;
            }
            else if (LuaStatics::console_executor_enabled && String::iequal(File::StringViewType{cmd}, STR("luarestart")))
            {
                stop_console_lua_executor();
                start_console_lua_executor();
                logln(STR("Console Lua executor restarted"));
                return true;
            }
            else if (String::iequal(File::StringViewType{cmd}, STR("clear")))
            {
                // TODO: Replace with proper implementation when we have UGameViewportClient and UConsole.
                //       This should be fairly cross-game & cross-engine-version compatible even without the proper implementation.
                //       This is because I don't think they've changed the layout here and we have a reflected property right before the unreflected one that we're looking for.
                Unreal::UObject** console = static_cast<Unreal::UObject**>(context->GetValuePtrByPropertyName(STR("ViewportConsole")));
                auto* default_texture_white = std::bit_cast<Unreal::TArray<Unreal::FString>*>(
                        static_cast<uint8_t*>((*console)->GetValuePtrByPropertyNameInChain(STR("DefaultTexture_White"))) + 0x8);
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
                    if (int status = luaL_loadstring(LuaStatics::console_executor->get_lua_state(), to_string(cmd).c_str()); status != LUA_OK)
                    {
                        LuaStatics::console_executor->throw_error(
                                fmt::format("luaL_loadstring returned {}", LuaStatics::console_executor->resolve_status_message(status, true)));
                    }

                    if (int status = lua_pcall(LuaStatics::console_executor->get_lua_state(), 0, LUA_MULTRET, 0); status != LUA_OK)
                    {
                        LuaStatics::console_executor->throw_error(
                                fmt::format("lua_pcall returned {}", LuaStatics::console_executor->resolve_status_message(status, true)));
                    }
                }
                catch (std::runtime_error& e)
                {
                    logln(to_wstring(e.what()));
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
                        auto command = File::StringViewType{cmd};
                        auto command_parts = explode_by_occurrence(cmd, ' ');

                        for (const auto& callback_data : m_process_console_exec_pre_callbacks)
                        {
                            set_is_in_game_thread(callback_data.lua, true);

                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                            {
                                callback_data.lua.registry().get_function_ref(registry_index.lua_index);

                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                                LuaType::RemoteUnrealParam::construct(callback_data.lua, &context, s_object_property_name);
                                callback_data.lua.set_string(to_string(command));
                                auto params_table = callback_data.lua.prepare_new_table();
                                for (size_t i = 1; i < command_parts.size(); ++i)
                                {
                                    const auto& command_part = command_parts[i];
                                    params_table.add_pair(i, to_string(command_part).c_str());
                                }
                                LuaType::FOutputDevice::construct(callback_data.lua, &ar);
                                LuaType::RemoteUnrealParam::construct(callback_data.lua, &executor, s_object_property_name);

                                callback_data.lua.call_function(5, 1);

                                if (callback_data.lua.is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua.discard_value();
                                }
                                else if (callback_data.lua.is_bool())
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua.get_bool();
                                }
                                else
                                {
                                    throw std::runtime_error{"A callback for 'RegisterProcessConsoleExecHook' must return bool or nil"};
                                }
                            }

                            set_is_in_game_thread(callback_data.lua, false);

                            return return_value;
                        }

                        return std::pair{false, false};
                    });
                });

        // RegisterProcessConsoleExecPostHook
        Unreal::Hook::RegisterProcessConsoleExecGlobalPostCallback(
                [](Unreal::UObject* context, const TCHAR* cmd, Unreal::FOutputDevice& ar, Unreal::UObject* executor) -> std::pair<bool, bool> {
                    return TRY([&] {
                        auto command = File::StringViewType{cmd};
                        auto command_parts = explode_by_occurrence(cmd, ' ');

                        for (const auto& callback_data : m_process_console_exec_post_callbacks)
                        {
                            set_is_in_game_thread(callback_data.lua, true);

                            std::pair<bool, bool> return_value{};

                            for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                            {
                                callback_data.lua.registry().get_function_ref(registry_index.lua_index);

                                static auto s_object_property_name = Unreal::FName(STR("ObjectProperty"));
                                LuaType::RemoteUnrealParam::construct(callback_data.lua, &context, s_object_property_name);
                                callback_data.lua.set_string(to_string(command));
                                auto params_table = callback_data.lua.prepare_new_table();
                                for (size_t i = 1; i < command_parts.size(); ++i)
                                {
                                    const auto& command_part = command_parts[i];
                                    params_table.add_pair(i, to_string(command_part).c_str());
                                }
                                LuaType::FOutputDevice::construct(callback_data.lua, &ar);
                                LuaType::RemoteUnrealParam::construct(callback_data.lua, &executor, s_object_property_name);

                                callback_data.lua.call_function(4, 1);

                                if (callback_data.lua.is_nil())
                                {
                                    return_value.first = false;
                                    callback_data.lua.discard_value();
                                }
                                else if (callback_data.lua.is_bool())
                                {
                                    return_value.first = true;
                                    return_value.second = callback_data.lua.get_bool();
                                }
                                else
                                {
                                    throw std::runtime_error{"A callback for 'RegisterProcessConsoleExecHook' must return bool or nil"};
                                }
                            }

                            set_is_in_game_thread(callback_data.lua, false);

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
                auto command = File::StringViewType{cmd};
                auto command_parts = explode_by_occurrence(cmd, ' ');
                File::StringType command_name;
                if (command_parts.size() > 1)
                {
                    command_name = command_parts[0];
                }
                else
                {
                    command_name = command;
                }

                if (auto it = m_custom_command_lua_pre_callbacks.find(command_name); it != m_custom_command_lua_pre_callbacks.end())
                {
                    const auto& callback_data = it->second;

                    // This is a promise that we're in the game thread, used by other functions to ensure that we don't execute when unsafe
                    set_is_in_game_thread(callback_data.lua, true);

                    bool return_value{};

                    for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                    {
                        callback_data.lua.registry().get_function_ref(registry_index.lua_index);
                        callback_data.lua.set_string(to_string(command));

                        auto params_table = callback_data.lua.prepare_new_table();
                        for (size_t i = 1; i < command_parts.size(); ++i)
                        {
                            const auto& command_part = command_parts[i];
                            params_table.add_pair(i, to_string(command_part).c_str());
                        }

                        LuaType::FOutputDevice::construct(callback_data.lua, &ar);

                        callback_data.lua.call_function(3, 1);

                        if (!callback_data.lua.is_bool())
                        {
                            throw std::runtime_error{"A custom console command handle must return true or false"};
                        }

                        return_value = callback_data.lua.get_bool();
                    }
                    // No longer promising to be in the game thread
                    set_is_in_game_thread(callback_data.lua, false);

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
                auto command = File::StringViewType{cmd};
                auto command_parts = explode_by_occurrence(cmd, ' ');
                File::StringType command_name;
                if (command_parts.size() > 1)
                {
                    command_name = command_parts[0];
                }
                else
                {
                    command_name = command;
                }

                if (auto it = m_global_command_lua_callbacks.find(command_name); it != m_global_command_lua_callbacks.end())
                {
                    const auto& callback_data = it->second;

                    // This is a promise that we're in the game thread, used by other functions to ensure that we don't execute when unsafe
                    set_is_in_game_thread(callback_data.lua, true);

                    bool return_value{};

                    for (const auto& [lua, registry_index] : callback_data.registry_indexes)
                    {
                        callback_data.lua.registry().get_function_ref(registry_index.lua_index);
                        callback_data.lua.set_string(to_string(command));

                        auto params_table = callback_data.lua.prepare_new_table();
                        for (size_t i = 1; i < command_parts.size(); ++i)
                        {
                            const auto& command_part = command_parts[i];
                            params_table.add_pair(i, to_string(command_part).c_str());
                        }

                        LuaType::FOutputDevice::construct(callback_data.lua, &ar);

                        callback_data.lua.call_function(3, 1);

                        if (!callback_data.lua.is_bool())
                        {
                            throw std::runtime_error{"A custom console command handle must return true or false"};
                        }

                        return_value = callback_data.lua.get_bool();
                    }

                    // No longer promising to be in the game thread
                    set_is_in_game_thread(callback_data.lua, false);

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
                                                                        to_wstring(action.type == LuaMod::ActionType::Loop ? "LoopAsync" : "DelayedAction"),
                                                                        to_wstring(e.what()));
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
