# Native Linux bootstrap

This directory contains the upstreamable, headless Linux-x86_64 foundation for
UE4SS. It currently provides:

- a minimal `LD_PRELOAD` shim and versioned C ABI;
- an isolated core with atomic status and logging;
- ELF module, segment, build-id, and SHA-256 inspection;
- current patternsleuth ELF resolvers with per-capability validation;
- explicit UTF-8/Unreal UTF-16 conversion and safe mod path resolution;
- a Zydis-backed x86_64 inline detour backend;
- a native UEPseudo Linux platform layer and broad reflection source gate;
- fail-closed binding of validated resolver addresses to UEPseudo state;
- read-only GUObjectArray layout validation with a bounded post-main monitor;
- an isolated, protected Lua 5.4 headless VM;
- live `FindFirstOf`, `FindAllOf`, `FindObjects`, and `StaticFindObject` bindings;
- reflected scalar, string, text, enum, object/interface, delegate, container,
  weak/soft-object property access and game-thread UFunction calls;
- `RegisterHook`/`UnregisterHook` for native and Blueprint UFunctions, including
  scoped `RemoteUnrealParam:get()`/`set()` parameters and return overrides;
- `NotifyOnNewObject`, BeginPlay hooks, `LoadAsset`, `ExecuteInGameThread`,
  `RunOnGameThread`, `ExecuteWithDelay`, and `LoopAsync`;
- isolated per-mod Lua loading after every resolver and ABI gate passes;
- fail-open and fail-closed integration tests and packaging.

The shipping artifact is built on Ubuntu 22.04 with Clang 18 and statically
linked libc++; this keeps its C++ runtime local to the core. Local preset builds
therefore require the libc++ development headers, or may explicitly use
`-DUE4SS_USE_LIBCXX=OFF` for platform-only development tests.

The private UEPseudo submodule requires a GitHub account linked to Epic Games.
Its Linux platform types and portable reflection sources now have a native
Clang compile gate covering `FName`, `UObject`, `UFunction`, `FProperty`, the
object registry, and versioned containers. Validated resolver addresses are
bound to dependency-light UEPseudo state, and GUObjectArray is checked with
read-only process reads after Unreal initializes it. The read-only UObject and
Lua discovery layers have been validated against the pinned Palworld
v1.0.1.100619 Linux server (Steam build 24181105, ELF build-id
`7f7e167407984ec3`). Reflected property access, UFunction calls, scheduling,
object notifications, and hooks remain fail-closed.

## Local build

```bash
git submodule update --init deps/first/patternsleuth deps/first/Unreal
cmake --preset linux-release
cmake --build --preset linux-release --parallel
ctest --preset linux-release
cmake --build build/linux-release --target package
```

For a glibc 2.35 artifact built with Clang 18 and Rust 1.88.0:

```bash
linux/docker/build-package.sh
```

Automated builders also need credentials for the private UEPseudo submodule.
The GitHub identity behind that token must be linked to Epic Games and accepted
into Epic's GitHub organization, exactly like a local Unreal Engine checkout.

Run an arbitrary ELF through the identity probe:

```bash
build/linux-release/linux/ue4ss_probe --json /path/to/PalServer-Linux-Shipping
```

Run a process through the bootstrap:

```bash
UE4SS_ROOT="$PWD/build/linux-release/linux" \
UE4SS_STATE_DIR="$PWD/build/ue4ss-state" \
LD_PRELOAD="$PWD/build/linux-release/linux/libue4ss_preload.so" \
/path/to/application
```

Set `UE4SS_SERVER_SHA256` to pin the target executable and
`UE4SS_EXPECTED_SHA256` to pin `libUE4SS.so`. Unknown or mismatched identities
never receive hooks. `UE4SS_REGISTRY_WAIT_SECONDS` controls the bounded
post-main object-registry readiness check (default 120, accepted range 1-600).
After the registry first becomes plausible,
`UE4SS_REGISTRY_SETTLE_SECONDS` requires its element count to remain unchanged
before any Unreal function is called (default 3, accepted range 1-30).

