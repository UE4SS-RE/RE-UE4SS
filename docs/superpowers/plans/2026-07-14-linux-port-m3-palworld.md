# UE4SS Linux Port — Plan 3/3: Palworld Signatures, Distribution, and Acceptance

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Resolve UE 5.1/Palworld ELF symbols, package a reproducible Linux launch flow, add Linux CI/xmake parity, and pass the approved Palworld dedicated-server acceptance suite.

**Architecture:** Runtime-independent ELF identification lives in patternsleuth/UEPseudo; game-build-specific AOBs live in the signature database with build fingerprints. A shell launcher owns `LD_PRELOAD` and diagnostics. A contributor-side harness installs the anonymous Palworld server, stages probe mods, runs acceptance checks, and records the pinned Steam build ID.

**Tech Stack:** patternsleuth Rust bindings, UE4SS signature tables, steamcmd, Bash, CMake/xmake, GitHub Actions, Palworld Linux dedicated server (app 2394010).

---

## Task 1: Prove ELF engine-version detection

**Files:**
- Modify: `deps/first/patternsleuth/**`
- Modify: `deps/first/patternsleuth_bind/**`
- Create: `tests/PatternSleuthElfTests.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1.1:** Add a synthetic ELF fixture containing version strings and code/data sections representative of UE 5.1, and assert the binding reports major 5, minor 1 and resolves a known fixture pattern.
- [ ] **Step 1.2:** Run the test and capture the first PE-only assumption as RED.
- [ ] **Step 1.3:** Teach patternsleuth to enumerate ELF `PT_LOAD` segments and symbols without changing PE parsing. Return structured failure when build metadata is absent.
- [ ] **Step 1.4:** Run the ELF test plus existing Rust tests. Commit the patternsleuth submodule first, then its root pointer.

## Task 2: Build the Palworld signature set

**Files:**
- Modify: `UE4SS/src/Signatures.cpp`
- Modify: `UE4SS/include/Signatures.hpp`
- Modify: `assets/CustomGameConfigs/Palworld/**`
- Create: `tools/linux/extract-palworld-signature-report.sh`

- [ ] **Step 2.1:** Install the anonymous dedicated server into a local ignored directory with `steamcmd +login anonymous +app_update 2394010 validate +quit`; record the app manifest build ID and SHA-256 of `PalServer-Linux-Shipping`.
- [ ] **Step 2.2:** Run UE4SS diagnose mode against the pinned binary and save every unresolved required symbol with nearby disassembly and segment metadata.
- [ ] **Step 2.3:** For each required symbol (`GUObjectArray`, FName construction/conversion, `StaticConstructObject`, `ProcessEvent`, engine tick and configured hooks), derive the narrowest stable ELF AOB, add it under Linux/UE 5.1/Palworld guards, and re-run until diagnose reports zero required misses.
- [ ] **Step 2.4:** Add an offline signature regression test that scans the pinned local binary when `PALWORLD_SERVER_ROOT` is set; skip with an explicit message in public CI where the binary cannot be redistributed.
- [ ] **Step 2.5:** Commit signatures with the Steam build ID and executable hash in the commit body.

## Task 3: Add diagnose and launcher support

**Files:**
- Create: `tools/linux/run_ue4ss.sh`
- Modify: `UE4SS/src/UE4SSProgram.cpp`
- Modify: `UE4SS/src/Platform/Linux/EntryLinux.cpp`
- Modify: `assets/UE4SS-settings.ini`

- [ ] **Step 3.1:** Add `UE4SS_DIAGNOSE=1` output for executable hash, ELF module ranges, glibc version, libstdc++ GLIBCXX ceiling, engine version, signature result/address, and inactive reason.
- [ ] **Step 3.2:** Implement `run_ue4ss.sh` with `set -euo pipefail`; resolve its directory, validate `libUE4SS.so` and target executable, prepend rather than overwrite `LD_PRELOAD`, preserve arguments, and `exec` the server command.
- [ ] **Step 3.3:** Add shell tests for paths containing spaces, an existing `LD_PRELOAD`, missing library, and argument preservation.
- [ ] **Step 3.4:** Commit with `git commit -m "feat(linux): add diagnostic launcher workflow"`.

## Task 4: Add Linux build parity and documentation

**Files:**
- Modify: `xmake.lua`
- Modify: `tools/xmakescripts/rules/build_rules.lua`
- Modify: related xmake project files selected by a native Linux configure
- Create: `.github/workflows/linux-test.yml`
- Modify: `README.md`
- Create: `docs/linux.md`

- [ ] **Step 4.1:** Add Linux to the xmake platform/mode matrix with `PLATFORM_LINUX`, `PLATFORM_UNIX`, `OVERRIDE_PLATFORM_HEADER_NAME=Linux`, `UBT_COMPILED_PLATFORM=Linux`, clang, and no Unicode/GUI defaults.
- [ ] **Step 4.2:** Configure and build the headless target with xmake; compare linked libraries and compile definitions to CMake.
- [ ] **Step 4.3:** Add GitHub Actions Ubuntu clang configuration, build `UE4SS`, and run all redistributable ctests. Keep Windows workflow unchanged.
- [ ] **Step 4.4:** Document glibc >= 2.35, GLIBCXX >= 3.4.32, clang, build commands, server staging, launcher use, diagnostics, known M1 degradations, and crash logs.
- [ ] **Step 4.5:** Commit with `git commit -m "ci(linux): add native build, docs, and xmake parity"`.

## Task 5: Stage acceptance probe mods

**Files:**
- Create: `tests/palworld/LuaProbeMod/**`
- Create: `tests/palworld/CppProbeMod/**`
- Create: `tests/palworld/run_acceptance.sh`

- [ ] **Step 5.1:** Lua probe registers a console command, hooks one Blueprint function, calls `FindAllOf`, reads/writes a live actor property, and logs stable `ACCEPT:` markers.
- [ ] **Step 5.2:** C++ probe builds as `.so`, logs `on_program_start`, `on_unreal_init`, and at least ten `on_update` callbacks.
- [ ] **Step 5.3:** Harness stages both mods, starts the server through `run_ue4ss.sh`, waits for the game-server-ready marker with a bounded timeout, and fails on crash, missing signature, or missing lifecycle marker.
- [ ] **Step 5.4:** Harness invokes object dump and USMAP generation and verifies non-empty output containing `PalPlayerCharacter`.

## Task 6: Run the binding Palworld acceptance suite

- [ ] **Step 6.1 Load:** Server reaches its ready marker with UE4SS preloaded; measure startup against a no-preload baseline and require <=10% regression.
- [ ] **Step 6.2 Detect:** `UE4SS.log` exists, engine version is 5.1, executable build ID/hash match the pinned record, and diagnose reports zero required signature misses.
- [ ] **Step 6.3 Hooks:** Probe markers prove `StaticConstructObject`, `ProcessEvent`, and FName activity during world load.
- [ ] **Step 6.4 Lua mods:** All Lua probe operations pass and one representative Palworld community Lua mod starts unmodified.
- [ ] **Step 6.5 C++ mods:** The `.so` probe loads and records program, Unreal-init, and update lifecycle callbacks.
- [ ] **Step 6.6 Dumpers:** Object dump and USMAP generation finish and contain known Palworld classes.
- [ ] **Step 6.7 Soak:** A client joins and plays for at least 30 minutes; server remains alive, saves successfully, and resident-memory growth is recorded and bounded.
- [ ] **Step 6.8 Fail-soft:** Run with a deliberately broken required signature; the server still reaches ready state, UE4SS logs deactivation, and a client can join.
- [ ] **Step 6.9:** Save the acceptance report under `docs/superpowers/verification/` with commands, build IDs, hashes, timings, log excerpts, and pass/fail for all eight gates.

## Task 7: Completion audit

- [ ] **Step 7.1:** Fresh CMake and xmake builds pass on Linux; all local tests pass.
- [ ] **Step 7.2:** Re-read the approved design line by line and map every M1 requirement to code plus verification evidence.
- [ ] **Step 7.3:** Run the Superpowers verification-before-completion checklist. Do not declare the port complete unless Task 6 is fully green.

