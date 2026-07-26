# Linux Process-Scoped Preload Verification Report

**Date:** 2026-07-15

**Branch:** `linux-port`

**Status:** Process-scoped launcher change verified; the overall Linux port remains incomplete because the live `ACCEPT:LUA_BLUEPRINT_HOOK` marker is still missing.

## Change commits

- `57b3b43c` — design process-scoped preload activation
- `a84214d5` — implementation plan
- `f292a45c` — launcher target/original-environment contract
- `0d71a021` — Linux startup-policy helper and unit tests
- `d744d0b0` — constructor integration and process-tree test
- `df96884c` — Linux usage and troubleshooting documentation

## Automated verification

### CMake

Commands:

```bash
cmake --build build_linux --parallel 2
PALWORLD_SERVER_ROOT=/home/liu/RE-UE4SS/build_linux/palworld-server \
  ctest --test-dir build_linux --output-on-failure
```

Result: build completed successfully and all `29/29` tests passed in `11.55s`. This included the non-skipped pinned `PalworldSignatureTests`, `LinuxLauncherTests`, `LinuxStartupPolicyTests`, and `LinuxPreloadScopeTests`.

The focused preload regression set also passed `5/5`:

```text
LinuxPreloadProbe
LinuxFailSoftProbe
LinuxLauncherTests
LinuxStartupPolicyTests
LinuxPreloadScopeTests
```

`LinuxPreloadScopeTests` exercised a wrapper, designated host, and helper process. It verified one host initialization, exact original `LD_PRELOAD` restoration, removal of all private launcher variables, and no mapped UE4SS library in the helper. `LinuxFailSoftProbe` verified that marker-free raw preload retains legacy startup behavior.

### xmake and ELF linkage

Commands:

```bash
xmake build -j 2 -y UE4SS
ldd -r build_linux/Game__Shipping__Linux/lib/libUE4SS.so
ldd -r Binaries/Game__Shipping__Linux/UE4SS/libUE4SS.so
tests/RunLinuxLinkParityTests.sh \
  build_linux/Game__Shipping__Linux/lib/libUE4SS.so \
  Binaries/Game__Shipping__Linux/UE4SS/libUE4SS.so
```

Result: xmake completed successfully in `19.513s`; both `ldd -r` commands exited zero with no unresolved symbols; linked-library parity passed. Both artifacts require only the loader plus `libgcc_s.so.1`, `libm.so.6`, and `libc.so.6`.

Artifact hashes:

```text
097f25948451cd580e3695cd5367782298e314cab14e0455efe2987b3d9e3a8c  CMake libUE4SS.so
acc67cbed9779afd06847943865e287da20b1b698dc54f91c54fde230ff8ecac  xmake libUE4SS.so
a05788ead7619db22a1509c43241c16d289ed7040e8bcbb2e36e13e223e822f9  PalServer-Linux-Shipping
```

## Live Palworld verification

The focused run used unused game/query ports `8341` and `27031`:

```bash
PALWORLD_SERVER_ROOT=/home/liu/RE-UE4SS/build_linux/palworld-server \
ACCEPTANCE_STAGE_DIR=/home/liu/RE-UE4SS/build_linux/palworld-process-scope-live-8341 \
READY_TIMEOUT_SECONDS=180 \
MARKER_TIMEOUT_SECONDS=60 \
DUMPER_TIMEOUT_SECONDS=60 \
UE4SS_ACCEPTANCE_DUMPER_DELAY_MS=600000 \
tests/palworld/run_acceptance.sh \
  -port=8341 -queryport=27031 \
  -useperfthreads -NoAsyncLoadingThread -UseMultithreadForDS
```

Observed required startup evidence:

```text
DIAG: executable=/home/liu/RE-UE4SS/build_linux/palworld-server/Pal/Binaries/Linux/PalServer-Linux-Shipping
DIAG: engine_version=5.1
ACCEPT:LUA_MOD_STARTED
ACCEPT:LUA_CONSOLE_REGISTERED
ACCEPT:CPP_START_MOD
ACCEPT:CPP_PROGRAM_START
ACCEPT:CPP_UNREAL_INIT
ACCEPT:CPP_UPDATE count=10
Running Palworld dedicated server on :8341
```

Process-scope checks:

- `UE4SS.log` contained exactly one `DIAG: executable=` line, for `PalServer-Linux-Shipping`.
- Neither `UE4SS.log` nor `palworld-server.log` contained `crashpad_handler`, `DIAG: inactive_reason=PS scan timed out`, or `Fatal Error: PS scan timed out`.
- The repository-local crashpad process started normally and exited during PID-scoped harness cleanup without producing UE4SS startup output.
- The port-8341 server and its repository-local crashpad were absent after cleanup.
- Protected unrelated PID `527131` on port `8211` was observed before, during, and after the run with the same command line and was never targeted by cleanup.

Retained evidence:

- `build_linux/palworld-process-scope-live-8341/UE4SS.log`
- `build_linux/palworld-process-scope-live-8341/palworld-server.log`

## Remaining acceptance gap

The harness crossed the previous crashpad failure boundary and stopped at the first remaining marker:

```text
timed out waiting for marker: ACCEPT:LUA_BLUEPRINT_HOOK
```

The dependent Lua property, FName, `StaticConstructObject`, object-dump, and USMAP markers therefore remain pending. This report verifies the process-scoped preload change only and does not declare all eight Palworld acceptance gates complete.
