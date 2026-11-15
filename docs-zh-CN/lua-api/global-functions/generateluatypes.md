# GenerateLuaTypes

在 `Mods/shared/types` 目录中为 [自定义 Lua 绑定](../../guides/using-custom-lua-bindings.md) 生成 Lua 类型定义。

该函数的作用与 UE4SS 调试工具（即 GUI 控制台）中 `Dumpers` 标签页下的 `Generate Lua Types` 按钮相同。

## 示例

```lua
RegisterKeyBind(Key.F1, function()
    GenerateLuaTypes()
end)
```
