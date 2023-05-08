#ifndef UE4SS_REWRITTEN_MOD_HPP
#define UE4SS_REWRITTEN_MOD_HPP

#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <File/File.hpp>
#include <Common.hpp>

namespace RC
{
    class UE4SSProgram;

    namespace Unreal
    {
        class UClass;
    }

    class RC_UE4SS_API Mod
    {
    public:
        UE4SSProgram& m_program;

    protected:
#pragma warning(disable : 4251)
        std::wstring m_mod_name;
        std::wstring m_mod_path;
#pragma warning(default : 4251)


    protected:
        // Whether the mod can be installed
        // This is true by default and is only false if the state of the mod won't allow for a successful installation
        bool m_installable{true};
        bool m_installed{false};
        mutable bool m_is_started{false};

    public:
        enum class IsTrueMod
        {
            Yes,
            No
        };

    public:
        Mod(UE4SSProgram&, std::wstring&& mod_name, std::wstring&& mod_path);
        virtual ~Mod() = default;

    public:
        auto get_name() const -> std::wstring_view;

        auto virtual start_mod() -> void = 0;
        auto virtual uninstall() -> void = 0;

        auto set_installable(bool) -> void;
        auto is_installable() const -> bool;
        auto set_installed(bool) -> void;
        auto is_installed() const -> bool;
        auto is_started() const -> bool;

    public:
        // Main update from the program
        auto virtual fire_update() -> void;

        auto virtual fire_unreal_init() -> void {};

        // Called once when the program is starting, after mods are setup but before any mods have been started
        auto virtual fire_program_start() -> void {};

        // Async update
        // Used when the main update function would block other mods from executing their scripts
        auto virtual update_async() -> void;
    };
}


#endif //UE4SS_REWRITTEN_MOD_HPP
