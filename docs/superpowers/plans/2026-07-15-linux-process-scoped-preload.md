# Linux Process-Scoped Preload Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Scope launcher-added `LD_PRELOAD` activation to the intended Unreal host, restore the user's original preload environment before helpers spawn, and document direct and wrapper launch workflows.

**Architecture:** `run_ue4ss.sh` exports a private expected-host and original-environment contract. A focused Linux startup-policy helper evaluates `/proc/self/exe`, distinguishes legacy/raw startup from launcher target/mismatch/error states, and restores the environment only in the accepted target; `EntryLinux.cpp` remains responsible for module-path resolution, diagnostics, the once guard, and spawning the UE4SS thread. Windows bootstrap files remain untouched.

**Tech Stack:** Bash, Linux `/proc/self/exe`, C++23 `std::filesystem`, `setenv`/`unsetenv`, CMake/CTest, xmake, `LD_PRELOAD`, Palworld dedicated server.

---

## File structure

- Create `UE4SS/include/Platform/Linux/StartupPolicy.hpp`: public-to-UE4SS Linux-only decision/result types, environment names, policy evaluation, and restoration declarations.
- Create `UE4SS/src/Platform/Linux/StartupPolicy.cpp`: filesystem-equivalence checks and exact environment restoration with no UE4SS runtime dependencies.
- Modify `UE4SS/src/Platform/Linux/EntryLinux.cpp`: consume the helper before the once guard and emit non-fatal skip diagnostics.
- Modify `tools/linux/run_ue4ss.sh`: parse `--host-executable`, validate ELF hosts, and export the private launcher contract.
- Modify `tests/LinuxLauncherProbe.cpp`: expose launcher contract values to shell assertions.
- Modify `tests/RunLinuxLauncherTests.sh`: cover direct targets, wrapper syntax, invalid scripts/ELFs, spaces, arguments, and inherited preload capture.
- Create `tests/LinuxStartupPolicyTests.cpp`: unit-test decisions, equivalence, malformed state, and unset/empty/non-empty restoration.
- Create `tests/LinuxPreloadScopeProbe.cpp`: host/helper executable for a real preload process-tree test.
- Create `tests/RunLinuxPreloadScopeTests.cmake`: stage UE4SS, run a wrapper and designated host, then prove the helper is clean.
- Modify `tests/CMakeLists.txt`: register the unit and process tests.
- Modify `docs/linux.md` and `README.md`: document process scoping, wrapper syntax, raw-preload caveat, Windows contrast, and diagnostics.
- Modify `docs/superpowers/specs/2026-07-15-linux-process-scoped-preload-design.md`: mark the reviewed design final.
- Modify `docs/superpowers/plans/2026-07-14-linux-port-m3-palworld.md`: record the launcher process-scope acceptance requirement.

`UE4SS/CMakeLists.txt` and `UE4SS/xmake.lua` already glob every Linux `.cpp` under `UE4SS/src/Platform/Linux`, so creating `StartupPolicy.cpp` followed by the explicit CMake reconfigure in Task 2 adds it to both builds without source-list edits.

## Task 1: Lock the launcher contract with shell tests

**Files:**
- Modify: `tests/LinuxLauncherProbe.cpp`
- Modify: `tests/RunLinuxLauncherTests.sh`
- Modify: `tools/linux/run_ue4ss.sh`

- [ ] **Step 1.1: Extend the probe output used by launcher tests**

Add these environment reads and output lines beside the existing `LD_PRELOAD` and `UE4SS_MODULE_PATH` output:

```cpp
const auto* launch_target = std::getenv("UE4SS_LAUNCH_TARGET_EXE");
const auto* preload_was_set = std::getenv("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET");
const auto* original_preload = std::getenv("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD");
std::printf("UE4SS_LAUNCH_TARGET_EXE=%s\n", launch_target ? launch_target : "");
std::printf("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=%s\n", preload_was_set ? preload_was_set : "");
std::printf("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=%s\n", original_preload ? original_preload : "");
```

- [ ] **Step 1.2: Add failing direct-target, wrapper, and validation assertions**

In the existing direct invocation, set stale private values so the test also proves that a nested launcher overwrites them:

```bash
existing_preload_value="${existing_preload}:${existing_preload}"
output="$(
    UE4SS_DISABLE_AUTO_START=1 \
    UE4SS_LAUNCH_TARGET_EXE=/stale/target \
    UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=0 \
    UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=stale.so \
    UE4SS_MODULE_PATH=/stale/libUE4SS.so \
    LD_PRELOAD="${existing_preload_value}" \
    "${launcher}" "${target}" "arg with spaces" "literal*glob" ""
)"
```

Preserve the existing spaces/arguments checks and add:

