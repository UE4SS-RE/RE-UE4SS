# Palworld Linux Acceptance Report

**Date:** 2026-07-15

**Branch:** `linux-port`

**Status:** All local Linux automated acceptance work is green. The overall port remains incomplete until the user performs the client building-placement check, the 30-minute gameplay/save/memory soak, and the fail-soft client join.

## Pinned artifacts

- Palworld dedicated server Steam app: `2394010`
- Steam build ID: `24088465`
- Server SHA-256: `a05788ead7619db22a1509c43241c16d289ed7040e8bcbb2e36e13e223e822f9`
- Clean CMake `libUE4SS.so` SHA-256: `4d6b0c619980e92951ea121f0af3e476b4f6ff4a87bc07c8ee337ace791821cf`
- Clean xmake `libUE4SS.so` SHA-256: `b98f7b7adb6b79536ee560e03178de9ae364e85db1f19dec5ddd670535e18f1a`

## Gate summary

| Gate | Automated result | Evidence |
|---|---|---|
| 1. Load and startup performance | Pass | Three alternating baseline/preload trials; `0.085%` median regression against a `10%` limit |
| 2. Engine and signature detection | Pass | UE `5.1`; every required runtime resolver found; optional `FUObjectHashTablesGet` miss handled |
| 3. Native hooks | Pass | FName, `StaticConstructObject`, `ProcessEvent`, Blueprint, and configured server-hook activity observed |
| 4. Lua behavior | Pass | Console registration, Blueprint hook, object search, live property read/write/restore, FName, and object construction markers observed |
| 5. C++ mod lifecycle | Pass | Start, program-start, Unreal-init, and update-count markers observed |
| 6. Dumpers | Pass | Object dump, CXX header generation, and USMAP completed and contain `PalPlayerCharacter` |
| 7. Client soak/save/memory | Manual pending | Requires a client and at least 30 minutes of play |
| 8. Broken required signature | Automated pass; client join pending | UE4SS deactivated before engine-state access while Palworld reached ready state and stayed healthy |

## Automated build and regression verification

Commands:

```bash
cmake -S . -B /tmp/ue4ss-build-linux-verify -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Linux \
  -DUE4SS_GUI=OFF \
  -DUE4SS_BUILD_TESTS=ON
cmake --build /tmp/ue4ss-build-linux-verify --parallel 2
PALWORLD_SERVER_ROOT="$PWD/build_linux/palworld-server" \
  ctest --test-dir /tmp/ue4ss-build-linux-verify --output-on-failure
xmake f -c -p linux -a x86_64 -m Game__Shipping__Linux -y
xmake build --rebuild -j 2 -y UE4SS
ldd -r /tmp/ue4ss-build-linux-verify/Game__Shipping__Linux/lib/libUE4SS.so
ldd -r Binaries/Game__Shipping__Linux/UE4SS/libUE4SS.so
tests/RunLinuxLinkParityTests.sh \
  /tmp/ue4ss-build-linux-verify/Game__Shipping__Linux/lib/libUE4SS.so \
  Binaries/Game__Shipping__Linux/UE4SS/libUE4SS.so
```

Results:

- The from-empty-directory CMake build completed all `465` Ninja actions.
- All `32` CTests passed in `11.42 seconds`; `PalworldSignatureTests` ran against the pinned server binary and passed.
- The clean xmake reconfiguration and forced rebuild completed in `30.823 seconds`.
- Both `ldd -r` checks exited successfully with no unresolved symbols.
- CMake/xmake `DT_NEEDED` parity passed.

## Startup-performance gate

Three alternating direct/preloaded trials used separate game ports under `/tmp/ue4ss-startup-baseline`:

- Baseline median: `3544 ms`
- UE4SS median: `3547 ms`
- Regression: `(3547 - 3544) / 3544 = 0.085%`
- Limit: `10%`

The result passes with `9.915` percentage points of headroom.

## Final full live acceptance

