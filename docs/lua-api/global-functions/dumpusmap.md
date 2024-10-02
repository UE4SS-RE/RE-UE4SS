# DumpUSMAP
Generates an Unreal Mapping file `Mappings.usmap`.

The function does the same as the `Generate .usmap file UnrealMappingsDumper by OutTheShade` button in the UE4SS Debugging Tools aka. the GUI Console under the `Dumpers` tab.

## Example
```lua
RegisterKeyBind(Key.F1, function()
    DumpUSMAP()
end)
```