```bash
canonical_target="$(readlink -f -- "${target}")"
actual_preload="$(sed -n 's/^LD_PRELOAD=//p' <<<"${output}")"
if [[ "${actual_preload}" != /proc/self/fd/*":${existing_preload_value}" ]]; then
    echo "launcher did not preserve the inherited preload list: ${actual_preload}" >&2
    exit 6
fi
grep -Fqx "UE4SS_LAUNCH_TARGET_EXE=${canonical_target}" <<<"${output}"
grep -Fqx 'UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=1' <<<"${output}"
grep -Fqx "UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=${existing_preload_value}" <<<"${output}"

empty_preload_output="$(
    LD_PRELOAD= \
    UE4SS_DISABLE_AUTO_START=1 \
    "${launcher}" "${target}"
)"
grep -Fqx 'UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=1' <<<"${empty_preload_output}"
grep -Fqx 'UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=' <<<"${empty_preload_output}"
if [[ -e "${launcher_directory}/UE4SS.log" ]]; then
    echo "UE4SS_DISABLE_AUTO_START did not override launcher activation" >&2
    exit 7
fi
```

Create a wrapper fixture that forwards to the probe and verify the unset-original case plus argument preservation:

```bash
wrapper="${target_directory}/server wrapper.sh"
cat > "${wrapper}" <<WRAPPER
#!/usr/bin/env bash
set -euo pipefail
exec "${target}" "\$@"
WRAPPER
chmod +x -- "${wrapper}"

wrapper_output="$(
    env -u LD_PRELOAD \
        UE4SS_DISABLE_AUTO_START=1 \
        "${launcher}" --host-executable "${target}" "${wrapper}" "wrapped arg"
)"
grep -Fqx "UE4SS_LAUNCH_TARGET_EXE=${canonical_target}" <<<"${wrapper_output}"
grep -Fqx 'UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=0' <<<"${wrapper_output}"
grep -Fqx 'UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=' <<<"${wrapper_output}"
grep -Fqx 'ARGC=2' <<<"${wrapper_output}"
grep -Fqx 'ARG[1]=wrapped arg' <<<"${wrapper_output}"
```

Add these concrete validation cases after the existing missing-target check:

```bash
set +e
script_without_host_output="$("${launcher}" "${wrapper}" 2>&1)"
script_without_host_status=$?
set -e
if [[ ${script_without_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a script without --host-executable" >&2
    exit 8
fi
grep -Fq 'script commands require --host-executable' <<<"${script_without_host_output}"

set +e
non_elf_host_output="$("${launcher}" --host-executable "${wrapper}" "${wrapper}" 2>&1)"
non_elf_host_status=$?
set -e
if [[ ${non_elf_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a non-ELF host" >&2
    exit 9
fi
grep -Fq 'host executable is not an ELF file' <<<"${non_elf_host_output}"

set +e
missing_host_output="$("${launcher}" --host-executable "${stage_directory}/missing host" "${wrapper}" 2>&1)"
missing_host_status=$?
set -e
if [[ ${missing_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a missing host" >&2
    exit 10
fi
grep -Fq 'host executable not found or not executable' <<<"${missing_host_output}"

non_executable_host="${target_directory}/non executable host"
cp -- "${target}" "${non_executable_host}"
chmod -x -- "${non_executable_host}"
set +e
non_executable_host_output="$("${launcher}" --host-executable "${non_executable_host}" "${wrapper}" 2>&1)"
non_executable_host_status=$?
set -e
if [[ ${non_executable_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a non-executable host" >&2
    exit 11
fi
grep -Fq 'host executable not found or not executable' <<<"${non_executable_host_output}"

non_regular_host="${target_directory}/host directory"
mkdir -- "${non_regular_host}"
set +e
non_regular_host_output="$("${launcher}" --host-executable "${non_regular_host}" "${wrapper}" 2>&1)"
non_regular_host_status=$?
set -e
if [[ ${non_regular_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a non-regular host" >&2
    exit 12
fi
grep -Fq 'host executable not found or not executable' <<<"${non_regular_host_output}"
```

- [ ] **Step 1.3: Run the launcher test and capture RED**

Run:

```bash
cmake --build build_linux --target LinuxLauncherProbe SharedLibraryFixture --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^LinuxLauncherTests$'
```

Expected: FAIL because the launcher does not recognize `--host-executable` and does not export the three launch-contract variables.

- [ ] **Step 1.4: Implement the minimal launcher contract**

Replace argument parsing in `run_ue4ss.sh` with:

```bash
usage() {
    echo "usage: $0 [--host-executable <ELF executable>] <command> [arguments ...]" >&2
}

host_executable=""
if [[ "${1:-}" == "--host-executable" ]]; then
    if [[ $# -lt 3 ]]; then
        usage
        exit 2
    fi
    host_executable="$2"
    shift 2
fi
if [[ $# -lt 1 ]]; then
    usage
    exit 2
fi

target_executable="$1"
shift
```

