# GenerateLuaTypes
Generates Lua types for [Custom Lua Bindings](../../guides/using-custom-lua-bindings.md) in the `Mods/shared/types` directory.

The function does the same as the `Generate Lua Types` button in the UE4SS Debugging Tools aka. the GUI Console under the `Dumpers` tab.

## Example
```lua
RegisterKeyBind(Key.F1, function()
    GenerateLuaTypes()
end)
```