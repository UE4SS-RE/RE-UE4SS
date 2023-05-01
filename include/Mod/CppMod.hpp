#ifndef UE4SS_REWRITTEN_CPPMOD_HPP
#define UE4SS_REWRITTEN_CPPMOD_HPP

#include <Windows.h>

#include <Mod/CppUserModBase.hpp>
#include <Mod/Mod.hpp>

namespace RC
{
    class CppMod : public Mod
    {
    private:
        typedef CppUserModBase* (*start_type)();
        typedef void (*uninstall_type)(CppUserModBase*);

    private:
        std::wstring m_dlls_path;

        HMODULE m_main_dll_module = NULL;
        start_type m_start_mod_func = nullptr;
        uninstall_type m_uninstall_mod_func = nullptr;

        CppUserModBase* m_mod = nullptr;

    public:
        CppMod(UE4SSProgram&, std::wstring&& mod_name, std::wstring&& mod_path);
        CppMod(CppMod&) = delete;
        CppMod(CppMod&&) = delete;
        ~CppMod();

    public:
        auto start_mod() -> void override;
        auto uninstall() -> void override;

        auto fire_unreal_init() -> void override;
        auto fire_program_start() -> void override;
        auto fire_update() -> void override;
    };
}

#endif // UE4SS_REWRITTEN_CPPMOD_HPP