The final run used the clean CMake artifact and game/query ports `8356` and `27046`. Its report is retained at `/tmp/ue4ss-palworld-acceptance-8356/acceptance-report.txt`.

Observed runtime evidence included:

```text
DIAG: engine_version=5.1
DIAG: signature=GUObjectArray status=resolved address=0xc11d878
DIAG: signature=FNameToString status=resolved address=0x7941e40
DIAG: signature=FNameCtorWchar status=resolved address=0x79409c0
DIAG: signature=GMalloc status=resolved address=0xc0482a8
DIAG: signature=StaticConstructObjectInternal status=resolved address=0x7b851c0
DIAG: signature=FTextFString status=resolved address=0x781f080
DIAG: signature=GNatives status=resolved address=0xc11c0a0
DIAG: signature=ConsoleManagerSingleton status=resolved address=0x77ba9d0
DIAG: signature=UGameEngineTick status=resolved address=0xa3928b0
ACCEPT:LUA_BLUEPRINT_HOOK
ACCEPT:LUA_FIND_ALL count=3014
ACCEPT:LUA_PROPERTY_READ value=true
ACCEPT:LUA_PROPERTY_WRITE value=false
ACCEPT:LUA_PROPERTY_RESTORED value=true
ACCEPT:LUA_FNAME value=UE4SSAcceptanceProbeObject
ACCEPT:LUA_STATIC_CONSTRUCT_OBJECT
ACCEPT:CPP_PROGRAM_START
ACCEPT:CPP_UNREAL_INIT
ACCEPT:CPP_UPDATE count=10
```

Dumper outputs:

- CXX headers: `2,250` `.hpp` files, `17 MiB`
- Object dump: `73,120,957` bytes
- USMAP: `2,310,396` bytes
- All three outputs contain `PalPlayerCharacter`.
- Startup time: `4 seconds`.
- The generated UE4SS crash log was empty.
- The test process and ports were absent after PID-scoped cleanup.

## Fail-soft root cause and correction

The first live broken-signature run on game port `8352` exited with `SIGSEGV`. The staged `GUObjectArray.lua` deliberately returned an impossible signature and `SecondsToScanBeforeGivingUp` was `0`.

The native patternsleuth loop always performs its first scan before evaluating the timeout. The Lua-override loop instead used a preconditioned `while (elapsed < timeout)`. At a zero-second timeout it performed zero attempts, left a default failed result with no fatal errors, marked scanning complete, and allowed `Initialize()` to reach `UObjectArray::GetNumElements()` with a null `GUObjectArray`.

The correction changes the override retry loop to `do/while`. Required overrides now receive one mandatory attempt before the timeout is evaluated, matching the native scanner. A fatal missing resolver throws out of Unreal initialization, and the existing Linux entry path reports UE4SS inactive without terminating the host.

TDD evidence:

- `LinuxRequiredScanFailSoftTests` wraps the native scanner, runs the real `ScanGame()` second-pass override path, and supplies a fatal missing `GUObjectArray` result.
- RED: before the correction, the executable exited `1` because the override was attempted zero times and no exception was raised.
- GREEN: after the correction, CTest passed; the override ran exactly once, the fatal message propagated, and `bScanFullyCompleted` remained false.

## Final live fail-soft verification

The final corrected run used the clean CMake artifact and game/query ports `8357` and `27047`, with evidence under `/tmp/ue4ss-palworld-failsoft-final-8357`.

Observed results:

```text
Lua Scan attempt 1 (Phase 2)
Was unable to find AOB for 'GUObjectArray' via Lua script
DIAG: inactive_reason=AOB scans could not be completed because of the following reasons:
Fatal Error: AOB scans could not be completed because of the following reasons:
Running Palworld dedicated server on :8357
```

