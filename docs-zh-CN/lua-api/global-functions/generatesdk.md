# GenerateSDK

在 `CXXHeaderDump` 目录中生成 C++ 头文件。

该函数的作用与 UE4SS 调试工具（即 GUI 控制台）中 `Dumpers` 标签页下的 `Dump CXX Headers` 按钮相同。

## 示例

```lua
RegisterKeyBind(Key.F1, function()
    GenerateSDK()
end)
```
