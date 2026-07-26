# Box64 Orphan Preload Fail-Safe Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prevent a marker-free Box64 reinjection of `libUE4SS.so` from initializing UE4SS in helper processes without changing native Linux launcher behavior.

**Architecture:** Extend the Linux startup-policy decision with the actual loaded module path and inspect `BOX64_LD_PRELOAD` only when the launcher target marker is absent. A positive exact, equivalent-path, or filename match fails closed before initialization; all native and marker-present paths retain their existing decisions.

**Tech Stack:** C++23, Linux `dladdr`, `std::filesystem`, CMake/CTest, CMake script integration tests, Markdown.

---

### Task 1: Add the startup-policy decision with TDD

**Files:**
- Modify: `UE4SS/include/Platform/Linux/StartupPolicy.hpp`
- Modify: `UE4SS/src/Platform/Linux/StartupPolicy.cpp`
- Test: `tests/LinuxStartupPolicyTests.cpp`

- [ ] **Step 1: Write failing unit cases for orphaned Box64 preloads**

Add `BOX64_LD_PRELOAD` to `EnvironmentNames`, pass the loaded module path to `evaluate`, and assert the desired behavior:

```cpp
clear_environment();
set_environment(RC::LinuxStartup::Box64PreloadEnv, "");
expect(RC::LinuxStartup::evaluate(self, self).kind == RC::LinuxStartup::DecisionKind::LegacyStart,
       "empty Box64 preload disabled legacy startup");

set_environment(RC::LinuxStartup::Box64PreloadEnv, ":unrelated.so::");
expect(RC::LinuxStartup::evaluate(self, self).kind == RC::LinuxStartup::DecisionKind::LegacyStart,
       "unrelated Box64 preload disabled legacy startup");

set_environment(RC::LinuxStartup::Box64PreloadEnv, self.string());
auto box64_decision = RC::LinuxStartup::evaluate(self, self);
expect(box64_decision.kind == RC::LinuxStartup::DecisionKind::Box64OrphanedPreload &&
               box64_decision.reason == "box64_target_missing",
       "exact orphaned Box64 preload was not rejected");

set_environment(RC::LinuxStartup::Box64PreloadEnv, self.filename().string());
expect(RC::LinuxStartup::evaluate(self, self).kind == RC::LinuxStartup::DecisionKind::Box64OrphanedPreload,
       "basename-only orphaned Box64 preload was not rejected");

set_environment(RC::LinuxStartup::Box64PreloadEnv, "first.so:" + symlink_path.string() + ":last.so");
expect(RC::LinuxStartup::evaluate(self, self).kind == RC::LinuxStartup::DecisionKind::Box64OrphanedPreload,
       "symlinked orphaned Box64 preload was not rejected");

set_environment(RC::LinuxStartup::Box64PreloadEnv, hard_link_path.string());
expect(RC::LinuxStartup::evaluate(self, self).kind == RC::LinuxStartup::DecisionKind::Box64OrphanedPreload,
       "hard-linked orphaned Box64 preload was not rejected");

set_environment(RC::LinuxStartup::TargetExecutableEnv, self.string());
expect(RC::LinuxStartup::evaluate(self, self).kind == RC::LinuxStartup::DecisionKind::LauncherStart,
       "valid launcher target did not take precedence over Box64 guard");
```

- [ ] **Step 2: Build the unit target and verify RED**

Run:

```bash
cmake --build build_linux --target LinuxStartupPolicyTests --parallel 2
```

Expected: compilation fails because `Box64PreloadEnv`, `Box64OrphanedPreload`, and the two-path `evaluate` overload do not exist.

- [ ] **Step 3: Implement the minimal policy API and matcher**

Add the environment constant, decision, and optional loaded-module argument:

