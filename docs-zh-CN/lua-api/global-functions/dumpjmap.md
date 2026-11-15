# DumpJMAP

生成 `.jmap` 反射转储（JSON 格式，由 [trumank](https://github.com/trumank/jmap) 设计）。

转储内容包含类、函数、结构体和枚举的完整反射数据（包括属性偏移、大小与标志），类默认对象的属性值，以及近似的虚表布局。jmap 命令行工具可以将它转换为 `.usmap` 或 C++ 头文件，`ue_binja` 插件则可以把它应用到 Binary Ninja 数据库中。

该函数的作用与 UE4SS 调试工具（即 GUI 控制台）中 `Dumpers` 标签页下的 `Generate .jmap file` 按钮相同。

文件会写入 UE4SS 的工作目录，文件名为 `<GameName>-<EngineVersion>-<UE4SSCommitSHA>.jmap`。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | bool（可选） | 除原生类型外，是否还包含蓝图生成的类、结构体与枚举（以及它们的 CDO）。默认值：`true` |

## 示例

```lua
RegisterKeyBind(Key.F1, function()
    DumpJMAP()
end)

-- 仅原生类型：
RegisterKeyBind(Key.F2, function()
    DumpJMAP(false)
end)
```