- Ready time: `3 seconds`
- Health window after ready: `30 seconds`
- RSS at ready: `1,033,036 KiB`
- RSS after 30 seconds: `1,102,748 KiB`
- Required override attempts: `1`
- The `Fatal Error` line is the caught required-scan exception that deactivates UE4SS; it did not terminate the host.
- No `Verifying FName constructor` or `Waiting for object construction` line followed the failure.
- No signal-11, segmentation-fault, or Unreal crash-handler marker appeared.
- The generated UE4SS crash-log file was empty.
- The test process and ports were absent after PID-scoped cleanup.
- Protected production Palworld PID `527131` and unrelated Steam PID `3801614` were not signaled or modified.

## M1 design-to-evidence audit

| Approved-design requirement | Implementation | Verification status |
|---|---|---|
| Native x86-64 ELF/glibc, headless, clang-only build | Linux platform definitions in CMake/xmake; GUI compile boundary in `UE4SS/CMakeLists.txt`; requirements in `README.md` and `docs/linux.md` | Clean CMake and xmake builds passed; both shared objects pass `ldd -r` |
| Linux entry, launcher, and process-scoped preload | `EntryLinux.cpp`, `StartupPolicy.cpp`, and `tools/linux/run_ue4ss.sh` | Launcher, startup-policy, preload-scope, helper-process, and live Palworld checks passed |
| ELF discovery and signature scanning | `SinglePassSigScanner` uses `dl_iterate_phdr`; patternsleuth supports ELF; Palworld resolvers are pinned to the server build | Synthetic ELF tests, real-binary `PalworldSignatureTests`, diagnostics, and live resolver evidence passed |
| Linux hooks and UE 5.1 layouts | PolyHook2's Linux path was validated instead of adding funchook; UEPseudo carries Linux/Unix/Clang support plus the Palworld LoadMap ABI and vtable corrections | Hook, layout, LoadMap ABI, FName, and live hook activity checks passed |
| POSIX file I/O and console/file logging | `LinuxFile`, DynamicOutput file/console devices, and headless setup | `FileTests` passed; live runs produced `UE4SS.log` and console diagnostics |
| Lua mod support | Linux-safe character boundaries, Lua discovery, and the Palworld Lua probe | Console command, Blueprint hook, object search, property write/restore, FName, and construction markers passed |
| C++ mod support | Linux shared-library wrapper and `.so` discovery/loading through `dlopen` | Loader tests and Palworld lifecycle/update probe passed |
| Object, CXX, and USMAP generators | Portable output paths and Linux generator path regression coverage | Final live run produced all three outputs with known Palworld classes |
| Headless M1 degradations | GUI excluded, queue input selected, signal-based crash reporter used in place of Win32 minidumps | Headless-header, crash-probe, and live empty-crash-log checks passed |
| Fail-soft diagnostics | Top-level Linux exception boundary, `UE4SS_DIAGNOSE`, and mandatory first Lua override attempt | Unit fail-soft tests and the final 30-second live broken-signature run passed |
| Linux CI/build documentation | `.github/workflows/linux-test.yml`, `README.md`, and `docs/linux.md` | Local equivalents passed; Windows CI exists but was not run locally, so this report does not claim Windows-green |
| Palworld M1 acceptance suite | `tests/palworld/run_acceptance.sh` plus pinned build/hash evidence | Gates 1-6 are green; Gate 7 and the client half of Gate 8 remain manual |
| From-source UE 5.1 demo server | The approved design identifies this as a contributor debugging aid, not the M1 acceptance gate | Not exercised here; Epic EULA constraints keep it out of public CI and this Palworld acceptance record |

## Remaining manual acceptance

The user will complete the final client-side work after the automated implementation:

1. Join the fully active UE4SS server and verify normal building placement behavior.
2. Play for at least 30 minutes, exercise ordinary world interaction, and trigger/observe a successful save.
3. Record server RSS at join, during play, and after save; confirm growth is bounded and the server remains responsive.
4. Join a fail-soft server with the deliberately broken required signature and confirm the client can connect while UE4SS remains inactive.

The Linux port must not be declared complete until these manual checks are recorded.