Add focused helpers:

```bash
canonical_path() {
    readlink -f -- "$1"
}

is_elf_file() {
    [[ "$(LC_ALL=C od -An -t x1 -N4 -- "$1" | tr -d '[:space:]')" == "7f454c46" ]]
}
```

After validating the command, select and validate the host:

```bash
if [[ -z "${host_executable}" ]]; then
    host_executable="${target_executable}"
    if ! is_elf_file "${host_executable}"; then
        echo "UE4SS launcher: script commands require --host-executable <ELF executable>" >&2
        exit 5
    fi
fi
if [[ ! -f "${host_executable}" || ! -x "${host_executable}" ]]; then
    echo "UE4SS launcher: host executable not found or not executable: ${host_executable}" >&2
    exit 6
fi
if ! is_elf_file "${host_executable}"; then
    echo "UE4SS launcher: host executable is not an ELF file: ${host_executable}" >&2
    exit 7
fi
```

Export the contract before changing `LD_PRELOAD`:

```bash
export UE4SS_LAUNCH_TARGET_EXE="$(canonical_path "${host_executable}")"
if [[ -v LD_PRELOAD ]]; then
    export UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=1
    export UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD="${LD_PRELOAD}"
else
    export UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=0
    export UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=""
fi
```

Keep the current `UE4SS_MODULE_PATH`, file-descriptor preload fallback, prepend behavior, and final `exec` unchanged.

- [ ] **Step 1.5: Run the launcher test and verify GREEN**

Run:

```bash
ctest --test-dir build_linux --output-on-failure -R '^LinuxLauncherTests$'
```

Expected: `LinuxLauncherTests` passes.

- [ ] **Step 1.6: Commit the launcher slice without unrelated dirty hunks**

Stage the two clean files and only the launcher-test hunks belonging to this task. Inspect the index before committing:

```bash
git add tools/linux/run_ue4ss.sh tests/LinuxLauncherProbe.cpp
git add -p tests/RunLinuxLauncherTests.sh
git diff --cached --check
git diff --cached --stat
git commit -m "feat(linux): add process-scoped launcher contract"
```

Expected: the commit contains only the launcher contract and its tests.

## Task 2: Add a testable Linux startup-policy helper

**Files:**
- Create: `UE4SS/include/Platform/Linux/StartupPolicy.hpp`
- Create: `UE4SS/src/Platform/Linux/StartupPolicy.cpp`
- Create: `tests/LinuxStartupPolicyTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 2.1: Write the helper API and failing unit tests**

Create `StartupPolicy.hpp` with this interface:

```cpp
#pragma once

#ifdef __linux__

#include <filesystem>
#include <optional>
#include <string>

namespace RC::LinuxStartup
{
    inline constexpr char TargetExecutableEnv[] = "UE4SS_LAUNCH_TARGET_EXE";
    inline constexpr char OriginalPreloadWasSetEnv[] = "UE4SS_LAUNCH_LD_PRELOAD_WAS_SET";
    inline constexpr char OriginalPreloadEnv[] = "UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD";
    inline constexpr char ModulePathEnv[] = "UE4SS_MODULE_PATH";

    enum class DecisionKind
    {
        LegacyStart,
        LauncherStart,
        TargetMismatch,
        InvalidLauncherState,
    };

    struct Decision
    {
        DecisionKind kind{DecisionKind::InvalidLauncherState};
        std::filesystem::path current_executable{};
        std::filesystem::path expected_executable{};
        std::string reason{};
    };

    auto evaluate(const std::filesystem::path& current_executable = "/proc/self/exe") noexcept -> Decision;
    auto restore_original_environment() noexcept -> std::optional<std::string>;
}

#endif
```

Create `StartupPolicy.cpp` as a declaration-only RED skeleton:

```cpp
#ifdef __linux__

#include <Platform/Linux/StartupPolicy.hpp>

#endif
```

Create `LinuxStartupPolicyTests.cpp` with the complete decision and restoration coverage:

```cpp
#include <Platform/Linux/StartupPolicy.hpp>