```cpp
inline constexpr char Box64PreloadEnv[] = "BOX64_LD_PRELOAD";

enum class DecisionKind
{
    LegacyStart,
    LauncherStart,
    TargetMismatch,
    Box64OrphanedPreload,
    InvalidLauncherState,
};

auto evaluate(const std::filesystem::path& current_executable = "/proc/self/exe",
              const std::filesystem::path& loaded_module = {}) noexcept -> Decision;
```

In `StartupPolicy.cpp`, add this helper and call it only when `TargetExecutableEnv` is absent:

```cpp
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
                if (std::filesystem::equivalent(candidate, loaded_module, equivalent_error) && !equivalent_error)
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
```

On a positive match, canonicalize the current executable when possible and return:

```cpp
return {
        .kind = DecisionKind::Box64OrphanedPreload,
        .current_executable = canonical_current,
        .reason = "box64_target_missing",
};
```

If the module path is empty, the Box64 variable is absent or empty, or no token matches, return `LegacyStart` unchanged.

- [ ] **Step 4: Build and run the focused unit test GREEN**

Run:

```bash
cmake --build build_linux --target LinuxStartupPolicyTests --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^LinuxStartupPolicyTests$'
```

Expected: build succeeds and `LinuxStartupPolicyTests` passes.

- [ ] **Step 5: Commit the policy and tests**

```bash
git add UE4SS/include/Platform/Linux/StartupPolicy.hpp \
        UE4SS/src/Platform/Linux/StartupPolicy.cpp \
        tests/LinuxStartupPolicyTests.cpp
git commit -m "fix(linux): reject orphaned Box64 preloads"
```

### Task 2: Verify the constructor fails before initialization

**Files:**
- Modify: `UE4SS/src/Platform/Linux/EntryLinux.cpp`
- Modify: `tests/LinuxPreloadScopeProbe.cpp`
- Modify: `tests/RunLinuxPreloadScopeTests.cmake`

- [ ] **Step 1: Add a failing process-level regression case**

Add this `--box64-orphan` mode to `LinuxPreloadScopeProbe`:

```cpp
if (argc == 3 && std::string_view{argv[1]} == "--box64-orphan")
{
    if (auto* ue4ss = dlopen(argv[2], RTLD_NOW | RTLD_NOLOAD))
    {
        dlclose(ue4ss);
        std::puts("UE4SS_BOX64_ORPHAN_PROBE_CLEAN");
        return 0;
    }
    return 21;
}
```

In `RunLinuxPreloadScopeTests.cmake`, invoke it before the normal launcher test with both preload variables naming the staged library and all launcher markers unset:

```cmake
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        --unset=UE4SS_LAUNCH_TARGET_EXE
        --unset=UE4SS_LAUNCH_LD_PRELOAD_WAS_SET
        --unset=UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD
        --unset=UE4SS_MODULE_PATH
        "LD_PRELOAD=${STAGE_DIRECTORY}/libUE4SS.so"
        "BOX64_LD_PRELOAD=${STAGE_DIRECTORY}/libUE4SS.so"
        "UE4SS_DIAGNOSE=1"
        "${PROBE_EXECUTABLE}" --box64-orphan "${STAGE_DIRECTORY}/libUE4SS.so"
    RESULT_VARIABLE orphan_result
    OUTPUT_VARIABLE orphan_stdout
    ERROR_VARIABLE orphan_stderr
    TIMEOUT 4
)
```

Assert a zero exit, the clean marker, `reason=box64_target_missing` on stderr, and the absence of `${STAGE_DIRECTORY}/UE4SS.log`.

- [ ] **Step 2: Build and run the process test RED**

Run:

```bash
cmake --build build_linux --target UE4SS LinuxPreloadScopeProbe --parallel 2
ctest --test-dir build_linux --output-on-failure -R '^LinuxPreloadScopeTests$'
```

Expected: `LinuxPreloadScopeTests` fails because `EntryLinux.cpp` does not pass its loaded module path to the policy, so the probe initializes instead of reporting `box64_target_missing`.

