# UE4SS Native Linux Port — Design

**Date:** 2026-07-14
**Status:** Approved design, pre-implementation
**Target:** Upstream contribution to [UE4SS-RE/RE-UE4SS](https://github.com/UE4SS-RE/RE-UE4SS) and [Re-UE4SS/UEPseudo](https://github.com/Re-UE4SS/UEPseudo)
**Primary use case:** UE4SS running inside the **native Palworld Linux dedicated server** (`PalServer-Linux-Shipping`, UE 5.1). Palworld is the driving demand behind Linux support (issues #807, #364) and is this port's definition of success: M1 is done when UE4SS loads, hooks, and runs mods in a production Palworld Linux server.

## Prior and existing work this design builds on

| Work | Status | What it contributes |
|---|---|---|
| [RE-UE4SS PR #384 "[Draft] Linux Porting"](https://github.com/UE4SS-RE/RE-UE4SS/pull/384) (Yangff) | Open draft, stale vs. main (~Sep 2025) | Reference map for the whole port: `Platform.hpp`, `EntryLinux.cpp`/`EntryWin32.cpp` split, LD_PRELOAD + `dl_iterate_phdr` loading, CMake/xmake changes, `linux-test.yml` CI, TUI backend prototype, catalog of MSVC-isms to fix |
| [UEPseudo PR #79 "Linux port"](https://github.com/Re-UE4SS/UEPseudo/pull/79) (Yangff) | Open draft, stale | Linux member-variable layouts for UE 5.1 (`generated_include/Platform/Linux/FunctionBodies/5_01_*`), UNIX headers, one-header-per-version policy |
| [PR #565 fmt lib](https://github.com/UE4SS-RE/RE-UE4SS/pull/565), [PR #574 StringType typedef](https://github.com/UE4SS-RE/RE-UE4SS/pull/574), [UEPseudo #96](https://github.com/Re-UE4SS/UEPseudo/pull/96) (closing [issue #566](https://github.com/UE4SS-RE/RE-UE4SS/issues/566)) | **Merged** | Platform-agnostic strings: `CharType = char16_t` on Linux (`deps/first/String/include/String/StringType.hpp`), fmt-based formatting that works without wide-char libc support |
| [PR #749 InputSource](https://github.com/UE4SS-RE/RE-UE4SS/pull/749) (replaced [#558](https://github.com/UE4SS-RE/RE-UE4SS/pull/558)) | **Merged** | `PlatformInputSource` abstraction with Win32Async / GLFW3 / Queue backends (`deps/first/Input/`) |
| [Issue #364](https://github.com/UE4SS-RE/RE-UE4SS/issues/364) | Open, active (June–July 2026) | Tracking thread. Maintainer position (2026-06-24): "We still want to do it, but we have very few maintainers, and even fewer that do Linux stuff." March 2025: next step is reviewing #384 and UEPseudo #79 |
| [Issue #807](https://github.com/UE4SS-RE/RE-UE4SS/issues/807) | Closed (dup of #364) | Confirms dedicated-server demand (Palworld) as the primary Linux use case |
| Existing cross-compile toolchains (`cmake/toolchains/{xwin,wine}-*`, README §"Building from Linux") | Merged | Linux **hosts** already build Windows DLLs; proves most non-Win32 code compiles under clang |
| Existing seams in-tree | Merged | `LinuxFile.hpp` stub (`deps/first/File`), `RC_OS_FILE_TYPE_INCLUDE_FILE` selector, `SEH_TRY/SEH_EXCEPT` no-op fallbacks (`UE4SS/include/ExceptionHandling.hpp`), `GLFW3_OpenGL3 + Backend_NoOS` GUI path (`UE4SS/src/GUI/GUI.cpp`), GLFW/glad/zydis/fmt/imgui deps already cross-platform |

## 1. Goals and non-goals

**Goal.** Native Linux (x86-64, ELF, glibc) support for UE4SS, delivered as a series of small upstreamable PRs, **with the Palworld Linux dedicated server as the binding target**. Milestone 1 is **headless**: dedicated servers and `-nullrhi` games, with Lua mods, C++ mods, object dumper, CXX header generator, USMAP generator, and console/file logging working. **UE 5.1+ only** — 5.1 is the first version Epic ships prebuilt Linux binaries for (rationale from PR #384), and it is Palworld's engine version. Every slice's acceptance ultimately rolls up to the Palworld acceptance suite in §6.3.

**Non-goals for M1** (deferred, not precluded): ImGui GUI, keyboard/mouse input hooks, minidump-style crash dumps, UVTD on Linux, ptrace/attach injection, ARM64, UE < 5.1. The GLFW+OpenGL GUI backend and PR #384's TUI prototype are M2 follow-ups.

**Decisions taken** (with alternatives considered):
- **Upstream-first**, not a long-lived fork — matches maintainer intent on #364.
- **Headless/server-first** — matches the actual demand (#807, Palworld) and avoids the most Windows-coupled subsystems.
- **Re-implement in slices, harvest #384/#79 as reference** — rejected wholesale rebase: both drafts are ~a year stale and main has since absorbed their string/input prerequisites; a 100-file monolith is unreviewable for a maintainer team this small.
- **LD_PRELOAD + inline detours** — rejected ptrace injector (fragile, no existing work, unneeded for servers) and Proton-bridge-only (not a port; useless for native ELF servers).

## 2. Architecture overview

```
game/server process (ELF, UE 5.1+)
 └─ LD_PRELOAD=libUE4SS.so
     ├─ EntryLinux.cpp: __attribute__((constructor)) → init thread
     ├─ Platform layer (Platform.hpp): module enum, mem protection, paths, threads
     ├─ SinglePassSigScanner: dl_iterate_phdr + /proc/self/maps
     ├─ patternsleuth (ELF support): engine version + core symbol resolution
     ├─ funchook: inline detours via mprotect (new third-party dep)
     ├─ UEPseudo: Linux member layouts (from UEPseudo #79)
     └─ portable core (unchanged): Lua layer, SDK generator, dumpers, DynamicOutput
```

Everything from `UE4SSProgram` initialization onward is already OS-agnostic or was made so by the merged string/input work. The port surface is what runs **before** that: build, load, discover, scan, hook.

## 3. Components

### 3.1 Build system
- xmake: lift `set_allowedplats("windows")` / add `Linux` platform to `tools/xmakescripts/rules/build_rules.lua` mode matrix (defines `PLATFORM_LINUX`, `UBT_COMPILED_PLATFORM=Linux`; no `UNICODE`).
- CMake: make `ASM_MASM` and `proxy_generator` conditional on `WIN32`; add a native Linux preset.
- Toolchain: **clang only** (matches Epic's Linux toolchain; codebase carries MSVC-isms GCC rejects). Pin minimum glibc/libstdc++ and document them (a community member on #364 already offered exactly this doc: GLIBC ≥ 2.35, GLIBCXX ≥ 3.4.32).
- Harvest: PR #384's CMake/xmake diffs, `.cargo/config.toml`, `linux-test.yml`.

### 3.2 Platform layer (pure refactor, PR slice 1)
`UE4SS/include/Platform.hpp` + `UE4SS/src/Platform/Win32/EntryWin32.cpp` and `UE4SS/src/Platform/Linux/EntryLinux.cpp` (names from #384). The current `DllMain`/`CreateToolhelp32Snapshot`/`QueueUserAPC` bootstrap in `UE4SS/src/main_ue4ss_rewritten.cpp` moves verbatim into EntryWin32. Windows behavior must be byte-identical; this slice is reviewable standalone.

### 3.3 Loader / bootstrap (Linux)
- Shared-object constructor spawns the init thread; it polls briefly for the engine image to be mapped (approach validated in #384) rather than hijacking the main thread.
- The Win32 `PLH::IatHook` interception of `LoadLibraryA/W/ExA/ExW` (`UE4SS/src/UE4SSProgram.cpp:318-345`) is replaced on Linux by a `dlopen` interposer exported from the preloaded .so (LD_PRELOAD symbol interposition — no GOT patching needed).
- Deliverable includes a `run_ue4ss.sh` launcher template (server operators on #364 currently share pastebin scripts for this).

### 3.4 Signature scanning
- `SinglePassSigScanner`: replace `VirtualQuery`/`MODULEINFO` enumeration with `dl_iterate_phdr` (executable `PT_LOAD` segments) + `/proc/self/maps` fallback.
- `patternsleuth`: verify/extend ELF image support for engine-version detection and AOB resolution.
- Linux-specific AOBs added to `Signatures.cpp`, tagged per-platform. **Highest-uncertainty component:** clang-built ELF code needs its own signature set; plan assumes iteration per engine version.

### 3.5 Hooking
- Add **funchook** (BSD, x86-64 Linux+Windows) behind a thin `PlatformHook` wrapper in UEPseudo. PolyHook stays as the Windows backend initially — zero Windows-behavior change.
- `mprotect` replaces `VirtualProtect` in all patching paths.
- Fix the MSVC member-function-pointer / fixed-enum-forward-declaration issues #384 catalogued (e.g. `MemberVariableLayout_HeaderWrapper_ULocalPlayer.hpp`).

### 3.6 UEPseudo Linux layouts
- Harvest UEPseudo #79: `generated_include/Platform/Linux/FunctionBodies/5_01_*`, UNIX headers.
- Keep **one header per version** across platforms; missing Linux offsets must fail the build loudly, not fork headers silently (Yangff's stated design concern on #79).
- Layout generation for further versions: a small **DWARF dumper** run against Epic's prebuilt Linux editor/server binaries — the ELF answer to the PDB-based UVTD. M1 ships with #79's UE 5.1 data (sufficient for both validation targets in §6, including Palworld, which is UE 5.1); the tool is productionized in M2.

### 3.7 File I/O and output
- Implement `LinuxFile` (`deps/first/File/include/File/FileType/LinuxFile.hpp` is an explicit "not implemented" stub) with POSIX open/read/write/mmap/rename behind the existing CRTP `FileInterface`. This unblocks the whole `Output::*` logging system on Linux.
- Console output device: stdout/stderr with ANSI color, replacing the Win32 console host (matches #384's "stderr for output" choice, done properly through DynamicOutput).

### 3.8 Degraded subsystems on Linux (M1)
- GUI: compiled out (`Backend_NoOS`/GLFW path reserved for M2).
- Input: only `QueueInputSource` registered (no headless equivalent of `GetAsyncKeyState`); GLFW3 source activates with the M2 GUI.
- CrashDumper: POSIX `SIGSEGV`/`SIGABRT` handler writing backtrace + active-mod list to the log, instead of dbghelp minidumps.
- All degradations live behind the platform layer, not `#ifdef`s sprinkled through callers.

## 4. Startup data flow (Linux)

`LD_PRELOAD` → constructor → init thread waits for engine image → patternsleuth/AOB scan resolves engine version, `GUObjectArray`, `FName` ctor, `StaticConstructObject`, etc. → funchook detours installed → `UE4SSProgram` init (settings, mods, Lua VM, C++ mods via `dlopen`) → normal event loop. Identical to Windows from `UE4SSProgram` onward.

## 5. Error handling

- **Fail soft and loud:** if version detection or a required AOB fails, log to stderr + `UE4SS.log` and deactivate without crashing the host process — a broken UE4SS install must not take down a production game server.
- Signal handler logs backtrace and active mods on crash.
- `UE4SS_DIAGNOSE=1` env var dumps scan results, glibc/libstdc++ versions, and engine hash for bug reports — pre-empting the support burden maintainers cite as a porting cost.

## 6. Testing and CI

### 6.1 Unit (GitHub Actions)
Existing test targets built and run natively on Linux (File, String, Input, sig-scanner against synthetic ELF fixtures). Harvest #384's `linux-test.yml`.

### 6.2 Integration ground truth (contributor-side)
From-source UE 5.1 `LinuxServer` demo-project build, driven by a documented contributor-side script (UE EULA prevents public-CI redistribution). Used to debug with full symbols/source truth when Palworld (a stripped shipping binary) misbehaves — not as the acceptance gate.

### 6.3 Palworld acceptance suite (the M1 gate)
Environment: a pinned Palworld dedicated-server build installed via `steamcmd` (app 2394010) on a glibc ≥ 2.35 distro; UE4SS loaded via `LD_PRELOAD` through the `run_ue4ss.sh` launcher wrapping `PalServer.sh` (replacing the hand-rolled pastebin scripts circulating on #364). The suite is a scripted checklist, re-run on every slice that touches runtime behavior and on every Palworld server update:

1. **Load:** server boots to "game server started" with UE4SS preloaded; no crash, no startup-time regression > 10%.
2. **Detect:** `UE4SS.log` created via `LinuxFile`; engine version detected as 5.1; all required AOBs resolve (diagnose mode reports zero missing signatures).
3. **Hooks:** core detours fire — `StaticConstructObject`, `ProcessEvent`, `FName` ctor — verified by a probe Lua mod logging object construction during world load.
4. **Lua mods:** a test mod registers a console command, hooks a BP function, iterates `FindAllOf`, and reads/writes a UProperty on a live actor; a representative popular Palworld Lua mod from Nexus runs unmodified (mod-API compatibility check).
5. **C++ mods:** a test `.so` C++ mod loads via `dlopen`, receives lifecycle callbacks (`on_update`, `on_unreal_init`).
6. **Dumpers:** object dump and USMAP generation complete on the live server and produce plausible output (spot-checked against known Palworld classes, e.g. `PalPlayerCharacter`).
7. **Gameplay soak:** a client joins the server, plays ≥ 30 minutes (spawns pals, saves world); no crash, no save corruption, memory growth bounded.
8. **Fail-soft:** with signatures deliberately broken, the server still boots and serves players; UE4SS logs the failure and deactivates (§5 requirement, critical for production servers).

### 6.4 Invariants
Windows CI green on every slice; slices 1–2 produce byte-identical Windows behavior. Palworld server updates can shift AOBs — the suite pins a server build ID per release and re-validates on updates.

## 7. Upstreaming strategy

1. **Coordinate first:** post this design on issue #364, credit and consult @Yangff (#384's author), confirm slicing with maintainers before code lands.
2. **~8 slice PRs**, each independently green on Windows:
   1. Platform-layer refactor (EntryWin32/EntryLinux split; no behavior change)
   2. Build-system Linux target (compiles a stub .so)
   3. `LinuxFile` + output devices
   4. Sig-scanner port (`dl_iterate_phdr`) + patternsleuth ELF work
   5. funchook + `PlatformHook` in UEPseudo; harvest #79 layouts (supersedes #79)
   6. Linux entry/loader + `dlopen` interposer
   7. Linux signatures + engine-version detection for UE 5.1+ (Palworld's binary is the first signature target)
   8. CI workflow, docs (build + Palworld server setup + glibc requirements), `run_ue4ss.sh`
3. Close #384/#79 with credit once superseded — Yangff explicitly framed them as insight-providers for future attempts.

## 8. Milestones

- **M1 (this design):** headless native Linux, UE 5.1+, Lua + C++ mods, dumpers, logging. **Acceptance gate: the Palworld suite in §6.3 passes end-to-end**; the from-source UE demo server (§6.2) is the debugging aid, not the gate.
- **M2 (enabled, not planned here):** TUI console (from #384), GLFW+OpenGL GUI + GLFW input, DWARF layout-dumper productionized.
- **M3:** wider UE version coverage, ARM64, attach-style injection if ever needed.
