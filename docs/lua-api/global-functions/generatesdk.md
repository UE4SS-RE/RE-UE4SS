# GenerateSDK
Generates C++ Headers in the `CXXHeaderDump` directory.

The function does the same as the `Dump CXX Headers` button in the UE4SS Debugging Tools aka. the GUI Console under the `Dumpers` tab.

## Example
```lua
RegisterKeyBind(Key.F1, function()
    GenerateSDK()
end)
```