#include <array>
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
            for (size_t index = 0; index < EnvironmentNames.size(); ++index)
            {
                if (const auto* value = std::getenv(EnvironmentNames[index].data()))
                {
                    m_values[index] = value;
                }
            }
        }

        ~EnvironmentSnapshot()
        {
            for (size_t index = 0; index < EnvironmentNames.size(); ++index)
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
        expect(setenv(name.data(), std::string{value}.c_str(), 1) == 0, "setenv failed");
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
        for (const auto name : {RC::LinuxStartup::TargetExecutableEnv,
                                RC::LinuxStartup::OriginalPreloadWasSetEnv,
                                RC::LinuxStartup::OriginalPreloadEnv,
                                RC::LinuxStartup::ModulePathEnv})
        {
            expect(std::getenv(name) == nullptr, "private launcher variable remained set");
        }
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
        expect(std::string_view{std::getenv("LD_PRELOAD")} == "one.so:two.so", "LD_PRELOAD order or contents changed");
        expect_private_environment_unset();

        clear_environment();
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        expect(RC::LinuxStartup::restore_original_environment() == "invalid_original_preload_state",
               "missing WAS_SET marker was not rejected");

        clear_environment();
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        set_environment(RC::LinuxStartup::OriginalPreloadWasSetEnv, "2");
        expect(RC::LinuxStartup::restore_original_environment() == "invalid_original_preload_state",
               "invalid WAS_SET marker was not rejected");

        clear_environment();
        set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
        set_environment(RC::LinuxStartup::OriginalPreloadWasSetEnv, "1");
        expect(RC::LinuxStartup::restore_original_environment() == "restore_ld_preload_failed",
               "missing original preload value was not rejected");
    }
    catch (const std::exception& exception)
    {
        std::fprintf(stderr, "LinuxStartupPolicyTests: %s\n", exception.what());
        return 1;
    }
    return 0;
}
```

Register the test in the Linux block:

```cmake
add_executable(LinuxStartupPolicyTests
    LinuxStartupPolicyTests.cpp
    "${CMAKE_SOURCE_DIR}/UE4SS/src/Platform/Linux/StartupPolicy.cpp"
)
target_include_directories(LinuxStartupPolicyTests PRIVATE "${CMAKE_SOURCE_DIR}/UE4SS/include")
add_test(NAME LinuxStartupPolicyTests COMMAND LinuxStartupPolicyTests)
```

- [ ] **Step 2.2: Run the unit test and capture RED**

Run:

```bash
cmake -S . -B build_linux
cmake --build build_linux --target LinuxStartupPolicyTests --parallel 2
```

Expected: link failure because `evaluate` and `restore_original_environment` are not implemented.

- [ ] **Step 2.3: Implement policy evaluation**

Replace the RED skeleton in `StartupPolicy.cpp` with this compilable evaluation-only file. `restore_original_environment` intentionally remains undefined until Step 2.4:

```cpp
#ifdef __linux__

#include <Platform/Linux/StartupPolicy.hpp>

#include <cstdlib>

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
} // namespace RC::LinuxStartup

#endif
```

- [ ] **Step 2.4: Implement exact environment restoration**

Add `#include <cstring>` and `#include <initializer_list>` beside `<cstdlib>`, then insert this function before the namespace closes:

```cpp
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
        if (!original || setenv("LD_PRELOAD", original, 1) != 0)
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
```

- [ ] **Step 2.5: Run the unit test and verify GREEN**

Run:

```bash
cmake --build build_linux --target LinuxStartupPolicyTests --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^LinuxStartupPolicyTests$'
```

Expected: `LinuxStartupPolicyTests` passes.

- [ ] **Step 2.6: Commit the helper slice**

```bash
git add UE4SS/include/Platform/Linux/StartupPolicy.hpp \
        UE4SS/src/Platform/Linux/StartupPolicy.cpp \
        tests/LinuxStartupPolicyTests.cpp
git add -p tests/CMakeLists.txt
git diff --cached --check
git commit -m "feat(linux): add preload startup policy"
```

## Task 3: Apply the policy in the ELF constructor and prove process scoping

**Files:**
- Modify: `UE4SS/src/Platform/Linux/EntryLinux.cpp`
- Create: `tests/LinuxPreloadScopeProbe.cpp`
- Create: `tests/RunLinuxPreloadScopeTests.cmake`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 3.1: Write the process-tree probe**

Create `LinuxPreloadScopeProbe.cpp` with complete host and helper modes. The host verifies restoration before spawning the helper, and the helper proves that UE4SS is not mapped:

