#ifndef UE4SS_REWRITTEN_CPPMODUSERBASE_HPP
#define UE4SS_REWRITTEN_CPPMODUSERBASE_HPP

#include <vector>

#include <Common.hpp>
#include <File/Macros.hpp>

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

    // When making C++ mods, keep in mind that they will break if UE4SS and the mod don't use the same C Runtime library version
    // This includes them being compiled in different configurations (Debug/Release).
    class CppUserModBase
    {
    public:
        StringType ModName{};
        StringType ModVersion{};
        StringType ModDescription{};
        StringType ModAuthors{};
        StringType ModIntendedSDKVersion{};

    public:
        RC_UE4SS_API CppUserModBase();
        RC_UE4SS_API virtual ~CppUserModBase() = default;

    public:
        RC_UE4SS_API auto virtual on_update() -> void {}

        // The 'Unreal' module has been initialized.
        // Before this fires, you cannot use anything in the 'Unreal' namespace.
        RC_UE4SS_API auto virtual on_unreal_init() -> void {}

        RC_UE4SS_API auto virtual on_program_start() -> void {}

        /**
         * Executes after a Lua mod of the same name is started.
         * @param lua This is the main Lua instance.
         * @param main_lua This is the main Lua thread instance.
         * @param async_lua This is the Lua instance for asynchronous things like ExecuteAsync and ExecuteWithDelay.
         * @param hook_luas This is a container of Lua instances that are used for game-thread hooks like ExecuteInGameThread.
         */
        RC_UE4SS_API auto virtual on_lua_start(LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void {}

        RC_UE4SS_API auto virtual on_dll_load(std::wstring_view dll_name) -> void {}
    };
}

#endif // UE4SS_REWRITTEN_CPPMODUSERBASE_HPP