## Proprietary PalServer conformance

The server binary is never copied into this repository. Given a local server
checkout, the conformance runner creates a separate sandbox, explicitly omits
`Pal/Saved`, enables the bundled read-only test mod, pins the copied executable
by SHA-256, and verifies the status document and mod output:

```bash
linux/tools/palworld_conformance.sh /path/to/palworld-server-root
```

An unchanged Lua mod can be added to the same save-free sandbox. A failure to
load either mod fails the run closed:

```bash
UE4SS_EXTRA_MOD_DIR=/path/to/MyMod \
  linux/tools/palworld_conformance.sh /path/to/palworld-server-root
```

Multiple unchanged mods can be tested together with a colon-separated list:

```bash
UE4SS_EXTRA_MOD_DIRS=/path/to/FirstMod:/path/to/SecondMod \
  linux/tools/palworld_conformance.sh /path/to/palworld-server-root
```

After the ready gate, the runner keeps PalServer alive for two additional
seconds and fails on protected-load or Lua callback errors. Override this
bounded window with `UE4SS_CONFORMANCE_SETTLE_SECONDS` (accepted range 0-60).

The unchanged FullSphereSummon 0.10.2 Lua script passes this load gate and its
native `AActor::BeginPlay` pre-hook is registered through the UE 5.1 Linux
Itanium vtable path. The conformance mod separately spawns and destroys a
transient actor and proves both BeginPlay pre- and post-dispatch. Its actual
sphere-throw feature still depends on
local player input, aiming UI, and client animation state, so running that mod
only on a dedicated server cannot provide its client-visible behaviour to an
unmodified client. This is a limitation of that mod's design, not of Lua mod
loading on Linux.

## Experimental Palworld image

Build a local image without placing the proprietary server in this repository:

```bash
linux/docker/build-palworld-image.sh \
  /path/to/palworld-server-root \
  /path/to/UE4SS-Linux-x86_64-<git-sha>.tar.gz \
  palworld-ue4ss-linux:experimental
```

UE4SS is off by default. Enable it with `UE4SS_ENABLED=true`; use
`UE4SS_REQUIRED=true` for fail-closed test runs. The image pins both the
PalServer executable and `libUE4SS.so` by SHA-256. Its healthcheck requires the
Lua, scheduler, native/Blueprint/BeginPlay hook, and object-notification capabilities unless
`UE4SS_HEALTH_REQUIRE_READY=false` is set. Mount these paths separately:

- `/palworld/Pal/Saved` for saves and backups;
- `/palworld/Pal/Binaries/Linux/ue4ss/Mods` for writable mods;
- `/palworld/ue4ss-state` for status and logs.

The remaining UE4SS package files are immutable in the image layer.

## Docker soak release gate

Run the experimental image with its save, mod, and state bind mounts, then
observe an already-running container for 24 hours before publishing an
experimental artifact:

```bash
UE4SS_SOAK_DURATION=24h \
UE4SS_SOAK_SAVE_ROOT=/host/palworld-saved \
  linux/tools/docker_soak.sh palworld-ue4ss soak-results/24h
```

The monitor requires both Docker health and the native UE4SS capability
healthcheck, rejects container restarts and sustained thread growth, records
RSS/save metrics as CSV, and scans the complete log for fatal or hook-retirement
errors. For the production-recommendation gate, run it for 72 hours and make
the monitor perform the final `SIGINT` shutdown plus a post-stop save manifest:

```bash
UE4SS_SOAK_DURATION=72h \
UE4SS_SOAK_STOP_ON_SUCCESS=true \
UE4SS_SOAK_SAVE_ROOT=/host/palworld-saved \
  linux/tools/docker_soak.sh palworld-ue4ss soak-results/72h
```

Exit codes 0, 130, and 143 are accepted after an intentional Docker stop, but
the UE4SS core shutdown log is still mandatory. Thresholds and short smoke
durations are configurable through the variables documented by
`linux/tools/docker_soak.sh`.