```cpp
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <thread>

#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
    auto print_environment(const char* prefix) -> void
    {
        const auto* preload = std::getenv("LD_PRELOAD");
        const auto* target = std::getenv("UE4SS_LAUNCH_TARGET_EXE");
        const auto* was_set = std::getenv("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET");
        const auto* original = std::getenv("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD");
        const auto* module = std::getenv("UE4SS_MODULE_PATH");
        std::printf("%s_LD_PRELOAD=%s\n", prefix, preload ? preload : "<unset>");
        std::printf("%s_TARGET=%s\n", prefix, target ? target : "<unset>");
        std::printf("%s_WAS_SET=%s\n", prefix, was_set ? was_set : "<unset>");
        std::printf("%s_ORIGINAL=%s\n", prefix, original ? original : "<unset>");
        std::printf("%s_MODULE=%s\n", prefix, module ? module : "<unset>");
    }
} // namespace

int main(int argc, char** argv)
{
    if (argc == 4 && std::string_view{argv[1]} == "--helper")
    {
        print_environment("HELPER");
        if (auto* ue4ss = dlopen(argv[2], RTLD_NOW | RTLD_NOLOAD))
        {
            dlclose(ue4ss);
            return 11;
        }

        const auto* preload = std::getenv("LD_PRELOAD");
        const auto* target = std::getenv("UE4SS_LAUNCH_TARGET_EXE");
        const auto* was_set = std::getenv("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET");
        const auto* original = std::getenv("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD");
        const auto* module = std::getenv("UE4SS_MODULE_PATH");
        if (!preload || std::string_view{preload} != argv[3] || target || was_set || original || module)
        {
            return 12;
        }
        std::puts("UE4SS_PRELOAD_SCOPE_HELPER_CLEAN");
        return 0;
    }

    if (argc != 3)
    {
        std::fprintf(stderr, "usage: %s <libUE4SS.so> <expected original LD_PRELOAD>\n", argv[0]);
        return 2;
    }

    print_environment("HOST");
    const auto* preload = std::getenv("LD_PRELOAD");
    if (!preload || std::string_view{preload} != argv[2] || std::getenv("UE4SS_LAUNCH_TARGET_EXE") ||
        std::getenv("UE4SS_LAUNCH_LD_PRELOAD_WAS_SET") || std::getenv("UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD") ||
        std::getenv("UE4SS_MODULE_PATH"))
    {
        return 3;
    }

    const auto child = fork();
    if (child < 0)
    {
        return 4;
    }
    if (child == 0)
    {
        execl(argv[0], argv[0], "--helper", argv[1], argv[2], static_cast<char*>(nullptr));
        _exit(127);
    }

    int status{};
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        return 5;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{1500});
    std::puts("UE4SS_PRELOAD_SCOPE_HOST_READY");
    return 0;
}
```

- [ ] **Step 3.2: Write the failing CMake integration driver**

Create `RunLinuxPreloadScopeTests.cmake` with the complete staged wrapper/process-tree check:

```cmake
foreach(required_variable
        UE4SS_LIBRARY
        LAUNCHER_SOURCE
        PROBE_EXECUTABLE
        EXISTING_PRELOAD
        SETTINGS_TEMPLATE
        STAGE_DIRECTORY)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable: ${required_variable}")
    endif()
endforeach()

file(REMOVE_RECURSE "${STAGE_DIRECTORY}")
file(MAKE_DIRECTORY "${STAGE_DIRECTORY}/UE4SS_Signatures")
file(COPY "${UE4SS_LIBRARY}" "${LAUNCHER_SOURCE}" "${EXISTING_PRELOAD}" DESTINATION "${STAGE_DIRECTORY}")
file(CHMOD "${STAGE_DIRECTORY}/run_ue4ss.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

get_filename_component(existing_preload_name "${EXISTING_PRELOAD}" NAME)
set(staged_existing_preload "${STAGE_DIRECTORY}/${existing_preload_name}")

file(READ "${SETTINGS_TEMPLATE}" settings_contents)
string(REPLACE
    "SecondsToScanBeforeGivingUp = 30"
    "SecondsToScanBeforeGivingUp = 0"
    settings_contents
    "${settings_contents}"
)
string(REPLACE "ConsoleEnabled = 1" "ConsoleEnabled = 0" settings_contents "${settings_contents}")
file(WRITE "${STAGE_DIRECTORY}/UE4SS-settings.ini" "${settings_contents}")
file(WRITE "${STAGE_DIRECTORY}/UE4SS_Signatures/GUObjectArray.lua" [=[
function Register()
    return "DE AD BE EF FE ED FA CE 01 23 45 67 89 AB CD EF"
end

function OnMatchFound(matchAddress)
    return matchAddress
end
]=])

file(WRITE "${STAGE_DIRECTORY}/wrapper.sh"
    "#!/bin/bash\nset -euo pipefail\nexec \"${PROBE_EXECUTABLE}\" \"$@\"\n")
file(CHMOD "${STAGE_DIRECTORY}/wrapper.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "LD_PRELOAD=${staged_existing_preload}"
        "UE4SS_DIAGNOSE=1"
        "${STAGE_DIRECTORY}/run_ue4ss.sh"
        --host-executable "${PROBE_EXECUTABLE}"
        "${STAGE_DIRECTORY}/wrapper.sh"
        "${STAGE_DIRECTORY}/libUE4SS.so"
        "${staged_existing_preload}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT 8
)

set(process_output "${stdout}\n${stderr}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Process-scope probe exited with ${result}:\n${process_output}")
endif()
foreach(expected_line
        "HOST_LD_PRELOAD=${staged_existing_preload}"
        "HOST_TARGET=<unset>"
        "HOST_WAS_SET=<unset>"
        "HOST_ORIGINAL=<unset>"
        "HOST_MODULE=<unset>"
        "HELPER_LD_PRELOAD=${staged_existing_preload}"
        "HELPER_TARGET=<unset>"
        "HELPER_WAS_SET=<unset>"
        "HELPER_ORIGINAL=<unset>"
        "HELPER_MODULE=<unset>"
        "UE4SS_PRELOAD_SCOPE_HELPER_CLEAN"
        "UE4SS_PRELOAD_SCOPE_HOST_READY")
    string(FIND "${process_output}" "${expected_line}" expected_position)
    if(expected_position EQUAL -1)
        message(FATAL_ERROR "Process-scope output did not contain '${expected_line}':\n${process_output}")
    endif()
endforeach()
if(NOT stderr MATCHES "DIAG: startup_skipped executable=.* expected=.* reason=target_mismatch")
    message(FATAL_ERROR "Wrapper mismatch did not emit startup_skipped:\n${stderr}")
endif()
if(process_output MATCHES "DIAG: inactive_reason=.*target_mismatch")
    message(FATAL_ERROR "Normal wrapper mismatch was reported as inactive:\n${process_output}")
endif()

set(ue4ss_log "${STAGE_DIRECTORY}/UE4SS.log")
if(NOT EXISTS "${ue4ss_log}")
    message(FATAL_ERROR "Accepted host did not create UE4SS.log")
endif()
file(READ "${ue4ss_log}" ue4ss_log_contents)
get_filename_component(probe_canonical "${PROBE_EXECUTABLE}" REALPATH)
string(REGEX MATCHALL "DIAG: executable=[^\r\n]*" executable_diagnostics "${ue4ss_log_contents}")
list(LENGTH executable_diagnostics executable_diagnostic_count)
if(NOT executable_diagnostic_count EQUAL 1)
    message(FATAL_ERROR "Expected one executable diagnostic, found ${executable_diagnostic_count}:\n${ue4ss_log_contents}")
endif()
list(GET executable_diagnostics 0 executable_diagnostic)
if(NOT executable_diagnostic STREQUAL "DIAG: executable=${probe_canonical}")
    message(FATAL_ERROR "Unexpected executable diagnostic '${executable_diagnostic}'")
endif()
if(ue4ss_log_contents MATCHES "crashpad_handler")
    message(FATAL_ERROR "Helper diagnostics leaked into UE4SS.log:\n${ue4ss_log_contents}")
endif()

file(REMOVE_RECURSE "${STAGE_DIRECTORY}")
```

