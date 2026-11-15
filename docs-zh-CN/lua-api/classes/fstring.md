# FString

`FString` 是虚幻引擎标准的宽字符字符串类型（基于 TCHAR）。

## 继承
[LocalObject](./localobject.md)

## 相关类型

- [FUtf8String](./futf8string.md) - UTF-8 编码的字符串类型
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
- **返回：** 字符串的字符长度。

### IsEmpty()

- **返回类型：** `boolean`
- **返回：** 字符串为空时返回 `true`，否则返回 `false`。

### Append(string)

- **参数：** `string` - 要追加的 Lua 字符串或另一个 FString
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

- **返回类型：** `FString`
- **返回：** 所有字符都转换为大写的新 FString。

### ToLower()

- **返回类型：** `FString`
- **返回：** 所有字符都转换为小写的新 FString。

## 用法示例

```lua
local myString = FString("Hello")
myString:Append(" World")
print(myString:ToString()) -- 输出："Hello World"

if myString:StartsWith("Hello") then
    print("String starts with Hello!")
end

local upperString = myString:ToUpper()
print(upperString:ToString()) -- 输出："HELLO WORLD"

local index = myString:Find("World")
print(index) -- 输出：7（从 1 开始的索引）
```
