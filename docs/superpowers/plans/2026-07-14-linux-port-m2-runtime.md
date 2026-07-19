# UE4SS Linux Port — Plan 2/3: Runtime and Headless Shared Library

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a native headless `libUE4SS.so` that loads safely into an ELF host, initializes UEPseudo, installs Linux-compatible inline hooks, watches mod directories, reports crashes, and loads Linux C++ mods without changing Windows behavior.

**Architecture:** GUI code is a CMake-selected optional subsystem; core headers expose GUI APIs only when `UE4SS_HAS_GUI` is defined. Windows-only crash dumping and IAT interception stay in their existing code paths, while Linux uses `sigaction`, `dlopen`/`dlsym`, and ELF module discovery. UEPseudo owns Unreal layouts and detours; UE4SS owns bootstrap, mod loading, fail-soft startup, and diagnostics.

**Tech Stack:** CMake/Ninja, clang C++23, ELF/glibc (`dlopen`, `dl_iterate_phdr`, `sigaction`, `inotify`), PolyHook2 x64 detours, ctest.

---

## Task 0: Preserve and commit the proven UEPseudo portability work

**Files:**
- Modify: `deps/first/Unreal/CMakeLists.txt`
- Modify: `deps/first/Unreal/include/Unreal/BPMacros.hpp`
- Modify: `deps/first/Unreal/include/Unreal/UnrealFlags.hpp`
- Modify: `deps/first/Unreal/include/Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtualBaseVC.hpp`
- Create: `deps/first/Unreal/include/Unreal/Core/{Clang,Linux,Unix}/**`
- Modify: `deps/first/Unreal/src/{FMemory,NameTypes,UAssetRegistry,UObject,UnrealInitializer,UnrealVersion}.cpp`
- Modify: `deps/first/Unreal/src/VersionedContainer/Container.cpp`
- Modify: `deps/first/Helpers/include/Helpers/Format.hpp`
- Modify: `deps/first/Helpers/src/Casting.cpp`

- [ ] **Step 0.1:** Confirm `deps/first/Unreal` is on its local `linux-port` branch and review every harvested header against `pr-79-linux`; retain only Linux/Unix/Clang platform definitions needed by the current UEPseudo revision.
- [ ] **Step 0.2:** Run `ninja -C build_linux -j1 Unreal`. Expected: `libUnreal.a` links with exit 0.
- [ ] **Step 0.3:** Run `ctest --test-dir build_linux --output-on-failure`. Expected: `FileTests` and `SigScannerTests` pass.
- [ ] **Step 0.4:** Commit inside the submodule with `git -C deps/first/Unreal add -A && git -C deps/first/Unreal commit -m "feat(linux): compile UEPseudo with ELF platform support"`, including the new platform headers in the same commit.
- [ ] **Step 0.5:** Commit the root helper changes and submodule pointer with `git commit -m "fix(linux): add portable helpers and UEPseudo runtime support"`.

## Task 1: Make headless mode a real compile boundary

