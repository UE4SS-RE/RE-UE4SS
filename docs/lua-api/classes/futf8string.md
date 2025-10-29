# FUtf8String

`FUtf8String` is Unreal Engine's UTF-8 encoded string type, optimized for UTF-8 character data.

## Inheritance
[LocalObject](./localobject.md)

## Related Types
- [FString](./fstring.md) - Wide-character (TCHAR) string type
- [FAnsiString](./fansistring.md) - ANSI encoded string type

## Methods

### ToString()

- **Return type:** `string`
- **Returns:** a UTF-8 encoded Lua string.

### Empty()

- Empties the string by removing all characters.

### Len()

- **Return type:** `integer`
- **Returns:** the length of the string in bytes (UTF-8 encoded).

### IsEmpty()

- **Return type:** `boolean`
- **Returns:** `true` if the string is empty, `false` otherwise.

### Append(string)

- **Parameter:** `string` - A Lua string or another FUtf8String to append
- Appends the given string to the end of this string.

### Find(search)

- **Parameter:** `search` - A Lua string to search for
- **Return type:** `integer` or `nil`
- **Returns:** 1-based index of the first occurrence of the search string, or `nil` if not found.

### StartsWith(prefix)

- **Parameter:** `prefix` - A Lua string to check
- **Return type:** `boolean`
- **Returns:** `true` if the string starts with the given prefix, `false` otherwise.

### EndsWith(suffix)

- **Parameter:** `suffix` - A Lua string to check
- **Return type:** `boolean`
- **Returns:** `true` if the string ends with the given suffix, `false` otherwise.

### ToUpper()

- **Return type:** `FUtf8String`
- **Returns:** a new FUtf8String with all characters converted to uppercase.

### ToLower()

- **Return type:** `FUtf8String`
- **Returns:** a new FUtf8String with all characters converted to lowercase.

## Example Usage

```lua
-- FUtf8String works seamlessly with UTF-8 data
local utf8String = FUtf8String("Hello UTF-8! 你好")
print(utf8String:ToString()) -- Output: "Hello UTF-8! 你好"

utf8String:Append(" 世界")
print(utf8String:ToString()) -- Output: "Hello UTF-8! 你好 世界"

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

## Notes

- `FUtf8String` is particularly useful when working with international text or when you need to ensure UTF-8 encoding.
- The `Len()` method returns the byte length, not character count, as UTF-8 uses variable-length encoding.
- All string operations maintain proper UTF-8 encoding.
