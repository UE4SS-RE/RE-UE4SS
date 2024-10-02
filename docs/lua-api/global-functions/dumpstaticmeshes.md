# DumpStaticMeshes
Dumps all static actor meshes from memory to the file `(timestamp here)-ue4ss_static_mesh_data.csv`.

The function does the same as the `Dump all static actor meshes to file` button in the UE4SS Debugging Tools aka. the GUI Console under the `Dumpers` tab.

## Example
```lua
RegisterKeyBind(Key.F1, function()
    DumpStaticMeshes()
end)
```