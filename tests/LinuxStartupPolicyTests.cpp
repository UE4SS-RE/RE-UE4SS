#include <Platform/Linux/StartupPolicy.hpp>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <unistd.h>

namespace
{
    constexpr std::array EnvironmentNames{
            std::string_view{"LD_PRELOAD"},
            std::string_view{RC::LinuxStartup::TargetExecutableEnv},
            std::string_view{RC::LinuxStartup::OriginalPreloadWasSetEnv},
            std::string_view{RC::LinuxStartup::OriginalPreloadEnv},
            std::string_view{RC::LinuxStartup::ModulePathEnv},
    };

    class EnvironmentSnapshot
    {
      private:
        std::array<std::optional<std::string>, EnvironmentNames.size()> m_values{};

      public:
        EnvironmentSnapshot()
        {
            for (std::size_t index = 0; index < EnvironmentNames.size(); ++index)
            {
                if (const auto* value = std::getenv(EnvironmentNames[index].data()))
                {
                    m_values[index] = value;
                }
            }
        }

        ~EnvironmentSnapshot()
        {
            for (std::size_t index = 0; index < EnvironmentNames.size(); ++index)
            {
                if (m_values[index])
                {
                    setenv(EnvironmentNames[index].data(), m_values[index]->c_str(), 1);
                }
                else
                {
                    unsetenv(EnvironmentNames[index].data());
                }
            }
        }
    };

    class TemporaryDirectory
    {
      public:
        std::filesystem::path path;

        explicit TemporaryDirectory(std::filesystem::path directory) : path(std::move(directory))
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
            std::filesystem::create_directories(path);
        }

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    };

    auto expect(bool condition, std::string_view message) -> void
    {
        if (!condition)
        {
            throw std::runtime_error{std::string{message}};
        }
    }

    auto set_environment(std::string_view name, std::string_view value) -> void
    {
        const std::string copied_value{value};
        expect(setenv(name.data(), copied_value.c_str(), 1) == 0, "setenv failed");
    }

    auto clear_environment() -> void
    {
        for (const auto name : EnvironmentNames)
        {
            expect(unsetenv(name.data()) == 0, "unsetenv failed");
        }
    }

    auto expect_private_environment_unset() -> void
    {
        for (const auto* name : {RC::LinuxStartup::TargetExecutableEnv,
                                 RC::LinuxStartup::OriginalPreloadWasSetEnv,
                                 RC::LinuxStartup::OriginalPreloadEnv,
                                 RC::LinuxStartup::ModulePathEnv})
        {
            expect(std::getenv(name) == nullptr, "private launcher variable remained set");
        }
    }

    auto expect_restore_error(std::string_view expected_error) -> void
    {
        const auto error = RC::LinuxStartup::restore_original_environment();
        expect(error && *error == expected_error, "unexpected environment restoration error");
    }
} // namespace

