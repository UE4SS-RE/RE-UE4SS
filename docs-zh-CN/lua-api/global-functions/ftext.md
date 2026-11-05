# FText

`FText` 函数用于从 `string` 创建一个 `FText` 对象。

当你需要为 mod 的 UI 与 `UserWidget` 相关类交互，并调用它们的 `SetText(FText("My New Text"))` 等方法时，它很有用。

## 参数（重载 #1）

这会根据所给字符串创建一个新的 `FText` 对象。

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 用于创建 FText 的内容 |

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | FText | 包含所传 `string` 的 FText 对象 |

## 示例

代码：

```lua
local my_text = FText("My Text")
print(string.format("Lua type: %s\n", type(my_text)))
print(string.format("Object type: %s\n", my_text:type()))
print(string.format("Content: %s\n", my_text:ToString()))
```

输出：

```
[Lua] Lua type: userdata
[Lua] Object type: FText
[Lua] Content: My Text
```
