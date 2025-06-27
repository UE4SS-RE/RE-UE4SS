v3.1.0
==============
TBD

some notes about most important changes such as: 
- changing of default ue4ss install location, overriding and its backwards compatibility 
- new build system 
- linux port 

## New

### General

Added support for UE Version 5.5 - ([UE4SS #708](https://github.com/UE4SS-RE/RE-UE4SS/pull/708)) 

Added support for UE Version 5.4 - ([UE4SS #503](https://github.com/UE4SS-RE/RE-UE4SS/pull/503)) 

Added basic support for Development/Debug/Test built Unreal Engine games ([UE4SS #607](https://github.com/UE4SS-RE/RE-UE4SS/pull/607)) 
- To use this functionality, set DebugBuild to true in UE4SS-Settings.ini 

Added new build definition "LessEqual421".  Using this definition for games on UE<=4.21 is not mandatory for UE4SS to function, but will ensure the correct alignment is used in containers. 

**BREAKING:** - This also changes the default FName alignment from 8 to 4. 
- To use this functionality, enter LessEqual421 in the <Target> section of the XMake configuration command. 

Added optional scans for GUObjectHashTables, GNatives and ConsoleManagerSingleton; made FText an optional scan; externed the found GNatives for use by mods([UE4SS #744](https://github.com/UE4SS-RE/RE-UE4SS/pull/744)) 
- GUObjectHashTables and ConsoleManagerSingleton are currently unused and a WIP.

UE Platform support, which allows for much easier internal implementation of new Unreal classes ([UEPseudo #80](https://github.com/Re-UE4SS/UEPseudo/pull/80)) - narknon, localcc 

Added new installation method by allowing overriding of the location of the `UE4SS.dll`, [documentation](https://docs.ue4ss.com/installation-guide.html#overriding-install-location). - ([UE4SS #506](https://github.com/UE4SS-RE/RE-UE4SS/pull/506)) - Buckminsterfullerene 

Add Github alerts pre-processor support to the documentation system ([UE4SS #611](https://github.com/UE4SS-RE/RE-UE4SS/pull/611)) - Buckminsterfullerene 

Added custom game configurations for Abiotic Factor ([UE4SS #709](https://github.com/UE4SS-RE/RE-UE4SS/pull/709)) 

Added custom game configurations for Psychonauts 2 ([UE4SS #731](https://github.com/UE4SS-RE/RE-UE4SS/pull/731)) 

Added custom game configurations for Lies of P ([UE4SS #787](https://github.com/UE4SS-RE/RE-UE4SS/pull/787))

The GUI can now be rendered in the game thread if `RenderMode` in UE4SS-settings.ini is set to
`EngineTick` or `GameViewportClientTick` ([UE4SS #768](https://github.com/UE4SS-RE/RE-UE4SS/pull/768), [UE4SS #794](https://github.com/UE4SS-RE/RE-UE4SS/pull/794)).

Added override Lua files for ProcessLocalScriptFunction and ProcessInternal [UE4SS #823](https://github.com/UE4SS-RE/RE-UE4SS/pull/823) - M3C3I

Added override Lua files for CallFunctionByNameWithArguments [UE4SS #848](https://github.com/UE4SS-RE/RE-UE4SS/pull/848) - M3C3I

Add error messages in places where only error codes were previously logged (e.g. load a C++ mod) [UE4SS #902](https://github.com/UE4SS-RE/RE-UE4SS/pull/902) 

Added `[f: <address_or_module_offset>` section to UE4SS_ObjectDump.txt [UE4SS #866](https://github.com/UE4SS-RE/RE-UE4SS/pull/866) 

### Live View 
Added search filter: `IncludeClassNames`. ([UE4SS #472](https://github.com/UE4SS-RE/RE-UE4SS/pull/472)) - Buckminsterfullerene

Added ability to call UFunctions directly from the GUI. ([UE4SS #851](https://github.com/UE4SS-RE/RE-UE4SS/pull/851))

### UHT Dumper 

### Lua API 

**Updated Lua version to 5.4.7** ([UE4SS #887](https://github.com/UE4SS-RE/RE-UE4SS/pull/887))
- This is necessary to compile with Clang.

Added `TMap` implementation. [UE4SS #755](https://github.com/UE4SS-RE/RE-UE4SS/issues/755)

Added global function `CreateInvalidObject`, which returns an invalid UObject. ([UE4SS #652](https://github.com/UE4SS-RE/RE-UE4SS/issues/652))  

Added GenerateLuaTypes function. ([UE4SS #664](https://github.com/UE4SS-RE/RE-UE4SS/pull/664))  

Added global Dumpers functions to Types.lua. ([UE4SS #664](https://github.com/UE4SS-RE/RE-UE4SS/pull/664))

Added global functions `RegisterEndPlayPreHook` and
`RegisterEndPlayPostHook`. ([UE4SS #769](https://github.com/UE4SS-RE/RE-UE4SS/pull/769))

Errors can now be caught using `pcall` and `xpcall`. ([UE4SS #833](https://github.com/UE4SS-RE/RE-UE4SS/pull/833))

Added custom module searcher with UTF-8 path support for Lua `require()` ([UE4SS #884](https://github.com/UE4SS-RE/RE-UE4SS/pull/884))
- Added support for case-insensitive Scripts/scripts directory detection
- Improved error messages showing all attempted paths when module loading fails
- Fixed `IterateGameDirectories` to properly handle UTF-8 paths and Unicode characters
- Fixed `CreateLogicModsDirectory` to properly create directories with Unicode paths and handle parent directory creation
- Fixed global Lua `print` function to properly handle UTF-8 string conversion

#### Types.lua [PR #650](https://github.com/UE4SS-RE/RE-UE4SS/pull/650) 
- Added `NAME_None` definition 
- Added `EFindName` enum definition 
- Added `FName` function overloads with FindType parameter 
- Added `TMap` definitions

#### Types.lua [PR #820](https://github.com/UE4SS-RE/RE-UE4SS/pull/820)

- Added `UStruct` definition
- Added `UClass` definition

#### UEHelpers 
- Added function `GetPlayer` which is just a fast way to get player controlled Pawn (the majority of the time it will be the player character) [PR #650](https://github.com/UE4SS-RE/RE-UE4SS/pull/650)
  
- Added functions: `GetEngine`, `GetGameInstance`, `GetGameViewportClient`,  `GetGameModeBase`, `GetGameStateBase`,`GetPersistentLevel` and `GetWorldSettings` [PR #650](https://github.com/UE4SS-RE/RE-UE4SS/pull/650)
  
- Added functions to get static objects: `GetKismetStringLibrary`, `GetKismetTextLibrary` [PR #650](https://github.com/UE4SS-RE/RE-UE4SS/pull/650)
  
- Added function `GetActorFromHitResult` which extracts the hit actor from a `FHitResult` struct based on UE's version [PR #650](https://github.com/UE4SS-RE/RE-UE4SS/pull/650)
  
- Added FName utility functions [PR #650](https://github.com/UE4SS-RE/RE-UE4SS/pull/650): 
  - `FindFName`: wrapper for `FName(Name, EFindName.FNAME_Find)` 
  - `AddFName`: wrapper for  `FName(Name, EFindName.FNAME_Add)`
    
- Added [Lua Server Annotations](https://luals.github.io/wiki/annotations/) to all UEHelpers functions [PR #650](https://github.com/UE4SS-RE/RE-UE4SS/pull/650)
  
- Added functions `GetAllPlayerStates` and `GetAllPlayers` [PR #688](https://github.com/UE4SS-RE/RE-UE4SS/pull/688) 

- Added annotation for function `FText` to Types.lua. ([UE4SS #788](https://github.com/UE4SS-RE/RE-UE4SS/pull/788))

### C++ API 
Key binds created with `UE4SSProgram::register_keydown_event` end up being duplicated upon mod hot-reload.  
- To fix this, `CppUserModBase::register_keydown_event` has been introduced.  
- It's used exactly the same way except without the `UE4SSProgram::` part. ([UE4SS #446](https://github.com/UE4SS-RE/RE-UE4SS/pull/446)) 

Added `on_ui_init()`, it fires when the UI is initialized.  
- It's intended to use the `UE4SS_ENABLE_IMGUI` macro in this function.  
- Failing to do so will cause a crash when you try to render something with imgui. 

BREAKING: Changed `FTransform` constructor to be identical to unreal. 

Added `OpenFor::ReadWrite`, to be used when calling `File::open`.  
- This can be used when calling `FileHandle::memory_map`, unlike `OpenFor::Writing`.  ([UE4SS #507](https://github.com/UE4SS-RE/RE-UE4SS/pull/507)) 

Added hook for `UGameViewportClient::Tick`. ([UE4SS #767](https://github.com/UE4SS-RE/RE-UE4SS/pull/767))

Added hook for `AActor::EndPlay`. ([UE4SS #769](https://github.com/UE4SS-RE/RE-UE4SS/pull/769))

Added function 'UE4SSProgram::delete_mod'. ([UE4SS #843](https://github.com/UE4SS-RE/RE-UE4SS/pull/843))

Added function 'UE4SSProgram::get_all_input_events'. ([UE4SS #843](https://github.com/UE4SS-RE/RE-UE4SS/pull/843))

Enhanced `TObjectPtr<>` implementation to function as a proper smart pointer. ([UE4SS-RE/RE-UE4SS #850](https://github.com/UE4SS-RE/RE-UE4SS/pull/850), [Re-UE4SS/UEPseudo #125](https://github.com/Re-UE4SS/UEPseudo/pull/125))

**BREAKING:** The `TObjectPtr<>` class now uses a proper pointer implementation rather than direct struct member access. Code that directly accessed `UnderlyingObjectPointer` will need to be updated:
```cpp
// Old way
TObjectPtr<UClass> ClassPtr;
UClass* RawPtr = ClassPtr.UnderlyingObjectPointer;

// New way
TObjectPtr<UClass> ClassPtr;
UClass* RawPtr = ClassPtr; // implicit conversion
// or
UClass* RawPtr = ClassPtr.Get(); // explicit access
```

Added improved string and path conversion utilities with proper UTF-8 support ([UE4SS #913](https://github.com/UE4SS-RE/RE-UE4SS/pull/913))
- Rewrote `to_charT_string_path()` to properly handle UTF-8 and UTF-16 encodings
- Added `ensure_str_as<CharT>()` for explicit target character type conversion
- Added `to_utf8_string()` for convenient UTF-8 string conversion
- Added `normalize_path_for_lua()` to convert paths to UTF-8 with forward slashes for Lua compatibility
- Added `utf8_to_wpath()` to convert UTF-8 paths to Windows wide strings
- **BREAKING:** `to_charT_string_path()` now returns UTF-8 encoded strings for char type instead of locale-dependent encoding

### BPModLoader 

### Experimental 


## Changes 

### General 

Updated ImGui from v1.89 to v1.91.9b for improved functionality and Clang compatability 

Updated ImGuiTextEdit to support newest ImGui API 

Changed the default location of the UE4SS release assets to be in `game executable directory/ue4ss/`. This change is backwards compatible with the old location. ([UE4SS #506](https://github.com/UE4SS-RE/RE-UE4SS/pull/506)) - Buckminsterfullerene 

Updated PatternSleuth submodule ([UE4SS #638](https://github.com/UE4SS-RE/RE-UE4SS/pull/638)) 

The `bUseUObjectArrayCache` option in UE4SS-settings.ini is now set to false by default in the non-zDev release ([UE4SS #747](https://github.com/UE4SS-RE/RE-UE4SS/pull/747)) 

The `GuiConsoleEnabled` option in UE4SS-settings.ini is now set to 0 by default in the non-zDev
release ([UE4SS #779](https://github.com/UE4SS-RE/RE-UE4SS/pull/779))

Removed some development mods, `README.md` & `Changelog.md` from non-zDev release, added `LICENCE` file to both release types ([UE4SS #830](https://github.com/UE4SS-RE/RE-UE4SS/pull/830))

### Live View 
Fixed the majority of the lag ([UE4SS #512](https://github.com/UE4SS-RE/RE-UE4SS/pull/512)) 

Added support for watching ArrayProperty and StructProperty ([UE4SS #419](https://github.com/UE4SS-RE/RE-UE4SS/pull/419)) 

The search filter `ExcludeClassName` can now be found in the `IncludeClassNames` dropdown list. ([UE4SS #472](https://github.com/UE4SS-RE/RE-UE4SS/pull/472)) - Buckminsterfullerene 

The following search filters now allow multiple values, with each value separated by a comma: `IncludeClassNames`, `ExcludeClassNames`, `HasProperty`, `HasPropertyType`. ([UE4SS #472](https://github.com/UE4SS-RE/RE-UE4SS/pull/472)) - Buckminsterfullerene 

### UHT Dumper 

### Lua API 
`print` now behaves like vanilla Lua (can now accept zero, one, or multiple arguments of any type) ([UE4SS #423](https://github.com/UE4SS-RE/RE-UE4SS/pull/423)) - Lyrth 

The callback of `NotifyOnNewObject` can now optionally return `true` to unregister itself ([UE4SS #432](https://github.com/UE4SS-RE/RE-UE4SS/pull/432)) - Lyrth 

`RegisterConsoleCommandHandler`, `RegisterConsoleCommandGlobalHandler`, `RegisterProcessConsoleExecPreHook` and `RegisterProcessConsoleExecPostHook` now support enclosing parameters in double quotes (`"`). This allows spaces within the quotes to be ignored, treating the enclosed string as a single parameter. ([UE4SS #653](https://github.com/UE4SS-RE/RE-UE4SS/pull/653), [UE4SS #669](https://github.com/UE4SS-RE/RE-UE4SS/pull/669))  

**BREAKING:** `RegisterProcessConsoleExecPreHook`,  `RegisterProcessConsoleExecPostHook` callback parameter `CommandParts` now includes the command name at the start of the array (lua array index **1**). ([UE4SS #669](https://github.com/UE4SS-RE/RE-UE4SS/pull/669)) 

Improved performance of script hooks created with `RegisterHook`, and
`RegisterCustomEvent`. ([UE4SS #801](https://github.com/UE4SS-RE/RE-UE4SS/pull/801))

**BREAKING:** `AActor:GetWorld()` and `AActor:GetLevel()` functions are now returning an invalid `UObject` instead of `nil`. ([UE4SS #810](https://github.com/UE4SS-RE/RE-UE4SS/pull/810))

Types with `get` or `Get` functions now have both variants. ([UE4SS #877](https://github.com/UE4SS-RE/RE-UE4SS/pull/877))

#### UEHelpers [UE4SS #650](https://github.com/UE4SS-RE/RE-UE4SS/pull/650) 
- Increased version to 3
  
- Reworked all UEHelpers functions to ensure that they always return an object which can be checked with the function `IsValid` for validation
  
- Reworked `UEHelpers.GetPlayerController` to return first valid player controller (It will now return a player controller even if it doesn't control a pawn at the time)
  
- Reworked `UEHelpers.GetWorld` function to use UWorld cache (UWorld usually never changes)
  
- Change `UEHelpers.GetWorldContextObject` function annotation to return `UObject`. (Any UObject with a GetWorld() function is a valid WorldContext)
  
- Removed duplicate function `UEHelpers.GetKismetMathLibrary` 

### C++ API 

### BPModLoader 
BPModLoader now supports loading mods from subdirectories within the `LogicMods` folder ([UE4SS #412](https://github.com/UE4SS-RE/RE-UE4SS/pull/412)) - Ethan Green 

### ConsoleEnablerMod 
Added additional console key **~** (Tilde) ([UE4SS #687](https://github.com/UE4SS-RE/RE-UE4SS/pull/687))  

The console keys **F10** and **~** are now added by the mod in addition to the existing keys instead of replacing them ([UE4SS #687](https://github.com/UE4SS-RE/RE-UE4SS/pull/687)) 

### Repo & Build Process 
Switch to xmake from cmake which makes building much more streamlined ([UE4SS #377](https://github.com/UE4SS-RE/RE-UE4SS/pull/377), [UEPseudo #81](https://github.com/Re-UE4SS/UEPseudo/pull/81)) - localcc 


## Fixes 

### General 
Fixed BPModLoaderMod not working in games made in UE 5.2+ - ([UE4SS #503](https://github.com/UE4SS-RE/RE-UE4SS/pull/503)) 

Fixed adding elements to TArray in Lua incorrectly resizing and zeroing out previous values ([UE4SS #436](https://github.com/UE4SS-RE/RE-UE4SS/pull/436)) - dnchattan 

Fixed some debug GUI layout alignments, especially with different GUI font scaling settings ([UE4SS #429](https://github.com/UE4SS-RE/RE-UE4SS/pull/429)) - Lyrth 

Fixed PalServer not accepting connections from players ([UE4SS #453](https://github.com/UE4SS-RE/RE-UE4SS/pull/453)) 

Fixed a crash that would occur randomly due to accidental execution of Live View search code ([UE4SS #475](https://github.com/UE4SS-RE/RE-UE4SS/pull/475)) 

Fixed freeze related to the error "Tried to execute UFunction::FuncPtr hook but there was no function map entry" ([UE4SS #468](https://github.com/UE4SS-RE/RE-UE4SS/pull/468)) 

Fixed `ModsFolderPath` in `UE4SS-settings.ini` not working ([UE4SS #609](https://github.com/UE4SS-RE/RE-UE4SS/pull/609)) 

Fixed `attempt to index a nil value (global 'NewController')` error in `SplitScreenMod` ([UE4SS #729](https://github.com/UE4SS-RE/RE-UE4SS/pull/729)) 

Fixed the GUI not closing properly with CTRL + O when OpenGL is enabled in
`UE4SS-settings.ini`. ([UE4SS #780](https://github.com/UE4SS-RE/RE-UE4SS/pull/780))

### Live View 
Fixed the "Write to file" checkbox not working for functions in the `Watches` tab ([UE4SS #419](https://github.com/UE4SS-RE/RE-UE4SS/pull/419)) 

Reduced the likelihood of a crash happening on shutdown when at least one watch is enabled ([UE4SS #419](https://github.com/UE4SS-RE/RE-UE4SS/pull/419)) 

Fixed constantly searching even if the search field is empty, and even if not on the tab ([UE4SS #475](https://github.com/UE4SS-RE/RE-UE4SS/pull/475)) 

Fixed crash caused by using invalid iterator. ([UE4SS #771](https://github.com/UE4SS-RE/RE-UE4SS/pull/771))

Fixed opened object getting clipped too early ([UE4SS #814](https://github.com/UE4SS-RE/RE-UE4SS/pull/814)) -
Corporalwill123

### UHT Dumper 
Fix SetupAttachment implementations randomly changing order ([UE4SS #606](https://github.com/UE4SS-RE/RE-UE4SS/pull/606)) - Buckminsterfullerene 

### Lua API 
Fixed FString use after free ([UE4SS #425](https://github.com/UE4SS-RE/RE-UE4SS/pull/425)) - localcc 

Fixed the "IterateGameDirectories" global function throwing "bad conversion" errors ([UE4SS #398](https://github.com/UE4SS-RE/RE-UE4SS/pull/398)) 

Fixed `FText` not working as a parameter (in `RegisterHook`, etc.) ([UE4SS #422](https://github.com/UE4SS-RE/RE-UE4SS/pull/422)) - localcc 

Fixed crash when calling UFunctions that take one or more 'out' params of type TArray<T>. ([UE4SS #477](https://github.com/UE4SS-RE/RE-UE4SS/pull/477)) 

Fixed `FindObject` not accepting UClass as a valid outer parameter. ([UE4SS #732](https://github.com/UE4SS-RE/RE-UE4SS/issues/732)) - GhostyPool

Fixed `RegisterProcessConsoleExecPostHook`. ([UE4SS #631](https://github.com/UE4SS-RE/RE-UE4SS/pull/631)) 

Fixed `FindFirstOf` return type annotation in `Types.lua` to signal that the return value will never be nil. ([UE4SS #652](https://github.com/UE4SS-RE/RE-UE4SS/issues/652)) 

Fixed non-const TArray outparams being skipped in UFunction property pushers. ([UE4SS #754](https://github.com/UE4SS-RE/RE-UE4SS/pull/754))

Fixed `RegisterLoadMapPreHook` not working at all. ([UE4SS #776](https://github.com/UE4SS-RE/RE-UE4SS/pull/776))

Fixed `Key::NUM_ZERO` being incorrectly mapped to
`Key::NUM_NINE`. ([UE4SS #762](https://github.com/UE4SS-RE/RE-UE4SS/pull/762))

Fixed table-in-table when used as a function param (i.e. FTransform) generating a Lua
error. ([UE4SS #775](https://github.com/UE4SS-RE/RE-UE4SS/pull/775))

Fixed script hooks corrupting the return value of the original
function. ([UE4SS #800](https://github.com/UE4SS-RE/RE-UE4SS/pull/800))

Fixed race condition when using RegisterCustomEvent or
UnregisterCustomEvent. ([UE4SS #805](https://github.com/UE4SS-RE/RE-UE4SS/pull/805))
  
Fixed frequent `StrProperty can only be set to a string or FString`
error. ([UE4SS #819](https://github.com/UE4SS-RE/RE-UE4SS/pull/819))

Fixed problems that caused issues for language servers. ([UE4SS #821](https://github.com/UE4SS-RE/RE-UE4SS/pull/821)

Fixed errors being logged twice. ([UE4SS #833](https://github.com/UE4SS-RE/RE-UE4SS/pull/833))

Fixed `RegisterHook` and `UnregisterHook` not working properly with functions that have spaces in their
names. ([UE4SS #827](https://github.com/UE4SS-RE/RE-UE4SS/pull/827)

Fixed an error with Object properties causing stack corruption. ([UE4SS #939](https://github.com/UE4SS-RE/RE-UE4SS/pull/939)

### C++ API 
Fixed a crash caused by a race condition enabled by C++ mods using `UE4SS_ENABLE_IMGUI` in their constructor ([UE4SS #481](https://github.com/UE4SS-RE/RE-UE4SS/pull/481)) 

Fixed the `std::span` returned by `FileHandle::memory_map` being improperly sized. ([UE4SS #507](https://github.com/UE4SS-RE/RE-UE4SS/pull/507)) 

Fixed 'File::Handle' unable to be moved if used in conjunction with memory_map. ([UE4SS #544](https://github.com/UE4SS-RE/RE-UE4SS/pull/544)) 

Fixed `Key::NUM_ZERO` being incorrectly mapped to `Key::NUM_NINE`. ([UE4SS #716](https://github.com/UE4SS-RE/RE-UE4SS/pull/716)) 

### BPModLoader 
Fixed "bad conversion" errors ([UE4SS #398](https://github.com/UE4SS-RE/RE-UE4SS/pull/398)) 

Fixed calling `PostBeginPlay` multiple times for each ModActor ([UE4SS #447](https://github.com/UE4SS-RE/RE-UE4SS/pull/447)) - Okaetsu 

Fixed displaying 'PostBeginPlay not valid' when VerboseLogging is set to false ([UE4SS #447](https://github.com/UE4SS-RE/RE-UE4SS/pull/447)) - Okaetsu 

Fixes mods not loading when UE4SS initializes too late ([UE4SS #454](https://github.com/UE4SS-RE/RE-UE4SS/pull/454)) - localcc 

Fixed `LoadMod` function issue that variables would go out-of-scope in the `ExecuteInGameThread` callback. Instead, `LoadMods` is now always called in a game thread. ([UE4SS #752](https://github.com/UE4SS-RE/RE-UE4SS/pull/752))

## Settings 

### Added 
```ini
[EngineVersionOverride]
; True if the game is built as Debug, Development, or Test.
; Default: false
DebugBuild =

[ObjectDumper]
; Whether to display the offset from the main executable for functions instead of the function pointer
; Default: 0
UseModuleOffsets = 0

[Debug]
RenderMode = ExternalThread

[Hooks]
HookLoadMap = 1
HookAActorTick = 1
HookEngineTick = 1
HookGameViewportClientTick = 1
```

### Removed
```ini
[Debug]
LiveViewObjectsPerGroup = 32768

[ExperimentalFeatures]
GUIUFunctionCaller
```


v3.0.1
==============
2024-02-14

This is a patch release for 3.0.0. Installing over 3.0 is as simple as replacing all files except for your `UE4SS-settings.ini` if you have custom configs.

If you have not yet installed since 2.5.2, please see the installation instructions in the v3.0.0 release notes. **Remember to delete the old xinput1_3.dll as it crashes the game if you still have it!**

**C++ mods must be rebuilt to work on 3.0.1.**

**Current known issues & solutions:**
1. If you have the GUI console enabled and visible, and on launch the window is blank/white, go into `UE4SS-settings.ini` and set `GraphicsAPI = dx11`.
2. If you're experiencing game crashes on startup, try setting `bUseUObjectArrayCache = false` in `UE4SS-settings.ini`. 
3. If mods aren't working as expected, make sure you follow their install instructions properly, not based on "what worked before". This is due to UE4SS supporting a number of different mod types and the way they're installed can vary.

**Additional couple of notes for mod authors:**
1. We have noticed a number of popular mods that have fallen down a pitfall that may very well lead to performance issues. Please refer to [this section](https://docs.ue4ss.com/dev/lua-api/global-functions/notifyonnewobject.html#what-not-to-do) in the docs for more information. 
2. If you're developing on the latest experimental version, please make sure to use the `docs.ue4ss.com/dev` version of the docs as this is updated with the latest changes.

-UE4SS team

## New

### General
Added new docs pages for devlogs, C++ examples, C++ BP macros ([UE4SS #355](https://github.com/UE4SS-RE/RE-UE4SS/pull/355)) - Buckminsterfullerene

Added examplanation of 'ai' to the Object Dumper doc page ([UE4SS #349](https://github.com/UE4SS-RE/RE-UE4SS/pull/349))

### Live View

### UHT Dumper

### Lua API

### C++ API

### Experimental


## Changes

### General
Update docs installation guide for 3.0.0 ([UE4SS #355](https://github.com/UE4SS-RE/RE-UE4SS/pull/355)) - Buckminsterfullerene

Added step to C++ prerequisits to join Epic Games org for repo access ([UE4SS #339](https://github.com/UE4SS-RE/RE-UE4SS/pull/339)) - groffta

Updated Lua doc for RegisterHook to accurately state how the callback params work ([UE4SS #372](https://github.com/UE4SS-RE/RE-UE4SS/pull/372))

Clarify Lua metamethod usage for some classes ([UE4SS #376](https://github.com/UE4SS-RE/RE-UE4SS/pull/376)) - Lyrth

### Live View

### UHT Dumper

### Lua API
Fixed bug where RegisterHook on native functions couldn't affect return values unless you had both a pre and a post hook ([UE4SS #366](https://github.com/UE4SS-RE/RE-UE4SS/pull/366))

### C++ API

### Repo & Build Process
Removed custom game configs for games that don't need them anymore ([UE4SS #386](https://github.com/UE4SS-RE/RE-UE4SS/pull/386)) - Buckminsterfullerene

Added mods directory zip as an explicit step to the bug report template ([UE4SS #358](https://github.com/UE4SS-RE/RE-UE4SS/pull/358))

Added PR template ([UE4SS #363](https://github.com/UE4SS-RE/RE-UE4SS/pull/363)) - Buckminsterfullerene


## Fixes

### General
Fixed "FindAllOf" dereferencing a nullptr ([UEPsuedo #78](https://github.com/Re-UE4SS/UEPseudo/pull/78)) - localcc

Fixed a memory leak when BPMacros.hpp was used ([UEPsuedo #75](https://github.com/Re-UE4SS/UEPseudo/pull/75))

Fixed modifier key issue causing keybinds to not work ([UE4SS #389](https://github.com/UE4SS-RE/RE-UE4SS/pull/389)) - Tangerie

### Live View
Fix GUI crash when clicking close window button ([UE4SS #337](https://github.com/UE4SS-RE/RE-UE4SS/pull/337)) - localcc

### UHT Dumper

### Lua API
Fix Lua FString setter ([UE4SS #325](https://github.com/UE4SS-RE/RE-UE4SS/pull/325)) - Yangff

Fix FindObject and Mod:SetSharedVariable userdata type matching ([UE4SS #342](https://github.com/UE4SS-RE/RE-UE4SS/pull/342)) - Lyrth

Fixed non-native UFunctions hooks not working if hooked by multiple mods ([UE4SS #351](https://github.com/UE4SS-RE/RE-UE4SS/pull/351))

### C++ API
Fix TProperty cpp mods linking issue ([UEPsuedo #77](https://github.com/Re-UE4SS/UEPseudo/pull/77)) - localcc

## Settings

### Added

### Removed


v3.0.0
==============
2024-02-04

After 9 months in development, we hope 3.0 was worth the wait. We want future releases to be more frequent, as constantly telling people "try latest experimental" is never a good solution.

For the next release, we have some exciting new features in development, so stay tuned for those. We're very excited to reveal what we've been working on soon, but we really needed to get 3.0 out first.

-UE4SS team

## Installation over 2.5.2

Remember to check the `README` for the latest installation instructions.

Since the `UE4SS-settings.ini` has changed in this release, you should use the new one, so if you have made settings changes, remember to back them up!

When installing 3.0 in the place of the previous release 2.5.2, you must replace all existing UE4SS files with new files, except for the `UE4SS_Signatures` folder if your game is using custom sigs. 

3.0 also uses two new dlls:
- `UE4SS.dll`
- `dwmapi.dll`

and no longer uses the older `xinput1_3.dll`, **so please also delete `xinput1_3.dll` if you have one**. 

## New

### General
Added support for UE Version 5.2 and 5.3 games

New proxy DLL loading system. - LocalCC  
Different proxy DLLs can now easily be compiled for cases where xinput1_3 cannot be used for any reason.  
Alternative proxys may be compiled by specifying `-DUE4SS_PROXY_PATH=/Path/To/DLL.dll` when running the CMake command.

Replace built-in address scans with [patternsleuth](https://github.com/trumank/patternsleuth) which should greatly improve accuracy and reduce need for manual AOB configuration - trumank, LongerWarrior

Added SuperStruct, `sps`, to Struct and ScriptStruct entries in the object dumper

### Live View
Can now view enum values in the Live View debugger

Added ClassFlags to UClass and derivatives

Added a checkbox that toggles search options globally, meaning when not searching

Added search filter `Function parameter flags`, it excludes objects that are non-UFunctions and UFunctions that don't have params with the specified flags

Added search filter `Non-instances only`

Added search filter `Include CDOs`, it includes objects that are not a ClassDefaultObject or an ArchetypeObject

Added search filter `CDOs only`, it excludes objects that are not a ClassDefaultObject or an ArchetypeObject

Added search filter `Use Regex for search` - HW12Dev

Added search filter `Exclude class name`, it excludes objects with a ClassPrivate name not containing the specified (case-sensitive) string

Added search filter `Has property`, it exclude objects that don't have a property (inheritance included) of the specified (case-sensitive) name

Added search filter `Has property of type`, it excludes objects that don't have a property (inheritance included) of the specified (case-sensitive) type

### UHT Dumper
Removed unnecessary explicit `_MAX` elements from enums

Made `FWeakObjectPtr` overridable unless used in a TArray or TMap

### Lua API
Added an optional third parameter to `RegisterHook`  
If provided, it will act as a post callback hook where out-params can be modified  
Note that for BP-only functions, both callbacks act as post callbacks  
If the hooked function has a return value, the second param to the post callback will be the return value

Out-params for script hooks (`RegisterCustomEvent` or `RegisterHook` on a BP-only UFunction) can now be set by doing `Param:set(<new-value>)`

Added the function `Empty` to TArray

Added `RegisterLoadMapPreHook`/`RegisterLoadMapPostHook` hooks for `UEngine::LoadMap`.

### C++ API
Finalize C++ API. - LocalCC; Truman

Removed need for "cppsdk" when linking C++ mods.  This is due to the new proxy system. - LocalCC  
Due to the above change, C++ mods now only need to link to UE4SS.

C++ mods are now loaded earlier, and will keep the game from starting until all mods have finished executing their `start_mod` function

Made calls to `UObject::StaticClass` work for custom UObject classes that have been made with the `DECLARE_EXTERNAL_OBJECT_CLASS` and `IMPLEMENT_EXTERNAL_OBJECT_CLASS` macros

Expose IMGui to C++ mods - Truman

Added `on_lua_start` for C++ mods.  
Overload #1: This function fires whenever any Lua mod is started.  
Overload #2: This function fires whenever a Lua mod by the same name as the C++ mod is started.  
It allows interactions with Lua from C++ mods.

Added `on_lua_stop` for C++ mods.  
Overload #1: This function fires whenever any Lua mod is about to be stopped.  
Overload #2: This function fires right before a Lua mod by the same name as the C++ mod is about to be stopped.

Added `UFunction::RegisterPreHookForInstance` and `UFunction::RegisterPostHookForInstance`  
These functions work the same as `UFunction::RegisterPreHook`/`UFunction::RegisterPostHook` except the callback is only fired if the context matches the specified instance  
These new functions need to be handled with care as they can cause crashes if you don't validate that the instance you're passing during registration is valid inside the callback

Added overloads for `UObject::GetFunctionByName` and `UObject::GetFunctionByNameInChain` that take an `FName` instead of a string

Added `UEnum::NumEnums`, which returns the number of enum values for the enum

Added `UEnum::GenerateEnumPrefix`, which is the same as https://docs.unrealengine.com/5.2/en-US/API/Runtime/CoreUObject/UObject/UEnum/GenerateEnumPrefix/

Added `UGameplayStatics::FindNearestActor`

Added the following functions to `AActor`: `K2_DestroyActor`, `K2_SetActorLocation`, `K2_SetActorLocationAndRotation`, `K2_GetActorRotation`, `K2_SetActorRotation`, `GetActorScale3D`, `SetActorScale3D`, `GetActorEnableCollision`, `SetActorEnableCollision`, `SetActorHiddenInGame`, `IsActorTickEnabled`, `SetActorTickEnabled`, `GetActorTickInterval`, `SetActorTickInterval`, `GetActorTimeDilation` - Okaetsu

### Experimental
Added ExperimentalFeatures section to UE4SS-settings.ini.  All experimental features will default to being turned off.  To use referenced features, change the relevant config setting to = 1

Added ability to call UFunctions from Live View GUI


## Changes

### General
The shortcut (CTRL + O) for opening the GUI is now a toggle, meaning it can also be used for closing the GUI

The shortcut (previously J) for dumping objects (generating UE4SS_ObjectDump.txt) has been changed to CTRL + J

The shortcut (previously D) for generating CXX headers has been changed to CTRL + H

Change AOB Sig Scanner backend to use std::find for major performance increase - inspired by Truman

Scan for specified time rather than number of attempts due to speed increase

Improved performance for U/FProperty lookups

Improved performance for UFunction lookups

Improved performance of the GUI log, it's now O(n) - trumank

BPModLoaderMod: Add ability to specify load order - Okaetsu

Add additional extensions to USMap dumper - Atenfyr; Archengius

Fix bug in USMap dumper with enums with 256 entries - Atenfyr

### Live View

### UHT Dumper

### Lua API
Improved reliability of `IsValid`

### C++ API
The callbacks for all hook registration functions inside the `Unreal::Hook` namespace can now take lambdas that capture variables

Changed many functions to use coroutines - LocalCC  
This means the syntax for those functions is now identical to a range-based for loop instead of a function taking a callback

### Repo & Build Process
Add automated release script - Truman

Change documentation build process; the docs will now have a dev version (at /dev) for changes on main branch and a current version (at normal root) for the state of the docs at the current release - Truman; Buckminsterfullerene

Removed libfmt dependency

Gradual work on getting Clang to work - LocalCC; Narknon


## Fixes

### General
Finish adding version 4.11 support

Fix case preserving names switch - LocalCC

### Live View
Fixed two crashes occurring when exploring structs nested in arrays or other structs

### UHT Dumper
Fixed enums inappropriately using `uint8`

### Lua API
Fixed `UnregisterHook`

Fixed FText:ToString - LocalCC

Improved stability when using hooks or `ExecuteInGameThread`

TArrays are now resized when being indexed into if necessary

### C++ API
Fixed FText constructor implementation via optional AOB - LocalCC

Fixed initialization functions not being correctly called when a mod is restarted - LocalCC

Fixed C++ mods not loading if a Lua mod with the same name is present


## Settings

### Added:
```[General]
; Whether the cache system for AOBs will be used.
; Default: 1
UseCache = 1

; The number of seconds the scanner will scan for before giving up
; Default: 30
SecondsToScanBeforeGivingUp = 30

[ExperimentalFeatures]
GUIUFunctionCaller = 1
```

### Removed:
```[General]
; The maximum number attempts the scanner will try before erroring out if an aob isn't found
; Default: 60
MaxScanAttemptsNormal = 60

; The maximum number attempts the scanner will try for modular games before erroring out if an aob isn't found
; Default: 2000
MaxScanAttemptsModular = 2500
```


v2.5.2
==============
2023-05-01

Note: This is a stopgap release to add Jedi Survivor support, and the below changelog is partially incomplete.  Full changelog will be posted when version 2.6 is released.

## Fixes
Fix more detached submodules (including Lua 5.4.4 update)

Expose certain additional classes to the member variable and vtable configs.

## New
Add USMAP dumper extensions (Atenfyr)

Add Lua Type generator for use by a Lua language server (TrumanK). See https://marketplace.visualstudio.com/items?itemName=sumneko.lua and https://github.com/LuaLS/lua-language-server/wiki/Annotations

Font scaling setting for Live View

### Live View
Add "PlayerControlled" to liveview tab

### Lua
Add IsValid() to TArray

Fixed UnregisterHook

Expanded RegisterHook to work on BP functions where it previously only worked on native functions

Added UnregisterCustomEvent

Fixed bug that could cause hooked functions to not process the return value properly

Lua_lock implementation/threading fixes (ParcelRot)

### UHT Dumper
Remove predeclarations in .cpp files

### QoL
Add font scaling setting to live view GUI

### Experimental
C++ Modding API (LocalCC + UE4SS) - Recommended to wait until 2.6 release to begin use.

v2.5.1
==============
2023-03-06

## New Features

### Blueprint ModLoading
Added blueprint mod loading/actor spawning mod.

To enable, set BPModLoaderMod : 1 in Mods/mods.txt.

Blueprint mods go in /Game/Content/Paks/LogicMods


Creating BP mods is the same as using UML, a tutorial can be found here: https://www.youtube.com/watch?v=fB3yT85XhVA

Added creating LogicMods folder automatically.

Thank you to [RussellJ](https://github.com/RussellJerome) for permission to implement this.

### Lua
Update to Lua 5.4.4

Added support for UInterface.

Added several global lua functions, including 'IterateGameDirectories'. See API.txt for full details.

Added additional hooks and functions to Lua. See full list in API.txt.  Note in particular:
    - UnregisterHook (unregister a previously hooked function using the ids returned from the hooked function)
    - RegisterCustomEvent (allows hooking of custom BP functions/events through ProcessInternal)
    
Non-struct out parameters now partially work. You need to create and pass a Lua table wherever there's an out-param. The value inside the table doesn't matter and is not passed to the UFunction. The table will have a field with the same name as the out-param which will contain the value.

Lua Mods:
Update to linetrace mod to use helpers

### Live View
Added filtering by Inheritance or Instances only when searching in the live view.  Accessed by right clicking in the search bar.

### TMap Override Dumper
Dumps TMap overrides for UAssetAPI/GUI and FModel serialization of assets.  This is a json file that gets input in the relevant program to allow it to parse TMaps with untyped structs as a key or value.

## Changes

### Live View
The live view array now uses a lower default number of objects per visual chunk (note that the objects are no longer 1 to 1 with the true UE "Chunks" in the array).

The default number of objects per chunk can be edited in the settings.

Added UHT Dumper and CXX Generator buttons to Dumpers tab.

Add timestamp to static mesh and actor dump file names.

### Lua
FName global function now has an optional second param.



## Fixes

### CXX Gen
Generator will continue in certain events where it would previously crash.

### UHT Generator
Generator will make a missed property comment in certain events where it would previously crash.

Fixed crash if FunctionSignature was nullptr in the UHT generator.

### Lua
Fix mods not being enabled if carriage returns are used in mods.txt (Praydog)

Fixed 'RegisterHook' not working properly if the UFunction has spaces in its name.

Fix typo in Lua cpath causing DLLs to not get found (Praydog)

Added a lock guard for ExecuteInGameThread to prevent a crash.

Fixed 'EFindName' not being exposed to Lua properly.

### Live View
Fixed a race condition in live view.

Fixed a crash when expanding an array of objects in live view.


### Other

Mod developers and compatibility troubleshooting users may wish to download the DEV- zip.  End users should download the normal files.

v2.3.1-Hotfix
==============
2023-01-25

Adds missed commit related to UHT gen ordering.  See v2.3.0 release for full changelog.

v2.3.0
==============
2023-01-25

## New

### Lua

* Added 'ModRef.SetSharedVariable' and 'ModRef.GetSharedVariable'
* Added UObject.HasAnyInternalFlags
* Added global table: 'EInternalObjectFlags'

* You can now set an ObjectProperty value to `nil`. Previously such an action would be ignored

* When calling 'IsValid()' on UObjects, whether the UObject is reachable is now taken into account

* The splitscreen mod now operates independently of the Lua state which means that hot-reloading shouldn't cause it to break

* Added shared module "UEHelpers" to provide shortcut functions to the Lua module for commonly used UE functions or classes

### Live View GUI

* Default renderer of the GUI has been changed to OpenGL for compatibility reasons.  This can be changed back to dx11 by editing the setting in UE4SS-settings.ini to "dx11"

## Changes

### UHT Generation

* Buckminsterfullerene - Includes and forward declarations are now ordered to allow for easier diffing
* Buckminsterfullerene - Added setting to force "Config = Engine" on UCLASS specifiers for classes with "DefaultConfig", "GlobalUserConfig" or "ProjectUserConfig"

### CXX Dump

* Buckminsterfullerene - Structs and classes are now ordered to allow for easier diffing

## Fixes

### UHT Generation

* Praydog - Fixes to build.cs generation
* Buckminsterfullerene - Tick functions now include the required template

v2.2.1-Hotfix
==============
2023-01-17

## Fixes:

### UHT Generation
* Fixes bug introduced in 2.2.0 that caused certain modules not to be included in the build.cs files

v2.2.0
==============
2023-01-03

## New

### Live View Debugger
* Add live editing of properties.  May be unstable with certain structs.  Please report crashes with an Issue.

![image](https://user-images.githubusercontent.com/73571427/212568926-c1c411b7-e89b-4bdf-8873-8e4cf8a66c54.png)

![image](https://user-images.githubusercontent.com/73571427/212568932-22c3fed6-b95a-42fd-ac61-3b9dfade8c89.png)


### Keybinds
* The old lua mods that only registered a key bind (ObjectDumperMod, UHTCompatibleHeaderGeneratorMod, & CXXHeaderGeneratorMod) have been removed.
* If you have an old mod installed, it takes precedence over the new mod.
* The new mod will have all keybinds listed at the top of the file to make it easy to see which key binds are already in use. 
* Added keybinds for the USMAP (CTRL + NUM_SIX), All Actor (CTRL + NUM_SEVEN) and Static Mesh (CTRL + NUM_EIGHT) dumpers.

### Lua
* Add mod - LineTraceMod - Prints name of object in center of camera to console.  Not enabled by default.
* Add mod - SplitScreenMod attempts to spawn additional player characters in splitscreen mode.  Edit the main.lua script to change how the first controller functions.  Not enabled by default.
* New ExecuteInGameThread function for improved compatibility with certain internal UE functions

## Fixes

### UHT Generation
* Praydog - Fixed certain possible crashes with UHT Generation
* Praydog - Fixed UHT Generation crashing on building stubs when Engine module dumping is enabled
* Allow Instanced specifier on Map properties where the value is a UObject base
* Add GetTypeHash inline function generation for structs used in Map properties

### General
* Praydog - StaticConstructObject: Fix crash when Params.Class is nullptr

## Known Issues
* Certain structs will cause crashes when live editing
* You must edit certain structs from the highest level rather than from the tree view (editing within the tree view is disabled currently).

v2.1.1-HotFix
==============
2023-01-03

## Fixes
* Fixed Hot Reload for Lua Scripting
* Fixed UE 5.1 compatibility

v2.1.0
==============
2023-01-02

## NEW:
* UnrealMappingsDumper by OutTheShade has been ported to UE4SS.  This dumper is used to generate .usmap files for parsing of assets using unversioned properties.  Based on Commit SHA 4da8c66 with some modifications.  See: https://github.com/OutTheShade/UnrealMappingsDumper.
* Actor and Static Mesh Dumpers no longer require a specific DLL for different engine versions.
* Unreal Engine 5.1 Support (only tested on demo game)
* GUI now uses Roboto font.

## UHT Gen:
* Fixed uint8 properties not being overridden as blueprintreadwrite.

## Fixes:
* Fixed a crash that would occur on exiting certain games.
* Fixed a crash that could occur with non-standard characters/glyphs when building in debug mode.
* Fixed UObjectArrayCache setting parsing.

v2.0.0.1Alpha-Hotfix
==============
2022-11-22

### Big update for UE4SS that adds a new debugger window and a map dumper.

Hotfix related to Lua mod loading.

## **NEW:**

* **Live View Debugger** - Allows runtime viewing of Objects in GObject with live updates to property and variable values.
Enable the Debug log in UE4SS-settings.ini to activate.
	[Debug]
	; Whether to enable the external UE4SS debug console.
	ConsoleEnabled = 1
	GuiConsoleEnabled = 1
	GuiConsoleVisible = 1

![image](https://user-images.githubusercontent.com/73571427/202961241-2dd5f867-c094-42aa-9cd5-a6f46c7d650a.png)


* **Watches** - Allows watching of values in an object instance. Can print to a new tab or print to a file with timestamps.

![image](https://user-images.githubusercontent.com/73571427/202961288-3431b452-cd5a-41cb-8143-bf98fe7a85ac.png)

![image](https://user-images.githubusercontent.com/73571427/202961315-d1339664-264a-41c7-ac54-7957720b3634.png)


* **Dumpers - NEW:** Static Mesh dumper and All Actor dumper - Dumps positions of all loaded SM or all actors to a CSV file which can be loaded in UE to recreate a map or spawn actors at specific locations.  Very early feature, will allow for more properties to be dumped over time.

### Note: Actor dumper requires a specific dll to be compiled correctly for <=4.19 and =>4.20 for now.  Download the version for the engine version you will be working on if you'd like to use the dumper.
Download UE419-v2.0 for 4.19 and below.
Download UE420-v2.0 for 4.20 and above.

![image](https://user-images.githubusercontent.com/73571427/202961337-6c646fe7-8ba0-4148-b29c-e1aa7234b37f.png)


Dumper also requires companion BP to spawn everything in editor (Barebones for now but will be updated).  See readme within.

## Lua:
* The global function "RegisterHook" now returns two integers that represent pre & post callback ids.
  These ids can be passed to the "UnregisterHook" global function to unhook a function.
* Note that the callbacks for "RegisterHook" and "NotifyOnNewObject" execute in the game thread so the code in these callbacks needs to be careful when accessing Lua variables outside the callbacks.
* Added the global function "UnregisterHook", see API.txt for more information.
* Added better support for TArray. Now it can be used as out parameter in functions correctly.

## Fixes
* Fixed a rare crash that could occur when interacting with parameters in callbacks passed to "RegisterHook".
* Fixed a bug which prohibited using variables, containing unreal objects created in other thread (in main from game and vise versa)
* Fixed most issues with GC of cirtain Lau functions.

v1.3.6
==============
2022-11-03

New
* Mods that have 'enabled.txt' present in their mod directory are now always loaded regardless of their state in mods.txt.

Lua
* Added better support for StrProperty/FString.
  They can now be set like any other variable (Lua literal strings get auto-converted to FStrings).
  Parameters for "RegisterHook" that are of type FString are now also supported.
* Added support for SoftClassProperty/TSoftClassPtr.
* Added support for FSoftObjectPath and TSoftClassPtr, see API.txt for more information.

Fixes
* Fixed a bug that caused an occasional crash on startup.
* Fix to accuracy of Instanced specifiers in UHT dumps.

Known Issues
*Issue with GC of certain Lua functions being run on main thread causing crashes after extended use.

v1.3.5-RE
==============
2022-11-03

UE4SS is being rehosted.

The original creator no longer wishes to be involved with or connected to this project, or to receive messages related to it. Please respect their wishes, and avoid using their past username(s) in connection with this project. It is being reuploaded as open source with their permission.  Support for/updates to this project will now be very limited.

Reupload of Release 1.3.5 from the original repository until 1.3.6 is ready (likely later today or tomorrow).  This release is not different from the prior 1.3.5 version that was available.

Changelog from 1.3.4

New

	* Fixed a bug in the UHT generator that caused some properties to have the "Export" parameter for the UPROPERTY macro instead of "Instanced".UE4SS is being rehosted.

The original creator no longer wishes to be involved with or connected to this project, or to receive messages related to it. Please respect their wishes, and avoid using their past username(s) in connection with this project. It is being reuploaded as open source with their permission.  Support for/updates to this project will now be very limited.

Reupload of Release 1.3.5 from the original repository until 1.3.6 is ready (likely later today or tomorrow).  This release is not different from the prior 1.3.5 version that was available.

Changelog from 1.3.4

New

	* Fixed a bug in the UHT generator that caused some properties to have the "Export" parameter for the UPROPERTY macro instead of "Instanced".
