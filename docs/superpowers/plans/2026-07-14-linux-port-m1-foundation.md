# UE4SS Linux Port — Plan 1/3: Foundation (Slices 1–4) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the UE4SS build system, entry layer, File I/O, logging, and signature scanner build and pass tests natively on Linux (clang, x86-64, glibc), with zero Windows behavior change — delivering upstreamable slices 1–4 of the approved design (`docs/superpowers/specs/2026-07-14-linux-port-design.md`).

**Architecture:** Platform code is split under `UE4SS/src/Platform/{Win32,Linux}/` and per-OS file selection happens in CMake (source-list filtering) and existing `RC_OS_FILE_TYPE_INCLUDE_FILE`-style selector headers. Windows keeps its exact current code paths (files moved verbatim); Linux gets POSIX implementations (`LinuxFile`, `dl_iterate_phdr` module enumeration). The full `libUE4SS.so` link is **not** in scope here — it lands in Plan 2 (hooking + UEPseudo). This plan's deliverable is: all first-party deps compile natively on Linux and File/SigScanner unit tests pass via ctest.

**Tech Stack:** CMake ≥ 3.22 + Ninja, clang (22 locally; ≥ 16 assumed), C++23, POSIX (open/mmap/fstat/dl_iterate_phdr), Rust/corrosion (patternsleuth), ctest for new native tests.

**Scope decisions (deviations from spec, with rationale):**
1. **CMake only; xmake deferred.** Current CI is CMake-only (`.github/workflows/cmake-ci.yml`); xmake.lua still exists but `set_allowedplats("windows")`. PR #384 was xmake-first, but replicating both build systems doubles review surface. xmake Linux parity is noted for the CI/docs slice (Plan 3).
2. **PolyHook2 stays for Linux hook evaluation** (Plan 2): `PolyHookOs.hpp` already defines `POLYHOOK2_OS_LINUX` with mprotect-backed `MemAccessor`; only `PLH::IatHook` is PE-specific. funchook remains the documented fallback if `x64Detour` fails validation on Linux. (Spec §3.5 assumed funchook was required; evidence says validate PolyHook first.)
3. **Follow-up plans:** Plan 2 = slices 5–6 (UEPseudo Linux compile + hooks + EntryLinux loader + `libUE4SS.so` links and loads). Plan 3 = slices 7–8 (Palworld signatures, CI, docs, `run_ue4ss.sh`). Entry criteria for Plan 2: this plan is complete + `deps/first/Unreal` and UEPseudo PR #79 reviewed in the initialized submodule.

**Branch:** all work on `linux-port` (created off `linux-port-design`). Commit per green step; slice boundaries below map 1:1 to future upstream PRs.

**Verification constraints on this machine:** Linux-native builds/tests run locally. Windows builds cannot run locally (no MSVC); Windows no-behavior-change is verified by (a) moves being byte-identical (`git diff --find-renames` shows 100% rename), (b) all Windows-only code staying behind unchanged `#ifdef _WIN32`/`if(WIN32)` guards, and (c) GitHub Actions `cmake-ci.yml` once pushed. Do not claim Windows-green without CI evidence.

---

## File structure (what this plan creates/modifies)

```
UE4SS/src/Platform/Win32/EntryWin32.cpp        # moved verbatim from UE4SS/src/main_ue4ss_rewritten.cpp
UE4SS/src/Platform/Linux/EntryLinux.cpp        # new: .so constructor + init thread (compiled only in Plan 2 link)
UE4SS/CMakeLists.txt                           # platform source filtering, conditional link libs
CMakeLists.txt                                 # ASM_MASM conditional
cmake/modules/ProjectConfig.cmake              # Linux platform type + defines, UNICODE gating
cmake/modules/CompilerOptions/clang.cmake      # native-clang flags (-fPIC, -fms-extensions, warnings)
deps/third/CMakeLists.txt                      # gate raw_pdb/GUI stack on WIN32/UE4SS_GUI
deps/first/File/include/File/FileType/LinuxFile.hpp   # real class def (int fd, POSIX identifying props)
deps/first/File/src/FileType/LinuxFile.cpp     # new: full POSIX implementation
deps/first/File/CMakeLists.txt                 # add LinuxFile.cpp (WinFile.cpp already _WIN32-guarded)
deps/first/SinglePassSigScanner/include/SigScanner/SinglePassSigScanner.hpp  # platform module-info split
deps/first/SinglePassSigScanner/src/SinglePassSigScanner.cpp                 # Linux scan paths
deps/first/Helpers/include/Helpers/SysError.hpp # POSIX errno branch
tests/CMakeLists.txt                           # new: UE4SS_BUILD_TESTS option, ctest targets
tests/FileTests.cpp                            # new: LinuxFile unit tests
tests/SigScannerTests.cpp                      # new: in-process scan test
```

---

## Task 0: Branch + build directory hygiene

- [ ] **Step 0.1:** `git checkout -b linux-port` (from `linux-port-design`).
- [ ] **Step 0.2:** Confirm submodules initialized: `git submodule status` shows `deps/first/Unreal` and `deps/first/patternsleuth` without `-` prefix.
- [ ] **Step 0.3:** Add `build_*/` to local ignore if not covered: check `git check-ignore build_linux` (the repo already ignores common build dirs; if not, use `.git/info/exclude`, do NOT edit `.gitignore` in this slice).

## Task 1 (Slice 1): Move Windows entry to Platform/Win32 — zero behavior change

**Files:**
- Move: `UE4SS/src/main_ue4ss_rewritten.cpp` → `UE4SS/src/Platform/Win32/EntryWin32.cpp`
- Modify: `UE4SS/CMakeLists.txt` (glob currently `file(GLOB_RECURSE UE4SS_SOURCES "src/**.cpp")` picks the move up automatically — add platform filtering)

- [ ] **Step 1.1:** `mkdir -p UE4SS/src/Platform/Win32 UE4SS/src/Platform/Linux && git mv UE4SS/src/main_ue4ss_rewritten.cpp UE4SS/src/Platform/Win32/EntryWin32.cpp`. No content edits whatsoever.
- [ ] **Step 1.2:** In `UE4SS/CMakeLists.txt`, immediately after the `file(GLOB_RECURSE UE4SS_SOURCES ...)` line, add platform filtering:

