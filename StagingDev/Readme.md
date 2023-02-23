# Unreal Engine 4/5 Scripting System

# Unreal Engine 4/5 Scripting System

Lua scripting system platform, SDK generator, live property editor and other dumping utilities for UE4/5 games. 

## Major features

- Lua scripting API: Write lua mods based on the UE object system
- Live property editor: Search, view, edit & watch the properties of every loaded object, great for debugging mods or figuring out how values are changed during runtime
- Generate Unreal Header Tool compatible C++ headers for creating a mirror .uproject for your game
- Generate standard C++ headers from reflected classes and blueprints, with offsets
- Unlock the game console
- Generate .usmap mapping files for unversioned properties
- Dump all loaded actors to file to generate .umaps in-editor

### The original creator no longer wishes to be involved in or connected to this project.  Please respect their wishes, and avoid using their past usernames in connection with this project.

### This tool has entered "Maintenance" mode. No more features will be added for the foreseeable future, but it will hopefully still be updated whenever new UE versions are released.

## Targeting UE Versions: From 4.12 To 5.1



The goal of UE4SS is not to be a plug-n-play solution that always works with every game.  
The goal is to have an underlying system that works for most games.  
You may need to update AOBs on your own, and there's a guide for that below.

## Links

[Full installation guide](https://github.com/UE4SS/UE4SS/blob/master/Guides/FullInstallationGuide.md)  
[Updating AOBs](https://github.com/UE4SS/UE4SS/blob/master/Guides/FixingBrokenAOBs.md)  
[Generating UHT compatible headers](https://github.com/UE4SS/UE4SS/wiki/Generating-UHT-compatible-headers)

## Credits

- **Original Creator** The original creator no longer wishes to be involved in or connected to  this project.  Please respect their wishes, and avoid using their past usernames in connection with this project.
- **Archengius**
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
- **Narknon**
  - Certain features/maintenance
- **Deadmoroz**
  - Certain features and Lua updates/maintenance
- **OutTheShade**
  - Unreal Mappings (USMAP) Generator
- **DmgVol**
  - Inspiration for map dumper

## Thanks to everyone who helped with testing

- GreenHouse
- Otis_Inf
- SunBeam
- Motoson
- hooter
- Synopis
- Buckinsterfullerene