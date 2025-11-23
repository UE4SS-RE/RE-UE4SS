# DumpAllObjects
Dumps all objects from memory to the file  `UE4SS_ObjectDump.txt`.

The function does the same as the `Dump Objects & Properties` button in the UE4SS Debugging Tools aka. the GUI Console.

## Example
```lua
RegisterKeyBind(Key.F1, function()
    DumpAllObjects()
end)
```