int main(int argc, char** argv)
{
    EnvironmentSnapshot environment_snapshot;
    try
    {
        expect(argc > 0, "missing argv[0]");
        const auto self = std::filesystem::canonical(argv[0]);
        TemporaryDirectory temporary_directory{
                self.parent_path() / ("linux-startup-policy-tests-" + std::to_string(getpid()))};
        const auto symlink_path = temporary_directory.path / "self-symlink";
        const auto hard_link_path = temporary_directory.path / "self-hard-link";
        const auto other_path = temporary_directory.path / "other-executable";
        std::filesystem::create_symlink(self, symlink_path);
        std::filesystem::create_hard_link(self, hard_link_path);
        std::filesystem::copy_file(self, other_path);

        clear_environment();
        expect(RC::LinuxStartup::evaluate(self).kind == RC::LinuxStartup::DecisionKind::LegacyStart,
               "marker-free startup was not legacy");

        set_environment(RC::LinuxStartup::TargetExecutableEnv, "");
        auto decision = RC::LinuxStartup::evaluate(self);
        expect(decision.kind == RC::LinuxStartup::DecisionKind::InvalidLauncherState && decision.reason == "empty_target",
               "empty target was not rejected");

        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        decision = RC::LinuxStartup::evaluate(self);
        expect(decision.kind == RC::LinuxStartup::DecisionKind::LauncherStart, "current executable did not match itself");

        set_environment(RC::LinuxStartup::TargetExecutableEnv, symlink_path.string());
        expect(RC::LinuxStartup::evaluate(self).kind == RC::LinuxStartup::DecisionKind::LauncherStart,
               "symlink target was not equivalent");

        set_environment(RC::LinuxStartup::TargetExecutableEnv, hard_link_path.string());
        expect(RC::LinuxStartup::evaluate(self).kind == RC::LinuxStartup::DecisionKind::LauncherStart,
               "hard-link target was not equivalent");

        set_environment(RC::LinuxStartup::TargetExecutableEnv, other_path.string());
        decision = RC::LinuxStartup::evaluate(self);
        expect(decision.kind == RC::LinuxStartup::DecisionKind::TargetMismatch && decision.reason == "target_mismatch",
               "different executable was not a target mismatch");

        set_environment(RC::LinuxStartup::TargetExecutableEnv, (temporary_directory.path / "missing-target").string());
        decision = RC::LinuxStartup::evaluate(self);
        expect(decision.kind == RC::LinuxStartup::DecisionKind::InvalidLauncherState && decision.reason == "target_compare_failed",
               "missing target was not rejected");

        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        decision = RC::LinuxStartup::evaluate(temporary_directory.path / "missing-current");
        expect(decision.kind == RC::LinuxStartup::DecisionKind::InvalidLauncherState &&
                       decision.reason == "current_executable_unresolved",
               "unresolved current executable was not rejected");

        clear_environment();
        set_environment("LD_PRELOAD", "launcher-added.so");
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        set_environment(RC::LinuxStartup::OriginalPreloadWasSetEnv, "0");
        set_environment(RC::LinuxStartup::OriginalPreloadEnv, "ignored.so");
        set_environment(RC::LinuxStartup::ModulePathEnv, "/tmp/libUE4SS.so");
        expect(!RC::LinuxStartup::restore_original_environment(), "unset original preload restoration failed");
        expect(std::getenv("LD_PRELOAD") == nullptr, "LD_PRELOAD should have been unset");
        expect_private_environment_unset();

        clear_environment();
        set_environment("LD_PRELOAD", "launcher-added.so");
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        set_environment(RC::LinuxStartup::OriginalPreloadWasSetEnv, "1");
        set_environment(RC::LinuxStartup::OriginalPreloadEnv, "");
        set_environment(RC::LinuxStartup::ModulePathEnv, "/tmp/libUE4SS.so");
        expect(!RC::LinuxStartup::restore_original_environment(), "empty original preload restoration failed");
        const auto* empty_preload = std::getenv("LD_PRELOAD");
        expect(empty_preload && empty_preload[0] == '\0', "LD_PRELOAD did not remain set to empty");
        expect_private_environment_unset();

        clear_environment();
        set_environment("LD_PRELOAD", "launcher-added.so");
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        set_environment(RC::LinuxStartup::OriginalPreloadWasSetEnv, "1");
        set_environment(RC::LinuxStartup::OriginalPreloadEnv, "one.so:two.so");
        set_environment(RC::LinuxStartup::ModulePathEnv, "/tmp/libUE4SS.so");
        expect(!RC::LinuxStartup::restore_original_environment(), "non-empty original preload restoration failed");
        const auto* restored_preload = std::getenv("LD_PRELOAD");
        expect(restored_preload && std::string_view{restored_preload} == "one.so:two.so",
               "LD_PRELOAD order or contents changed");
        expect_private_environment_unset();

        clear_environment();
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        expect_restore_error("invalid_original_preload_state");

        clear_environment();
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        set_environment(RC::LinuxStartup::OriginalPreloadWasSetEnv, "2");
        expect_restore_error("invalid_original_preload_state");

        clear_environment();
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        set_environment(RC::LinuxStartup::OriginalPreloadWasSetEnv, "1");
        expect_restore_error("restore_ld_preload_failed");
    }
    catch (const std::exception& exception)
    {
        std::fprintf(stderr, "LinuxStartupPolicyTests: %s\n", exception.what());
        return 1;
    }
    return 0;
}
