# DumpStaticMeshes

把内存中所有静态 actor 网格体转储到文件 `(此处为时间戳)-ue4ss_static_mesh_data.csv`。

该函数的作用与 UE4SS 调试工具（即 GUI 控制台）中 `Dumpers` 标签页下的 `Dump all static actor meshes to file` 按钮相同。

## 示例

```lua
RegisterKeyBind(Key.F1, function()
    DumpStaticMeshes()
end)
```
