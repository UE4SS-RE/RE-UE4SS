# FUtf8String

`FUtf8String` 是虚幻引擎的 UTF-8 编码字符串类型，针对 UTF-8 字符数据做过优化。

## 继承
[LocalObject](./localobject.md)

## 相关类型

- [FString](./fstring.md) - 宽字符（TCHAR）字符串类型
- [FAnsiString](./fansistring.md) - ANSI 编码的字符串类型

## 方法

### ToString()

- **返回类型：** `string`
- **返回：** UTF-8 编码的 Lua 字符串。

### Empty()

- 通过移除所有字符来清空字符串。

### Clear()

- 通过移除所有字符来清空字符串（与 Empty 相同）。

### Len()

- **返回类型：** `integer`
- **返回：** UTF-8 编码单元（字节）的数量，而不是 Unicode 码点的数量。
例如 `"Hello 你好"` 返回 14（字节），而不是 8（字符）。

### IsEmpty()

- **返回类型：** `boolean`
- **返回：** 字符串为空时返回 `true`，否则返回 `false`。

### Append(string)

- **参数：** `string` - 要追加的 Lua 字符串或另一个 FUtf8String
- 把给定字符串追加到本字符串末尾。

### Find(search)

- **参数：** `search` - 要搜索的 Lua 字符串
- **返回类型：** `integer` 或 `nil`
- **返回：** 搜索字符串首次出现的位置（从 1 开始的索引），未找到则返回 `nil`。

### StartsWith(prefix)

- **参数：** `prefix` - 要检查的 Lua 字符串
- **返回类型：** `boolean`
- **返回：** 字符串以给定前缀开头则返回 `true`，否则返回 `false`。

### EndsWith(suffix)

- **参数：** `suffix` - 要检查的 Lua 字符串
- **返回类型：** `boolean`
- **返回：** 字符串以给定后缀结尾则返回 `true`，否则返回 `false`。

### ToUpper()

- **返回类型：** `FUtf8String`
- **返回：** 把 ASCII 字符（a-z）转换为大写的新 FUtf8String。
- **⚠️ 注意：** 与虚幻引擎的行为一致，只有 ASCII 字符会被转换。多字节 UTF-8 字符（带变音符号的字符、西里尔字母、中文等）保持不变。例如 `"café"` 会变成 `"cafÉ"`，而不是 `"CAFÉ"`。

### ToLower()

- **返回类型：** `FUtf8String`
- **返回：** 把 ASCII 字符（A-Z）转换为小写的新 FUtf8String。
- **⚠️ 注意：** 与虚幻引擎的行为一致，只有 ASCII 字符会被转换。多字节 UTF-8 字符（带变音符号的字符、西里尔字母、中文等）保持不变。例如 `"CAFÉ"` 会变成 `"cafÉ"`，而不是 `"café"`。

## 用法示例

```lua
-- FUtf8String 可以无缝地处理 UTF-8 数据
local utf8String = FUtf8String("Hello UTF-8! 你好")
print(utf8String:ToString()) -- 输出："Hello UTF-8! 你好"

utf8String:Append(" 世界")
print(utf8String:ToString()) -- 输出："Hello UTF-8! 你好 世界"

if utf8String:StartsWith("Hello") then
    print("String starts with Hello!")
end

local upperString = utf8String:ToUpper()
print(upperString:ToString())

local index = utf8String:Find("UTF-8")
if index then
    print("Found at index: " .. index)
end
```

## 说明

- 处理国际化文本或需要确保 UTF-8 编码时，`FUtf8String` 特别有用。
- `Len()` 方法返回的是**字节长度**而非字符数量，因为 UTF-8 使用变长编码（每个字符 1-4 字节）。
- `Find()`、`StartsWith()` 与 `EndsWith()` 可以正确处理多字节 UTF-8 字符。
- `ToUpper()` 与 `ToLower()` 只转换 ASCII 字符（a-z、A-Z），这与虚幻引擎文档中描述的行为一致。这既是性能优化，也与 Epic Games 在所有字符串类型上的实现方式相符。

## 完整 UTF-8 大小写转换的替代方案

如果你需要对非 ASCII 字符进行大小写转换，可以考虑以下替代方案：

```lua
-- 方案 1：使用 Lua 的 string 库（如果可用且支持 UTF-8）
local text = "Café"
local upper = text:upper()  -- 视 Lua 的 locale 设置可能有效

-- 方案 2：通过 FString 转换（Windows 的 TCHAR 具有更好的 Unicode 支持）
local utf8str = FUtf8String("Café")
local fstr = FString(utf8str:ToString())
local upper = fstr:ToUpper()  -- 对大多数常见变音字符有效
local result = FUtf8String(upper:ToString())

-- 方案 3：在 Lua 中自行实现大小写转换
-- 使用支持 UTF-8 的库或 Unicode 大小写映射表
```