**Files:**
- Create: `tests/HeadlessHeadersTests.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `UE4SS/CMakeLists.txt`
- Modify: `UE4SS/include/UE4SSProgram.hpp`
- Modify: `UE4SS/include/SettingsManager.hpp`
- Modify: `UE4SS/include/Mod/CppUserModBase.hpp`
- Modify: `UE4SS/src/UE4SSProgram.cpp`
- Modify: `UE4SS/src/SettingsManager.cpp`
- Modify: `UE4SS/src/Mod/CppUserModBase.cpp`
- Modify: `UE4SS/src/Mod/Mod.cpp`
- Modify: `UE4SS/src/Mod/LuaMod.cpp`

- [ ] **Step 1.1:** Add `HeadlessHeadersTests.cpp` that includes `UE4SSProgram.hpp`, `SettingsManager.hpp`, and `Mod/CppUserModBase.hpp`, constructs no objects, and returns 0. Link it to the same non-GUI public dependencies as `UE4SS`, without imgui/TextEditor include directories.
- [ ] **Step 1.2:** Build with `ninja -C build_linux HeadlessHeadersTests`. Expected RED: compilation fails through `GUI/Console.hpp` or missing imgui/TextEditor headers.
- [ ] **Step 1.3:** In `UE4SS/CMakeLists.txt`, filter `src/GUI/**` out of `UE4SS_SOURCES` when `UE4SS_GUI=OFF`. Define `UE4SS_HAS_GUI=1` only when the GUI dependency block is enabled.
- [ ] **Step 1.4:** Guard GUI includes, members, rendering methods, ImGui helper macros, settings fields, parsing branches, and GUI-only C++ mod APIs with `#if defined(UE4SS_HAS_GUI)`. Keep their Windows definitions byte-for-byte inside the guard.
- [ ] **Step 1.5:** Remove unconditional `GUI/Dumpers.hpp` includes from `Mod.cpp` and `LuaMod.cpp`; object/USMAP/CXX dump functionality remains in its non-GUI namespaces and GUI menu registration stays guarded.
- [ ] **Step 1.6:** On headless builds, configure only `Output::NewFileDevice` and `Output::DebugConsoleDevice`; do not instantiate `GUI::ConsoleDevice` or start a render thread. `fire_ui_init_for_cpp_mods()` becomes a no-op call site on headless builds.
- [ ] **Step 1.7:** Rebuild `HeadlessHeadersTests`. Expected GREEN: exit 0 without imgui or TextEditor include paths.
- [ ] **Step 1.8:** Run `ninja -C build_linux -k 20 UE4SS` and record only the next non-GUI error group before proceeding.
- [ ] **Step 1.9:** Commit with `git commit -m "build(linux): enforce headless UE4SS compile boundary"`.

## Task 2: Normalize Unreal character types on Linux

**Files:**
- Modify: `UE4SS/include/LuaType/LuaUObject.hpp`
- Modify: `UE4SS/include/LuaType/LuaUnrealString.hpp`
- Modify: `UE4SS/src/LuaLibrary.cpp`
- Modify: `UE4SS/src/LuaType/{LuaFOutputDevice,LuaUObject,LuaXDelegateProperty,LuaXProperty}.cpp`
- Modify: `UE4SS/src/Mod/LuaMod.cpp`
- Modify: any non-GUI SDK/dumper source reported by the same compile pass

- [ ] **Step 2.1:** Use the current `UE4SS` build as RED and save the first error for each distinct type mismatch (`TCHAR`, wide literal, mixed `wchar_t`/`char16_t`).
- [ ] **Step 2.2:** Qualify engine character types as `Unreal::TCHAR`; use `STR("...")` for Unreal/FName text; use `ensure_str`, `FromCharTypePtr<Unreal::TCHAR>`, and `ToCharTypePtr` at explicit boundaries. Do not introduce Linux-only casts that change Windows data width.
- [ ] **Step 2.3:** Rebuild after each mismatch class. A class is GREEN only when its original diagnostic disappears and no replacement conversion warning is emitted.
- [ ] **Step 2.4:** Run `rg -n '\\bTCHAR\\b' UE4SS --glob '*.{hpp,cpp}'`; every remaining unqualified occurrence must be inside a Windows-only GUI/proxy source excluded from Linux or be changed to `Unreal::TCHAR`.
- [ ] **Step 2.5:** Commit with `git commit -m "fix(linux): use Unreal character types across Lua bindings"`.

## Task 3: Replace the obsolete Linux filesystem watcher using TDD

**Files:**
- Create: `tests/FilesystemWatcherTests.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `UE4SS/include/FilesystemWatcher_Linux.cpp_impl`
- Modify: `UE4SS/include/FilesystemWatcher.hpp` only if an owning file-descriptor field is required

- [ ] **Step 3.1:** Add a test that creates two temporary directories, calls `add_dir` for both, registers a `FilesystemWatch` for `Example`, starts polling, creates `libExample.so` in the second directory, and waits on a condition variable for a callback containing the created path and `match_all == false`.
- [ ] **Step 3.2:** Add a second test that watches `*`, creates `AnyName.so`, expects `match_all == true`, then destroys a never-started watcher and a started watcher. Both destructors must return without throwing or joining a non-joinable thread.
- [ ] **Step 3.3:** Run `ninja -C build_linux FilesystemWatcherTests`. Expected RED: the outdated implementation fails to compile because it references `m_path`, old constructors, and a one-argument callback.
- [ ] **Step 3.4:** Implement one nonblocking inotify descriptor owned by `m_handle`; `add_dir` adds `IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE` watches and stores watch-descriptor-to-path state in `m_handles`/parallel owned state. `poll` reads all pending events, reconstructs the full path, strips optional `lib` and `.so` only for matching, applies the debounce, and calls `notify(full_path, watch.name == "*")`.
- [ ] **Step 3.5:** Move construction transfers descriptor/thread/stop state and invalidates the source. Destruction requests stop, joins only when joinable, and closes every valid descriptor. Errors log and return; they never terminate the host process.
- [ ] **Step 3.6:** Run `ctest --test-dir build_linux -R FilesystemWatcherTests --output-on-failure`. Expected GREEN.
- [ ] **Step 3.7:** Commit with `git commit -m "feat(linux): implement inotify filesystem watcher"`.

## Task 4: Add a POSIX crash reporter without altering Win32 dumps

**Files:**
- Move: `UE4SS/src/CrashDumper.cpp` -> `UE4SS/src/Platform/Win32/CrashDumper.cpp`
- Create: `UE4SS/src/Platform/Linux/CrashDumper.cpp`
- Modify: `UE4SS/include/CrashDumper.hpp`
- Create: `tests/CrashDumperProbe.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `UE4SS/CMakeLists.txt`

- [ ] **Step 4.1:** Move the Windows implementation without content edits and verify a 100% rename with `git diff --find-renames`.
- [ ] **Step 4.2:** Add `<cstdint>` to the header. Keep the public constructor/destructor/`enable`/`set_full_memory_dump` API; place IAT-hook members behind `_WIN32` and Linux signal state behind `__linux__`.
- [ ] **Step 4.3:** Add `CrashDumperProbe`, which sets `UE4SS_CRASH_LOG_DIR` to a temporary directory, enables `CrashDumper`, raises `SIGABRT`, and is registered as a `WILL_FAIL` ctest followed by a verifier that checks `crash_*.log` contains the signal number and at least one frame address.
- [ ] **Step 4.4:** Run the probe. Expected RED: no Linux implementation/link symbols.
- [ ] **Step 4.5:** Implement Linux `enable()` with `sigaction` for `SIGSEGV`, `SIGABRT`, `SIGILL`, `SIGFPE`, and `SIGBUS`. Pre-open the crash file during `enable`; the handler uses `write`, `backtrace`, and `backtrace_symbols_fd`, restores the previous handler, then re-raises the signal. `set_full_memory_dump` logs that full minidumps are unsupported on Linux M1.
- [ ] **Step 4.6:** Run the crash probe and verifier. Expected GREEN. Then run `ctest --test-dir build_linux --output-on-failure`.
- [ ] **Step 4.7:** Commit with `git commit -m "feat(linux): add signal-based crash reporting"`.

## Task 5: Link and audit the native shared object

**Files:**
- Modify: files reported by `ninja -C build_linux -k 20 UE4SS`
- Modify: `UE4SS/CMakeLists.txt`

- [ ] **Step 5.1:** Repeatedly build `UE4SS`, fixing one root-cause error class per commit: platform includes, Windows APIs outside guards, character-width mismatch, then unresolved symbols.
- [ ] **Step 5.2:** Keep `PolyHook_2`, `dl`, `pthread`, `Unreal`, Lua, dumpers, and generators linked on Linux; keep d3d11/glfw/glad/opengl32/dbghelp/psapi/ws2_32/ntdll/userenv Windows-only.
- [ ] **Step 5.3:** Run `ninja -C build_linux -j1 UE4SS`. Expected: `libUE4SS.so` links.
- [ ] **Step 5.4:** Run `ldd -r build_linux/UE4SS/libUE4SS.so` (use the actual target path from Ninja). Expected: no `undefined symbol` diagnostics.
- [ ] **Step 5.5:** Run `readelf -d` and confirm `NEEDED` contains only intended runtime libraries and no GUI libraries in headless mode.
- [ ] **Step 5.6:** Commit with `git commit -m "feat(linux): link headless libUE4SS shared library"`.

## Task 6: Validate Linux detours and shared-object bootstrap

**Files:**
- Modify: `UE4SS/src/Platform/Linux/EntryLinux.cpp`
- Modify: `UE4SS/src/UE4SSProgram.cpp`
- Modify: `UE4SS/include/UE4SSProgram.hpp`
- Modify: `deps/first/Unreal/src/Hooks.cpp` and its hook abstraction if build/runtime evidence requires it
- Create: `tests/LinuxPreloadProbe.cpp`
- Create: `tests/LinuxHookTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 6.1:** Add `LinuxHookTests`: detour a noinline local function with `PLH::x64Detour`, assert the replacement runs and the trampoline returns the original value, unhook, and assert original behavior returns.
- [ ] **Step 6.2:** Run the hook test before platform changes. If RED, capture whether failure is allocation, relocation, memory protection, or instruction decoding; fix the UEPseudo hook backend at that boundary. Switch to funchook only if PolyHook cannot pass this minimal ELF test.
- [ ] **Step 6.3:** Add `LinuxPreloadProbe`, a tiny executable that prints a ready marker, `dlopen`s a fixture `.so`, and exits. Its ctest launches with `LD_PRELOAD=<libUE4SS.so>` and `UE4SS_DISABLE_AUTO_START=1`; it must reach the ready marker and exit 0.
- [ ] **Step 6.4:** In `EntryLinux.cpp`, honor `UE4SS_DISABLE_AUTO_START=1`, use an atomic once guard, resolve the library path with `dladdr`, start a detached init thread, and never throw across the ELF constructor boundary.
- [ ] **Step 6.5:** Keep Windows `LoadLibrary` IAT hooks behind `_WIN32`. On Linux, export a `dlopen` interposer that resolves the real function with `dlsym(RTLD_NEXT, "dlopen")`, uses a thread-local recursion guard, and calls `fire_dll_load_for_cpp_mods` after successful loads when `UE4SSProgram::s_program` exists.
- [ ] **Step 6.6:** Run `LinuxHookTests` and `LinuxPreloadProbe`. Expected GREEN.
- [ ] **Step 6.7:** Commit with `git commit -m "feat(linux): validate ELF detours and preload bootstrap"`.

## Task 7: Load Linux C++ mods and fail soft

**Files:**
- Modify: `UE4SS/src/Mod/CppMod.cpp`
- Modify: `UE4SS/include/Mod/CppMod.hpp`
- Create: `tests/fixtures/CppModProbe/**`
- Create: `tests/CppModLoaderTests.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `UE4SS/src/Platform/Linux/EntryLinux.cpp`

- [ ] **Step 7.1:** Build a fixture C++ mod as `libCppModProbe.so` exporting the same start/uninstall symbols as Windows C++ mods and recording lifecycle callbacks to a temporary file.
- [ ] **Step 7.2:** Add a loader test that points `CppMod` at the fixture, loads it, invokes program/unreal/update lifecycle callbacks, unloads it, and verifies the record.
- [ ] **Step 7.3:** Run the test. Expected RED at Windows `LoadLibrary`/`.dll` assumptions.
- [ ] **Step 7.4:** Add a platform library-handle wrapper: Windows retains `LoadLibraryW/GetProcAddress/FreeLibrary`; Linux uses `dlopen(..., RTLD_NOW | RTLD_LOCAL)`, `dlsym`, and `dlclose`. Accept `.so` and `lib<Name>.so` while preserving `.dll` behavior on Windows.
- [ ] **Step 7.5:** Wrap the Linux init thread in a top-level exception boundary. Required scan/version/hook failures log to stderr and `UE4SS.log`, set an inactive state, and return without terminating the host.
- [ ] **Step 7.6:** Run the C++ mod loader test and a preload probe with a deliberately invalid signature override. Both host processes must exit normally.
- [ ] **Step 7.7:** Run the complete local suite and commit with `git commit -m "feat(linux): load C++ mods with dlopen and fail soft"`.

## Task 8: Runtime verification gate

- [ ] **Step 8.1:** Fresh configure: `cmake -S . -B build_linux_verify -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Game__Shipping__Linux -DUE4SS_GUI=OFF -DUE4SS_BUILD_TESTS=ON`.
- [ ] **Step 8.2:** Build: `ninja -C build_linux_verify -j1 UE4SS FileTests SigScannerTests HeadlessHeadersTests FilesystemWatcherTests LinuxHookTests LinuxPreloadProbe CppModLoaderTests`.
- [ ] **Step 8.3:** Test: `ctest --test-dir build_linux_verify --output-on-failure`. Expected: zero failures.
- [ ] **Step 8.4:** Audit removed Windows lines and platform guards; do not claim Windows-green without CI.
