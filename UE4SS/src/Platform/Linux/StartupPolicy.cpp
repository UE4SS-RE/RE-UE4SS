#ifdef __linux__

#include <Platform/Linux/StartupPolicy.hpp>

#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string_view>

namespace RC::LinuxStartup
{
    namespace
    {
        auto box64_preload_contains_module(const char* preload_value,
                                           const std::filesystem::path& loaded_module) noexcept -> bool
        {
            if (!preload_value || preload_value[0] == '\0' || loaded_module.empty())
            {
                return false;
            }

            try
            {
                std::string_view remaining{preload_value};
                while (!remaining.empty())
                {
                    const auto separator = remaining.find(':');
                    const auto token = remaining.substr(0, separator);
                    if (!token.empty())
                    {
                        const std::filesystem::path candidate{token};
                        if (candidate == loaded_module ||
                            (!candidate.filename().empty() && candidate.filename() == loaded_module.filename()))
                        {
                            return true;
                        }

                        std::error_code equivalent_error;
                        const bool equivalent = std::filesystem::equivalent(candidate, loaded_module, equivalent_error);
                        if (!equivalent_error && equivalent)
                        {
                            return true;
                        }
                    }

                    if (separator == std::string_view::npos)
                    {
                        break;
                    }
                    remaining.remove_prefix(separator + 1);
                }
            }
            catch (...)
            {
                return false;
            }
            return false;
        }
    } // namespace

    auto evaluate(const std::filesystem::path& current_executable,
                  const std::filesystem::path& loaded_module) noexcept -> Decision
    {
        const auto* expected_value = std::getenv(TargetExecutableEnv);
        if (!expected_value)
        {
            if (box64_preload_contains_module(std::getenv(Box64PreloadEnv), loaded_module))
            {
                Decision result{
                        .kind = DecisionKind::Box64OrphanedPreload,
                        .reason = "box64_target_missing",
                };
                std::error_code current_error;
                result.current_executable = std::filesystem::canonical(current_executable, current_error);
                return result;
            }
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
