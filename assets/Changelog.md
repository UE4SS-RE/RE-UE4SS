v2.6.0
==============
TBD
## New

### General
Added support for UE Version 5.2 games

Added additional AOB for `FName::ToString` - LongerWarrior

### C++ API
Finalize C++ API and add required dll to builds - LocalCC; Truman

Expose IMGui to C++ mods - Truman

Fixed initialization functions not being correctly called when a mod is restarted - LocalCC

Fixed attempted mod loading when cppsdk doesn't exist - LocalCC

Added `UFunction::RegisterPreHookForInstance` and `UFunction::RegisterPostHookForInstance`  
These functions work the same as `UFunction::RegisterPreHook`/`UFunction::RegisterPostHook` except the callback is only fired if the context matches the specified instance  
These new functions need to be handled with care as they can cause crashes if you don't validate that the instance you're passing during registration is valid inside the callback

Added overloads for `UObject::GetFunctionByName` and `UObject::GetFunctionByNameInChain` that take an `FName` instead of a string

Added `UEnum::NumEnums`, which returns the number of enum values for the enum

Added `UEnum::GenereateEnumPrefix`, which is the same as https://docs.unrealengine.com/5.2/en-US/API/Runtime/CoreUObject/UObject/UEnum/GenerateEnumPrefix/

### Live View
Can now view enum values in the Live View debugger

Added a search option to exclude objects of a class with a name containing the specified (case-sensitive) string

Added a checkbox that toggles search options globally, meaning when not searching

### UHT Dumper
Removed unnecessary explicit `_MAX` elements from enums

Fixed enums inappropriately using `uint8`

Made `FWeakObjectPtr` overridable unless used in a TArray or TMap

### Experimental
Added ExperimentalFeatures section to UE4SS-settings.ini.  All experimental features will default to being turned off.  To use referenced features, change the relevant config setting to = 1

Added ability to call UFunctions from Live View GUI


## Fixes

### General
Finish adding version 4.11 support

Fix case preserving names switch - LocalCC

Add common TArray instantiations

### Lua API
Fixed unregisterhook

Improved stability when using hooks or `ExecuteInGameThread`

## Changes

### USMap Dumper
Add additional extensions - Atenfyr; Archengius

Fix bug with enums with 256 entries - Atenfyr

### C++ API
The callbacks for all hook registration functions inside the `Unreal::Hook` namespace can now take lambdas that capture variables

### Sig Scanner
Change AOB Sig Scanner backend to use std::find for major performance increase - inspired by Truman

Scan for specified time rather than number of attempts due to speed increase

### Performance
Change to generators for certain major iterators - LocalCC

Improved performance for U/FProperty lookups

Improved performance for UFunction lookups

Improved performance of the live log, it's now O(n) - trumank

### BP Mod Loader
Add ability to specify load order - Okaetsu


## Misc.
Changes to IsValid for increased reliability

Add destroy listener

### Repo
Add automated release script - Truman

Change documentation build process - Truman; Buckminsterfullerene

Removed FMT dependency

### Build Process
Fix compiling for Clang - LocalCC; Narknon

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
