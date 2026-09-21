#pragma once

#include <memory>
#include <vector>

#include <Common.hpp>
#include <File/Macros.hpp>
#include <GUI/GUITab.hpp>
#include <Input/Handler.hpp>

#include <String/StringType.hpp>
#include <LibBoundaryWrappers.hpp>

namespace RC
{
    struct ModMetadata
    {
        const StringType ModName{};
        const StringType ModVersion{};
        const StringType ModDescription{};
        const StringType ModAuthors{};
        const StringType ModIntendedSDKVersion{};
    };

    namespace LuaMadeSimple
    {
        class Lua;
    }

    using ModEvent_OnUE4SSUpdate = void(*)(class CppUserModBase*);
    using ModEvent_OnUnrealInit = void(*)(class CppUserModBase*);
    using ModEvent_OnUIInit = void(*)(class CppUserModBase*);
    using ModEvent_OnProgramStart = void(*)(class CppUserModBase*);
    using ModEvent_OnDllLoad = void(*)(class CppUserModBase*, CStringView);
    using ModEvent_OnLuaStart = void(*)(class CppUserModBase*, CStringView, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua*);
    using ModEvent_OnLuaStartSelf = void(*)(class CppUserModBase*, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua*);
    using ModEvent_OnLuaStop = void(*)(class CppUserModBase*, CStringView, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua*);
    using ModEvent_OnLuaStopSelf = void(*)(class CppUserModBase*, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua*);
    using ModEvent_OnAllCppModsLoaded = void(*)(class CppUserModBase*);

    class ModEventDispatcher;

    // When making C++ mods, keep in mind that they will break if UE4SS and the mod don't use the same C Runtime library version
    // This includes them being compiled in different configurations (Debug/Release).
    class CppUserModBase
    {
      protected:
        std::vector<std::shared_ptr<GUI::GUITab>> GUITabs{};

      public:
        StringType ModName{};
        StringType ModVersion{};
        StringType ModDescription{};
        StringType ModAuthors{};
        StringType ModIntendedSDKVersion{};

      public:
        RC_UE4SS_API CppUserModBase();
        RC_UE4SS_API virtual ~CppUserModBase();

      public:
        RC_UE4SS_API virtual auto on_update() -> void
        {
        }

        // The 'Unreal' module has been initialized.
        // Before this fires, you cannot use anything in the 'Unreal' namespace.
        RC_UE4SS_API virtual auto on_unreal_init() -> void
        {
        }

        // The UI module has been initialized.
        // This is where you need to use the 'UE4SS_ENABLE_IMGUI' macro if you want to utilize the imgui context of UE4SS.
        RC_UE4SS_API virtual auto on_ui_init() -> void
        {
        }

        RC_UE4SS_API virtual auto on_program_start() -> void
        {
        }