```cmake
# Platform-specific entry/bootstrap sources: compile only the current platform's directory.
list(FILTER UE4SS_SOURCES EXCLUDE REGEX "/src/Platform/")
if(WIN32)
    file(GLOB_RECURSE UE4SS_PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Win32/**.cpp")
else()
    file(GLOB_RECURSE UE4SS_PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/Linux/**.cpp")
endif()
list(APPEND UE4SS_SOURCES ${UE4SS_PLATFORM_SOURCES})
```

- [ ] **Step 1.3:** Verify rename detection: `git status` + `git diff --cached --find-renames --stat` after `git add -A` shows `renamed: ... (100%)` for the entry file.
- [ ] **Step 1.4:** Commit: `git commit -m "refactor: move Windows DllMain bootstrap to Platform/Win32/EntryWin32.cpp"` (message notes zero behavior change; CMake filters platform sources).

## Task 2 (Slice 1): Linux entry skeleton

**Files:**
- Create: `UE4SS/src/Platform/Linux/EntryLinux.cpp`

- [ ] **Step 2.1:** Create the file. It is a modernized version of PR #384's `EntryLinux.cpp` (init-thread approach; the `__libc_start_main`/`__cxa_throw` interposition experiments are **not** carried over — the constructor + poll approach from spec §3.3 is used instead). It will not be compiled until the Linux `UE4SS` target links in Plan 2, but must be complete and self-consistent now:

```cpp
#ifdef __linux__

#include <cstdio>
#include <thread>

#include <dlfcn.h>

#include "UE4SSProgram.hpp"
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

using namespace RC;

namespace
{
    bool s_ue4ss_started = false;

    auto thread_so_start(UE4SSProgram* program) -> void
    {
        program->init();

        if (auto e = program->get_error_object(); e->has_error())
        {
            if (!Output::has_internal_error())
            {
                Output::send<LogLevel::Error>(STR("Fatal Error: {}\n"), ensure_str(e->get_message()));
            }
            else
            {
                printf("Error: %s\n", e->get_message());
            }
        }
    }
} // namespace

// Called when the .so is loaded (LD_PRELOAD or dlopen). We must not block here:
// spawn the init thread and return so the dynamic linker can finish.
__attribute__((constructor)) static void ue4ss_so_attached()
{
    if (s_ue4ss_started)
    {
        return;
    }
    s_ue4ss_started = true;

    // Locate our own .so path; UE4SSProgram derives all directories from it.
    Dl_info dl_info{};
    if (dladdr(reinterpret_cast<void*>(&ue4ss_so_attached), &dl_info) == 0 || !dl_info.dli_fname)
    {
        fprintf(stderr, "UE4SS: failed to determine libUE4SS.so path; aborting startup\n");
        return;
    }

    auto program = new UE4SSProgram(std::filesystem::path{dl_info.dli_fname}, {});
    std::thread{&thread_so_start, program}.detach();
}

__attribute__((destructor)) static void ue4ss_so_detached()
{
    if (s_ue4ss_started)
    {
        UE4SSProgram::static_cleanup();
        s_ue4ss_started = false;
    }
}

#endif // __linux__
```

Note: `UE4SSProgram`'s constructor takes `const std::filesystem::path&` (see `UE4SS/src/UE4SSProgram.cpp:192`), so passing a path built from `dli_fname` is correct on both char widths.

