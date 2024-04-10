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

## Targeting UE Versions: From 4.12 To 5.3

The goal of UE4SS is not to be a plug-n-play solution that always works with every game.
The goal is to have an underlying system that works for most games.
You may need to update AOBs on your own, and there's a guide for that below.

## Basic Installation

The easiest installation is via downloading the non-dev version of the latest non-experimental build from [Releases](https://github.com/UE4SS-RE/RE-UE4SS/releases) and extracting the zip content to `/{Gameroot}/GameName/Binaries/Win64/`.

If your game is in the custom config list, extract the contents from the relevant folder to `Win64` as well.

If you are planning on doing mod development using UE4SS, you can do the same as above but download the zDEV version instead. 

## Links

  [Full installation guide](https://docs.ue4ss.com/dev/installation-guide.html)

  [Fixing compatibility problems](https://docs.ue4ss.com/dev/guides/fixing-compatibility-problems.html)

  [Lua API - Overview](https://docs.ue4ss.com/dev/lua-api.html)

  [Generating UHT compatible headers](https://docs.ue4ss.com/dev/guides/generating-uht-compatible-headers.html)

  [Custom Game Configs](https://docs.ue4ss.com/dev/custom-game-configs.html)

  [Creating Compatible Blueprint Mods](https://www.youtube.com/watch?v=fB3yT85XhVA)

  [UE4SS Discord Server Invite](https://discord.gg/7qhRGHF9Tt)

  [Unreal Engine Modding Discord Server Invite](https://discord.gg/zVvsE9mEEa)

## Build requirements

- A computer running Windows.
  - Linux support might happen at some point but not soon.
- A version of MSVC that supports C++20, including std::format.
  - Visual Studio 2019 (recent versions), and Visual Studio 2022 will work.
  - More compilers will hopefully be supported in the future.
- Rust toolchain 1.73.0 or greater
- [xmake](https://xmake.io/#/)


## Build instructions

1. Clone the repo.
2. Execute this command: `git submodule update --init --recursive`
    Make sure your Github account is linked to your Epic Games account for UE source access.
    Do not use the `--remote` option because that will force third-party dependencies to update to the latest commit, and that can break things.
    You will need your github account to be linked to an Epic games account to pull the Unreal pseudo code submodule.

There are several different ways you can build UE4SS.

### Building from cli

Configure the project using this command: `xmake f -m "<BuildMode>"`

The build modes are structured as follows: `<Target>__<Config>__<Platform>`

Currently supported options for these are:

* `Target`
  * `Game` - for regular games
  * `CasePreserving` - for games built with case preserving enabled

* `Config`
  * `Dev` - development build
  * `Debug` - debug build
  * `Shipping` - shipping(release) build
  * `Test` - build for tests

* `Platform`
  * `Win64` - 64-bit windows


Now to build it, just run `xmake`

### Opening in an IDE

#### Visual Studio / Rider

To generate Visual Studio project files, run the `xmake project -k vsxmake2022` command.

Afterwards open the generated `.sln` file inside of the `vsxmake2022` directory

Note that you should also commit & push the submodules that you've updated if the reason why you updated was not because someone else pushed an update, and you're just catching up to it.

**Note the following about how xmake interacts with VS**

> The vs. build plugin performs the compile operation by directly calling the xmake command under vs, and also supports intellisense and definition jumps, as well as breakpoint debugging.

This means that modifying the project properties within Visual Studio will not affect which flags are passed to the build when VS executes `xmake`. XMake provides some configurable project settings
which can be found in VS under the `Project Properties -> Configuration Properties -> Xmake` menu.

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
