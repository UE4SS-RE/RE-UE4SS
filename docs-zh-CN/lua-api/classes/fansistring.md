# FAnsiString

`FAnsiString` 是虚幻引擎的 ANSI 编码字符串类型，使用单字节字符编码。

## 继承
[LocalObject](./localobject.md)

## 相关类型

- [FString](./fstring.md) - 宽字符（TCHAR）字符串类型
- [FUtf8String](./futf8string.md) - UTF-8 编码的字符串类型

## 方法

### ToString()

- **返回类型：** `string`
- **返回：** 使用 ANSI 编码的 Lua 字符串。

### Empty()

- 通过移除所有字符来清空字符串。

### Clear()

- 通过移除所有字符来清空字符串（与 Empty 相同）。

### Len()

- **返回类型：** `integer`
- **返回：** 字符串的字符长度（每个 ANSI 字符占 1 字节）。

### IsEmpty()

- **返回类型：** `boolean`
- **返回：** 字符串为空时返回 `true`，否则返回 `false`。

### Append(string)

- **参数：** `string` - 要追加的 Lua 字符串或另一个 FAnsiString
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

- **返回类型：** `FAnsiString`
- **返回：** 所有字符都转换为大写的新 FAnsiString。

### ToLower()

- **返回类型：** `FAnsiString`
- **返回：** 所有字符都转换为小写的新 FAnsiString。

## 用法示例

```lua
-- 用于 ANSI 编码数据的 FAnsiString
local ansiString = FAnsiString("Hello ANSI")
print(ansiString:ToString()) -- 输出："Hello ANSI"

ansiString:Append(" World")
print(ansiString:ToString()) -- 输出："Hello ANSI World"

if ansiString:EndsWith("World") then
    print("String ends with World!")
end

local lowerString = ansiString:ToLower()
print(lowerString:ToString()) -- 输出："hello ansi world"

local index = ansiString:Find("ANSI")
if index then
    print("Found at index: " .. index)
end
```

## 说明

- `FAnsiString` 使用单字节 ANSI 编码，因此适合 ASCII 兼容的文本。
- 当需要与期望 ANSI 字符串的旧系统或 API 交互时，这个类型很有用。
- 若需要支持国际化文本，请考虑改用 [FUtf8String](./futf8string.md) 或 [FString](./fstring.md)。
