# DumpUSMAP
Generates an Unreal Mapping file `Mappings.usmap`.

The function does the same as the `Generate .usmap file UnrealMappingsDumper by OutTheShade` button in the UE4SS Debugging Tools aka. the GUI Console under the `Dumpers` tab.

## Parameters
| # | Type | Information |
|---|------|-------------|
| 1 | bool (optional) | Whether to include Blueprint-generated classes, structs and enums in addition to native types. Default: `true` |

## Example
```lua
RegisterKeyBind(Key.F1, function()
    DumpUSMAP()
end)

-- Native types only:
RegisterKeyBind(Key.F2, function()
    DumpUSMAP(false)
end)
```