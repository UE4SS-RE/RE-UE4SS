#include <type_traits>
#include <functional>
#include <optional>

#include <Mod/SolMod.hpp>
#include <Helpers/String.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/World.hpp>
#include <Unreal/FOutputDevice.hpp>
#include <Unreal/TMap.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/AGameModeBase.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/Property/FSoftObjectProperty.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>

using namespace RC::Unreal;

namespace RC
{
    std::recursive_mutex SolMod::m_thread_actions_mutex{};
    std::vector<std::unique_ptr<LuaUnrealScriptFunctionData>> SolMod::m_hooked_script_function_data{};
    std::vector<LuaCallbackData> SolMod::m_static_construct_object_lua_callbacks{};
    std::unordered_map<StringType, LuaCallbackData> SolMod::m_custom_event_callbacks{};
    std::vector<LuaCallbackData> SolMod::m_init_game_state_pre_callbacks{};
    std::vector<LuaCallbackData> SolMod::m_init_game_state_post_callbacks{};
    std::unordered_map<StringType, LuaCallbackData> SolMod::m_script_hook_callbacks{};
    std::unordered_map<int32_t, int32_t> SolMod::m_generic_hook_id_to_native_hook_id{};
    int32_t SolMod::m_last_generic_hook_id{};

    auto ParamPtrWrapper::unwrap(lua_State* lua_state) -> void
    {
        property_pusher({lua_state, nullptr, value_ptr, PushType::ToLua, nullptr});
    }

    auto ParamPtrWrapper::rewrap(lua_State* lua_state) -> void
    {
        property_pusher({lua_state, nullptr, value_ptr, PushType::FromLua, nullptr});
    }

    static constexpr uint32_t g_max_property_pushers = 0xFF;
    static std::unordered_map<uint32_t, PropertyPusherFunction> g_property_pushers{};
    static auto lookup_property_pusher(FName name) -> PropertyPusherFunction
    {
        //auto comparison_index = name.GetComparisonIndex();
        //if (comparison_index > g_max_property_pushers) { return nullptr; }
        //return g_property_pushers[comparison_index];
        if (auto it = g_property_pushers.find(name.GetComparisonIndex()); it != g_property_pushers.end())
        {
            return it->second;
        }
        else
        {
            return nullptr;
        }
    }

    static bool is_mid_script_execution{};

    auto lua_unreal_script_function_hook_pre(UnrealScriptFunctionCallableContext context, void* custom_data) -> void
    {
        std::lock_guard<decltype(SolMod::m_thread_actions_mutex)> guard{SolMod::m_thread_actions_mutex};

        is_mid_script_execution = true;

        // Fetch the data corresponding to this UFunction
        auto& lua_data = *static_cast<LuaUnrealScriptFunctionData*>(custom_data);

        // This is a promise that we're in the game thread, used by other functions to ensure that we don't execute when unsafe
        //set_is_in_game_thread(lua_data.lua, true);

        // Storage for all UFunction parameters. This is passed to sol before the callback is called.
        std::vector<ParamPtrWrapper> param_wrappers{};

        // Set up the first param (context / this-ptr)
        push_objectproperty({lua_data.lua, nullptr, &context.Context, PushType::ToLuaParam, &param_wrappers});

        // Attempt at dynamically fetching the params
        uint16_t return_value_offset = context.TheStack.CurrentNativeFunction()->GetReturnValueOffset();

        // 'ReturnValueOffset' is 0xFFFF if the UFunction return type is void
        lua_data.has_return_value = return_value_offset != 0xFFFF;

        uint8_t num_unreal_params = context.TheStack.CurrentNativeFunction()->GetNumParms();
        if (lua_data.has_return_value)
        {
            // Subtract one from the number of params if there's a return value
            // This is because Unreal treats the return value as a param, and it's included in the 'NumParms' member variable
            --num_unreal_params;
        }

        bool has_properties_to_process = lua_data.has_return_value || num_unreal_params > 0;
        if (has_properties_to_process && context.TheStack.Locals())
        {
            //context.TheStack.CurrentNativeFunction()->ForEachProperty([&](Unreal::FProperty* param) {
            for (auto param : context.TheStack.CurrentNativeFunction()->ForEachProperty())
            {
                // Skip this property if it's not a parameter
                if (!param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_Parm))
                {
                    continue;
                }

                // Skip if this property corresponds to the return value
                if (lua_data.has_return_value && param->GetOffset_Internal() == return_value_offset)
                {
                    lua_data.return_property = param;
                    continue;
                }

                if (auto property_pusher = lookup_property_pusher(param->GetClass().GetFName()); property_pusher)
                {
                    void* data = param->ContainerPtrToValuePtr<void>(context.TheStack.Locals());
                    if (auto error_msg = property_pusher({lua_data.lua, nullptr, data, PushType::ToLuaParam, &param_wrappers}); error_msg)
                    {
                        EXIT_NATIVE_HOOK_WITH_ERROR(break, lua_data, std::format(STR("Error setting parameter value: {}\n"), to_wstring(error_msg)))
                    }
                }
                else
                {
                    EXIT_NATIVE_HOOK_WITH_ERROR(break, lua_data,
                        std::format(STR("[unreal_script_function_hook_pre] Tried accessing unreal property without a registered handler. Property type '{}' not supported.\n"),
                        param->GetClass().GetName()))
                }
            };
        }

