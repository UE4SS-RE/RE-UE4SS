# Native Linux support

UE4SS Milestone 1 supports native x86-64 Linux builds for headless Unreal Engine 5.1+ games and dedicated servers. Palworld's native Linux dedicated server is the binding target. Windows builds and the existing Linux-to-Windows cross-compilation workflows remain unchanged.

## Requirements

- x86-64 Linux
- glibc 2.35 or newer
- libstdc++ exposing `GLIBCXX_3.4.32` or newer
- clang with C++23 support; GCC is not a supported compiler for this port
- CMake 3.22 or newer and Ninja, or xmake 2.9.3
- Rust 1.73 or newer

Ubuntu 24.04 or a similarly current distribution is recommended. Check a host before deploying:

```bash
getconf GNU_LIBC_VERSION
strings "$(clang++ -print-file-name=libstdc++.so.6)" \
  | grep -E '^GLIBCXX_[0-9.]+$' \
  | sort -V \
  | tail -n 1
```

The second command must print `GLIBCXX_3.4.32` or a later version. A binary built on a newer distribution can acquire newer ABI requirements, so release artifacts should be produced in the project's baseline build environment.

Clone all submodules before configuring:

```bash
git submodule update --init --recursive
```

## Build with CMake

The Linux target is headless by default:

```bash
cmake -S . -B build_linux -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Linux \
  -DUE4SS_GUI=OFF \
  -DUE4SS_BUILD_TESTS=ON

cmake --build build_linux --parallel
ctest --test-dir build_linux --output-on-failure
ldd -r build_linux/Game__Shipping__Linux/lib/libUE4SS.so
```

`PalworldSignatureTests` reports a skip unless `PALWORLD_SERVER_ROOT` points to a locally installed Palworld dedicated server. The server binary cannot be redistributed in public CI.

## Build with xmake

The repository pins xmake 2.9.3 and selects clang for native Linux builds:

```bash
xmake f -p linux -a x86_64 -m Game__Shipping__Linux -y
xmake build -j "$(nproc)" -y UE4SS
ldd -r Binaries/Game__Shipping__Linux/UE4SS/libUE4SS.so
```

The CMake and xmake Linux targets both omit Windows-only proxy generation, UVTD, GUI sources, and Windows system libraries. They link the native loader and threading libraries instead.

## Stage a dedicated server

UE4SS resolves its settings, mods, log, and dumper output relative to `libUE4SS.so`. Place the launcher and runtime files together. For a Palworld server installed at `$PALWORLD_SERVER_ROOT`:

```bash
stage="$PALWORLD_SERVER_ROOT/Pal/Binaries/Linux"

install -m 755 build_linux/Game__Shipping__Linux/lib/libUE4SS.so "$stage/libUE4SS.so"
install -m 755 tools/linux/run_ue4ss.sh "$stage/run_ue4ss.sh"
install -m 644 assets/UE4SS-settings.ini "$stage/UE4SS-settings.ini"
install -d "$stage/Mods"
cp -a assets/Mods/. "$stage/Mods/"
```

Add game-specific mods or settings under the same staging directory. Do not overwrite an existing server configuration without backing it up.

## Launch

Run the server through the supplied launcher instead of setting `LD_PRELOAD` by hand:

```bash
export PALWORLD_SERVER_ROOT=/srv/palworld
stage="$PALWORLD_SERVER_ROOT/Pal/Binaries/Linux"
mkdir -p "$stage/UE4SS-crashes"

cd "$PALWORLD_SERVER_ROOT"
UE4SS_CRASH_LOG_DIR="$stage/UE4SS-crashes" \
  "$stage/run_ue4ss.sh" ./PalServer.sh \
  -useperfthreads -NoAsyncLoadingThread -UseMultithreadForDS
```

`run_ue4ss.sh` validates both paths, preserves every server argument, prepends UE4SS to an existing `LD_PRELOAD`, and then replaces itself with the server process. It also supports installation paths containing spaces.

## Diagnostics and logs

Set `UE4SS_DIAGNOSE=1` for a support-oriented startup report:

```bash
cd "$PALWORLD_SERVER_ROOT"
UE4SS_DIAGNOSE=1 \
UE4SS_CRASH_LOG_DIR="$stage/UE4SS-crashes" \
  "$stage/run_ue4ss.sh" ./PalServer.sh
```

The diagnostic report includes the executable path and SHA-256, loaded ELF ranges, glibc version, highest detected GLIBCXX version, engine detection, per-signature status/address, and the reason UE4SS deactivated if startup failed. Share the diagnostic block together with:

- `$stage/UE4SS.log`
- the Palworld Steam build ID from `steamapps/appmanifest_2394010.acf`
- the server executable SHA-256
- the relevant `$stage/UE4SS-crashes/crash_<pid>.log`, if present

With `[CrashDump] EnableDumping = 1`, Linux installs signal handlers for `SIGSEGV`, `SIGABRT`, `SIGILL`, `SIGFPE`, and `SIGBUS`. Linux crash files contain the signal number and a native backtrace. Windows-style minidumps and full-memory dumps are not available.

If a required engine version or signature cannot be resolved, UE4SS is designed to log the failure and deactivate without terminating the host server. Diagnose mode makes that inactive reason explicit.

## Milestone 1 limitations

- Headless Unreal Engine 5.1+ and dedicated-server use only
- x86-64 only; ARM64 is not supported
- ImGui GUI and live GUI tools are compiled out
- Keyboard and mouse hooks are unavailable; headless input is queue-based
- UVTD and PDB-based tooling are unavailable
- No ptrace/attach injector; native loading uses `LD_PRELOAD`
- Linux crash handling produces signal backtraces, not minidumps

Object dumping, CXX header generation, USMAP generation, Lua mods, C++ `.so` mods, and console/file logging are part of the M1 target, but the port is not considered complete until the live Palworld acceptance suite passes all eight gates documented in the approved Linux-port design.
