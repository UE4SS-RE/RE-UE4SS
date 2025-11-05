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

### Clear()

- Empties the string by removing all characters (identical to Empty).

### Len()

- **Return type:** `integer`
- **Returns:** the number of UTF-8 code units (bytes), not Unicode code points. For example, "Hello 你好" returns 14 (bytes), not 8 (characters).

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
- **Returns:** a new FUtf8String with ASCII characters (a-z) converted to uppercase.
- **⚠️ Note:** Only ASCII characters are converted, matching Unreal Engine behavior. Multibyte UTF-8 characters (accented characters, Cyrillic, Chinese, etc.) are left unchanged. For example, `"café"` becomes `"cafÉ"`, not `"CAFÉ"`.

### ToLower()

- **Return type:** `FUtf8String`
- **Returns:** a new FUtf8String with ASCII characters (A-Z) converted to lowercase.
- **⚠️ Note:** Only ASCII characters are converted, matching Unreal Engine behavior. Multibyte UTF-8 characters (accented characters, Cyrillic, Chinese, etc.) are left unchanged. For example, `"CAFÉ"` becomes `"cafÉ"`, not `"café"`.


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
- The `Len()` method returns the **byte length**, not character count, as UTF-8 uses variable-length encoding (1-4 bytes per character).
- `Find()`, `StartsWith()`, and `EndsWith()` work correctly with multibyte UTF-8 characters.
- `ToUpper()` and `ToLower()` only convert ASCII characters (a-z, A-Z), matching Unreal Engine's documented behavior. This is a performance optimization and matches how Epic Games implements these functions across all string types.

## Workarounds for Full UTF-8 Case Conversion

If you need case conversion for non-ASCII characters, consider these alternatives:

```lua
-- Option 1: Use Lua's string library (if available and UTF-8 aware)
local text = "Café"
local upper = text:upper()  -- May work depending on Lua locale settings

-- Option 2: Convert through FString (Windows TCHAR has better Unicode support)
local utf8str = FUtf8String("Café")
local fstr = FString(utf8str:ToString())
local upper = fstr:ToUpper()  -- Works for most common accented characters
local result = FUtf8String(upper:ToString())

-- Option 3: Implement custom case conversion in Lua
-- Use UTF-8 aware libraries or Unicode case mapping tables
```