- [ ] **Step 3: Pass the loaded module identity into the policy**

In `ue4ss_so_attached`, use `dladdr(&ue4ss_so_attached, ...)` to capture the actual loaded shared-object path before evaluating the policy. Pass that path as the second `evaluate` argument and reuse it as the runtime module path when `UE4SS_MODULE_PATH` is absent:

```cpp
std::filesystem::path loaded_module_path;
Dl_info dl_info{};
if (dladdr(reinterpret_cast<void*>(&ue4ss_so_attached), &dl_info) != 0 && dl_info.dli_fname)
{
    loaded_module_path = dl_info.dli_fname;
}

const auto startup_decision = LinuxStartup::evaluate("/proc/self/exe", loaded_module_path);
```

Handle `Box64OrphanedPreload` before module initialization:

```cpp
if (startup_decision.kind == LinuxStartup::DecisionKind::Box64OrphanedPreload)
{
    if (LinuxDiagnostics::is_enabled())
    {
        std::fprintf(stderr,
                     "DIAG: startup_skipped executable=%s reason=box64_target_missing\n",
                     startup_decision.current_executable.c_str());
    }
    return;
}
```

Do not create output objects, logs, threads, or inactive-reason diagnostics on this path.

- [ ] **Step 4: Build and run startup/process tests GREEN**

Run:

```bash
cmake --build build_linux --target UE4SS LinuxStartupPolicyTests LinuxPreloadScopeProbe --parallel 2
ctest --test-dir build_linux --output-on-failure \
  -R '^(LinuxStartupPolicyTests|LinuxPreloadScopeTests|LinuxLauncherTests|LinuxFailSoftProbe)$'
```

Expected: all four tests pass.

- [ ] **Step 5: Commit constructor integration**

```bash
git add UE4SS/src/Platform/Linux/EntryLinux.cpp \
        tests/LinuxPreloadScopeProbe.cpp \
        tests/RunLinuxPreloadScopeTests.cmake
git commit -m "test(linux): cover orphaned Box64 helper preload"
```

### Task 3: Document limitations and verify the branch

**Files:**
- Modify: `docs/linux.md`

- [ ] **Step 1: Document best-effort Box64 behavior**

Add a paragraph after the manual-preload compatibility note:

```markdown
Box64 is not a supported target. As a best-effort safety measure, UE4SS skips startup when the launcher target marker is absent and `BOX64_LD_PRELOAD` names the loaded UE4SS library. This prevents recognizable orphaned preloads from initializing in helpers, but it is not full Box64 support; marker-free UE4SS injection through `BOX64_LD_PRELOAD` is intentionally rejected.
```

Add `reason=box64_target_missing` to the diagnostics explanation and retain the existing ARM64 limitation.

- [ ] **Step 2: Run formatting and focused verification**

Run:

```bash
git diff --check
cmake --build build_linux --target UE4SS LinuxStartupPolicyTests LinuxPreloadScopeProbe --parallel 2
ctest --test-dir build_linux --output-on-failure \
  -R '^(LinuxStartupPolicyTests|LinuxPreloadScopeTests|LinuxLauncherTests|LinuxFailSoftProbe)$'
```

Expected: no whitespace errors; all focused tests pass.

- [ ] **Step 3: Run the complete CTest suite**

Run:

```bash
ctest --test-dir build_linux --output-on-failure
```

Expected: all configured tests pass, with environment-dependent Palworld signature coverage allowed to report its existing CTest skip.

- [ ] **Step 4: Commit documentation**

```bash
git add docs/linux.md
git commit -m "docs(linux): clarify Box64 fail-safe scope"
```

- [ ] **Step 5: Review, push, and answer the PR comment**

Inspect `git diff fork/linux-port...HEAD`, push `linux-port` to `fork`, and add an issue-comment reply describing the fail-safe, its compatibility boundary, and the passing tests. Do not claim full Box64 support.
