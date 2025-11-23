# GenerateUHTCompatibleHeaders
Generates [Unreal Header Tool](https://dev.epicgames.com/documentation/de-de/unreal-engine/unreal-header-tool-for-unreal-engine) Headers in the `UHTHeaderDump` directory.

The function does the same as the `Generate UHT Compatible Headers` button in the UE4SS Debugging Tools aka. the GUI Console under the `Dumpers` tab.

## Example
```lua
RegisterKeyBind(Key.F1, function()
    GenerateUHTCompatibleHeaders()
end)
```