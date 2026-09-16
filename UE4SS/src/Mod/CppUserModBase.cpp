#include <vector>
#include <unordered_map>

#include <Mod/CppMod.hpp>
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>
#include <String/StringType.hpp>

namespace RC
{
    class ModEventDispatcher
    {
    private:
        ModEventDispatcher();

    public:
        // Singleton for retrieving (and lazy-constructing) a ModEventDispatcher for the supplied mod.
        static auto Get(CppUserModBase*) -> ModEventDispatcher&;

    public:
        // Members may not use STL types.
        ModEvent_OnUE4SSUpdate on_ue4ss_update{};
        ModEvent_OnUnrealInit on_unreal_init{};
        ModEvent_OnUIInit on_ui_init{};
        ModEvent_OnProgramStart on_program_start{};
        ModEvent_OnDllLoad on_dll_load{};
        ModEvent_OnLuaStart on_lua_start{};
        ModEvent_OnLuaStartSelf on_lua_start_self{};
        ModEvent_OnLuaStop on_lua_stop{};
        ModEvent_OnLuaStopSelf on_lua_stop_self{};
        ModEvent_OnAllCppModsLoaded on_all_cpp_mods_loaded{};
    };

    ModEventDispatcher::ModEventDispatcher() = default;

    auto ModEventDispatcher::Get(CppUserModBase* mod) -> ModEventDispatcher&
    {
        static std::unordered_map<CppUserModBase*, ModEventDispatcher> s_dispatchers{};
        auto [it, _] = s_dispatchers.try_emplace(mod, ModEventDispatcher{});
        return it->second;
    }

    CppUserModBase::CppUserModBase()
    {
        if (ModIntendedSDKVersion.empty())
        {
            ModIntendedSDKVersion = fmt::format(STR("{}.{}.{}"), UE4SS_LIB_VERSION_MAJOR, UE4SS_LIB_VERSION_MINOR, UE4SS_LIB_VERSION_HOTFIX);
        }
    }

    CppUserModBase::~CppUserModBase()
    {
        for (const auto& tab : GUITabs)
        {
            if (tab)
            {
                UE4SSProgram::get_program().remove_gui_tab(tab);
            }
        }
        GUITabs.clear();

#ifdef HAS_INPUT
        UE4SSProgram::get_program().m_input_handler.get_events_safe([&](auto& key_set) {
            std::erase_if(key_set.key_data, [&](auto& item) -> bool {
                auto& [_, key_data] = item;
                bool were_all_events_registered_from_this_mod = true;
                std::erase_if(key_data, [&](Input::KeyData& key_data) -> bool {
                    // custom_data == 1: Bind came from Lua, and custom_data2 is nullptr.
                    // custom_data == 2: Bind came from C++, and custom_data2 is a pointer to KeyDownEventData. Must free it.
                    auto event_data = static_cast<KeyDownEventData*>(key_data.custom_data2);
                    if (key_data.custom_data == 2 && event_data && event_data->mod == this)
                    {
                        delete event_data;
                        return true;
                    }
                    else
                    {
                        were_all_events_registered_from_this_mod = false;
                        return false;
                    }
                });

                return were_all_events_registered_from_this_mod;
            });
        });
#endif
    }

    auto CppUserModBase::register_tab(StringViewType tab_name, GUI::GUITab::RenderFunctionType render_function) -> void
    {
        auto& tab = GUITabs.emplace_back(std::make_shared<GUI::GUITab>(tab_name, render_function, this));
        UE4SSProgram::get_program().add_gui_tab(tab);
    }

    auto CppUserModBase::register_keydown_event(Input::Key key, const Input::EventCallbackCallable& callback, uint8_t custom_data) -> void
    {
        UE4SSProgram::get_program().register_keydown_event(key, callback, 2, new KeyDownEventData{custom_data, this});
    }

    auto CppUserModBase::register_keydown_event(Input::Key key,
                                                const Input::Handler::ModifierKeyArray& callback,
                                                const Input::EventCallbackCallable& modifier_keys,
                                                uint8_t custom_data) -> void
    {
        UE4SSProgram::get_program().register_keydown_event(key, callback, modifier_keys, 2, new KeyDownEventData{custom_data, this});
    }

