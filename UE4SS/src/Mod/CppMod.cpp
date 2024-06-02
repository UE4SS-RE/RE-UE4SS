#define NOMINMAX

#include <filesystem>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <Mod/CppMod.hpp>

#ifdef LINUX
#define printf_s printf
#define _GNU_SOURCE
#include <dlfcn.h>
#endif

namespace RC
{
    CppMod::CppMod(UE4SSProgram& program, SystemStringType&& mod_name, SystemStringType&& mod_path) : Mod(program, std::move(mod_name), std::move(mod_path))
    {
        auto resolved_dlls_path = File::get_path_if_exists(m_mod_path, SYSSTR("dlls"));
        if (resolved_dlls_path)
        {
            m_dlls_path = to_system_string(*resolved_dlls_path);
        }
        else
        {
            Output::send<LogLevel::Warning>(SYSSTR("Could not find the dlls folder for mod {}\n"), m_mod_name);
            set_installable(false);
            return;
        }
#define STRINGIFY(x) #x
#define CONCATENATE_WIDE_STRING(name, ext) SYSSTR(name) STRINGIFY(ext)
        auto resolved_dll_path = File::get_path_if_exists(m_dlls_path, CONCATENATE_WIDE_STRING("main", DLLEXT));
        if (!resolved_dll_path)
        {
            Output::send<LogLevel::Warning>(SYSSTR("Could not find the main dll for mod {}\n"), m_mod_name);
            set_installable(false);
            return;
        }
        auto dll_path = to_system_string(*resolved_dll_path);
#ifdef WIN32
        // Add mods dlls directory to search path for dynamic/shared linked libraries in mods
        m_dlls_path_cookie = AddDllDirectory(m_dlls_path.c_str());
        m_main_dll_module = LoadLibraryExW(dll_path.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

        if (!m_main_dll_module)
        {
            Output::send<LogLevel::Warning>(SYSSTR("Failed to load dll <{}> for mod {}, error code: 0x{:x}\n"),
                                            to_system_string(dll_path),
                                            m_mod_name,
                                            GetLastError());
            set_installable(false);
            return;
        }

        m_start_mod_func = reinterpret_cast<start_type>(GetProcAddress(m_main_dll_module, "start_mod"));
        m_uninstall_mod_func = reinterpret_cast<uninstall_type>(GetProcAddress(m_main_dll_module, "uninstall_mod"));

        if (!m_start_mod_func || !m_uninstall_mod_func)
        {
            Output::send<LogLevel::Warning>(SYSSTR("Failed to find exported mod lifecycle functions for mod {}\n"), m_mod_name);

            FreeLibrary(m_main_dll_module);
            m_main_dll_module = NULL;

            set_installable(false);
            return;
        }

#else
        // use RTLD_LOCAL to avoid symbol conflicts
        m_dl_handle = dlopen(dll_path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!m_dl_handle)
        {
            Output::send<LogLevel::Warning>(SYSSTR("Failed to load dll <{}> for mod {}, because: {}\n"), to_system_string(dll_path), m_mod_name, dlerror());
            set_installable(false);
            return;
        }

        m_start_mod_func = reinterpret_cast<start_type>(dlsym(m_dl_handle, "start_mod"));
        m_uninstall_mod_func = reinterpret_cast<uninstall_type>(dlsym(m_dl_handle, "uninstall_mod"));

        if (!m_start_mod_func || !m_uninstall_mod_func)
        {
            Output::send<LogLevel::Warning>(SYSSTR("Failed to find exported mod lifecycle functions for mod {}\n"), m_mod_name);
            dlclose(m_dl_handle);
            m_dl_handle = NULL;
            set_installable(false);
            return;
        }
#endif
    }

    auto CppMod::start_mod() -> void
    {
        try
        {
            m_mod = m_start_mod_func();
            m_is_started = m_mod != nullptr;
        }
        catch (std::exception& e)
        {
            if (!Output::has_internal_error())
            {
                Output::send<LogLevel::Warning>(SYSSTR("Failed to load dll <{}> for mod {}, because: {}\n"),
                                                to_system_string(std::filesystem::path{m_dlls_path} / CONCATENATE_WIDE_STRING("main", DLLEXT)),
                                                m_mod_name,
                                                e.what());
            }
            else
            {
                printf_s("Internal Error: %s\n", e.what());
            }
        }
    }

    auto CppMod::uninstall() -> void
    {
        Output::send(SYSSTR("Stopping C++ mod '{}' for uninstall\n"), m_mod_name);
        if (m_mod && m_uninstall_mod_func)
        {
            m_uninstall_mod_func(m_mod);
        }
    }

    auto CppMod::fire_on_lua_start(SystemStringViewType mod_name,
                                   LuaMadeSimple::Lua& lua,
                                   LuaMadeSimple::Lua& main_lua,
                                   LuaMadeSimple::Lua& async_lua,
                                   std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
    {
        if (m_mod)
        {
            m_mod->on_lua_start(mod_name, lua, main_lua, async_lua, hook_luas);
        }
    }

    auto CppMod::fire_on_lua_start(LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, std::vector<LuaMadeSimple::Lua*>& hook_luas)
            -> void
    {
        if (m_mod)
        {
            m_mod->on_lua_start(lua, main_lua, async_lua, hook_luas);
        }
    }

    auto CppMod::fire_on_lua_stop(SystemStringViewType mod_name,
                                  LuaMadeSimple::Lua& lua,
                                  LuaMadeSimple::Lua& main_lua,
                                  LuaMadeSimple::Lua& async_lua,
                                  std::vector<LuaMadeSimple::Lua*>& hook_luas) -> void
    {
        if (m_mod)
        {
            m_mod->on_lua_stop(mod_name, lua, main_lua, async_lua, hook_luas);
        }
    }

    auto CppMod::fire_on_lua_stop(LuaMadeSimple::Lua& lua, LuaMadeSimple::Lua& main_lua, LuaMadeSimple::Lua& async_lua, std::vector<LuaMadeSimple::Lua*>& hook_luas)
            -> void
    {
        if (m_mod)
        {
            m_mod->on_lua_stop(lua, main_lua, async_lua, hook_luas);
        }
    }

    auto CppMod::fire_unreal_init() -> void
    {
        if (m_mod)
        {
            m_mod->on_unreal_init();
        }
    }

    auto CppMod::fire_ui_init() -> void
    {
        if (m_mod)
        {
            m_mod->on_ui_init();
        }
    }

    auto CppMod::fire_program_start() -> void
    {
        if (m_mod)
        {
            m_mod->on_program_start();
        }
    }

    auto CppMod::fire_update() -> void
    {
        if (m_mod)
        {
            m_mod->on_update();
        }
    }

    auto CppMod::fire_dll_load(SystemStringViewType dll_name) -> void
    {
        if (m_mod)
        {
            m_mod->on_dll_load(dll_name);
        }
    }

    CppMod::~CppMod()
    {
#ifdef WIN32
        if (m_main_dll_module)
        {
            FreeLibrary(m_main_dll_module);
            RemoveDllDirectory(m_dlls_path_cookie);
        }
#else
        if (m_dl_handle)
        {
            dlclose(m_dl_handle);
            m_dl_handle = NULL;
        }
#endif
    }
} // namespace RC