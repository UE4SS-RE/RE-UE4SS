#pragma once

#include <memory>
#include <vector>

#include <Common.hpp>
#include <File/Macros.hpp>
#ifdef HAS_UI
#include <GUI/GUITab.hpp>
#endif

namespace RC
{
    struct ModMetadata
    {
        const UEStringType ModName{};
        const UEStringType ModVersion{};
        const UEStringType ModDescription{};
        const UEStringType ModAuthors{};
        const UEStringType ModIntendedSDKVersion{};
    };

    namespace LuaMadeSimple
    {
        class Lua;
    }

    // When making C++ mods, keep in mind that they will break if UE4SS and the mod don't use the same C Runtime library version
    // This includes them being compiled in different configurations (Debug/Release).
    class CppUserModBase
    {
      protected:
#ifdef HAS_UI
        std::vector<std::shared_ptr<GUI::GUITab>> GUITabs{};
#endif
      public:
        UEStringType ModName{};
        UEStringType ModVersion{};
        UEStringType ModDescription{};
        UEStringType ModAuthors{};
        UEStringType ModIntendedSDKVersion{};

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

        RC_UE4SS_API virtual auto on_program_start() -> void
        {
        }

        /**
         * Executes after a Lua mod is started.
         * Executes for every Lua mod that is starting.
         * @param mod_name This is the name of the Lua mod that was started.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas This is a container of Lua instances that are used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API virtual auto on_lua_start(SystemStringViewType mod_name,
                                               LuaMadeSimple::Lua& lua,
                                               LuaMadeSimple::Lua& main_lua,
                                               LuaMadeSimple::Lua& async_lua,
                                               std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
        {
        }

        /**
         * Executes after a Lua mod of the same name is started.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas This is a container of Lua instances that are used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API virtual auto on_lua_start(LuaMadeSimple::Lua& lua,
                                               LuaMadeSimple::Lua& main_lua,
                                               LuaMadeSimple::Lua& async_lua,
                                               std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
        {
        }

        /**
         * Executes before a Lua mod is about to be stopped.
         * Executes for every Lua mod that is stopping.
         * @param mod_name This is the name of the Lua mod that is about to be stopped.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas This is a container of Lua instances that are used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API virtual auto on_lua_stop(SystemStringViewType mod_name,
                                              LuaMadeSimple::Lua& lua,
                                              LuaMadeSimple::Lua& main_lua,
                                              LuaMadeSimple::Lua& async_lua,
                                              std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
        {
        }

        /**
         * Executes before a Lua mod of the same name is about to be stopped.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas This is a container of Lua instances that are used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API virtual auto on_lua_stop(LuaMadeSimple::Lua& lua,
                                              LuaMadeSimple::Lua& main_lua,
                                              LuaMadeSimple::Lua& async_lua,
                                              std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
        {
        }

        RC_UE4SS_API virtual auto on_dll_load(SystemStringViewType dll_name) -> void
        {
        }

#ifdef HAS_UI
        RC_UE4SS_API virtual auto render_tab() -> void{};

      protected:
        RC_UE4SS_API auto register_tab(UEStringViewType tab_name, GUI::GUITab::RenderFunctionType) -> void;
#endif
    };
} // namespace RC