    auto CppUserModBase::register_on_ue4ss_update(ModEvent_OnUE4SSUpdate func) -> void
    {
        ModEventDispatcher::Get(this).on_ue4ss_update = func;
    }

    auto CppUserModBase::register_on_unreal_init(ModEvent_OnUnrealInit func) -> void
    {
        ModEventDispatcher::Get(this).on_unreal_init = func;
    }

    auto CppUserModBase::register_on_ui_init(ModEvent_OnUIInit func) -> void
    {
        ModEventDispatcher::Get(this).on_ui_init = func;
    }

    auto CppUserModBase::register_on_program_start(ModEvent_OnProgramStart func) -> void
    {
        ModEventDispatcher::Get(this).on_program_start = func;
    }

    auto CppUserModBase::register_on_dll_load(ModEvent_OnDllLoad func) -> void
    {
        ModEventDispatcher::Get(this).on_dll_load = func;
    }

    auto CppUserModBase::register_on_lua_start(ModEvent_OnLuaStart func) -> void
    {
        ModEventDispatcher::Get(this).on_lua_start = func;
    }

    auto CppUserModBase::register_on_lua_start(ModEvent_OnLuaStartSelf func) -> void
    {
        ModEventDispatcher::Get(this).on_lua_start_self = func;
    }

    auto CppUserModBase::register_on_lua_stop(ModEvent_OnLuaStop func) -> void
    {
        ModEventDispatcher::Get(this).on_lua_stop = func;
    }

    auto CppUserModBase::register_on_lua_stop(ModEvent_OnLuaStopSelf func) -> void
    {
        ModEventDispatcher::Get(this).on_lua_stop_self = func;
    }

    auto CppUserModBase::register_on_all_cpp_mods_loaded(ModEvent_OnAllCppModsLoaded func) -> void
    {
        ModEventDispatcher::Get(this).on_all_cpp_mods_loaded = func;
    }

    auto CppUserModBase::dispatch_on_ue4ss_update() -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_ue4ss_update;
        if (func)
        {
            func(this);
        }
    }

    auto CppUserModBase::dispatch_on_unreal_init() -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_unreal_init;
        if (func)
        {
            func(this);
        }
    }

    auto CppUserModBase::dispatch_on_ui_init() -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_ui_init;
        if (func)
        {
            func(this);
        }
    }

    auto CppUserModBase::dispatch_on_program_start() -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_program_start;
        if (func)
        {
            func(this);
        }
    }

    auto CppUserModBase::dispatch_on_dll_load(StringViewType dll_name) -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_dll_load;
        if (func)
        {
            func(this, dll_name);
        }
    }

    auto CppUserModBase::dispatch_on_lua_start(
            StringViewType mod_name, LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, LuaMadeSimple::Lua* hook_lua) -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_lua_start;
        if (func)
        {
            func(this, mod_name, lua, main_lua, async_lua, hook_lua);
        }
    }

    auto CppUserModBase::dispatch_on_lua_start(LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, LuaMadeSimple::Lua* hook_lua)
            -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_lua_start_self;
        if (func)
        {
            func(this, lua, main_lua, async_lua, hook_lua);
        }
    }

    auto CppUserModBase::dispatch_on_lua_stop(
            StringViewType mod_name, LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, LuaMadeSimple::Lua* hook_lua) -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_lua_stop;
        if (func)
        {
            func(this, mod_name, lua, main_lua, async_lua, hook_lua);
        }
    }

    auto CppUserModBase::dispatch_on_lua_stop(LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, LuaMadeSimple::Lua* hook_lua)
            -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_lua_stop_self;
        if (func)
        {
            func(this, lua, main_lua, async_lua, hook_lua);
        }
    }

    auto CppUserModBase::dispatch_on_all_cpp_mods_loaded()
            -> void
    {
        const auto func = ModEventDispatcher::Get(this).on_all_cpp_mods_loaded;
        if (func)
        {
            func(this);
        }
    }
} // namespace RC
