#pragma once

#ifdef __linux__

#include <filesystem>
#include <optional>
#include <string>

namespace RC::LinuxStartup
{
    inline constexpr char Box64PreloadEnv[] = "BOX64_LD_PRELOAD";
    inline constexpr char TargetExecutableEnv[] = "UE4SS_LAUNCH_TARGET_EXE";
    inline constexpr char OriginalPreloadWasSetEnv[] = "UE4SS_LAUNCH_LD_PRELOAD_WAS_SET";
    inline constexpr char OriginalPreloadEnv[] = "UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD";
    inline constexpr char ModulePathEnv[] = "UE4SS_MODULE_PATH";

    enum class DecisionKind
    {
        LegacyStart,
        LauncherStart,
        TargetMismatch,
        Box64OrphanedPreload,
        InvalidLauncherState,
    };

    struct Decision
    {
        DecisionKind kind{DecisionKind::InvalidLauncherState};
        std::filesystem::path current_executable{};
        std::filesystem::path expected_executable{};
        std::string reason{};
    };

    auto evaluate(const std::filesystem::path& current_executable = "/proc/self/exe",
                  const std::filesystem::path& loaded_module = {}) noexcept -> Decision;
    auto restore_original_environment() noexcept -> std::optional<std::string>;
} // namespace RC::LinuxStartup

#endif
