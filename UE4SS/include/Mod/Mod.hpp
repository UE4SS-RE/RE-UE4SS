#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <Common.hpp>
#include <File/File.hpp>

namespace RC
{
    class UE4SSProgram;

    namespace Unreal
    {
        class UClass;
    }

    // Unique identifier for mods, used for safe cross-thread references
    using ModId = size_t;
    constexpr ModId InvalidModId = 0;

    class RC_UE4SS_API Mod
    {
      private:
        static inline std::atomic<ModId> s_next_mod_id{1};

      public:
        UE4SSProgram& m_program;

      protected:
        ModId m_mod_id{InvalidModId};
        StringType m_mod_name;
        std::filesystem::path m_mod_path;

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
        Mod(UE4SSProgram&, StringType&& mod_name, std::filesystem::path&& mod_path);
        virtual ~Mod() = default;

      public:
        auto get_id() const -> ModId { return m_mod_id; }
        auto get_name() const -> StringViewType;
        auto get_path() const -> const std::filesystem::path& { return m_mod_path; }

        virtual auto start_mod() -> void = 0;
        virtual auto uninstall() -> void = 0;

        auto set_installable(bool) -> void;
        auto is_installable() const -> bool;
        auto set_installed(bool) -> void;
        auto is_installed() const -> bool;
        auto is_started() const -> bool;

      public:
        auto get_program() -> UE4SSProgram& { return m_program; }
        auto get_program() const -> const UE4SSProgram& { return m_program; }
        // Main update from the program
        virtual auto fire_update() -> void;

        virtual auto fire_unreal_init() -> void {};

        virtual auto fire_ui_init() -> void {};

        // Called once when the program is starting, after mods are setup but before any mods have been started
        virtual auto fire_program_start() -> void {};

        // Async update
        // Used when the main update function would block other mods from executing their scripts
        virtual auto update_async() -> void;
    };
} // namespace RC