        /**
         * Executes after a Lua mod is started (DEPRECATED).
         * @deprecated Use the overload with LuaMadeSimple::Lua* hook_lua instead. This overload may be removed in the next release.
         * @param mod_name This is the name of the Lua mod that was started.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas DEPRECATED: This container previously held multiple hook Lua instances. Now only one hook instance is used.
         */
        [[deprecated("The hook_luas vector parameter is deprecated. Use the single hook_lua pointer overload instead. This overload may be removed in the next release")]]
        RC_UE4SS_API virtual auto on_lua_start(StringViewType mod_name,
                                               LuaMadeSimple::Lua& lua,
                                               LuaMadeSimple::Lua& main_lua,
                                               LuaMadeSimple::Lua& async_lua,
                                               std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
        {
        }

        /**
         * Executes after a Lua mod of the same name is started (DEPRECATED).
         * @deprecated Use the overload with LuaMadeSimple::Lua* hook_lua instead. This overload may be removed in the next release.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas DEPRECATED: This container previously held multiple hook Lua instances. Now only one hook instance is used.
         */
        [[deprecated("The hook_luas vector parameter is deprecated. Use the single hook_lua pointer overload instead. This overload may be removed in the next release")]]
        RC_UE4SS_API virtual auto on_lua_start(LuaMadeSimple::Lua& lua,
                                               LuaMadeSimple::Lua& main_lua,
                                               LuaMadeSimple::Lua& async_lua,
                                               std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
        {
        }

        /**
         * Executes before a Lua mod is about to be stopped (DEPRECATED).
         * @deprecated Use the overload with LuaMadeSimple::Lua* hook_lua instead. This overload may be removed in the next release.
         * @param mod_name This is the name of the Lua mod that is about to be stopped.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas DEPRECATED: This container previously held multiple hook Lua instances. Now only one hook instance is used.
         */
        [[deprecated("The hook_luas vector parameter is deprecated. Use the single hook_lua pointer overload instead. This overload may be removed in the next release")]]
        RC_UE4SS_API virtual auto on_lua_stop(StringViewType mod_name,
                                              LuaMadeSimple::Lua& lua,
                                              LuaMadeSimple::Lua& main_lua,
                                              LuaMadeSimple::Lua& async_lua,
                                              std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
        {
        }

        /**
         * Executes before a Lua mod of the same name is about to be stopped (DEPRECATED).
         * @deprecated Use the overload with LuaMadeSimple::Lua* hook_lua instead. This overload may be removed in the next release.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas DEPRECATED: This container previously held multiple hook Lua instances. Now only one hook instance is used.
         */
        [[deprecated("The hook_luas vector parameter is deprecated. Use the single hook_lua pointer overload instead. This overload may be removed in the next release")]]
        RC_UE4SS_API virtual auto on_lua_stop(LuaMadeSimple::Lua& lua,
                                              LuaMadeSimple::Lua& main_lua,
                                              LuaMadeSimple::Lua& async_lua,
                                              std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
        {
        }

        RC_UE4SS_API virtual auto on_dll_load(StringViewType dll_name) -> void
        {
        }

        RC_UE4SS_API virtual auto render_tab() -> void {};

        /**
         * Executes after a Lua mod is started.
         * Executes for every Lua mod that is starting.
         * @param mod_name This is the name of the Lua mod that was started.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_lua This is the Lua instance that is used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API virtual auto on_lua_start(StringViewType mod_name,
                                               LuaMadeSimple::Lua& lua,
                                               LuaMadeSimple::Lua& main_lua,
                                               LuaMadeSimple::Lua& async_lua,
                                               LuaMadeSimple::Lua* hook_lua) -> void
        {
        }

        /**
         * Executes after a Lua mod of the same name is started.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_lua This is the Lua instance that is used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API virtual auto on_lua_start(LuaMadeSimple::Lua& lua,
                                               LuaMadeSimple::Lua& main_lua,
                                               LuaMadeSimple::Lua& async_lua,
                                               LuaMadeSimple::Lua* hook_lua) -> void
        {
        }

        /**
         * Executes before a Lua mod is about to be stopped.
         * Executes for every Lua mod that is stopping.
         * @param mod_name This is the name of the Lua mod that is about to be stopped.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_lua This is the Lua instance that is used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API virtual auto on_lua_stop(StringViewType mod_name,
                                              LuaMadeSimple::Lua& lua,
                                              LuaMadeSimple::Lua& main_lua,
                                              LuaMadeSimple::Lua& async_lua,
                                              LuaMadeSimple::Lua* hook_lua) -> void
        {
        }

        /**
         * Executes before a Lua mod of the same name is about to be stopped.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_lua This is the Lua instance that is used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API virtual auto on_lua_stop(LuaMadeSimple::Lua& lua,
                                              LuaMadeSimple::Lua& main_lua,
                                              LuaMadeSimple::Lua& async_lua,
                                              LuaMadeSimple::Lua* hook_lua) -> void
        {
        }

        /**
         * Executes after every C++ mod has been loaded.
         */
        RC_UE4SS_API virtual auto on_cpp_mods_loaded() -> void
        {
        }

      protected:
        RC_UE4SS_API auto register_tab(StringViewType tab_name, GUI::GUITab::RenderFunctionType) -> void;
        RC_UE4SS_API auto register_keydown_event(Input::Key, const Input::EventCallbackCallable&, uint8_t custom_data = 0) -> void;
        RC_UE4SS_API auto register_keydown_event(Input::Key,
                                                 const Input::Handler::ModifierKeyArray&,
                                                 const Input::EventCallbackCallable&,
                                                 uint8_t custom_data = 0) -> void;

        // Mod event dispatch registration.
        // Call in your mod constructor.
        RC_UE4SS_API auto register_on_ue4ss_update(ModEvent_OnUE4SSUpdate) -> void;
        RC_UE4SS_API auto register_on_unreal_init(ModEvent_OnUnrealInit) -> void;
        RC_UE4SS_API auto register_on_ui_init(ModEvent_OnUIInit) -> void;
        RC_UE4SS_API auto register_on_program_start(ModEvent_OnProgramStart) -> void;
        RC_UE4SS_API auto register_on_dll_load(ModEvent_OnDllLoad) -> void;
        RC_UE4SS_API auto register_on_lua_start(ModEvent_OnLuaStart) -> void;
        RC_UE4SS_API auto register_on_lua_start(ModEvent_OnLuaStartSelf) -> void;
        RC_UE4SS_API auto register_on_lua_stop(ModEvent_OnLuaStop) -> void;
        RC_UE4SS_API auto register_on_lua_stop(ModEvent_OnLuaStopSelf) -> void;
        RC_UE4SS_API auto register_on_all_cpp_mods_loaded(ModEvent_OnAllCppModsLoaded) -> void;

    public:
        // Do not call! These functions are used internally to dispatch callbacks, for example on_unreal_init.
        auto dispatch_on_ue4ss_update() -> void;
        auto dispatch_on_unreal_init() -> void;
        auto dispatch_on_ui_init() -> void;
        auto dispatch_on_program_start() -> void;
        auto dispatch_on_dll_load(StringViewType) -> void;
        auto dispatch_on_lua_start(StringViewType, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua*) -> void;
        auto dispatch_on_lua_start(LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua*) -> void;
        auto dispatch_on_lua_stop(StringViewType, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua*) -> void;
        auto dispatch_on_lua_stop(LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua&, LuaMadeSimple::Lua*) -> void;
        auto dispatch_on_all_cpp_mods_loaded() -> void;
    };
} // namespace RC
