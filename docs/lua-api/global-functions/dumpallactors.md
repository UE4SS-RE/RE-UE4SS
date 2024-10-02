# DumpAllActors
Dumps all actors from memory to the file `(timestamp here)-ue4ss_actor_data.csv`.

The function does the same as the `Dump all actors to file` button in the UE4SS Debugging Tools aka. the GUI Console under the `Dumpers` tab.

## Example
```lua
RegisterKeyBind(Key.F1, function()
    DumpAllActors()
end)
```