- [ ] **Step 2.2:** Compile-syntax sanity check (header deps aren't Linux-clean yet, so only parse-check the file standalone): `clang++ -std=c++23 -fsyntax-only -I UE4SS/include -I deps/first/DynamicOutput/include -I deps/first/File/include -I deps/first/String/include -I deps/first/Helpers/include -I deps/first/Constructs/include -I deps/first/MProgram/include UE4SS/src/Platform/Linux/EntryLinux.cpp` — expected to FAIL at this point on Windows-only transitive includes; record the failure output in the commit message body as the to-do list for Task 4+. (This is a discovery step, not a gate.)
- [ ] **Step 2.3:** Commit: `git commit -m "feat(linux): add Linux shared-object entry skeleton"`.

## Task 3 (Slice 2): Build system — Linux platform type

**Files:**
- Modify: `CMakeLists.txt:10` (ASM_MASM)
- Modify: `cmake/modules/ProjectConfig.cmake:14,56-57,84,87-89`
- Modify: `cmake/modules/CompilerOptions/clang.cmake`
- Modify: `deps/third/CMakeLists.txt` (raw_pdb, GUI stack)
- Modify: `UE4SS/CMakeLists.txt` (link libs)

- [ ] **Step 3.1:** `CMakeLists.txt` line 10: replace `enable_language(CXX ASM_MASM)` with:

```cmake
if(WIN32)
    enable_language(CXX ASM_MASM)
else()
    enable_language(CXX)
endif()
```

- [ ] **Step 3.2:** `cmake/modules/ProjectConfig.cmake`: make the platform list host-conditional and add Linux defines. Replace line 14 with:

```cmake
if(WIN32)
    set(UE4SS_PLATFORM_TYPES "Win64" CACHE STRING "Supported platform types")
else()
    set(UE4SS_PLATFORM_TYPES "Linux" CACHE STRING "Supported platform types")
endif()
```

After the `UE4SS_Win64_DEFINITIONS` block (lines 56-57) add:

```cmake
set(UE4SS_Linux_DEFINITIONS PLATFORM_LINUX PLATFORM_UNIX LINUX OVERRIDE_PLATFORM_HEADER_NAME=Linux UBT_COMPILED_PLATFORM=Linux)
set(UE4SS_Linux_VARS CMAKE_SYSTEM_PROCESSOR=x86_64)
```

In `ue4ss_initialize_project()`, gate the Unicode defines (line 84) to Windows:

```cmake
    # Unicode support (Windows APIs); Linux uses char16_t via FORCE_U16 in String/StringType.hpp
    if(WIN32)
        list(APPEND TARGET_COMPILE_DEFINITIONS _UNICODE UNICODE)
    endif()
```

(Keep the existing `if(WIN32)` `_WIN32_WINNT` block as is. The CI workflow greps `set(UE4SS_PLATFORM_TYPES` — the `if(WIN32)` branch keeps a one-line `set(...)` so the regex still matches; verify `grep 'set(UE4SS_PLATFORM_TYPES' cmake/modules/ProjectConfig.cmake` returns the Win64 line first.)

- [ ] **Step 3.3:** `cmake/modules/CompilerOptions/clang.cmake`: read it first; ensure it provides at minimum (append if missing, keeping existing content):

```cmake
set(DEFAULT_COMPILER_FLAGS "-fms-extensions;-Wno-unknown-pragmas;-Wno-unused-parameter;-Wignored-attributes;-fPIC;-fcolor-diagnostics")
set(DEFAULT_EXE_LINKER_FLAGS "")
set(Shipping_FLAGS "-g")
```

(Flags harvested from PR #384's `CLANG_COMPILE_OPTIONS`; `-fms-extensions` is required because the codebase carries MSVC-isms.)

- [ ] **Step 3.4:** `deps/third/CMakeLists.txt`: wrap the `raw_pdb` FetchContent/target block in `if(WIN32) ... endif()`. Introduce a GUI gate at the top:

```cmake
if(WIN32)
    option(UE4SS_GUI "Build the ImGui debugging GUI" ON)
else()
    option(UE4SS_GUI "Build the ImGui debugging GUI" OFF)
endif()
```

and wrap the ImGui/ImGuiTextEdit/IconFontCppHeaders/GLFW/glad blocks in `if(UE4SS_GUI) ... endif()`. Keep PolyHook_2_0, fmt, glaze, zydis, corrosion, tracy unconditional (all are cross-platform).
- [ ] **Step 3.5:** `UE4SS/CMakeLists.txt`: split link libraries (current lines ~134-138). Replace the Windows-libs line with:

```cmake
if(WIN32)
    target_link_libraries(UE4SS PUBLIC d3d11 glfw glad opengl32 dbghelp psapi ws2_32 ntdll userenv)
else()
    target_link_libraries(UE4SS PUBLIC dl pthread)
endif()
if(UE4SS_GUI)
    target_compile_definitions(UE4SS PRIVATE UE4SS_HAS_GUI)
endif()
```

(ImGui/GLFW references in the main `target_link_libraries` list also move under `if(UE4SS_GUI)`.) Also wrap `add_subdirectory(proxy_generator)` (wherever it is invoked — check `UE4SS/CMakeLists.txt` and top level) in `if(WIN32)`.
- [ ] **Step 3.6:** Gate UVTD: in `cmake/modules/ProjectConfig.cmake` line 11, make the project list conditional:

```cmake
if(WIN32)
    set(UE4SS_PROJECTS "UE4SS" "UVTD" CACHE STRING "List of main project targets")
else()
    set(UE4SS_PROJECTS "UE4SS" CACHE STRING "List of main project targets")
endif()
```

Similarly gate `add_subdirectory("cppmods")` in the top-level `CMakeLists.txt` behind `if(WIN32)` for now (cppmods link against UE4SS; revisit in Plan 2).
- [ ] **Step 3.7:** Configure natively:

```bash
cmake -B build_linux -G Ninja \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Linux
```

Expected: configure completes (FetchContent downloads; corrosion picks host Rust target). Fix configure-time errors as they appear — each fix is one commit. Known likely issues: `Findzydis`, GLFW subdirectory when `UE4SS_GUI=OFF` (must be skipped), version checks (MSVC check must be skipped for non-MSVC — `VersionChecks.cmake` already skips for clang-cl; extend to plain clang if it errors).
- [ ] **Step 3.8:** Commit: `git commit -m "build: add native Linux platform to CMake (Game__*__Linux modes)"`.

## Task 4 (Slice 2): First-party deps compile natively — the compile-fix loop

**Procedure (repeat per target, in dependency order):** `ninja -C build_linux <target>` → fix errors → commit `fix(linux): compile <target> natively`. Rules for fixes:
- Prefer portable code over `#ifdef` (e.g. `printf_s` → `printf` is NOT portable-safe to change on Windows; instead add `#define printf_s printf` is forbidden — use `fmt::print` or guard).
- Windows-only code stays behind `#ifdef _WIN32` exactly as-is; Linux branches added as `#elif defined(__linux__)` or `#else`.
- Never change a Windows code path to "clean up" — no-behavior-change invariant.

Target order and known work items (from PR #384's catalog + explorer findings):

- [ ] **Step 4.1:** `String` — expected near-clean (`FORCE_U16` path already exists).
- [ ] **Step 4.2:** `Constructs`, `Function`, `ParserBase`, `IniParser`, `JSON`, `ArgsParser`, `ScopedTimer`, `Profiler` — expected mostly clean; fix includes (`<Windows.h>`) if hit.
- [ ] **Step 4.3:** `Helpers` — `SysError.hpp`: add POSIX branch returning `strerror_r` text for `errno` (keep the same public shape: constructible from an error code, convertible via `to_string`). Read the header first; implement the Linux branch in the same header/source layout:

```cpp
#ifdef _WIN32
// existing GetLastError/FormatMessage implementation, unchanged
#else
#include <cerrno>
#include <cstring>
namespace RC
{
    class SysError
    {
      public:
        explicit SysError(int error_code) : m_code(error_code)
        {
            char buf[256]{};
            // GNU strerror_r may return a static string instead of filling buf
            m_message = strerror_r(error_code, buf, sizeof(buf));
        }
        auto code() const -> int { return m_code; }
        auto message() const -> const std::string& { return m_message; }
      private:
        int m_code{};
        std::string m_message{};
    };
} // namespace RC
#endif
```

Adapt to the actual existing class shape after reading the file — the Windows API surface (whatever `to_string(SysError(GetLastError()))` relies on) must exist identically for `to_string(SysError(errno))`.
- [ ] **Step 4.4:** `File` — will fail to fully build until Task 5 provides `LinuxFile.cpp`; acceptable to interleave (Task 5 is next). Object-compile of `File.cpp` must succeed now.
- [ ] **Step 4.5:** `DynamicOutput` — `DebugConsoleDevice.cpp` already `#ifdef _WIN32`-guards console-mode; fix any `to_wstring` vs `char16_t` mismatches by using `to_charT_string`/`to_u16string` helpers from `Helpers/String.hpp`.
- [ ] **Step 4.6:** `Input` — `PlatformInit.cpp` registers `Win32AsyncInputSource` unconditionally; gate it:

```cpp
#ifdef _WIN32
    register_input_source(std::make_shared<Win32AsyncInputSource>(L"ConsoleWindowClass", L"UnrealWindow"));
#endif
#ifdef HAS_GLFW
    register_input_source(std::make_shared<GLFW3InputSource>());
#endif
```

(Check how GLFW3InputSource is currently gated — mirror the existing macro; on headless Linux only `QueueInputSource` remains, per spec §3.8.) Exclude `src/Platform/Win32*` sources from the Linux build in `deps/first/Input/CMakeLists.txt` the same `list(FILTER ...)` way as Task 1.2.
- [ ] **Step 4.7:** `LuaRaw`/`LuaMadeSimple` — Lua C sources are portable; fix build-script issues only.
- [ ] **Step 4.8:** `ASMHelper` (zydis-based) and `patternsleuth_bind` (Rust) — expected portable; corrosion must select the host triple (no `Rust_CARGO_TARGET` override outside the xwin toolchain file — verify none leaks in).
- [ ] **Step 4.9:** `SinglePassSigScanner` — compiles only after Task 6's port; object files for the shared parts should compile now.
- [ ] **Step 4.10:** Verification gate for this task: `ninja -C build_linux String Constructs Function IniParser ParserBase JSON Helpers DynamicOutput Input LuaMadeSimple ASMHelper patternsleuth_bind` → exit 0. (Adjust target names to the actual CMake target names discovered.)

## Task 5 (Slice 3): LinuxFile — TDD

**Files:**
- Create: `tests/CMakeLists.txt`, `tests/FileTests.cpp`
- Modify: `CMakeLists.txt` (add `add_subdirectory(tests)` behind option), `deps/first/File/include/File/FileType/LinuxFile.hpp`
- Create: `deps/first/File/src/FileType/LinuxFile.cpp`
- Modify: `deps/first/File/CMakeLists.txt` (source list — check whether it globs; if explicit, add the new .cpp)

- [ ] **Step 5.1:** Test infra. Top-level `CMakeLists.txt` after `add_subdirectory("deps")`:

```cmake
option(UE4SS_BUILD_TESTS "Build native unit tests" OFF)
if(UE4SS_BUILD_TESTS)
    enable_testing()
    add_subdirectory("tests")
endif()
```

`tests/CMakeLists.txt`:

```cmake
add_executable(FileTests FileTests.cpp)
target_link_libraries(FileTests PRIVATE File Helpers)
target_compile_definitions(FileTests PRIVATE RC_FILE_BUILD_STATIC)
add_test(NAME FileTests COMMAND FileTests)
```

- [ ] **Step 5.2:** Write the failing test (`tests/FileTests.cpp`). Assert-macro style, no framework:

```cpp
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <string>

#include <File/File.hpp>
#include <Helpers/String.hpp>

using namespace RC;

#define CHECK(cond)                                                                                                                                            \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (!(cond))                                                                                                                                           \
        {                                                                                                                                                      \
            fprintf(stderr, "FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__);                                                                               \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

static auto test_write_and_read_roundtrip(const std::filesystem::path& dir) -> int
{
    auto path = dir / "ue4ss_file_test.txt";
    {
        auto handle = File::open(path, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        handle.write_string_to_file(STR("hello linux åäö\n"));
        handle.close();
    }
    {
        auto handle = File::open(path, File::OpenFor::Reading);
        auto contents = handle.read_all();
        CHECK(contents == StringType{STR("hello linux åäö\n")});
        handle.close();
    }
    return 0;
}

static auto test_append(const std::filesystem::path& dir) -> int
{
    auto path = dir / "ue4ss_append_test.txt";
    std::filesystem::remove(path);
    {
        auto h = File::open(path, File::OpenFor::Appending, File::OverwriteExistingFile::No, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("a"));
        h.close();
    }
    {
        auto h = File::open(path, File::OpenFor::Appending, File::OverwriteExistingFile::No, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("b"));
        h.close();
    }
    auto h = File::open(path, File::OpenFor::Reading);
    CHECK(h.read_all() == StringType{STR("ab")});
    h.close();
    return 0;
}

static auto test_memory_map(const std::filesystem::path& dir) -> int
{
    auto path = dir / "ue4ss_mmap_test.bin";
    {
        auto h = File::open(path, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("mmap-data"));
        h.close();
    }
    auto h = File::open(path, File::OpenFor::Reading);
    auto span = h.memory_map();
    CHECK(span.size() >= 9);
    CHECK(span[0] == 'm' && span[8] == 'a');
    h.close();
    return 0;
}

static auto test_open_missing_throws(const std::filesystem::path& dir) -> int
{
    bool threw = false;
    try
    {
        auto h = File::open(dir / "does_not_exist_ue4ss.txt", File::OpenFor::Reading);
    }
    catch (const std::exception&)
    {
        threw = true;
    }
    CHECK(threw);
    return 0;
}

static auto test_delete(const std::filesystem::path& dir) -> int
{
    auto path = dir / "ue4ss_delete_test.txt";
    {
        auto h = File::open(path, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("x"));
        h.close();
    }
    File::delete_file(path);
    CHECK(!std::filesystem::exists(path));
    return 0;
}

int main()
{
    auto dir = std::filesystem::temp_directory_path();
    int rc = 0;
    rc |= test_write_and_read_roundtrip(dir);
    rc |= test_append(dir);
    rc |= test_memory_map(dir);
    rc |= test_open_missing_throws(dir);
    rc |= test_delete(dir);
    if (rc == 0) printf("FileTests: all passed\n");
    return rc;
}
```

(`File::Handle::write_string_to_file`/`read_all`/`close`/`memory_map` — verify exact `HandleTemplate` member names in `deps/first/File/include/File/HandleTemplate.hpp` before finalizing; adjust test calls to the real handle API.)
- [ ] **Step 5.3:** Run to verify failure: `cmake -B build_linux -DUE4SS_BUILD_TESTS=ON && ninja -C build_linux FileTests`. Expected: **link failure** — undefined `RC::File::LinuxFile::open_file` etc.
- [ ] **Step 5.4:** Rewrite `deps/first/File/include/File/FileType/LinuxFile.hpp`: keep the class name, public API, and `FileInterface` overrides exactly as-is, but replace the Windows-copied internals with POSIX ones:
  - `using HANDLE = void*` and `HANDLE m_file{}` → `int m_fd{-1};`
  - `HANDLE m_map_handle{}` → `size_t m_map_size{};`
  - `IdentifyingProperties` → `{ uint64_t device{}; uint64_t inode{}; int64_t mtime_sec{}; int64_t mtime_nsec{}; uint64_t file_size{}; }`
  - `set_file(HANDLE)`/`get_file()` → `set_file(int fd)`/`get_file() -> int`
  - Remove the stale "included ONLY if Windows is detected" comment; keep the `Handle = HandleTemplate<LinuxFile>` alias.
- [ ] **Step 5.5:** Implement `deps/first/File/src/FileType/LinuxFile.cpp`, modeled 1:1 on `WinFile.cpp` (same function order, same THROW_INTERNAL_FILE_ERROR contract, same serialization cache logic):

```cpp
#ifdef __linux__
#include <fstream>

#include <File/File.hpp>
#include <File/FileType/LinuxFile.hpp>
#include <File/HandleTemplate.hpp>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include <Helpers/String.hpp>
#include <fmt/core.h>

namespace RC::File
{
    static auto errno_string() -> std::string
    {
        char buf[256]{};
        return std::string{strerror_r(errno, buf, sizeof(buf))};
    }

    auto LinuxFile::is_valid() noexcept -> bool
    {
        return m_fd >= 0;
    }

    auto LinuxFile::invalidate_file() noexcept -> void
    {
        m_fd = -1;
        m_memory_map = nullptr;
        m_map_size = 0;
    }

    auto LinuxFile::delete_file(const std::filesystem::path& file_path_and_name) -> void
    {
        if (::unlink(file_path_and_name.c_str()) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::delete_file] Was unable to delete file, error: {}", errno_string()))
        }
    }

    auto LinuxFile::delete_file() -> void
    {
        if (m_is_file_open)
        {
            close_file();
        }
        delete_file(m_file_path_and_name);
    }

    auto LinuxFile::set_file(int new_fd) -> void
    {
        m_fd = new_fd;
    }

    auto LinuxFile::get_file() -> int
    {
        return m_fd;
    }

    auto LinuxFile::set_is_file_open(bool new_is_open) -> void
    {
        m_is_file_open = new_is_open;
    }

    auto LinuxFile::get_raw_handle() noexcept -> void*
    {
        return reinterpret_cast<void*>(static_cast<intptr_t>(m_fd));
    }

    auto LinuxFile::get_file_path() const noexcept -> const std::filesystem::path&
    {
        return m_file_path_and_name;
    }

    auto LinuxFile::set_serialization_output_file(const std::filesystem::path& output_file) noexcept -> void
    {
        m_serialization_file_path_and_name = output_file;
    }

    auto LinuxFile::serialization_file_exists() -> bool
    {
        return std::filesystem::exists(m_serialization_file_path_and_name);
    }

    template <typename DataType>
    auto write_to_file(LinuxFile& file, const DataType* data, size_t num_bytes_to_write) -> void
    {
        if (!file.is_file_open())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::write_to_file] Tried writing to file but the file is not open")
        }

        const auto* bytes = reinterpret_cast<const uint8_t*>(data);
        size_t total_written{};
        while (total_written < num_bytes_to_write)
        {
            ssize_t written = ::write(file.get_file(), bytes + total_written, num_bytes_to_write - total_written);
            if (written < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                THROW_INTERNAL_FILE_ERROR(
                        fmt::format("[LinuxFile::write_to_file] Tried writing to file but was unable to complete operation. error: {}", errno_string()))
            }
            total_written += static_cast<size_t>(written);
        }
    }

    // Serialization Format (Linux):
    // device
    // inode
    // mtime_sec
    // mtime_nsec
    // file_size
    // user_data
    auto LinuxFile::serialize_identifying_properties() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_identifying_properties]: Path & file name for serialization file is empty, please call "
                                      "'set_serialization_output_file'")
        }

        struct stat file_info{};
        if (::fstat(m_fd, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::serialize_identifying_properties] fstat failed, error: {}", errno_string()))
        }

        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLongLong, .data_ulonglong = static_cast<unsigned long long>(file_info.st_dev)}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLongLong, .data_ulonglong = static_cast<unsigned long long>(file_info.st_ino)}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::SignedLongLong, .data_longlong = static_cast<signed long long>(file_info.st_mtim.tv_sec)}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::SignedLongLong, .data_longlong = static_cast<signed long long>(file_info.st_mtim.tv_nsec)}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLongLong, .data_ulonglong = static_cast<unsigned long long>(file_info.st_size)}, true);
    }

    auto LinuxFile::deserialize_identifying_properties() -> void
    {
        m_identifying_properties.device = *static_cast<uint64_t*>(get_serialized_item(sizeof(uint64_t), true));
        m_identifying_properties.inode = *static_cast<uint64_t*>(get_serialized_item(sizeof(uint64_t), true));
        m_identifying_properties.mtime_sec = *static_cast<int64_t*>(get_serialized_item(sizeof(int64_t), true));
        m_identifying_properties.mtime_nsec = *static_cast<int64_t*>(get_serialized_item(sizeof(int64_t), true));
        m_identifying_properties.file_size = *static_cast<uint64_t*>(get_serialized_item(sizeof(uint64_t), true));

        m_offset_to_next_serialized_item = sizeof(IdentifyingProperties);
        m_has_cached_identifying_properties = true;
    }

    auto LinuxFile::is_deserialized_and_live_equal() -> bool
    {
        if (!m_has_cached_identifying_properties)
        {
            if (!std::filesystem::exists(m_serialization_file_path_and_name))
            {
                return false;
            }
            deserialize_identifying_properties();
        }

        struct stat live_info{};
        if (::fstat(m_fd, &live_info) != 0)
        {
            return false;
        }

        return static_cast<uint64_t>(live_info.st_dev) == m_identifying_properties.device &&
               static_cast<uint64_t>(live_info.st_ino) == m_identifying_properties.inode &&
               static_cast<int64_t>(live_info.st_mtim.tv_sec) == m_identifying_properties.mtime_sec &&
               static_cast<int64_t>(live_info.st_mtim.tv_nsec) == m_identifying_properties.mtime_nsec &&
               static_cast<uint64_t>(live_info.st_size) == m_identifying_properties.file_size;
    }

    auto LinuxFile::invalidate_serialization() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::invalidate_serialization] Could not invalidate serialization file because "
                                      "'m_serialization_file_path_and_name' was empty, please call 'set_serialization_output_file'")
        }

        if (std::filesystem::exists(m_serialization_file_path_and_name))
        {
            delete_file(m_serialization_file_path_and_name);
        }
    }

    template <typename DataType>
    auto serialize_typed_item(DataType data, Handle& output_file) -> void
    {
        write_to_file(output_file.get_underlying_type(), &data, sizeof(DataType));
    }

    auto LinuxFile::serialize_item(const GenericItemData& data, bool is_internal_item) -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR(
                    "[LinuxFile::serialize_item]: Path & file name for serialization file is empty, please call 'set_serialization_output_file'")
        }

        if (!serialization_file_exists() && !is_internal_item)
        {
            serialize_identifying_properties();
        }

        Handle serialization_file = open(m_serialization_file_path_and_name, OpenFor::Appending, OverwriteExistingFile::No, CreateIfNonExistent::Yes);

        switch (data.data_type)
        {
        case GenericDataType::UnsignedLong:
            serialize_typed_item<unsigned long>(data.data_ulong, serialization_file);
            serialization_file.get_underlying_type().m_offset_to_next_serialized_item += sizeof(unsigned long);
            break;
        case GenericDataType::SignedLong:
            serialize_typed_item<signed long>(data.data_long, serialization_file);
            serialization_file.get_underlying_type().m_offset_to_next_serialized_item += sizeof(signed long);
            break;
        case GenericDataType::UnsignedLongLong:
            serialize_typed_item<unsigned long long>(data.data_ulonglong, serialization_file);
            serialization_file.get_underlying_type().m_offset_to_next_serialized_item += sizeof(unsigned long long);
            break;
        case GenericDataType::SignedLongLong:
            serialize_typed_item<signed long long>(data.data_longlong, serialization_file);
            serialization_file.get_underlying_type().m_offset_to_next_serialized_item += sizeof(signed long long);
            break;
        }

        serialization_file.close();
    }

    auto LinuxFile::get_serialized_item(size_t data_size, bool is_internal_item) -> void*
    {
        if (!m_has_cache_in_memory)
        {
            if (m_serialization_file_path_and_name.empty())
            {
                THROW_INTERNAL_FILE_ERROR(
                        "[LinuxFile::get_serialized_item]: Path & file name for serialization file is empty, please call 'set_serialization_output_file'")
            }

            Handle cache_file = open(m_serialization_file_path_and_name);

            ssize_t bytes_read = ::read(static_cast<int>(reinterpret_cast<intptr_t>(cache_file.get_raw_handle())), &m_cache, cache_size);
            if (bytes_read < 0)
            {
                THROW_INTERNAL_FILE_ERROR(
                        fmt::format("[LinuxFile::get_serialized_item] Tried deserializing file but was unable to complete operation. error: {}", errno_string()))
            }

            cache_file.close();
            m_has_cache_in_memory = true;
        }

        if (!m_has_cached_identifying_properties && !is_internal_item)
        {
            deserialize_identifying_properties();
        }

        void* data_ptr = &m_cache[m_offset_to_next_serialized_item];
        m_offset_to_next_serialized_item += data_size;
        return data_ptr;
    }

    auto LinuxFile::close_current_file() -> void
    {
        close_file();
    }

    auto LinuxFile::create_all_directories(const std::filesystem::path& file_name_and_path) -> void
    {
        if (file_name_and_path.parent_path().empty())
        {
            return;
        }

        try
        {
            std::filesystem::create_directories(file_name_and_path.parent_path());
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::create_all_directories] Tried creating directories '{}' but encountered an error. error: {}",
                                                  file_name_and_path.string(),
                                                  e.what()))
        }
    }

    auto LinuxFile::close_file() -> void
    {
        if (m_memory_map)
        {
            if (::munmap(m_memory_map, m_map_size) != 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::close_file] Was unable to unmap file, error: {}", errno_string()))
            }
            m_memory_map = nullptr;
            m_map_size = 0;
        }

        if (!is_valid() || !is_file_open())
        {
            return;
        }

        if (::close(m_fd) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::close_file] Was unable to close file, {}", errno_string()))
        }
        set_is_file_open(false);
        m_fd = -1;
    }

    auto LinuxFile::is_file_open() const -> bool
    {
        return m_is_file_open;
    }

    auto LinuxFile::write_string_to_file(StringViewType string_to_write) -> void
    {
        // StringType is UTF-16 (char16_t) on Linux; files are written as UTF-8.
        auto string_converted_to_utf8 = to_string(string_to_write);
        write_to_file(*this, string_converted_to_utf8.data(), string_converted_to_utf8.size());
    }

    auto LinuxFile::is_same_as(LinuxFile& other_file) -> bool
    {
        struct stat file_info{};
        if (::fstat(m_fd, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::is_same_as] fstat failed. {}", errno_string()))
        }

        struct stat other_file_info{};
        if (::fstat(other_file.get_file(), &other_file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::is_same_as] fstat failed. {}", errno_string()))
        }

        return file_info.st_dev == other_file_info.st_dev && file_info.st_ino == other_file_info.st_ino;
    }

    auto LinuxFile::read_all() const -> StringType
    {
        std::ifstream stream{get_file_path(), std::ios::in | std::ios::binary};
        if (!stream)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::read_all] Tried to read entire file but returned error {}", errno))
        }

        std::string file_contents;
        stream.seekg(0, std::ios::end);
        auto size = stream.tellg();
        if (size == -1)
        {
            return {};
        }
        file_contents.resize(static_cast<size_t>(size));
        stream.seekg(0, std::ios::beg);
        stream.read(file_contents.data(), static_cast<std::streamsize>(file_contents.size()));
        stream.close();

        // Strip UTF-8 BOM if present
        if (file_contents.size() >= 3 && static_cast<uint8_t>(file_contents[0]) == 0xEF && static_cast<uint8_t>(file_contents[1]) == 0xBB &&
            static_cast<uint8_t>(file_contents[2]) == 0xBF)
        {
            file_contents.erase(0, 3);
        }

        // Files on disk are UTF-8; StringType is UTF-16 (char16_t) on Linux.
        return to_u16string(file_contents);
    }

    auto LinuxFile::memory_map() -> std::span<uint8_t>
    {
        int prot{};
        switch (m_open_properties.open_for)
        {
        case OpenFor::Writing:
        case OpenFor::Appending:
        case OpenFor::ReadWrite:
            prot = PROT_READ | PROT_WRITE;
            break;
        case OpenFor::Reading:
            prot = PROT_READ;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::memory_map] Tried to memory map file but 'm_open_properties' contains invalid data.")
        }

        struct stat file_info{};
        if (::fstat(m_fd, &file_info) != 0 || file_info.st_size == 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::memory_map] fstat failed or file empty. {}", errno_string()))
        }

        void* mapped = ::mmap(nullptr, static_cast<size_t>(file_info.st_size), prot, MAP_SHARED, m_fd, 0);
        if (mapped == MAP_FAILED)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::memory_map] mmap failed. {}", errno_string()))
        }

        m_memory_map = static_cast<uint8_t*>(mapped);
        m_map_size = static_cast<size_t>(file_info.st_size);
        return std::span(m_memory_map, m_map_size);
    }

    auto LinuxFile::open_file(const std::filesystem::path& file_name_and_path, const OpenProperties& open_properties) -> LinuxFile
    {
        if (file_name_and_path.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::open_file] Tried to open file but file_name_and_path was empty.")
        }

        int flags{};
        switch (open_properties.open_for)
        {
        case OpenFor::Writing:
            flags = O_WRONLY;
            break;
        case OpenFor::Appending:
            flags = O_WRONLY | O_APPEND;
            break;
        case OpenFor::Reading:
            flags = O_RDONLY;
            break;
        case OpenFor::ReadWrite:
            flags = O_RDWR;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::open_file] Tried to open file but received invalid data for the 'OpenFor' parameter.")
        }

        if (open_properties.overwrite_existing_file == OverwriteExistingFile::Yes)
        {
            create_all_directories(file_name_and_path);
            flags |= O_CREAT | O_TRUNC;
        }
        else if (open_properties.create_if_non_existent == CreateIfNonExistent::Yes)
        {
            create_all_directories(file_name_and_path);
            flags |= O_CREAT;
        }

        LinuxFile file{};
        file.set_file(::open(file_name_and_path.c_str(), flags, 0644));

        if (file.get_file() < 0)
        {
            std::string_view open_type = open_properties.open_for == OpenFor::Writing || open_properties.open_for == OpenFor::Appending ? "writing" : "reading";
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::open_file] Tried opening file for {} but encountered an error. Path & File: {} | error: {}\n",
                                                  open_type,
                                                  file_name_and_path.string(),
                                                  errno_string()))
        }

        file.m_file_path_and_name = file_name_and_path;
        file.set_is_file_open(true);
        file.m_open_properties = open_properties;

        return file;
    }
} // namespace RC::File

#endif // __linux__
```

Notes: `serialize_item`/`get_serialized_item` need to be `friend`s of or accessors on the handle exactly like WinFile (check `m_offset_to_next_serialized_item` visibility — WinFile.cpp accesses it through `get_underlying_type()`; mirror whatever access WinFile has, including any `friend` declarations in the header). `to_string(StringViewType)`/`to_u16string(std::string)` come from `Helpers/String.hpp` (verified present, lines 260/286).
- [ ] **Step 5.6:** Add the source to `deps/first/File/CMakeLists.txt` (if explicit source list) and build: `ninja -C build_linux FileTests`. Expected: builds.
- [ ] **Step 5.7:** Run: `ctest --test-dir build_linux -R FileTests --output-on-failure`. Expected: PASS.
- [ ] **Step 5.8:** Commit: `git commit -m "feat(linux): implement LinuxFile (POSIX) with native unit tests"`.

## Task 6 (Slice 4): SinglePassSigScanner Linux port — TDD

**Files:**
- Modify: `deps/first/SinglePassSigScanner/include/SigScanner/SinglePassSigScanner.hpp`
- Modify: `deps/first/SinglePassSigScanner/src/SinglePassSigScanner.cpp`
- Create: `tests/SigScannerTests.cpp`; modify `tests/CMakeLists.txt`

Approach (minimal-diff, unlike PR #384's three-file split): keep one header + one source; introduce a platform section for module info and region enumeration. Windows code paths remain literally untouched inside `#ifdef _WIN32`.

- [ ] **Step 6.1:** Header changes. In `SinglePassSigScanner.hpp`, wrap the existing `WIN_MODULEINFO` struct and its `operator=(MODULEINFO)` in `#ifdef _WIN32`, and add the Linux equivalent (spec §3.4: `dl_iterate_phdr`-derived):

```cpp
#ifdef _WIN32
    // existing WIN_MODULEINFO — unchanged
    using OSModuleInfo = WIN_MODULEINFO;
#else
    struct LINUX_MODULEINFO
    {
        void* lpBaseOfDll{};            // keep Windows-era field names so UEPseudo call sites stay identical
        unsigned long SizeOfImage{};
        void* EntryPoint{};
        // Readable PT_LOAD segments (absolute start, size, ELF p_flags), sorted by start
        std::vector<std::tuple<uint8_t*, size_t, int>> readable_segments{};
    };
    using OSModuleInfo = LINUX_MODULEINFO;
#endif
```

and change `ScanTargetArray`'s element type to `OSModuleInfo` (verify the array's current declaration and keep its name/shape).
- [ ] **Step 6.2:** Add a public static population helper (Linux only) to `SigScannerStaticData`:

```cpp
#ifdef __linux__
    // Populates m_modules_info[MainExe] (and any ScanTarget whose name matches a loaded .so)
    // from dl_iterate_phdr. Returns number of modules recorded.
    static auto populate_modules_from_dl_iterate_phdr() -> size_t;
#endif
```

- [ ] **Step 6.3:** Write the failing test `tests/SigScannerTests.cpp`:

```cpp
#include <cstdio>
#include <cstring>

#include <SigScanner/SinglePassSigScanner.hpp>

using namespace RC;

// A unique, position-independent byte pattern placed in this executable's rodata.
// The 0xDE 0xC0 0xAD 0x0B 0x5E 0xA1 ... sequence must not occur elsewhere by chance.
__attribute__((used, section(".rodata"))) static const unsigned char g_needle[] =
        {0xDE, 0xC0, 0xAD, 0x0B, 0x5E, 0xA1, 0x77, 0x13, 0x37, 0x42, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45};

int main()
{
    if (SigScannerStaticData::populate_modules_from_dl_iterate_phdr() == 0)
    {
        fprintf(stderr, "FAILED: no modules enumerated\n");
        return 1;
    }

    SinglePassScanner::SignatureContainerMap containers;
    bool found = false;
    void* found_addr = nullptr;

    std::vector<SignatureContainer> sigs;
    sigs.emplace_back(SignatureContainer{
            {{"DE C0 AD 0B 5E A1 77 13 37 42 AB CD EF 01 23 45"}},
            [&](SignatureContainer& self) -> bool {
                found = true;
                found_addr = self.get_match_address();
                return true;
            },
            [](SignatureContainer&) {},
    });
    containers.emplace(ScanTarget::MainExe, std::move(sigs));

    SinglePassScanner::start_scan(containers);

    if (!found)
    {
        fprintf(stderr, "FAILED: signature not found in own executable\n");
        return 1;
    }
    if (found_addr != static_cast<const void*>(g_needle))
    {
        fprintf(stderr, "FAILED: match address %p != needle address %p\n", found_addr, static_cast<const void*>(g_needle));
        return 1;
    }
    printf("SigScannerTests: all passed\n");
    return 0;
}
```

(Verify the exact public types before finalizing: `SignatureContainerMap` name, `start_scan` signature, `get_match_address()` accessor — read the current header and use the real names. The needle-address equality assertion may need relaxing to "found within main executable range" if the scanner reports interior matches.) Add to `tests/CMakeLists.txt`:

```cmake
add_executable(SigScannerTests SigScannerTests.cpp)
target_link_libraries(SigScannerTests PRIVATE SinglePassSigScanner Helpers Profiler)
add_test(NAME SigScannerTests COMMAND SigScannerTests)
```

- [ ] **Step 6.4:** Run to verify failure: `ninja -C build_linux SigScannerTests` — expected compile/link failure (`populate_modules_from_dl_iterate_phdr` undefined; Windows-only code in the .cpp).
- [ ] **Step 6.5:** Port `SinglePassSigScanner.cpp`:
  1. Wrap Windows includes and the `VirtualQuery`/`GetSystemInfo` code paths in `#ifdef _WIN32`.
  2. Implement `populate_modules_from_dl_iterate_phdr()`:

```cpp
#ifdef __linux__
#include <link.h>
#include <elf.h>

namespace RC
{
    auto SigScannerStaticData::populate_modules_from_dl_iterate_phdr() -> size_t
    {
        struct CallbackState
        {
            size_t modules_recorded{};
        } state{};

        dl_iterate_phdr(
                [](struct dl_phdr_info* info, size_t, void* data) -> int {
                    auto* s = static_cast<CallbackState*>(data);

                    // Empty name at index 0 == the main executable.
                    const bool is_main_exe = (info->dlpi_name == nullptr || info->dlpi_name[0] == '\0');
                    if (!is_main_exe)
                    {
                        return 0; // ScanTarget name matching for UE modules lands with UEPseudo work (Plan 2)
                    }

                    auto& module_info = SigScannerStaticData::m_modules_info[ScanTarget::MainExe];
                    uint8_t* lowest = nullptr;
                    uint8_t* highest = nullptr;
                    for (int i = 0; i < info->dlpi_phnum; ++i)
                    {
                        const auto& phdr = info->dlpi_phdr[i];
                        if (phdr.p_type != PT_LOAD)
                        {
                            continue;
                        }
                        auto* seg_start = reinterpret_cast<uint8_t*>(info->dlpi_addr + phdr.p_vaddr);
                        auto seg_size = static_cast<size_t>(phdr.p_memsz);
                        if ((phdr.p_flags & PF_R) != 0)
                        {
                            module_info.readable_segments.emplace_back(seg_start, seg_size, static_cast<int>(phdr.p_flags));
                        }
                        if (!lowest || seg_start < lowest)
                        {
                            lowest = seg_start;
                        }
                        if (!highest || seg_start + seg_size > highest)
                        {
                            highest = seg_start + seg_size;
                        }
                    }
                    module_info.lpBaseOfDll = lowest;
                    module_info.SizeOfImage = static_cast<unsigned long>(highest - lowest);
                    std::sort(module_info.readable_segments.begin(), module_info.readable_segments.end());
                    s->modules_recorded++;
                    return 0;
                },
                &state);

        m_is_modular = false;
        return state.modules_recorded;
    }
} // namespace RC
#endif
```

  3. In the scan loop(s) (`scanner_work_thread` / `scanner_work_thread_stdfind`): replace the per-page `VirtualQuery` region discovery with, on Linux, iteration over `readable_segments` clamped to `[start_address, end_address)` — scan each readable segment contiguously (segments are always committed and readable; no guard pages to skip). Keep the inner byte-matching loops shared and untouched.
  4. `string_scan`: on Linux, `throw std::runtime_error{"[SinglePassSigScanner::string_scan] Not implemented on Linux"}` for now (mirrors PR #384; it's only used by a GUI feature).
- [ ] **Step 6.6:** Build + run: `ninja -C build_linux SigScannerTests && ctest --test-dir build_linux -R SigScannerTests --output-on-failure`. Expected: PASS.
- [ ] **Step 6.7:** Also re-run `ctest --test-dir build_linux --output-on-failure` (both suites green).
- [ ] **Step 6.8:** Commit: `git commit -m "feat(linux): port SinglePassSigScanner to dl_iterate_phdr segment enumeration"`.

## Task 7: Windows regression check + wrap-up

- [ ] **Step 7.1:** `git diff main...linux-port -- '*.cpp' '*.hpp' | grep -B2 -A2 '^-' | grep -v '^---'` — review every **removed** line: each must be either (a) inside a new `#ifdef _WIN32` wrapper with identical content, or (b) part of the verbatim file move. Anything else is a Windows behavior change — fix it.
- [ ] **Step 7.2:** If `xwin`/wine cross-toolchain is available locally (`command -v xwin`), run a Windows cross-configure as a smoke check: `cmake -B build_xwin --toolchain cmake/toolchains/xwin-clang-cl-toolchain.cmake -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 -G Ninja`. Otherwise skip — CI is the gate.
- [ ] **Step 7.3:** Push branch and confirm `cmake-ci.yml` matrix passes on GitHub (this repo's fork remote — do NOT push to upstream). If no push access/user hasn't asked to push, report status and stop at local verification.
- [ ] **Step 7.4:** Update `docs/superpowers/plans/` checkboxes; write Plan 2 (slices 5–6) as a separate document informed by what compiled cleanly.

---

## Self-review notes

- **Spec coverage:** §3.1 (build) → Tasks 3–4; §3.2 (platform refactor) → Tasks 1–2; §3.7 (File/output) → Task 5; §3.4 (scanner) → Task 6. §3.3/3.5/3.6/3.8, §4–5, §6.2–6.3, §7 CI are Plans 2–3 by design (documented above).
- **Known uncertainties called out inline:** exact `HandleTemplate` member names (Step 5.2), `SignatureContainer` accessor names (Step 6.3), CMake target names (Step 4.10), `clang.cmake` current contents (Step 3.3). Each step says "read first, adapt names" — the surrounding contract is fixed.
- **Type consistency:** `LinuxFile` header changes (Step 5.4) match the .cpp (Step 5.5): `m_fd:int`, `m_map_size:size_t`, POSIX `IdentifyingProperties`, `set_file(int)`/`get_file()->int`.
