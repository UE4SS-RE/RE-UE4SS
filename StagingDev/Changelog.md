# v2.5.0 Changelog

## New Features

### Blueprint ModLoading
Added blueprint mod loading/actor spawning mod.
To enable, set BPModLoaderMod : 1 in Mods/mods.txt.
Blueprint mods go in /Game/Content/Paks/LogicMods

Creating BP mods is the same as using UML, a tutorial can be found here: https://www.youtube.com/watch?v=fB3yT85XhVA

Added creating LogicMods folder automatically.

Thank you to RussellJ for permission to implement this.

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
Fixed 'EFindName' not being exposed to Lua properly.


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

### Live View
Fixed a race condition in live view.
Fixed a crash when expanding an array of objects in live view.


## Other Changes

* 



