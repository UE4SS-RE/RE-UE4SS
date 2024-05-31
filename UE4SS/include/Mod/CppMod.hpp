#pragma once

#include <vector>

#define NOMINMAX
#include <Windows.h>

#include <Mod/CppUserModBase.hpp>
#include <Mod/Mod.hpp>

namespace RC
{
    namespace LuaMadeSimple
    {
        class Lua;
    }

    class CppMod : public Mod
    {
      private:
        typedef CppUserModBase* (*start_type)();
        typedef void (*uninstall_type)(CppUserModBase*);

      private:
        std::wstring m_dlls_path;

        HMODULE m_main_dll_module = NULL;
        DLL_DIRECTORY_COOKIE m_dlls_path_cookie = NULL;
        start_type m_start_mod_func = nullptr;
        uninstall_type m_uninstall_mod_func = nullptr;

        CppUserModBase* m_mod = nullptr;

      public:
        CppMod(UE4SSProgram&, std::wstring&& mod_name, std::wstring&& mod_path);
        CppMod(CppMod&) = delete;
        CppMod(CppMod&&) = delete;
        ~CppMod() override;

      public:
        auto start_mod() -> void override;
        auto uninstall() -> void override;

        auto fire_on_lua_start(StringViewType mod_name,
                               LuaMadeSimple::Lua& lua,
                               LuaMadeSimple::Lua& main_lua,
                               LuaMadeSimple::Lua& async_lua,
                               std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void;

        auto fire_on_lua_start(LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, std::vector<LuaMadeSimple::Lua*>& hook_luas)
                -> void;

        auto fire_on_lua_stop(StringViewType mod_name,
                              LuaMadeSimple::Lua& lua,
                              LuaMadeSimple::Lua& main_lua,
                              LuaMadeSimple::Lua& async_lua,
                              std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void;

        auto fire_on_lua_stop(LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, std::vector<LuaMadeSimple::Lua*>& hook_luas)
                -> void;

        auto fire_unreal_init() -> void override;
        auto fire_ui_init() -> void override;
        auto fire_program_start() -> void override;
        auto fire_update() -> void override;
        auto fire_dll_load(std::wstring_view dll_name) -> void;
    };
} // namespace RC
