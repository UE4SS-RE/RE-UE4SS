# Experimental Changelog
Fix more detached submodules (including Lua 5.4.4 update)
Add USMAP dumper extensions (Atenfyr)

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