# DumpUSMAP

生成 Unreal Mapping 文件 `Mappings.usmap`。

该函数的作用与 UE4SS 调试工具（即 GUI 控制台）中 `Dumpers` 标签页下的 `Generate .usmap file UnrealMappingsDumper by OutTheShade` 按钮相同。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | bool（可选） | 除原生类型外，是否还包含蓝图生成的类、结构体与枚举。默认值：`true` |

## 示例

```lua
RegisterKeyBind(Key.F1, function()
    DumpUSMAP()
end)

-- 仅原生类型：
RegisterKeyBind(Key.F2, function()
    DumpUSMAP(false)
end)
```
