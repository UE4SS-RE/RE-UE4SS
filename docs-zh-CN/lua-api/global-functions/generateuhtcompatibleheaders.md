# GenerateUHTCompatibleHeaders

在 `UHTHeaderDump` 目录中生成 [Unreal Header Tool](https://dev.epicgames.com/documentation/de-de/unreal-engine/unreal-header-tool-for-unreal-engine) 头文件。

该函数的作用与 UE4SS 调试工具（即 GUI 控制台）中 `Dumpers` 标签页下的 `Generate UHT Compatible Headers` 按钮相同。

## 示例

```lua
RegisterKeyBind(Key.F1, function()
    GenerateUHTCompatibleHeaders()
end)
```
