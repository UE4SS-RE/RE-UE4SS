# Changelog

Note: This is a stopgap release to add Jedi Survivor support, and the below changelog is incomplete.  Full changelog from 2.5.1 will be posted when version 2.6 is released.

# Fixes
Fix more detached submodules (including Lua 5.4.4 update)
Expose certain additional classes to the member variable and vtable configs.

# New
Add USMAP dumper extensions (Atenfyr)
Add Lua Type generator for use by a Lua language server. See https://marketplace.visualstudio.com/items?itemName=sumneko.lua and https://github.com/LuaLS/lua-language-server/wiki/Annotations
Font scaling setting for Live View

## Live View
Add "PlayerControlled" to liveview tab

## Lua
Add IsValid() to TArray
Fixed UnregisterHook
Expanded RegisterHook to work on BP functions where it previously only worked on native functions
Added UnregisterCustomEvent
Fixed bug that could cause hooked functions to not process the return value properly
Lua_lock implementation/threading fixes (ParcelRot)

## UHT Dumper
Remove predeclarations in .cpp files

## QoL
Add font scaling setting to live view GUI

### Experimental
C++ Modding API - Recommended to wait until 2.6 release to begin use.