Register:

```cmake
add_executable(LinuxPreloadScopeProbe LinuxPreloadScopeProbe.cpp)
target_link_libraries(LinuxPreloadScopeProbe PRIVATE dl)
add_test(NAME LinuxPreloadScopeTests
    COMMAND "${CMAKE_COMMAND}"
        "-DUE4SS_LIBRARY=$<TARGET_FILE:UE4SS>"
        "-DLAUNCHER_SOURCE=${CMAKE_SOURCE_DIR}/tools/linux/run_ue4ss.sh"
        "-DPROBE_EXECUTABLE=$<TARGET_FILE:LinuxPreloadScopeProbe>"
        "-DEXISTING_PRELOAD=$<TARGET_FILE:SharedLibraryFixture>"
        "-DSETTINGS_TEMPLATE=${CMAKE_SOURCE_DIR}/assets/UE4SS-settings.ini"
        "-DSTAGE_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/linux-preload-scope-stage"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/RunLinuxPreloadScopeTests.cmake"
)
```

- [ ] **Step 3.3: Run the integration test and capture RED**

Run:

```bash
cmake -S . -B build_linux
cmake --build build_linux --target UE4SS LinuxPreloadScopeProbe SharedLibraryFixture --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^LinuxPreloadScopeTests$'
```

Expected: FAIL because `EntryLinux.cpp` ignores the target marker and does not restore the environment.

- [ ] **Step 3.4: Integrate the policy before the once guard**

Include the helper:

```cpp
#include <Platform/Linux/StartupPolicy.hpp>
```

Replace `ue4ss_so_attached` with this complete ordering. It copies the module path before restoration, restores only in the accepted host, and claims the once guard only after restoration succeeds:

