#include <bit>

#include <Mod/SolGlobals.hpp>
#include <Mod/SolMod.hpp>
#include <Mod/SolHelpers.hpp>
#include <Input/Handler.hpp>
#include <UE4SSProgram.hpp>
#include <GUI/Dumpers.hpp>
#include <USMapGenerator/Generator.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/World.hpp>
#include <Unreal/FOutputDevice.hpp>
#include <Unreal/TMap.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/Property/FSoftObjectProperty.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>

namespace RC
{
    using namespace Unreal;

    auto SolMod::setup_lua_global_functions(sol::state_view sol) -> void
    {
        sol.set_function("StaticFindObject",
                         sol::policies(sol::overload(
                                               [](const StringType& name) {
                                                   return UObjectGlobals::StaticFindObject(nullptr, nullptr, name);
                                               },
                                               [](UClass* uclass, UObject* in_outer, const StringType& name) {
                                                   return UObjectGlobals::StaticFindObject(uclass, in_outer, name);
                                               },
                                               [](UClass* uclass, UObject* in_outer, const StringType& name, bool exact_class) {
                                                   return UObjectGlobals::StaticFindObject(uclass, in_outer, name, exact_class);
                                               }),
                                       &pointer_policy));

        sol.new_enum<Input::Key>("Key",
                                 {
                                         {"LEFT_MOUSE_BUTTON", Input::Key::LEFT_MOUSE_BUTTON},
                                         {"RIGHT_MOUSE_BUTTON", Input::Key::RIGHT_MOUSE_BUTTON},
                                         {"CANCEL", Input::Key::CANCEL},
                                         {"MIDDLE_MOUSE_BUTTON", Input::Key::MIDDLE_MOUSE_BUTTON},
                                         {"XBUTTON_ONE", Input::Key::XBUTTON_ONE},
                                         {"XBUTTON_TWO", Input::Key::XBUTTON_TWO},
                                         {"BACKSPACE", Input::Key::BACKSPACE},
                                         {"TAB", Input::Key::TAB},
                                         {"CLEAR", Input::Key::CLEAR},
                                         {"RETURN", Input::Key::RETURN},
                                         {"PAUSE", Input::Key::PAUSE},
                                         {"CAPS_LOCK", Input::Key::CAPS_LOCK},
                                         {"IME_KANA", Input::Key::IME_KANA},
                                         {"IME_HANGUEL", Input::Key::IME_HANGUEL},
                                         {"IME_HANGUL", Input::Key::IME_HANGUL},
                                         {"IME_ON", Input::Key::IME_ON},
                                         {"IME_JUNJA", Input::Key::IME_JUNJA},
                                         {"IME_FINAL", Input::Key::IME_FINAL},
                                         {"IME_HANJA", Input::Key::IME_HANJA},
                                         {"IME_KANJI", Input::Key::IME_KANJI},
                                         {"IME_OFF", Input::Key::IME_OFF},
                                         {"ESCAPE", Input::Key::ESCAPE},
                                         {"IME_CONVERT", Input::Key::IME_CONVERT},
                                         {"IME_NONCONVERT", Input::Key::IME_NONCONVERT},
                                         {"IME_ACCEPT", Input::Key::IME_ACCEPT},
                                         {"IME_MODECHANGE", Input::Key::IME_MODECHANGE},
                                         {"SPACE", Input::Key::SPACE},
                                         {"PAGE_UP", Input::Key::PAGE_UP},
                                         {"PAGE_DOWN", Input::Key::PAGE_DOWN},
                                         {"END", Input::Key::END},
                                         {"HOME", Input::Key::HOME},
                                         {"LEFT_ARROW", Input::Key::LEFT_ARROW},
                                         {"UP_ARROW", Input::Key::UP_ARROW},
                                         {"RIGHT_ARROW", Input::Key::RIGHT_ARROW},
                                         {"DOWN_ARROW", Input::Key::DOWN_ARROW},
                                         {"SELECT", Input::Key::SELECT},
                                         {"PRINT", Input::Key::PRINT},
                                         {"EXECUTE", Input::Key::EXECUTE},
                                         {"PRINT_SCREEN", Input::Key::PRINT_SCREEN},
                                         {"INS", Input::Key::INS},
                                         {"DEL", Input::Key::DEL},
                                         {"HELP", Input::Key::HELP},
                                         {"ZERO", Input::Key::ZERO},
                                         {"ONE", Input::Key::ONE},
                                         {"TWO", Input::Key::TWO},
                                         {"THREE", Input::Key::THREE},
                                         {"FOUR", Input::Key::FOUR},
                                         {"FIVE", Input::Key::FIVE},
                                         {"SIX", Input::Key::SIX},
                                         {"SEVEN", Input::Key::SEVEN},
                                         {"EIGHT", Input::Key::EIGHT},
                                         {"NINE", Input::Key::NINE},
                                         {"A", Input::Key::A},
                                         {"B", Input::Key::B},
                                         {"C", Input::Key::C},
                                         {"D", Input::Key::D},
                                         {"E", Input::Key::E},
                                         {"F", Input::Key::F},
                                         {"G", Input::Key::G},
                                         {"H", Input::Key::H},
                                         {"I", Input::Key::I},
                                         {"J", Input::Key::J},
                                         {"K", Input::Key::K},
                                         {"L", Input::Key::L},
                                         {"M", Input::Key::M},
                                         {"N", Input::Key::N},
                                         {"O", Input::Key::O},
                                         {"P", Input::Key::P},
                                         {"Q", Input::Key::Q},
                                         {"R", Input::Key::R},
                                         {"S", Input::Key::S},
                                         {"T", Input::Key::T},
                                         {"U", Input::Key::U},
                                         {"V", Input::Key::V},
                                         {"W", Input::Key::W},
                                         {"X", Input::Key::X},
                                         {"Y", Input::Key::Y},
                                         {"Z", Input::Key::Z},
                                         {"LEFT_WIN", Input::Key::LEFT_WIN},
                                         {"RIGHT_WIN", Input::Key::RIGHT_WIN},
                                         {"APPS", Input::Key::APPS},
                                         {"SLEEP", Input::Key::SLEEP},
                                         {"NUM_ZERO", Input::Key::NUM_ZERO},
                                         {"NUM_ONE", Input::Key::NUM_ONE},
                                         {"NUM_TWO", Input::Key::NUM_TWO},
                                         {"NUM_THREE", Input::Key::NUM_THREE},
                                         {"NUM_FOUR", Input::Key::NUM_FOUR},
                                         {"NUM_FIVE", Input::Key::NUM_FIVE},
                                         {"NUM_SIX", Input::Key::NUM_SIX},
                                         {"NUM_SEVEN", Input::Key::NUM_SEVEN},
                                         {"NUM_EIGHT", Input::Key::NUM_EIGHT},
                                         {"NUM_NINE", Input::Key::NUM_NINE},
                                         {"MULTIPLY", Input::Key::MULTIPLY},
                                         {"ADD", Input::Key::ADD},
                                         {"SEPARATOR", Input::Key::SEPARATOR},
                                         {"SUBTRACT", Input::Key::SUBTRACT},
                                         {"DECIMAL", Input::Key::DECIMAL},
                                         {"DIVIDE", Input::Key::DIVIDE},
                                         {"F1", Input::Key::F1},
                                         {"F2", Input::Key::F2},
                                         {"F3", Input::Key::F3},
                                         {"F4", Input::Key::F4},
                                         {"F5", Input::Key::F5},
                                         {"F6", Input::Key::F6},
                                         {"F7", Input::Key::F7},
                                         {"F8", Input::Key::F8},
                                         {"F9", Input::Key::F9},
                                         {"F10", Input::Key::F10},
                                         {"F11", Input::Key::F11},
                                         {"F12", Input::Key::F12},
                                         {"F13", Input::Key::F13},
                                         {"F14", Input::Key::F14},
                                         {"F15", Input::Key::F15},
                                         {"F16", Input::Key::F16},
                                         {"F17", Input::Key::F17},
                                         {"F18", Input::Key::F18},
                                         {"F19", Input::Key::F19},
                                         {"F20", Input::Key::F20},
                                         {"F21", Input::Key::F21},
                                         {"F22", Input::Key::F22},
                                         {"F23", Input::Key::F23},
                                         {"F24", Input::Key::F24},
                                         {"NUM_LOCK", Input::Key::NUM_LOCK},
                                         {"SCROLL_LOCK", Input::Key::SCROLL_LOCK},
                                         {"BROWSER_BACK", Input::Key::BROWSER_BACK},
                                         {"BROWSER_FORWARD", Input::Key::BROWSER_FORWARD},
                                         {"BROWSER_REFRESH", Input::Key::BROWSER_REFRESH},
                                         {"BROWSER_STOP", Input::Key::BROWSER_STOP},
                                         {"BROWSER_SEARCH", Input::Key::BROWSER_SEARCH},
                                         {"BROWSER_FAVORITES", Input::Key::BROWSER_FAVORITES},
                                         {"BROWSER_HOME", Input::Key::BROWSER_HOME},
                                         {"VOLUME_MUTE", Input::Key::VOLUME_MUTE},
                                         {"VOLUME_DOWN", Input::Key::VOLUME_DOWN},
                                         {"VOLUME_UP", Input::Key::VOLUME_UP},
                                         {"MEDIA_NEXT_TRACK", Input::Key::MEDIA_NEXT_TRACK},
                                         {"MEDIA_PREV_TRACK", Input::Key::MEDIA_PREV_TRACK},
                                         {"MEDIA_STOP", Input::Key::MEDIA_STOP},
                                         {"MEDIA_PLAY_PAUSE", Input::Key::MEDIA_PLAY_PAUSE},
                                         {"LAUNCH_MAIL", Input::Key::LAUNCH_MAIL},
                                         {"LAUNCH_MEDIA_SELECT", Input::Key::LAUNCH_MEDIA_SELECT},
                                         {"LAUNCH_APP1", Input::Key::LAUNCH_APP1},
                                         {"LAUNCH_APP2", Input::Key::LAUNCH_APP2},
                                         {"OEM_ONE", Input::Key::OEM_ONE},
                                         {"OEM_PLUS", Input::Key::OEM_PLUS},
                                         {"OEM_COMMA", Input::Key::OEM_COMMA},
                                         {"OEM_MINUS", Input::Key::OEM_MINUS},
                                         {"OEM_PERIOD", Input::Key::OEM_PERIOD},
                                         {"OEM_TWO", Input::Key::OEM_TWO},
                                         {"OEM_THREE", Input::Key::OEM_THREE},
                                         {"OEM_FOUR", Input::Key::OEM_FOUR},
                                         {"OEM_FIVE", Input::Key::OEM_FIVE},
                                         {"OEM_SIX", Input::Key::OEM_SIX},
                                         {"OEM_SEVEN", Input::Key::OEM_SEVEN},
                                         {"OEM_EIGHT", Input::Key::OEM_EIGHT},
                                         {"OEM_102", Input::Key::OEM_102},
                                         {"IME_PROCESS", Input::Key::IME_PROCESS},
                                         {"PACKET", Input::Key::PACKET},
                                         {"ATTN", Input::Key::ATTN},
                                         {"CRSEL", Input::Key::CRSEL},
                                         {"EXSEL", Input::Key::EXSEL},
                                         {"EREOF", Input::Key::EREOF},
                                         {"PLAY", Input::Key::PLAY},
                                         {"ZOOM", Input::Key::ZOOM},
                                         {"PA1", Input::Key::PA1},
                                         {"OEM_CLEAR", Input::Key::OEM_CLEAR},
                                 });

        sol.new_enum<Input::ModifierKey>("ModifierKey",
                                         {
                                                 {"SHIFT", Input::ModifierKey::SHIFT},
                                                 {"CONTROL", Input::ModifierKey::CONTROL},
                                                 {"ALT", Input::ModifierKey::ALT},
                                         });

        sol.new_enum<EObjectFlags>("EObjectFlags",
                                   {
                                           {"RF_NoFlags", EObjectFlags::RF_NoFlags},
                                           {"RF_Public", EObjectFlags::RF_Public},
                                           {"RF_Standalone", EObjectFlags::RF_Standalone},
                                           {"RF_MarkAsNative", EObjectFlags::RF_MarkAsNative},
                                           {"RF_Transactional", EObjectFlags::RF_Transactional},
                                           {"RF_ClassDefaultObject", EObjectFlags::RF_ClassDefaultObject},
                                           {"RF_ArchetypeObject", EObjectFlags::RF_ArchetypeObject},
                                           {"RF_Transient", EObjectFlags::RF_Transient},
                                           {"RF_MarkAsRootSet", EObjectFlags::RF_MarkAsRootSet},
                                           {"RF_TagGarbageTemp", EObjectFlags::RF_TagGarbageTemp},
                                           {"RF_NeedInitialization", EObjectFlags::RF_NeedInitialization},
                                           {"RF_NeedLoad", EObjectFlags::RF_NeedLoad},
                                           {"RF_KeepForCooker", EObjectFlags::RF_KeepForCooker},
                                           {"RF_NeedPostLoad", EObjectFlags::RF_NeedPostLoad},
                                           {"RF_NeedPostLoadSubobjects", EObjectFlags::RF_NeedPostLoadSubobjects},
                                           {"RF_NewerVersionExists", EObjectFlags::RF_NewerVersionExists},
                                           {"RF_BeginDestroyed", EObjectFlags::RF_BeginDestroyed},
                                           {"RF_FinishDestroyed", EObjectFlags::RF_FinishDestroyed},
                                           {"RF_BeingRegenerated", EObjectFlags::RF_BeingRegenerated},
                                           {"RF_DefaultSubObject", EObjectFlags::RF_DefaultSubObject},
                                           {"RF_WasLoaded", EObjectFlags::RF_WasLoaded},
                                           {"RF_TextExportTransient", EObjectFlags::RF_TextExportTransient},
                                           {"RF_LoadCompleted", EObjectFlags::RF_LoadCompleted},
                                           {"RF_InheritableComponentTemplate", EObjectFlags::RF_InheritableComponentTemplate},
                                           {"RF_DuplicateTransient", EObjectFlags::RF_DuplicateTransient},
                                           {"RF_StrongRefOnFrame", EObjectFlags::RF_StrongRefOnFrame},
                                           {"RF_NonPIEDuplicateTransient", EObjectFlags::RF_NonPIEDuplicateTransient},
                                           {"RF_Dynamic", EObjectFlags::RF_Dynamic},
                                           {"RF_WillBeLoaded", EObjectFlags::RF_WillBeLoaded},
                                           {"RF_HasExternalPackage", EObjectFlags::RF_HasExternalPackage},
                                           {"RF_AllFlags", EObjectFlags::RF_AllFlags},
                                   });

        sol.set("NAME_None", NAME_None);

        sol.set_function("FindFirstOf", CHOOSE_FREE_OVERLOAD(UObjectGlobals::FindFirstOf, const std::wstring&));
        sol.set_function("FindAllOf", CHOOSE_FREE_OVERLOAD(UObjectGlobals::FindAllOf, const std::wstring&, std::vector<UObject*>&));
        sol.set_function("IsKeyBindRegistered",
                         sol::overload(
                                 [](Input::Key key) {
                                     return UE4SSProgram::get_program().is_keydown_event_registered(key);
                                 },
                                 [](Input::Key key, sol::nested<Input::Handler::ModifierKeyArray> modifier_keys) {
                                     return UE4SSProgram::get_program().is_keydown_event_registered(key, modifier_keys.value());
                                 }));
        // When calling 'register_keydown_event', we use the pointer to the mod for 'custom_data' to signify that this keydown event was created by that mod.
        // We then use this data to unregister keybinds belonging to this mod later.
        sol.set_function("RegisterKeyBindAsync",
                         sol::overload(
                                 [&](sol::this_state state, Input::Key key, sol::function callback) {
                                     UE4SSProgram::get_program().register_keydown_event(
                                             key,
                                             [=] {
                                                 call_function_dont_wrap_params_safe(state, callback);
                                             },
                                             std::bit_cast<uintptr_t>(this));
                                 },
                                 [&](sol::this_state state, Input::Key key, sol::nested<std::vector<Input::ModifierKey>> modifier_keys, sol::function callback) {
                                     Input::Handler::ModifierKeyArray modifier_keys_array{};
                                     for (size_t i = 0; i < modifier_keys.value().size(); ++i)
                                     {
                                         if (i >= Input::max_modifier_keys) break;
                                         modifier_keys_array[i] = modifier_keys.value()[i];
                                     }
                                     UE4SSProgram::get_program().register_keydown_event(
                                             key,
                                             modifier_keys_array,
                                             [=] {
                                                 call_function_dont_wrap_params_safe(state, callback);
                                             },
                                             std::bit_cast<uintptr_t>(this));
                                 }));
        sol.set_function("RegisterKeyBind",
                         sol::overload(
                                 [&](sol::this_state state, Input::Key key, sol::function callback) {
                                     UE4SSProgram::get_program().register_keydown_event(
                                             key,
                                             [=] {
                                                 call_function_with_manual_handler_safe(state, [&](const std::vector<ParamPtrWrapper>& params) {
                                                     std::lock_guard<std::recursive_mutex> guard{SolMod::m_thread_actions_mutex};
                                                     return callback(sol::as_args(params));
                                                 });
                                             },
                                             std::bit_cast<uintptr_t>(this));
                                 },
                                 [&](sol::this_state state, Input::Key key, sol::nested<std::vector<Input::ModifierKey>> modifier_keys, sol::function callback) {
                                     Input::Handler::ModifierKeyArray modifier_keys_array{};
                                     for (size_t i = 0; i < modifier_keys.value().size(); ++i)
                                     {
                                         if (i >= Input::max_modifier_keys) break;
                                         modifier_keys_array[i] = modifier_keys.value()[i];
                                     }
                                     UE4SSProgram::get_program().register_keydown_event(
                                             key,
                                             modifier_keys_array,
                                             [=] {
                                                 call_function_with_manual_handler_safe(state, [&](const std::vector<ParamPtrWrapper>& params) {
                                                     std::lock_guard<std::recursive_mutex> guard{SolMod::m_thread_actions_mutex};
                                                     return callback(sol::as_args(params));
                                                 });
                                             },
                                             std::bit_cast<uintptr_t>(this));
                                 }));
        sol.set_function("RegisterHook", [&](sol::this_state state, StringType function_name, sol::function callback) {
            sol::variadic_results return_values{};

            StringType function_name_no_prefix{};
            try
            {
                function_name_no_prefix = function_name.substr(function_name.find_first_of(L"/"), function_name.size());
            }
            catch (...)
            {
                return exit_script_with_error<2>(state, STR("Param #1 to 'RegisterHook' was not a valid name for a UFunction"));
            }

            auto unreal_function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, function_name_no_prefix);
            if (!unreal_function)
            {
                return exit_script_with_error<2>(
                        state,
                        STR("Tried to register a hook with Lua function 'RegisterHook' but no UFunction with the specified name was found."));
            }

            auto mod_ref = get_mod_ref(state);
            if (!mod_ref)
            {
                return exit_script_with_error<2>(state, STR("Tried to register a hook with Lua function 'RegisterHook' but no ModRef global was found"));
            }

            auto gameplay_state = m_sol_gameplay_states.emplace_back(sol::thread::create(state)).lua_state();

            // Move the Lua callback from the UE4SS Lua state to the gameplay state, which is a Lua thread.
            sol::function callback_in_gameplay_state = sol::function(gameplay_state, callback);

            int32_t generic_pre_id{};
            int32_t generic_post_id{};
            auto func_ptr = unreal_function->GetFunc();
            if (func_ptr && func_ptr != UObject::ProcessInternalInternal.get_function_address() && unreal_function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Native))
            {
                auto& custom_data = m_hooked_script_function_data.emplace_back(std::make_unique<LuaUnrealScriptFunctionData>(
                        LuaUnrealScriptFunctionData{0, 0, unreal_function, this, gameplay_state, std::move(callback_in_gameplay_state)}));
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
            else if (func_ptr && func_ptr == UObject::ProcessInternalInternal.get_function_address() &&
                     !unreal_function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Native))
            {
                ++m_last_generic_hook_id;
                auto [callback_data, _] = m_script_hook_callbacks.emplace(unreal_function->GetFullName(), LuaCallbackData{gameplay_state, nullptr, {}});
                callback_data->second.registry_indexes.emplace_back(LuaCallbackData::RegistryIndex{callback_in_gameplay_state, m_last_generic_hook_id});
                generic_pre_id = m_last_generic_hook_id;
                generic_post_id = m_last_generic_hook_id;
                Output::send<LogLevel::Verbose>(STR("[RegisterHook] Registered script hook ({}, {}) for {}\n"),
                                                generic_pre_id,
                                                generic_post_id,
                                                unreal_function->GetFullName());
            }
            else
            {
                StringType error_message{STR("Was unable to register a hook with Lua function 'RegisterHook', information:\n")};
                error_message.append(std::format(STR("UFunction::Func: {}\n"), std::bit_cast<void*>(func_ptr)));
                error_message.append(std::format(STR("ProcessInternal: {}\n"), UObject::ProcessInternalInternal.get_function_address()));
                error_message.append(std::format(STR("FUNC_Native: {}"), static_cast<uint32_t>(unreal_function->HasAnyFunctionFlags(EFunctionFlags::FUNC_Native))));
                return exit_script_with_error<2>(state, error_message);
            }

            return return_from_function_with_values(state, generic_pre_id, generic_post_id);
        });
        sol.set_function("UnregisterHook", [](sol::this_state state, StringType function_name, int32_t pre_id, int32_t post_id) {
            auto function_name_no_prefix = function_name.substr(function_name.find_first_of(L"/"), function_name.size());
            auto unreal_function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, function_name_no_prefix);
            if (!unreal_function)
            {
                return exit_script_with_error(
                        state,
                        STR("Tried to unregister a hook with Lua function 'UnregisterHook' but no UFunction with the specified name was found."));
            }

            bool is_native{};

            auto native_hook_pre_id_it = SolMod::m_generic_hook_id_to_native_hook_id.find(pre_id);
            auto native_hook_post_id_it = SolMod::m_generic_hook_id_to_native_hook_id.find(post_id);
            if (native_hook_pre_id_it != SolMod::m_generic_hook_id_to_native_hook_id.end() &&
                native_hook_post_id_it != SolMod::m_generic_hook_id_to_native_hook_id.end())
            {
                is_native = true;
            }

            if (is_native)
            {
                Output::send<LogLevel::Verbose>(STR("Unregistering native hook with pre-id: {}\n"), native_hook_pre_id_it->first);
                unreal_function->UnregisterHook(static_cast<int32_t>(native_hook_pre_id_it->second));
                Output::send<LogLevel::Verbose>(STR("Unregistering native hook with post-id: {}\n"), native_hook_post_id_it->first);
                unreal_function->UnregisterHook(static_cast<int32_t>(native_hook_post_id_it->second));
            }
            else
            {
                if (auto callback_data_it = SolMod::m_script_hook_callbacks.find(unreal_function->GetFullName());
                    callback_data_it != SolMod::m_script_hook_callbacks.end())
                {
                    Output::send<LogLevel::Verbose>(STR("Unregistering script hook with id: {}\n"), post_id);
                    auto& registry_indexes = callback_data_it->second.registry_indexes;
                    registry_indexes.erase(std::remove_if(registry_indexes.begin(),
                                                          registry_indexes.end(),
                                                          [&](LuaCallbackData::RegistryIndex& registry_index) -> bool {
                                                              return post_id == registry_index.identifier;
                                                          }),
                                           registry_indexes.end());
                }
            }
        });
        sol.set_function("DumpAllObjects", [] {
            UE4SSProgram::dump_all_objects_and_properties(UE4SSProgram::get_program().get_object_dumper_output_directory() + STR("\\") +
                                                          UE4SSProgram::m_object_dumper_file_name);
        });
        sol.set_function("GenerateSDK", [] {
            auto working_dir = StringType{UE4SSProgram::get_program().get_working_directory()};
            UE4SSProgram::get_program().generate_cxx_headers(working_dir + STR("\\CXXHeaderDump"));
        });
        sol.set_function("GenerateUHTCompatibleHeaders", [] {
            UE4SSProgram::get_program().generate_uht_compatible_headers();
        });
        sol.set_function("DumpStaticMeshes", [] {
            GUI::Dumpers::call_generate_static_mesh_file();
        });
        sol.set_function("DumpAllActors", [] {
            GUI::Dumpers::call_generate_all_actor_file();
        });
        sol.set_function("DumpUSMAP", [] {
            OutTheShade::generate_usmap();
        });
        sol.set_function("Test1", [](FName test_name) {
            Output::send<LogLevel::Verbose>(STR("test_name: {}\n"), test_name.ToString());
        });
        sol.set_function("Test2", [](EObjectFlags test_flags) {
            Output::send<LogLevel::Verbose>(STR("test_flags: {}\n"), (int)test_flags);
        });
        sol.set_function("Test3", [](EInternalObjectFlags test_internal_flags) {
            Output::send<LogLevel::Verbose>(STR("test_internal_flags: {}\n"), (int)test_internal_flags);
        });
        sol.set_function("Test4", [](bool test_bool) {
            Output::send<LogLevel::Verbose>(STR("test_bool: {}\n"), test_bool);
        });
        sol.set_function("Test5",
                         sol::policies(
                                 [](UObject* object) {
                                     Output::send<LogLevel::Verbose>(STR("object: {}\n"), object ? STR("non-nullptr") : STR("nullptr"));
                                 },
                                 &pointer_policy));
        sol.set_function("Test6",
                         sol::policies(
                                 [](UClass* uclass) {
                                     Output::send<LogLevel::Verbose>(STR("class: {}\n"), uclass ? STR("non-nullptr") : STR("nullptr"));
                                 },
                                 &pointer_policy));
        sol.set_function("StaticConstructObject",
                         sol::policies(sol::overload(
                                               [](UClass* uclass, UObject* outer) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               },
                                               [](UClass* uclass, UObject* outer, FName name) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   params.Name = name;
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               },
                                               [](UClass* uclass, UObject* outer, FName name, EObjectFlags flags) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   params.Name = name;
                                                   params.SetFlags = flags;
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               },
                                               [](UClass* uclass, UObject* outer, FName name, EObjectFlags flags, EInternalObjectFlags internal_flags) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   params.Name = name;
                                                   params.SetFlags = flags;
                                                   params.InternalSetFlags = internal_flags;
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               },
                                               [](UClass* uclass, UObject* outer, FName name, EObjectFlags flags, EInternalObjectFlags internal_flags, bool copy_transients_from_class_defaults) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   params.Name = name;
                                                   params.SetFlags = flags;
                                                   params.InternalSetFlags = internal_flags;
                                                   params.bCopyTransientsFromClassDefaults = copy_transients_from_class_defaults;
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               },
                                               [](UClass* uclass,
                                                  UObject* outer,
                                                  FName name,
                                                  EObjectFlags flags,
                                                  EInternalObjectFlags internal_flags,
                                                  bool copy_transients_from_class_defaults,
                                                  bool assume_template_is_archetype) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   params.Name = name;
                                                   params.SetFlags = flags;
                                                   params.InternalSetFlags = internal_flags;
                                                   params.bCopyTransientsFromClassDefaults = copy_transients_from_class_defaults;
                                                   params.bAssumeTemplateIsArchetype = assume_template_is_archetype;
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               },
                                               [](UClass* uclass,
                                                  UObject* outer,
                                                  FName name,
                                                  EObjectFlags flags,
                                                  EInternalObjectFlags internal_flags,
                                                  bool copy_transients_from_class_defaults,
                                                  bool assume_template_is_archetype,
                                                  UObject* tmplate) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   params.Name = name;
                                                   params.SetFlags = flags;
                                                   params.InternalSetFlags = internal_flags;
                                                   params.bCopyTransientsFromClassDefaults = copy_transients_from_class_defaults;
                                                   params.bAssumeTemplateIsArchetype = assume_template_is_archetype;
                                                   params.Template = tmplate;
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               },
                                               [](UClass* uclass,
                                                  UObject* outer,
                                                  FName name,
                                                  EObjectFlags flags,
                                                  EInternalObjectFlags internal_flags,
                                                  bool copy_transients_from_class_defaults,
                                                  bool assume_template_is_archetype,
                                                  UObject* tmplate,
                                                  FObjectInstancingGraph* instance_graph) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   params.Name = name;
                                                   params.SetFlags = flags;
                                                   params.InternalSetFlags = internal_flags;
                                                   params.bCopyTransientsFromClassDefaults = copy_transients_from_class_defaults;
                                                   params.bAssumeTemplateIsArchetype = assume_template_is_archetype;
                                                   params.Template = tmplate;
                                                   params.InstanceGraph = instance_graph;
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               },
                                               [](UClass* uclass,
                                                  UObject* outer,
                                                  FName name,
                                                  EObjectFlags flags,
                                                  EInternalObjectFlags internal_flags,
                                                  bool copy_transients_from_class_defaults,
                                                  bool assume_template_is_archetype,
                                                  UObject* tmplate,
                                                  FObjectInstancingGraph* instance_graph,
                                                  UPackage* external_package) {
                                                   FStaticConstructObjectParameters params{uclass, outer};
                                                   params.Name = name;
                                                   params.SetFlags = flags;
                                                   params.InternalSetFlags = internal_flags;
                                                   params.bCopyTransientsFromClassDefaults = copy_transients_from_class_defaults;
                                                   params.bAssumeTemplateIsArchetype = assume_template_is_archetype;
                                                   params.Template = tmplate;
                                                   params.InstanceGraph = instance_graph;
                                                   params.ExternalPackage = external_package;
                                                   return UObjectGlobals::StaticConstructObject(params);
                                               }),
                                       &pointer_policy));
        sol.set_function("RegisterCustomProperty", []() {
            // Re-implement to make registration simpler ?
            // If so we'd have to have a way for mods to provide a Lua callback to handle setting/getting property values.
            // In this case, we'd also need to make available some low-level tools to interact with memory.
        });
        sol.set_function("ForEachUObject", [](sol::this_state state, sol::function callback) {
            std::lock_guard<decltype(SolMod::m_thread_actions_mutex)> guard{SolMod::m_thread_actions_mutex};
            UObjectGlobals::ForEachUObject([&](UObject* object, int32_t chunk_index, int32_t object_index) {
                LoopAction return_value = LoopAction::Continue;
                auto lua_callback_result = call_function_dont_wrap_params_safe(state, callback, object, chunk_index, object_index);
                if (lua_callback_result.valid() && lua_callback_result.return_count() > 0)
                {
                    auto maybe_result = lua_callback_result.get<std::optional<int>>();
                    if (maybe_result.has_value() && maybe_result.value() == 1)
                    {
                        return_value = LoopAction::Break;
                    }
                }
                return return_value;
            });
        });
        sol.set_function("NotifyOnNewObject", [&](sol::this_state state, StringType class_name, sol::function callback) {
            auto instance_of_class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, class_name);
            if (!instance_of_class)
            {
                return exit_script_with_error(state,
                                              std::format(STR("Could not register callback to notify on new object because no class was found with name '{}'"),
                                                          class_name));
            }
            auto mod = get_mod_ref(state);
            if (!mod)
            {
                return exit_script_with_error(state, STR("Could not register callback to notify on new object because the pointer to 'Mod' was nullptr"));
            }
            auto gameplay_state = m_sol_gameplay_states.emplace_back(sol::thread::create(state)).lua_state();
            auto callback_in_gameplay_state = sol::function(gameplay_state, callback);
            m_static_construct_object_lua_callbacks.emplace_back(LuaCallbackData{gameplay_state, instance_of_class, {{std::move(callback_in_gameplay_state)}}});
        });
        sol.set_function("ExecuteWithDelay", [](sol::this_state state, int64_t delay_in_milliseconds, sol::function callback) {
            auto mod = get_mod_ref(state);
            if (!mod)
            {
                return exit_script_with_error(state, STR("Could not register callback to execute with delay because the pointer to 'Mod' was nullptr"));
            }

            sol::function callback_in_async_state = sol::function(mod->sol(State::Async), callback);

            std::lock_guard<decltype(m_actions_lock)> guard{mod->m_actions_lock};
            mod->m_pending_actions.emplace_back(AsyncAction{
                    callback_in_async_state,
                    ActionType::Delayed,
                    std::chrono::steady_clock::now(),
                    delay_in_milliseconds,
            });
        });
        sol.set_function("RegisterCustomEvent", [&](sol::this_state state, StringType event_name, sol::function callback) {
            auto mod = get_mod_ref(state);
            if (!mod)
            {
                return exit_script_with_error(state, STR("Could not register callback to execute custom event because the pointer to 'Mod' was nullptr"));
            }

            auto gameplay_state = m_sol_gameplay_states.emplace_back(sol::thread::create(state)).lua_state();
            sol::function callback_in_gameplay_state = sol::function(gameplay_state, callback);

            SolMod::m_custom_event_callbacks.emplace(event_name,
                                                     LuaCallbackData{
                                                             .lua = gameplay_state,
                                                             .instance_of_class = nullptr,
                                                             .registry_indexes = {{callback_in_gameplay_state}},
                                                     });
        });
        sol.set_function("UnregisterCustomEvent", [](StringType event_name) {
            SolMod::m_custom_event_callbacks.erase(event_name);
        });
        sol.set_function("RegisterInitGameStatePreHook", [&](sol::this_state state, sol::function callback) {
            auto mod = get_mod_ref(state);
            if (!mod)
            {
                return exit_script_with_error(state, STR("Could not register InitGameState pre-hook because the pointer to 'Mod' was nullptr"));
            }

            auto gameplay_state = m_sol_gameplay_states.emplace_back(sol::thread::create(state)).lua_state();
            sol::function callback_in_gameplay_state = sol::function(gameplay_state, callback);

            SolMod::m_init_game_state_pre_callbacks.emplace_back(LuaCallbackData{
                    .lua = gameplay_state,
                    .instance_of_class = nullptr,
                    .registry_indexes = {{callback_in_gameplay_state}},
            });
        });
        sol.set_function("RegisterInitGameStatePostHook", [&](sol::this_state state, sol::function callback) {
            auto mod = get_mod_ref(state);
            if (!mod)
            {
                return exit_script_with_error(state, STR("Could not register InitGameState post-hook because the pointer to 'Mod' was nullptr"));
            }

            auto gameplay_state = m_sol_gameplay_states.emplace_back(sol::thread::create(state)).lua_state();
            sol::function callback_in_gameplay_state = sol::function(gameplay_state, callback);

            SolMod::m_init_game_state_post_callbacks.emplace_back(LuaCallbackData{
                    .lua = gameplay_state,
                    .instance_of_class = nullptr,
                    .registry_indexes = {{callback_in_gameplay_state}},
            });
        });
        sol.set_function("RegisterConsoleCommandHandler", [&](sol::this_state state, const StringType& command_name, sol::function callback) {
            auto mod = get_mod_ref(state);
            if (!mod)
            {
                return exit_script_with_error(state, STR("Could not register a console command handler because the pointer to 'Mod' was nullptr"));
            }

            auto gameplay_state = m_sol_gameplay_states.emplace_back(sol::thread::create(state)).lua_state();
            sol::function callback_in_gameplay_state = sol::function(gameplay_state, callback);

            if (auto iter = SolMod::m_custom_command_lua_pre_callbacks.find(command_name); iter != SolMod::m_custom_command_lua_pre_callbacks.end())
            {
                iter->second.registry_indexes.emplace_back(callback_in_gameplay_state);
            }
            else
            {
                SolMod::m_custom_command_lua_pre_callbacks.emplace(command_name,
                                                                   LuaCallbackData{.lua = gameplay_state,
                                                                                   .instance_of_class = nullptr,
                                                                                   .registry_indexes = {{callback_in_gameplay_state}}});
            }
        });
    }
} // namespace RC
