#ifndef UE4SS_REWRITTEN_CPPMODULE_HPP
#define UE4SS_REWRITTEN_CPPMODULE_HPP

#include <Windows.h>

#include <Mod/ICPPMod.hpp>
#include <Mod/Mod.hpp>

namespace RC
{
    class CppModule : public Mod
    {
    private:
        typedef ICPPMod* (*start_type)();
        typedef void (*uninstall_type)(ICPPMod*);

    private:
        std::wstring m_dlls_path;

        HMODULE m_main_dll_module = NULL;
        start_type m_start_mod_func = nullptr;
        uninstall_type m_uninstall_mod_func = nullptr;

        ICPPMod* m_mod = nullptr;

    public:
        CppModule(UE4SSProgram&, std::wstring&& mod_name, std::wstring&& mod_path);
        CppModule(CppModule&) = delete;
        CppModule(CppModule&&) = delete;
        ~CppModule();

    public:
        auto start_mod() -> void override;
        auto uninstall() -> void override;

        auto update() -> void override;
    };
}

#endif // UE4SS_REWRITTEN_CPPMODULE_HPP