```cpp
__attribute__((constructor(101))) static void ue4ss_so_attached()
{
    if (const auto* disable_auto_start = std::getenv("UE4SS_DISABLE_AUTO_START");
        disable_auto_start && std::strcmp(disable_auto_start, "1") == 0)
    {
        return;
    }

    const auto startup_decision = LinuxStartup::evaluate();
    if (startup_decision.kind == LinuxStartup::DecisionKind::TargetMismatch)
    {
        if (LinuxDiagnostics::is_enabled())
        {
            std::fprintf(stderr,
                         "DIAG: startup_skipped executable=%s expected=%s reason=target_mismatch\n",
                         startup_decision.current_executable.c_str(),
                         startup_decision.expected_executable.c_str());
        }
        return;
    }
    if (startup_decision.kind == LinuxStartup::DecisionKind::InvalidLauncherState)
    {
        std::fprintf(stderr, "UE4SS: launcher startup disabled: %s\n", startup_decision.reason.c_str());
        return;
    }

    std::filesystem::path module_path;
    try
    {
        if (const auto* configured_module_path = std::getenv("UE4SS_MODULE_PATH");
            configured_module_path && configured_module_path[0] != '\0')
        {
            module_path = configured_module_path;
        }
        else
        {
            Dl_info dl_info{};
            if (dladdr(reinterpret_cast<void*>(&ue4ss_so_attached), &dl_info) == 0 || !dl_info.dli_fname)
            {
                std::fputs("UE4SS: failed to determine libUE4SS.so path; startup disabled\n", stderr);
                return;
            }
            module_path = dl_info.dli_fname;
        }
    }
    catch (const std::exception& exception)
    {
        std::fprintf(stderr, "UE4SS: failed to determine initialization path: %s\n", exception.what());
        return;
    }
    catch (...)
    {
        std::fputs("UE4SS: failed to determine initialization path with an unknown exception\n", stderr);
        return;
    }

    if (startup_decision.kind == LinuxStartup::DecisionKind::LauncherStart)
    {
        if (const auto restore_error = LinuxStartup::restore_original_environment())
        {
            std::fprintf(stderr, "UE4SS: launcher environment restoration failed: %s\n", restore_error->c_str());
            return;
        }
    }

    bool expected = false;
    if (!s_ue4ss_started.compare_exchange_strong(expected, true))
    {
        return;
    }

    try
    {
        std::thread init_thread{&thread_so_start, std::move(module_path)};
        init_thread.detach();
    }
    catch (const std::exception& exception)
    {
        std::fprintf(stderr, "UE4SS: failed to start initialization: %s\n", exception.what());
        s_ue4ss_started = false;
    }
    catch (...)
    {
        std::fputs("UE4SS: failed to start initialization with an unknown exception\n", stderr);
        s_ue4ss_started = false;
    }
}
```

Keep legacy marker-free startup, module-path fallback through `dladdr`, detached-thread creation, exception handling, and Windows code unchanged.

- [ ] **Step 3.5: Run unit, launcher, and process tests and verify GREEN**

Run:

```bash
cmake --build build_linux --target UE4SS LinuxStartupPolicyTests LinuxLauncherProbe \
  LinuxPreloadScopeProbe SharedLibraryFixture --parallel 2
ctest --test-dir build_linux --output-on-failure \
  -R '^(LinuxStartupPolicyTests|LinuxLauncherTests|LinuxPreloadScopeTests|LinuxPreloadProbe|LinuxFailSoftProbe)$'
```

Expected: all selected tests pass.

- [ ] **Step 3.6: Prove raw preload compatibility**

Run the existing fail-soft test without launcher markers:

```bash
ctest --test-dir build_linux --output-on-failure -R '^LinuxFailSoftProbe$'
```

Expected: PASS with `UE4SS_FAIL_SOFT_PROBE_READY` and the expected fail-soft log, proving marker-free raw preload still starts UE4SS.

- [ ] **Step 3.7: Commit the constructor integration**

```bash
git add UE4SS/src/Platform/Linux/EntryLinux.cpp \
        tests/LinuxPreloadScopeProbe.cpp \
        tests/RunLinuxPreloadScopeTests.cmake
git add -p tests/CMakeLists.txt
git diff --cached --check
git commit -m "fix(linux): scope preload startup to the host process"
```

## Task 4: Document the supported workflow

**Files:**
- Modify: `docs/linux.md`
- Modify: `README.md`
- Modify: `docs/superpowers/specs/2026-07-15-linux-process-scoped-preload-design.md`
- Modify: `docs/superpowers/plans/2026-07-14-linux-port-m3-palworld.md`

- [ ] **Step 4.1: Update direct and wrapper launch examples**

Replace the Palworld wrapper example with:

```bash
server="$PALWORLD_SERVER_ROOT/Pal/Binaries/Linux/PalServer-Linux-Shipping"
wrapper="$PALWORLD_SERVER_ROOT/PalServer.sh"

UE4SS_CRASH_LOG_DIR="$stage/UE4SS-crashes" \
  "$stage/run_ue4ss.sh" \
  --host-executable "$server" \
  "$wrapper" \
  -useperfthreads -NoAsyncLoadingThread -UseMultithreadForDS
```

Also show the shorter direct-ELF form used by the acceptance harness.

- [ ] **Step 4.2: Document process scoping and raw compatibility**

Add a concise section explaining:

