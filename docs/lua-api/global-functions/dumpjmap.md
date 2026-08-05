# DumpJMAP
Generates a `.jmap` reflection dump (JSON format by [trumank](https://github.com/trumank/jmap)).

The output is a superset of `.usmap`: it contains full reflection data for classes, functions, structs and enums (including property offsets, sizes and flags), class default object property values, and approximate vtable layouts. The jmap CLI can convert it to `.usmap` or C++ headers, and the `ue_binja` plugin can apply it to Binary Ninja databases.

The function does the same as the `Generate .jmap file` button in the UE4SS Debugging Tools aka. the GUI Console under the `Dumpers` tab.

The file is written to the UE4SS working directory as `<GameName>-<EngineVersion>-<UE4SSCommitSHA>.jmap`.

## Parameters
| # | Type | Information |
|---|------|-------------|
| 1 | bool (optional) | Whether to include Blueprint-generated classes, structs and enums (and their CDOs) in addition to native types. Default: `true` |

## Example
```lua
RegisterKeyBind(Key.F1, function()
    DumpJMAP()
end)

-- Native types only:
RegisterKeyBind(Key.F2, function()
    DumpJMAP(false)
end)
```
