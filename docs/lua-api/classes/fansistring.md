# FAnsiString

`FAnsiString` is Unreal Engine's ANSI-encoded string type, using single-byte character encoding.

## Inheritance
[LocalObject](./localobject.md)

## Related Types
- [FString](./fstring.md) - Wide-character (TCHAR) string type
- [FUtf8String](./futf8string.md) - UTF-8 encoded string type

## Methods

### ToString()

- **Return type:** `string`
- **Returns:** a Lua string with ANSI encoding.

### Empty()

- Empties the string by removing all characters.

### Clear()

- Empties the string by removing all characters (identical to Empty).

### Len()

- **Return type:** `integer`
- **Returns:** the length of the string in characters (each ANSI character is 1 byte).

### IsEmpty()

- **Return type:** `boolean`
- **Returns:** `true` if the string is empty, `false` otherwise.

### Append(string)

- **Parameter:** `string` - A Lua string or another FAnsiString to append
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

- **Return type:** `FAnsiString`
- **Returns:** a new FAnsiString with all characters converted to uppercase.

### ToLower()

- **Return type:** `FAnsiString`
- **Returns:** a new FAnsiString with all characters converted to lowercase.

## Example Usage

```lua
-- FAnsiString for ANSI-encoded data
local ansiString = FAnsiString("Hello ANSI")
print(ansiString:ToString()) -- Output: "Hello ANSI"

ansiString:Append(" World")
print(ansiString:ToString()) -- Output: "Hello ANSI World"

if ansiString:EndsWith("World") then
    print("String ends with World!")
end

local lowerString = ansiString:ToLower()
print(lowerString:ToString()) -- Output: "hello ansi world"

local index = ansiString:Find("ANSI")
if index then
    print("Found at index: " .. index)
end
```

## Notes

- `FAnsiString` uses single-byte ANSI encoding, making it suitable for ASCII-compatible text.
- This type is useful when interfacing with legacy systems or APIs that expect ANSI strings.
- For international text support, consider using [FUtf8String](./futf8string.md) or [FString](./fstring.md) instead.
