# Unreal Engine 4/5 Scripting System

Lua scripting system platform, C++ Modding API, SDK generator, blueprint mod loader, live property editor and other dumping utilities for UE4/5 games.

## Major features

- [Lua Scripting API](https://docs.ue4ss.com/dev/lua-api.html): Write lua mods based on the UE object system
- [Blueprint Modloading](https://docs.ue4ss.com/dev/feature-overview/blueprint-modloader.html): Spawn blueprint mods automatically without editing/replacing game files
- [C++ Modding API](https://docs.ue4ss.com/dev/guides/creating-a-c%2B%2B-mod.html): Write C++ mods based on the UE object system
- [Live Property Viewer and Editor](https://docs.ue4ss.com/dev/feature-overview/live-view.html): Search, view, edit & watch the properties of every loaded object, great for debugging mods or figuring out how values are changed during runtime
- [UHT Dumper](https://docs.ue4ss.com/dev/feature-overview/dumpers.html#unreal-header-tool-uht-dumper): Generate Unreal Header Tool compatible C++ headers for creating a mirror .uproject for your game
- [C++ Header Dumper](https://docs.ue4ss.com/dev/feature-overview/dumpers.html#c-header-generator): Generate standard C++ headers from reflected classes and blueprints, with offsets
- [Universal UE Mods](https://docs.ue4ss.com/dev/feature-overview/universal-mods.html): Unlock the game console and other universal mods
- [Dumpers for File Parsing](https://docs.ue4ss.com/dev/feature-overview/dumpers.html#usmap-dumper): Generate `.usmap` mapping files for unversioned properties
- [UMAP Recreation Dumper](https://docs.ue4ss.com/dev/feature-overview/dumpers.html#umap-recreation-dumper): Dump all loaded actors to file to generate `.umaps` in-editor
- Other Features, including [Experimental](https://docs.ue4ss.com/dev/feature-overview/experimental.html) features at times

## Targeting UE Versions: From 4.12 To 5.7

The goal of UE4SS is not to be a plug-n-play solution that always works with every game.
The goal is to have an underlying system that works for most games.
You may need to update AOBs on your own, and there's a guide for that below.

## Basic Installation

The easiest installation is via downloading the non-dev version of the latest non-experimental build from [Releases](https://github.com/UE4SS-RE/RE-UE4SS/releases/latest) and extracting the zip content to `{game directory}/GameName/Binaries/Win64/`.

If your game is in the custom config list, extract the contents from the relevant folder to `Win64` as well.

If you are planning on doing mod development using UE4SS, you can do the same as above but download the zDEV version instead.

### Command Line Options

If RE-UE4SS is installed via proxy DLL, the following command line options are available:

- `--disable-ue4ss` - Temporarily disable UE4SS without uninstalling by launching the game with this argument.
- `--ue4ss-path <path>` - Specify a custom path to UE4SS.dll. Supports both absolute paths (e.g., `C:\custom\UE4SS.dll`) and relative paths (e.g., `dev\builds\UE4SS.dll` relative to the game executable directory). Useful for testing different UE4SS builds without modifying installation files. 

### Environment Variables

RE-UE4SS supports the following environment variables:

- `UE4SS_MODS_PATHS` - Semicolon-separated list of additional mods directories to load. Paths are processed in reverse order (first entry has highest priority), similar to the `PATH` variable. Example: `C:\SharedMods;D:\GameMods;E:\TestMods`.

## Links

  [Full installation guide](https://docs.ue4ss.com/dev/installation-guide.html)

  [Fixing compatibility problems](https://docs.ue4ss.com/dev/guides/fixing-compatibility-problems.html)

  [Lua API - Overview](https://docs.ue4ss.com/dev/lua-api.html)

  [Generating UHT compatible headers](https://docs.ue4ss.com/dev/guides/generating-uht-compatible-headers.html)

  [Custom Game Configs](https://docs.ue4ss.com/dev/custom-game-configs.html)

  [Creating Compatible Blueprint Mods](https://www.youtube.com/watch?v=fB3yT85XhVA)

  [UE4SS Discord Server Invite](https://discord.gg/7qhRGHF9Tt)

  [Unreal Engine Modding Discord Server Invite](https://discord.gg/unreal-engine-modding-876613187204685934)

## Build requirements

- A computer running Windows.
  - Linux support might happen at some point but not soon.
- A version of MSVC that supports C++23:
  - MSVC toolset version >= 14.39.0
  - MSVC version >= 19.39.0
  - Visual Studio version >= 17.9
  - More compilers will hopefully be supported in the future.
- [Rust toolchain >= 1.73.0](https://www.rust-lang.org/tools/install)
- [CMake >= 3.22](https://cmake.org/download/)
- A build system: either [Ninja](https://ninja-build.org/) or MSVC (included with Visual Studio)

## Build instructions

1. Clone the repo.
2. Execute this command: `git submodule update --init --recursive`
    Make sure your Github account is linked to your Epic Games account for UE source access.
    Do not use the `--remote` option because that will force third-party dependencies to update to the latest commit, and that can break things.
    You will need your github account to be linked to an Epic games account to pull the Unreal pseudo code submodule.

There are several different ways you can build UE4SS.

## Building from CLI

### Build Modes

The build modes are structured as follows: `<Target>__<Config>__<Platform>`

Currently supported options for these are:

* `Target`
  * `Game` - for regular games on UE versions greater than UE 4.21
  * `LessEqual421` - for regular games on UE versions less than or equal to UE 4.21
  * `CasePreserving` - for games built with case preserving enabled

* `Config`
  * `Dev` - development build
  * `Debug` - debug build
  * `Shipping` - shipping(release) build
  * `Test` - build for tests

* `Platform`
  * `Win64` - 64-bit windows

### Basic Build Commands

To build UE4SS with CMake, use the following commands:

```bash
# Configure with Ninja (recommended for faster builds, single-configuration)
cmake -B build_cmake_Game__Shipping__Win64 -G Ninja -DCMAKE_BUILD_TYPE=Game__Shipping__Win64

# Build with Ninja
cmake --build build_cmake_Game__Shipping__Win64

# Or configure with MSVC (multi-configuration, allows switching configs without reconfiguring)
cmake -B build_cmake_Game__Shipping__Win64 -G "Visual Studio 17 2022"

# Build with MSVC (requires --config flag)
cmake --build build_cmake_Game__Shipping__Win64 --config Game__Shipping__Win64
```

### Configuration Options

CMake allows you to configure various build options. Here are some useful options:

#### Proxy Path

By default, UE4SS generates a proxy based on `C:\Windows\System32\dwmapi.dll`. To change this, set the CMake variable:

```bash
cmake -B build -DUE4SS_PROXY_PATH="<path to proxy dll>" -DCMAKE_BUILD_TYPE=Game__Shipping__Win64
```

#### Profiler Flavor

By default, UE4SS has profiling disabled (`None`). To enable profiling, you need both a profiler flavor AND a build configuration that includes STATS:

```bash
# STATS are enabled by default in Dev and Test builds
cmake -B build -DPROFILER_FLAVOR=<Tracy|Superluminal|None> -DCMAKE_BUILD_TYPE=Game__Dev__Win64
```

> [!NOTE]
> Profiling requires STATS support. By default, `Dev` and `Test` configurations include STATS, while `Shipping` and `Debug` do not. You can manually enable STATS for any configuration by adding compile definitions:
> ```bash
> cmake -B build -DPROFILER_FLAVOR=Tracy -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 -DCMAKE_CXX_FLAGS="-DSTATS"
> ```

### Helpful CMake Commands

| Command | Description |
| --- | --- |
| `cmake -B <build_dir> -G <generator>` | Configure the project with a specific generator (Ninja or "Visual Studio 17 2022") |
| `cmake --build <build_dir>` | Build with Ninja (single-config generator) |
| `cmake --build <build_dir> --config <mode>` | Build with MSVC (multi-config generator, `--config` required) |
| `cmake --build <build_dir> --clean-first` | Clean and rebuild (add `--config <mode>` for MSVC) |
| `cmake --build <build_dir> --target <target>` | Build a specific target (add `--config <mode>` for MSVC) |
| `cmake --build <build_dir> --verbose` | Build with verbose output (add `--config <mode>` for MSVC) |

### Opening in an IDE

#### Visual Studio

CMake has built-in support for generating Visual Studio solutions:

```bash
cmake -B build -G "Visual Studio 17 2022"
```

Then open the generated `.sln` file in the `build` directory.

Alternatively, Visual Studio 2022 has native CMake support - you can open the folder directly in Visual Studio and it will automatically detect the CMakeLists.txt file.

#### CLion / Other CMake IDEs

Most modern IDEs (CLion, Visual Studio Code with CMake Tools, etc.) have native CMake support. Simply open the project folder and the IDE will automatically detect and configure the CMake project.

Note that you should also commit & push the submodules that you've updated if the reason why you updated was not because someone else pushed an update, and you're just catching up to it.

### Cross-Compiling Windows Binaries on Linux

UE4SS supports cross-compilation from Linux to Windows using two approaches: **xwin** (recommended) or **msvc-wine**.

#### Prerequisites for All Cross-Compilation

- [Rust toolchain >= 1.73.0](https://www.rust-lang.org/tools/install) with the `x86_64-pc-windows-msvc` target:
  ```bash
  rustup target add x86_64-pc-windows-msvc
  ```
- [CMake >= 3.22](https://cmake.org/download/)
- [Ninja build system](https://ninja-build.org/)

#### Option 1: Cross-Compiling with xwin (Recommended)

**xwin** downloads and packages the Microsoft CRT headers/libraries and Windows SDK headers/libraries needed for cross-compilation, without requiring a Windows installation.

##### Prerequisites

- LLVM/Clang with Windows target support:
  ```bash
  # On Ubuntu/Debian
  sudo apt install clang lld llvm

  # On Arch Linux
  sudo pacman -S clang lld llvm
  ```
- [xwin](https://github.com/Jake-Shadle/xwin):
  ```bash
  cargo install xwin
  ```

##### Setup

1. Download the Microsoft tools and SDK using xwin (this only needs to be done once):
   ```bash
   xwin --accept-license splat --output ~/.xwin
   ```
   This will download approximately 300MB and can take a few minutes.

2. Set the XWIN_DIR environment variable:
   ```bash
   export XWIN_DIR=~/.xwin
   ```

##### Building Manually with CMake

```bash
# Configure with xwin-clang-cl toolchain (uses clang with MSVC-compatible flags)
XWIN_DIR=~/.xwin cmake -B build_xwin \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xwin-clang-cl-toolchain.cmake

# Or use xwin-clang toolchain (uses clang with GNU-style flags)
XWIN_DIR=~/.xwin cmake -B build_xwin \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xwin-clang-toolchain.cmake

# Build
cmake --build build_xwin
```

##### Building with build.sh Script

```bash
# Set XWIN_DIR
export XWIN_DIR=~/.xwin

# Build with xwin-clang-cl
./tools/buildscripts/build.sh --toolchain xwin-clang-cl

# Or build with xwin-clang
./tools/buildscripts/build.sh --toolchain xwin-clang

# Build specific configuration
./tools/buildscripts/build.sh --toolchain xwin-clang-cl --build-config Game__Debug__Win64

# Clean build with verbose output
./tools/buildscripts/build.sh --toolchain xwin-clang-cl --clean --verbose
```

#### Option 2: Cross-Compiling with msvc-wine

**msvc-wine** uses actual MSVC tools running under Wine. This provides maximum compatibility but requires more setup.

##### Prerequisites

- Wine:
  ```bash
  # On Ubuntu/Debian
  sudo apt install wine wine64 winbind

  # On Arch Linux
  sudo pacman -S wine samba
  ```
- [msvc-wine](https://github.com/mstorsjo/msvc-wine) - Follow their installation guide to install MSVC tools
- Clang (for wine-clang-cl mode) or use MSVC's cl.exe (for wine-msvc mode)

##### Setup

1. Install msvc-wine following the [official instructions](https://github.com/mstorsjo/msvc-wine#installation).
   By default, this installs to `~/my_msvc/opt/msvc`.

2. Make sure the msvc-wine tools are in your PATH:
   ```bash
   export PATH="$HOME/my_msvc/opt/msvc/bin/x64:$PATH"
   ```

3. Set the Wine prefix (optional):
   ```bash
   export WINE_PREFIX=~/.wine
   ```

##### Building Manually with CMake

```bash
# Configure with wine-clang-cl toolchain (clang-cl under wine)
cmake -B build_wine \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wine-clang-cl-toolchain.cmake

# Or use wine-msvc toolchain (MSVC cl.exe under wine)
cmake -B build_wine \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wine-msvc-toolchain.cmake

# Build
cmake --build build_wine
```

##### Building with build.sh Script

```bash
# Build with wine-clang-cl
./tools/buildscripts/build.sh --toolchain wine-clang-cl

# Or build with wine-msvc
./tools/buildscripts/build.sh --toolchain wine-msvc

# Build specific configuration
./tools/buildscripts/build.sh --toolchain wine-clang-cl --build-config Game__Debug__Win64
```

#### Build Output

Cross-compiled binaries will be in the build directory under `<BuildMode>/bin/`:
```
build_xwin_Game__Shipping__Win64/
└── Game__Shipping__Win64/
    └── bin/
        ├── UE4SS.dll
        ├── dwmapi.dll  (proxy DLL)
        └── ... (other files)
```

### Debugging Under Wine

When using wine-based toolchains, you can debug crashes and issues using Wine's debugger.

#### Using winedbg

```bash
# Debug a running program
winedbg ./path/to/game.exe

# Debug a crash dump
winedbg crash_2024_12_26_07_39_15.dmp
```

#### Important Notes for Debugging

- Debug symbols (.pdb files) are **not** stored in minidump files
- You **must** have the exact same .pdb file that corresponds to the .dll that crashed
- The easiest way to ensure matching symbols:
  1. Note the git commit hash when you built UE4SS
  2. When debugging a crash, checkout that exact commit
  3. Rebuild to generate matching .pdb files
  4. Then debug the minidump

#### Tips

- Set `WINEDEBUG=-all` to reduce Wine's debug output during builds (already done by build.sh)
- If you encounter "access denied" errors, ensure you have `winbind` or `samba` installed
- PDB files are generated in the same directory as the DLL files

## Updating git submodules

If you want to update git submodules, you do so one of three ways:
1. You can execute `git submodule update --init --recursive` to update all submodules.
2. You can also choose to update submodules one by one, by executing `git submodule update --init --recursive deps/<first-or-third>/<Repo>`.
    Do not use the `--remote` option unless you actually want to update to the latest commit.
3. If you would rather pick a specific commit or branch to update a submodule to then `cd` into the submodule directory for that dependency and execute `git checkout <branch name or commit>`.
The main dependency you might want to update from time to time is `deps/first/Unreal`.
## Credits

All contributors since the project became open source: https://github.com/UE4SS-RE/RE-UE4SS/graphs/contributors


- **Original Creator** The original creator no longer wishes to be involved in or connected to  this project.  Please respect their wishes, and avoid using their past usernames in connection with this project.
- [**Archengius**](https://github.com/Archengius/)
  - UHT compatible header generator
- **CasualGamer**
  - Injector code & aob scanner is heavily based on his work, 90% of that code is his.
- **SunBeam**
  - Extra signature for function 'GetFullName' for UE4.25.
  - Regex to check for proper signature format when loaded from ini.
  - Lots and lots of work on signatures
- **tomsa**
  - const char* to vector\<int> converter
    - tomsa: Idea & most of the code
    - Original Creator: Nibblet support
- **boop** / **usize**
  - New UFunction hook method
- [**RussellJ**](https://github.com/RussellJerome)
  - Blueprint Modloader inspiration
- [**Narknon**](https://github.com/narknon)
  - Certain features and maintenance/rehosting of the project
- **DeadMor0z**
  - Certain features and Lua updates/maintenance
- [**OutTheShade**](https://github.com/OutTheShade/UnrealMappingsDumper)
  - Unreal Mappings (USMAP) Generator
- **DmgVol**
  - Inspiration for map dumper
- [**Buckminsterfullerene**](https://github.com/Buckminsterfullerene02/)
  - Rewriting the documentation, various fixes
- **trumank**
  - Lua bindings generator, various fixes, automation & improvements
- **localcc**
  - C++ API

## Thanks to everyone who helped with testing

- GreenHouse
- Otis_Inf
- SunBeam
- Motoson
- hooter
- Synopis
- Buckminsterfullerene