```text
The launcher records the intended host executable. Wrapper interpreters receive the preload but skip UE4SS initialization. When the intended host loads, UE4SS restores the original LD_PRELOAD before starting, so later helpers do not inherit the launcher-added library. Windows does not need this because proxy/manual DLL injection is process-scoped. Raw LD_PRELOAD remains supported but retains normal Linux child inheritance.
```

Document `DIAG: startup_skipped ... reason=target_mismatch` as normal for wrapper interpreters and distinguish it from `inactive_reason`.

- [ ] **Step 4.3: Update README and planning records**

Add the wrapper option to the README's Linux link/usage text. Mark the design status as approved and reviewed. Add the process-scope requirement to Plan 3 Task 3 and record that acceptance must show no crashpad UE4SS startup.

- [ ] **Step 4.4: Verify documentation examples and formatting**

Run:

```bash
rg -n "host-executable|startup_skipped|raw.*LD_PRELOAD|Windows" docs/linux.md README.md \
  docs/superpowers/specs/2026-07-15-linux-process-scoped-preload-design.md \
  docs/superpowers/plans/2026-07-14-linux-port-m3-palworld.md
git diff --check
```

Expected: all required concepts are present and `git diff --check` reports no errors.

- [ ] **Step 4.5: Commit documentation without unrelated plan changes**

```bash
git add docs/linux.md README.md \
        docs/superpowers/specs/2026-07-15-linux-process-scoped-preload-design.md
git add -p docs/superpowers/plans/2026-07-14-linux-port-m3-palworld.md
git diff --cached --check
git commit -m "docs(linux): document process-scoped preload launching"
```

## Task 5: Fresh builds and live Palworld verification

**Files:**
- Modify only if a verification failure identifies a root cause covered by this design.
- Preserve: unrelated server PID `527131` on port `8211`.

- [ ] **Step 5.1: Run the complete CMake suite**

Run:

```bash
cmake --build build_linux --parallel 2
PALWORLD_SERVER_ROOT=/home/liu/RE-UE4SS/build_linux/palworld-server \
  ctest --test-dir build_linux --output-on-failure
```

Expected: all registered tests pass, including the pinned Palworld signature test with no skip.

- [ ] **Step 5.2: Build with xmake and verify ELF dependencies**

Run:

```bash
xmake build -j 2 -y UE4SS
ldd -r build_linux/Game__Shipping__Linux/lib/libUE4SS.so
ldd -r Binaries/Game__Shipping__Linux/UE4SS/libUE4SS.so
tests/RunLinuxLinkParityTests.sh \
  build_linux/Game__Shipping__Linux/lib/libUE4SS.so \
  Binaries/Game__Shipping__Linux/UE4SS/libUE4SS.so
```

Expected: both builds succeed, both `ldd -r` commands report no unresolved symbols, and linked-library parity passes.

- [ ] **Step 5.3: Run a focused live server check on unused ports**

Stage the new CMake library, launcher, settings, and both probe mods in a fresh directory. Start the pinned repository-local Palworld server through the direct-ELF launcher form on unused ports, with dumpers disabled. Use a cleanup trap scoped to the new server PID and repository-local crashpad path; never signal PID `527131`.

Require:

```text
DIAG: executable=.../PalServer-Linux-Shipping
DIAG: engine_version=5.1
ACCEPT:LUA_MOD_STARTED
ACCEPT:LUA_CONSOLE_REGISTERED
ACCEPT:CPP_PROGRAM_START
ACCEPT:CPP_UNREAL_INIT
ACCEPT:CPP_UPDATE count=10
Running Palworld dedicated server on :<test-port>
```

Reject every occurrence of:

```text
DIAG: executable=.../crashpad_handler
DIAG: inactive_reason=PS scan timed out
Fatal Error: PS scan timed out
```

- [ ] **Step 5.4: Run the acceptance harness through the crashpad boundary**

Run the Palworld acceptance harness with the new library. It may later stop on currently pending Blueprint/dumper markers, but it must no longer fail because of crashpad diagnostics or an overwritten helper log. Record the first remaining marker failure exactly.

- [ ] **Step 5.5: Save verification evidence**

Create or update the Linux verification report under `docs/superpowers/verification/` with:

- commit hashes for the design and implementation slices;
- CMake/xmake commands and results;
- test count and pinned signature result;
- `ldd -r` and parity results;
- the focused Palworld command and port;
- one main executable diagnostic;
- zero crashpad UE4SS diagnostics;
- the next unresolved acceptance marker, if any.

- [ ] **Step 5.6: Final verification commit**

Stage only the verification report and any plan checkbox updates produced by this task:

```bash
git add docs/superpowers/verification/
git add -p docs/superpowers/plans/2026-07-14-linux-port-m3-palworld.md
git diff --cached --check
git commit -m "test(linux): verify process-scoped Palworld preload"
```

Do not declare the Linux port complete unless all eight Palworld acceptance gates are green.
