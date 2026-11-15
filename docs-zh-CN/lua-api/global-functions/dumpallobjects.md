# DumpAllObjects

把内存中的所有对象转储到文件 `UE4SS_ObjectDump.txt`。

该函数的作用与 UE4SS 调试工具（即 GUI 控制台）中的 `Dump Objects & Properties` 按钮相同。

## 示例

```lua
RegisterKeyBind(Key.F1, function()
    DumpAllObjects()
end)
```