        if (lua_data.function_processing_failed) { return; }

        // Call the Lua function with the correct number of parameters & return values
        call_function_dont_wrap_params_safe(lua_data.lua, lua_data.lua_callback, param_wrappers);
        /*
        auto function_name_only = context.TheStack.CurrentNativeFunction()->GetName();
        auto class_name = context.TheStack.CurrentNativeFunction()->GetOuterPrivate()->GetName();
        auto callback = sol::state_view{lua_data.lua}.get<sol::function>(std::format("__ue4ss_sol_{}_{}_callback_in_state", to_string(class_name), to_string(function_name_only)));
        callback(sol::as_args(param_wrappers));
        //*/

        // The params for the Lua script will be 'userdata' and they will have get/set functions
        // Use these functions in the Lua script to access & mutate the parameter values

        // After the Lua function has been executed you should call the original function
        // This will execute any internal UE4 scripting functions & native functions depending on the type of UFunction
        // The API will automatically call the original function
        // This function continues in 'lua_unreal_script_function_hook_post' which executes immediately after the original function gets called

        // No longer promising to be in the game thread
        //set_is_in_game_thread(lua_data.lua, false);
    }

    auto lua_unreal_script_function_hook_post(UnrealScriptFunctionCallableContext context, void* custom_data) -> void
    {
        std::lock_guard<decltype(SolMod::m_thread_actions_mutex)> guard{SolMod::m_thread_actions_mutex};

        // Fetch the data corresponding to this UFunction
        auto& lua_data = *static_cast<LuaUnrealScriptFunctionData*>(custom_data);

        if (lua_data.function_processing_failed) { return; }

        // This is a promise that we're in the game thread, used by other functions to ensure that we don't execute when unsafe
        //set_is_in_game_thread(lua_data.lua, true);

        // Fetch & override the return value from Lua if one exists
        if (lua_data.has_return_value && lua_data.lua_callback_result.valid() && lua_data.lua_callback_result.return_count() > 0)
        {
            if (auto property_pusher = lookup_property_pusher(lua_data.return_property->GetClass().GetFName()); property_pusher)
            {
                // Keep in mind that the if this was a Blueprint UFunction then the entire byte-code will already have executed
                // That means that changing the return value here won't affect the script itself
                // If this was a native UFunction then changing the return value here will have the desired effect
                if (auto error_msg = property_pusher({lua_data.lua, &lua_data.lua_callback_result, context.RESULT_DECL, PushType::FromLua, nullptr}); error_msg)
                {
                    EXIT_NATIVE_HOOK_WITH_ERROR(void, lua_data, std::format(STR("Error setting return value: {}\n"), to_wstring(error_msg)))
                }
            }
            else
            {
                // If the type wasn't supported then we simply output a warning and then do nothing
                auto parameter_type_name = lua_data.return_property->GetClass().GetName();
                auto parameter_name = lua_data.return_property->GetName();
                Output::send(STR("Tried altering return value of a hooked UFunction without a registered handler for return type Return property '{}' of type '{}' not supported.\n"), parameter_name, parameter_type_name);
            }
        }

        // No longer promising to be in the game thread
        //set_is_in_game_thread(lua_data.lua, false);

        is_mid_script_execution = false;
    }

    static auto script_hook([[maybe_unused]] UObject* Context, FFrame& Stack, [[maybe_unused]] void* RESULT_DECL) -> void
    {
        std::lock_guard<decltype(SolMod::m_thread_actions_mutex)> guard{SolMod::m_thread_actions_mutex};
        auto execute_hook = [&](std::unordered_map<StringType, LuaCallbackData>& callback_container, bool precise_name_match) {
            if (callback_container.empty()) { return; }

            bool exit_early{};
            if (auto it = callback_container.find(precise_name_match ? Stack.Node()->GetFullName() : Stack.Node()->GetName()); it != callback_container.end())
            {
                const auto& callback_data = it->second;
                for (const auto& registry_index : callback_data.registry_indexes)
                {
                    sol::state_view lua = callback_data.lua;

                    //set_is_in_game_thread(lua, true);

                    std::vector<ParamPtrWrapper> param_wrappers{};
                    push_objectproperty({lua, nullptr, &Context, PushType::ToLuaParam, &param_wrappers});

                    auto node = Stack.Node();
                    auto return_value_offset = node->GetReturnValueOffset();
                    auto has_return_value = return_value_offset != 0xFFFF;
                    auto num_unreal_params = node->GetNumParms();
                    if (has_return_value && num_unreal_params > 0) { --num_unreal_params; }

                    if (has_return_value || num_unreal_params > 0)
                    {
                        for (auto param : node->ForEachProperty())
                        {
                            if (!param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_Parm)) { continue; }
                            if (has_return_value && param->GetOffset_Internal() == return_value_offset) { continue; }

                            auto param_type = param->GetClass().GetFName();
                            if (auto property_pusher = lookup_property_pusher(param->GetClass().GetFName()); property_pusher)
                            {
                                void* data = param->ContainerPtrToValuePtr<void>(Stack.Locals());
                                if (auto error_msg = property_pusher({lua, nullptr, data, PushType::ToLuaParam, &param_wrappers}); error_msg)
                                {
                                    Output::send<LogLevel::Error>(STR("Error setting parameter value: {}\n"), to_wstring(error_msg));
                                }
                            }
                            else
                            {
                                Output::send<LogLevel::Error>(STR("[script_hook] Tried accessing unreal property without a registered handler. Property type '{}' not supported.\n"),
                                    param_type.ToString());
                                exit_early = true;
                                break;
                            }
                        };
                    }

                    if (exit_early) { break; }

                    auto lua_callback_result = call_function_dont_wrap_params_safe(lua, registry_index.lua_callback);

                    if (has_return_value && RESULT_DECL && lua_callback_result.valid() && lua_callback_result.return_count() > 0)
                    {
                        auto return_property = node->GetReturnProperty();
                        if (return_property)
                        {
                            if (auto property_pusher = lookup_property_pusher(return_property->GetClass().GetFName()); property_pusher)
                            {
                                // Keep in mind that the if this was a Blueprint UFunction then the entire byte-code will already have executed
                                // That means that changing the return value here won't affect the script itself
                                // If this was a native UFunction then changing the return value here will have the desired effect
                                if (auto error_msg = property_pusher({lua, &lua_callback_result, RESULT_DECL, PushType::FromLua, nullptr}); error_msg)
                                {
                                    Output::send<LogLevel::Error>(STR("Error setting return value: {}\n"), to_wstring(error_msg));
                                }
                            }
                            else
                            {
                                // If the type wasn't supported then we simply output a warning and then do nothing
                                auto return_property_type_name = return_property->GetClass().GetName();
                                auto return_property_name = return_property->GetName();
                                Output::send(STR("Tried altering return value of a custom BP function without a registered handler for return type Return property '{}' of type '{}' not supported.\n"), return_property_name, return_property_type_name);
                            }
                        }

                        //set_is_in_game_thread(lua, false);
                    }
                }
            }
        };
        
        execute_hook(SolMod::m_custom_event_callbacks, false);
        execute_hook(SolMod::m_script_hook_callbacks, true);
    }

    auto SolMod::process_delayed_actions() -> void
    {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

        {
            std::lock_guard<decltype(m_actions_lock)> guard{m_actions_lock};
            m_delayed_actions.insert(m_delayed_actions.end(), std::make_move_iterator(m_pending_actions.begin()), std::make_move_iterator(m_pending_actions.end()));
            m_pending_actions.clear();
        }
        
        m_delayed_actions.erase(std::remove_if(m_delayed_actions.begin(), m_delayed_actions.end(),
            [&](AsyncAction& action) -> bool {
                auto passed = now - std::chrono::duration_cast<std::chrono::milliseconds>(action.created_at.time_since_epoch()).count();
                auto duration_since_creation = (action.type == ActionType::Immediate || passed >= action.delay);
                if (duration_since_creation)
                {
                    std::lock_guard<decltype(SolMod::m_thread_actions_mutex)> guard{SolMod::m_thread_actions_mutex};
                    bool result = true;
                    try
                    {
                        if (action.type == ActionType::Loop)
                        {
                            auto lua_callback_result = call_function_dont_wrap_params_safe(sol(State::Async), action.lua_action_function);
                            if (lua_callback_result.valid() && lua_callback_result.return_count() > 0)
                            {
                                auto maybe_result = lua_callback_result.get<std::optional<bool>>();
                                if (maybe_result.has_value()) { result = maybe_result.value(); }
                            }
                            action.created_at = std::chrono::steady_clock::now();
                        }
                        else
                        {
                            call_function_dont_wrap_params_safe(sol(State::Async), action.lua_action_function);
                        }
                    }
                    catch (std::runtime_error& e)
                    {
                        Output::send(STR("[{}] {}\n"), to_wstring(action.type == ActionType::Loop ? "LoopAsync" : "DelayedAction"), to_wstring(e.what()));
                    }

                    return result;
                }
                else
                {
                    return false;
                }
            }), m_delayed_actions.end());
    }

    auto SolMod::sol(State state) -> sol::state_view
    {
        if (state == State::UE4SS)
        {
            //if (!m_sol_state_ue4ss)
            //{
            //    Output::send<LogLevel::Error>(STR("[SolMod::sol] m_sol_state_ue4ss is nullptr, crashing\n"));
            //}
            return m_sol_state_ue4ss;
        }
        else
        {
            if (!m_sol_state_async)
            {
                Output::send<LogLevel::Error>(STR("[SolMod::sol] m_sol_state_async is nullptr, crashing\n"));
            }
            return m_sol_state_async.thread_state();
        }
    }

    SolMod::SolMod(UE4SSProgram& program, StringType&& mod_name, StringType&& mod_path) : Mod(program, std::move(mod_name), std::move(mod_path))
    {
        Output::send(STR("SolMod constructed\n"));

        m_scripts_path = m_mod_path + L"\\scripts";
        if (!std::filesystem::exists(m_scripts_path))
        {
            set_installable(false);
            return;
        }
    }

    SolMod::~SolMod()
    {
        /*
        if (m_sol_state_ue4ss)
        {
            delete m_sol_state_ue4ss;
            m_sol_state_ue4ss = nullptr;
        }
        //*/

        /*
        if (m_sol_state_keybind)
        {
            delete m_sol_state_keybind;
            m_sol_state_keybind = nullptr;
        }
        //*/

        /*
        if (m_sol_state_gameplay)
        {
            delete m_sol_state_gameplay;
            m_sol_state_gameplay = nullptr;
        }
        //*/
    }

    static auto UObject_IsValid(UObject* object) -> bool
    {
        return object && !object->IsUnreachable()/* && is_object_in_global_unreal_object_map(object)*/;
    }

    static auto Test() -> void
    {
        Output::send(STR("TEST\n"));
    }

    auto get_class_name(sol::this_state state) -> StringType
    {
        // There's some kind of bug where the '__name' field doesn't equal the '__name' field in the metatable.
        // To circumvet this, we're manually putting the metatable on the stack before calling 'get_field'.
        // This should probably be further investigated to make sure this isn't an effect of a serious bug.
        // Note that this works fine without manually getting the metatable for types other than 'InvalidObject'.
        lua_getmetatable(state, 1);
        sol::stack::get_field(state.lua_state(), "__name");
        return sol::stack::get<StringType>(state.lua_state());
    }

    auto SolMod::start_async_thread() -> void
    {
        m_async_thread = std::jthread{&Mod::update_async, this};
    }

    static auto exception_handler_test(lua_State* lua_state, sol::optional<const std::exception&> maybe_exception, std::string_view description) -> int
    {
        Output::send(STR("exception_handler_test\n"));
        return 0;
    }

    static auto default_handler_test(lua_State* lua_state, sol::optional<const std::exception&> maybe_exception, std::string_view description) -> int
    {
        Output::send(STR("default_handler_test\n"));
        return 0;
    }

    static auto panic_test(lua_State* lua_state) -> int
    {
        Output::send(STR("panic_test\n"));
        return 0;
    }

    static auto luajit_exception_handler_test(lua_State* lua_state, int(*func)(lua_State*)) -> int
    {
        Output::send(STR("luajit_exception_handler_test\n"));
        return 0;
    }

    static auto handle_metatable_lookups(sol::state_view state, sol::userdata& sol_object, const StringType& metatable_field_name) -> int
    {
        auto value = sol_object[sol::metatable_key].get<sol::table>().get<sol::object>(metatable_field_name);
        sol::stack::push(state, value);
        return 1;
    }
    
    static auto handle_uobject_property_getter(lua_State* lua_state) -> int
    {
        auto state = sol::state_view{lua_state};
        auto sol_object = sol::stack::get<sol::userdata>(state, 1);
        
        // If 'self' is the wrong type or if it's nullptr then we assume the UObject wasn't found so lets return an invalid UObject to Lua
        // This allows the safe chaining of "__index" as long as the Lua script checks ":IsValid()" before using the object
        auto maybe_self = sol_object.as<std::optional<UObject*>>();
        if (!maybe_self.has_value() || !maybe_self.value())
        {
            sol::stack::push(state, InvalidObject{});
            return 1;
        }
        
        auto& self = *maybe_self.value();
        auto maybe_key = sol::stack::get<std::optional<StringType>>(state, 2);
        if (!maybe_key.has_value())
        {
            sol::stack::push(state, InvalidObject{});
            return 1;
        }
        auto key = maybe_key.value();

        auto member_name = key;
        auto property_name = FName(member_name);
        // Uncomment when custom properties have been implemented.
        //FField* field = LuaCustomProperty::StaticStorage::property_list.find_or_nullptr(self, member_name);
        FField* field = nullptr;

        if (!field)
        {
            auto obj_as_struct = Cast<UStruct>(&self);
            if (!obj_as_struct) { obj_as_struct = self.GetClassPrivate(); }
            field = obj_as_struct->FindProperty(property_name);
        }

        if (!field || field->GetClass().GetFName() == GFunctionName)
        {
            // Return the UFunction.
            // Implement this later.
            // For now, assume we might be looking for something in the metamethod like __name.
            return handle_metatable_lookups(state, sol_object, member_name);
        }
        else
        {
            // Casting to FProperty here so that we can get access to property members.
            // It needed to be FField above so that it could be converted to UFunction without force.
            // This is because UFunction & FProperty both inherit from FField, but UFunction doesn't inherit from FProperty.
            auto property = static_cast<FProperty*>(field);

            if (auto property_pusher = lookup_property_pusher(property->GetClass().GetFName()); property_pusher)
            {
                auto data = static_cast<uint8_t*>(static_cast<void*>(&self)) + property->GetOffset_Internal();
                if (auto error_msg = property_pusher({state, nullptr, data, PushType::ToLua, nullptr}); error_msg)
                {
                    Output::send<LogLevel::Error>(STR("Error setting parameter value: {}\n"), to_wstring(error_msg));
                    return 0;
                }
            }
            else
            {
                Output::send<LogLevel::Error>(STR("[handle_uobject_property_access] Tried accessing unreal property without a registered handler. Property type '{}' not supported.\n"),
                    property->GetClass().GetName());
                return 0;
            }
        }
        
        return 1;
    }

    static auto handle_uobject_property_setter(lua_State* lua_state) -> int
    {
        Output::send<LogLevel::Verbose>(STR("handle_uobject_property_setter\n"));
        return 0;
    }
    
    static auto setup_lua_classes(sol::state_view sol) -> void
    {
        sol.set_function("print", [](const StringType& message) {
            Output::send(STR("[Lua] {}"), message);
        });

        sol.new_usertype<ParamPtrWrapper>("ParamPtrWrapper",
            "type", [](sol::this_state state) { return get_class_name(state); },
            "get", [](lua_State* lua_state) -> int {
                auto maybe_self = sol::stack::get<std::optional<ParamPtrWrapper>>(lua_state);
                if (!maybe_self.has_value())
                {
                    Output::send<LogLevel::Warning>(STR("ParamPtrWrapper.get called on a non-ParamPtrWrapper instance\n"));
                    sol::stack::push(lua_state, InvalidObject{});
                }
                else
                {
                    maybe_self->unwrap(lua_state);
                }
                return 1;
            },
            "set", [](lua_State* lua_state) -> int {
                auto maybe_self = sol::stack::get<std::optional<ParamPtrWrapper>>(lua_state, 1);
                if (!maybe_self.has_value())
                {
                    Output::send<LogLevel::Warning>(STR("ParamPtrWrapper.set called on a non-ParamPtrWrapper instance\n"));
                }
                else
                {
                    maybe_self->rewrap(lua_state);
                }
                return 0;
            }
        );

        auto unreal_class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.Actor"));

        auto sol_class_InvalidObject = sol.new_usertype<InvalidObject>("InvalidObject",
            sol::meta_function::index, &InvalidObject::Index,
            "type", [](sol::this_state state) { return get_class_name(state); },
            "IsValid", [] { return false; }
        );

        sol.new_usertype<FName>("FName",
            sol::no_constructor,
            sol::call_constructor, sol::factories(
                [] { return FName(); },
                [](int64_t index_and_number) { return FName(index_and_number); },
                [](const StringType& name) { return FName(name, FNAME_Add); },
                [](const StringType& name, EFindName find_name) { return FName(name, find_name); }
            ),
            "type", [](sol::this_state state) { return get_class_name(state); }
        );
        sol.new_usertype<UObject>("UObject",
            sol::no_constructor,
            sol::meta_function::index, sol::policies(&handle_uobject_property_getter, &pointer_policy),
            sol::meta_function::new_index, sol::policies(&handle_uobject_property_setter, &pointer_policy),
            "type", [](sol::this_state state) { return get_class_name(state); },
            "IsValid", &UObject_IsValid,
            "GetClassPrivate", CHOOSE_MEMBER_OVERLOAD(UObject, GetClassPrivate),
            "GetInternalIndex", CHOOSE_MEMBER_OVERLOAD(UObject, GetInternalIndex),
            "GetNamePrivate", CHOOSE_MEMBER_OVERLOAD(UObject, GetNamePrivate),
            "GetObjectFlags", CHOOSE_MEMBER_OVERLOAD(UObject, GetObjectFlags),
            "GetOuterPrivate", CHOOSE_MEMBER_OVERLOAD(UObject, GetOuterPrivate),
            "SetFlagsTo", CHOOSE_MEMBER_OVERLOAD(UObject, SetFlagsTo, EObjectFlags), // Temporary! Remove before release! Only for testing the CHOOSE macro.
            "PostRename", CHOOSE_MEMBER_OVERLOAD(UObject, PostRename, UObject*, const FName), // Temporary! Remove before release! Only for testing the CHOOSE macro.
            "GetFullName", sol::overload(
                [](UObject* self) { return self->GetFullName(); },
                CHOOSE_CONST_MEMBER_OVERLOAD(UObject, GetFullName, UObject*)
            )
        );

        sol.new_usertype<UClass>("UClass",
            sol::no_constructor,
            sol::base_classes, sol::bases<UObject>(),
            "GetClassAddReferencedObjects", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassAddReferencedObjects),
            "GetClassCastFlags", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassCastFlags),
            "GetClassConfigName", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassConfigName),
            "GetClassConstructor", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassConstructor),
            "GetClassDefaultObject", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassDefaultObject),
            "GetClassFlags", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassFlags),
            "GetClassGeneratedBy", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassGeneratedBy),
            "GetClassUnique", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassUnique),
            "GetClassVTableHelperCtorCaller", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassVTableHelperCtorCaller),
            "GetClassWithin", CHOOSE_MEMBER_OVERLOAD(UClass, GetClassWithin),
            "GetFirstOwnedClassRep", CHOOSE_MEMBER_OVERLOAD(UClass, GetFirstOwnedClassRep),
            "GetInterfaces", CHOOSE_MEMBER_OVERLOAD(UClass, GetInterfaces),
            "GetNetFields", CHOOSE_MEMBER_OVERLOAD(UClass, GetNetFields),
            "GetSparseClassData", CHOOSE_MEMBER_OVERLOAD(UClass, GetSparseClassData),
            "GetSparseClassDataStruct", CHOOSE_MEMBER_OVERLOAD(UClass, GetSparseClassDataStruct),
            "GetUberGraphFramePointerProperty", CHOOSE_MEMBER_OVERLOAD(UClass, GetUberGraphFramePointerProperty),
            "GetbCooked", CHOOSE_MEMBER_OVERLOAD(UClass, GetbCooked),
            "GetbLayoutChanging", CHOOSE_MEMBER_OVERLOAD(UClass, GetbLayoutChanging),

            "Test", &Test
        );

        sol.set("c", unreal_class);
        sol.set("empty_object", InvalidObject{});
    }

    auto SolMod::state_init(sol::state_view sol) -> void
    {
        sol.open_libraries();

        StringType current_paths = sol["package"]["path"];
        current_paths.append(std::format(STR(";{}\\{}\\Scripts\\?.lua"), m_program.get_mods_directory(), get_name()));
        current_paths.append(std::format(STR(";{}\\shared\\?.lua"), m_program.get_mods_directory()));
        current_paths.append(std::format(STR(";{}\\shared\\?\\?.lua"), m_program.get_mods_directory()));
        sol["package"]["path"] = current_paths;

        StringType current_cpaths = sol["package"]["cpath"];
        current_cpaths.append(std::format(STR(";{}\\{}\\Scripts\\?.dll"), m_program.get_mods_directory(), get_name()));
        current_cpaths.append(std::format(STR(";{}\\{}\\?.dll"), m_program.get_mods_directory(), get_name()));
        sol["package"]["cpath"] = current_cpaths;

        sol.set("ModRef", this);
        setup_lua_classes(sol);
        setup_lua_global_functions(sol);
    }

    auto SolMod::start_mod() -> void
    {
        Output::send(STR("SolMod::start_mod\n"));

        m_sol_state_ue4ss = sol::state{};
        m_sol_state_async = sol::thread::create(sol());

        state_init(sol());
        //state_init(sol(State::Gameplay));

        start_async_thread();

        sol().safe_script_file(to_string(m_scripts_path + STR("\\main.lua")));
    }

    auto SolMod::uninstall() -> void
    {
        Output::send<LogLevel::Verbose>(STR("Stopping mod '{}' for uninstall\n"), m_mod_name);

        if (m_async_thread.joinable())
        {
            m_async_thread.request_stop();
            m_async_thread.join();
        }

        m_sol_gameplay_states.clear();

        // Unhook all UFunctions for this mod & remove from the map that keeps track of which UFunctions have been hooked
        std::erase_if(m_hooked_script_function_data, [&](std::unique_ptr<LuaUnrealScriptFunctionData>& item) -> bool {
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

        while (is_mid_script_execution) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); }

        auto& key_events = UE4SSProgram::get_program().get_events();
        std::erase_if(key_events, [&](Input::KeySet& input_event) -> bool {
            bool were_all_events_registered_from_lua = true;
            for (auto&[key, vector_of_key_data] : input_event.key_data)
            {
                std::erase_if(vector_of_key_data, [&](Input::KeyData& key_data) -> bool {
                    if (std::bit_cast<SolMod*>(key_data.custom_data) == this)
                    {
                        return true;
                    }
                    else
                    {
                        were_all_events_registered_from_lua = false;
                        return false;
                    }
                });
            }

            return were_all_events_registered_from_lua;
        });

        clear_delayed_actions();
    }

    auto SolMod::update_async() -> void
    {
        while (!m_async_thread.get_stop_token().stop_requested())
        {
            process_delayed_actions();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    auto SolMod::on_program_start() -> void
    {
        //g_property_pushers.resize(g_max_property_pushers, nullptr);
        auto add_property_pusher = [](FName name, PropertyPusherFunction pusher) {
            // Turns out that not all properties have a low comparison index.
            //if (name.GetComparisonIndex() < g_max_property_pushers)
            //{
                Output::send(STR("{}: {:X}\n"), name.ToString(), name.GetComparisonIndex());
                g_property_pushers[name.GetComparisonIndex()] = pusher;
            //}
            //else
            //{
            //    throw std::runtime_error{std::format(
            //        "[add_property_pusher] ComparisonIndex (0x{:X}) of property '{}' was too high to be usable", name.GetComparisonIndex(), to_string(name.ToString())
            //    )};
            //}
        };

        add_property_pusher(FName(L"ObjectProperty"), &push_objectproperty);
        add_property_pusher(FName(L"ClassProperty"), &push_classproperty);
        add_property_pusher(FName(L"Int8Property"), &push_int8property);
        add_property_pusher(FName(L"Int16Property"), &push_int16property);
        add_property_pusher(FName(L"IntProperty"), &push_intproperty);
        add_property_pusher(FName(L"Int64Property"), &push_int64property);
        add_property_pusher(FName(L"ByteProperty"), &push_byteproperty);
        add_property_pusher(FName(L"UInt16Property"), &push_uint16property);
        add_property_pusher(FName(L"UInt32Property"), &push_uint32property);
        add_property_pusher(FName(L"UInt64Property"), &push_uint64property);
        //add_property_pusher(FName(L"StructProperty"), &push_structproperty); // Needs custom implementation for implicit Lua table conversions.
        //add_property_pusher(FName(L"ArrayProperty"), &push_arrayproperty); // Needs custom implementation. for implicit Lua table conversions.
        add_property_pusher(FName(L"FloatProperty"), &push_floatproperty);
        add_property_pusher(FName(L"DoubleProperty"), &push_doubleproperty);
        add_property_pusher(FName(L"BoolProperty"), &push_boolproperty);
        //add_property_pusher(FName(L"EnumProperty"), &push_enumproperty); // Needs custom implementation to keep compatibility.
        add_property_pusher(FName(L"WeakObjectProperty"), &push_weakobjectproperty); // Might need a custom implementation, for now let's try the macro.
        add_property_pusher(FName(L"NameProperty"), &push_nameproperty);
        //add_property_pusher(FName(L"TextProperty"), &push_textproperty); // Might need custom implementation to keep compatiblity.
        //add_property_pusher(FName(L"StrProperty"), &push_strproperty); // Might need custom implementation for implicit conversions from Lua string to FString.
        add_property_pusher(FName(L"SoftClassProperty"), &push_softclassproperty);
        add_property_pusher(FName(L"InterfaceProperty"), &push_interfaceproperty);

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

        Unreal::Hook::RegisterStaticConstructObjectPostCallback([](const Unreal::FStaticConstructObjectParameters&, Unreal::UObject* constructed_object) {
            Unreal::UStruct* object_class = constructed_object->GetClassPrivate();
            std::lock_guard<decltype(SolMod::m_thread_actions_mutex)> guard{SolMod::m_thread_actions_mutex};
            while (object_class)
            {
                for (const auto& callback_data : m_static_construct_object_lua_callbacks)
                {
                    if (callback_data.instance_of_class == object_class)
                    {
                        for (const auto& registry_index : callback_data.registry_indexes)
                        {
                            call_function_dont_wrap_params_safe(callback_data.lua, registry_index.lua_callback, constructed_object);
                        }
                    }
                }

                object_class = object_class->GetSuperStruct();
            }

            return constructed_object;
        });

        Unreal::Hook::RegisterInitGameStatePreCallback([](AGameModeBase* Context) {
            for (const auto& callback_data : m_init_game_state_pre_callbacks)
            {
                const auto& lua = callback_data.lua;
                //set_is_in_game_thread(lua, true);

                for (const auto registry_index : callback_data.registry_indexes)
                {
                    call_function_wrap_params_safe(lua, registry_index.lua_callback, Context);
                }

                //set_is_in_game_thread(lua, false);
            }
        });

        Unreal::Hook::RegisterInitGameStatePostCallback([](AGameModeBase* Context) {
            for (const auto& callback_data : m_init_game_state_post_callbacks)
            {
                const auto& lua = callback_data.lua;
                //set_is_in_game_thread(lua, true);

                for (const auto registry_index : callback_data.registry_indexes)
                {
                    call_function_wrap_params_safe(lua, registry_index.lua_callback, Context);
                }

                //set_is_in_game_thread(lua, false);
            }
        });
    }

    auto SolMod::global_uninstall() -> void
    {
        SolMod::m_static_construct_object_lua_callbacks.clear();
        //SolMod::m_process_console_exec_pre_callbacks.clear();
        //SolMod::m_process_console_exec_post_callbacks.clear();
        //SolMod::m_global_command_lua_callbacks.clear();
        //SolMod::m_custom_command_lua_pre_callbacks.clear();
        SolMod::m_custom_event_callbacks.clear();
        SolMod::m_init_game_state_pre_callbacks.clear();
        SolMod::m_init_game_state_post_callbacks.clear();
        //SolMod::m_begin_play_pre_callbacks.clear();
        //SolMod::m_begin_play_post_callbacks.clear();
        //SolMod::m_call_function_by_name_with_arguments_pre_callbacks.clear();
        //SolMod::m_call_function_by_name_with_arguments_post_callbacks.clear();
        //SolMod::m_local_player_exec_pre_callbacks.clear();
        //SolMod::m_local_player_exec_post_callbacks.clear();
        SolMod::m_script_hook_callbacks.clear();
        SolMod::m_generic_hook_id_to_native_hook_id.clear();
    }

    auto SolMod::clear_delayed_actions() -> void
    {
        std::lock_guard<decltype(m_actions_lock)> guard{m_actions_lock};
        m_pending_actions.clear();
        m_delayed_actions.clear();
    }
}
