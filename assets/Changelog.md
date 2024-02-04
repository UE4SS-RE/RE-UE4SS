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
