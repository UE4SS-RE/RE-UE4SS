#ifdef __linux__

#include <Platform/Linux/StartupPolicy.hpp>

#include <cstdlib>
#include <cstring>
#include <initializer_list>

namespace RC::LinuxStartup
{
    auto evaluate(const std::filesystem::path& current_executable) noexcept -> Decision
    {
        const auto* expected_value = std::getenv(TargetExecutableEnv);
        if (!expected_value)
        {
            return {.kind = DecisionKind::LegacyStart};
        }
        if (expected_value[0] == '\0')
        {
            return {.kind = DecisionKind::InvalidLauncherState, .reason = "empty_target"};
        }

        Decision result{.expected_executable = expected_value};
        std::error_code current_error;
        result.current_executable = std::filesystem::canonical(current_executable, current_error);
        if (current_error)
        {
            result.kind = DecisionKind::InvalidLauncherState;
            result.reason = "current_executable_unresolved";
            return result;
        }

        std::error_code equivalent_error;
        const bool equivalent = std::filesystem::equivalent(result.current_executable, result.expected_executable, equivalent_error);
        if (equivalent_error)
        {
            result.kind = DecisionKind::InvalidLauncherState;
            result.reason = "target_compare_failed";
            return result;
        }

        result.kind = equivalent ? DecisionKind::LauncherStart : DecisionKind::TargetMismatch;
        result.reason = equivalent ? "target_match" : "target_mismatch";
        return result;
    }

    auto restore_original_environment() noexcept -> std::optional<std::string>
    {
        const auto* was_set = std::getenv(OriginalPreloadWasSetEnv);
        if (!was_set || (std::strcmp(was_set, "0") != 0 && std::strcmp(was_set, "1") != 0))
        {
            return "invalid_original_preload_state";
        }

        if (std::strcmp(was_set, "1") == 0)
        {
            const auto* original = std::getenv(OriginalPreloadEnv);
            if (!original)
            {
                return "restore_ld_preload_failed";
            }
            const std::string original_value{original};
            if (setenv("LD_PRELOAD", original_value.c_str(), 1) != 0)
            {
                return "restore_ld_preload_failed";
            }
        }
        else if (unsetenv("LD_PRELOAD") != 0)
        {
            return "unset_ld_preload_failed";
        }

        for (const auto* name : {TargetExecutableEnv, OriginalPreloadWasSetEnv, OriginalPreloadEnv, ModulePathEnv})
        {
            if (unsetenv(name) != 0)
            {
                return std::string{"unset_private_environment_failed:"} + name;
            }
        }
        return std::nullopt;
    }
} // namespace RC::LinuxStartup

#endif
