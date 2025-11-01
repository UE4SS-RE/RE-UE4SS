# FString

`FString` is Unreal Engine's standard wide-character string type (TCHAR-based).

## Inheritance
[LocalObject](./localobject.md)

## Related Types
- [FUtf8String](./futf8string.md) - UTF-8 encoded string type
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
- **Returns:** the length of the string in characters.

### IsEmpty()

- **Return type:** `boolean`
- **Returns:** `true` if the string is empty, `false` otherwise.

### Append(string)

- **Parameter:** `string` - A Lua string or another FString to append
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

- **Return type:** `FString`
- **Returns:** a new FString with all characters converted to uppercase.

### ToLower()

- **Return type:** `FString`
- **Returns:** a new FString with all characters converted to lowercase.

## Example Usage

```lua
local myString = FString("Hello")
myString:Append(" World")
print(myString:ToString()) -- Output: "Hello World"

if myString:StartsWith("Hello") then
    print("String starts with Hello!")
end

local upperString = myString:ToUpper()
print(upperString:ToString()) -- Output: "HELLO WORLD"

local index = myString:Find("World")
print(index) -- Output: 7 (1-based index)
```