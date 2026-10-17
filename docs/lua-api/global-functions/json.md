# json

The `json` global provides JSON encoding and decoding functionality, powered by the [glaze](https://github.com/stephenberry/glaze) library.

## json.encode

Converts a Lua value to a JSON string.

```lua
local jsonString, error = json.encode(value, pretty)
```

| # | Type | Information |
|---|------|-------------|
| 1 | any | The Lua value to encode (nil, boolean, number, string, or table) |
| 2 | boolean? | If `true`, format with indentation and newlines (default: `false`) |
| Return 1 | string? | JSON string on success, `nil` on failure |
| Return 2 | string? | Error message on failure, `nil` on success |

### Supported Types

| Lua Type | JSON Output |
|----------|-------------|
| `nil` | `null` |
| `boolean` | `true` / `false` |
| `number` (integer) | `123` |
| `number` (float) | `3.14` |
| `string` | `"text"` |
| table (array*) | `[1, 2, 3]` |
| table (object) | `{"key": "value"}` |

\* Tables with sequential integer keys starting at 1 are encoded as JSON arrays.

### Unsupported Types

The following Lua types cannot be encoded and will cause `json.encode` to return `nil` with an error message:

- `function`
- `userdata` (UObjects, etc.)
- `thread` (coroutines)

### Examples

```lua
-- Simple encoding
local str = json.encode({ name = "test", count = 42 })
-- Result: {"count":42,"name":"test"}

-- Pretty printing
local pretty = json.encode({ a = 1, b = 2 }, true)
-- Result:
-- {
--   "a": 1,
--   "b": 2
-- }

-- Array encoding (sequential integer keys)
local arr = json.encode({ "apple", "banana", "cherry" })
-- Result: ["apple","banana","cherry"]

-- Error handling
local result, err = json.encode({ callback = function() end })
if not result then
    print("Encode failed: " .. err)
end
```

## json.decode

Parses a JSON string into a Lua value.

```lua
local value, error = json.decode(jsonString)
```

| # | Type | Information |
|---|------|-------------|
| 1 | string | The JSON string to parse |
| Return 1 | any | Decoded Lua value on success, `nil` on failure |
| Return 2 | string? | Error message on failure, `nil` on success |

### JSON to Lua Type Mapping

| JSON Type | Lua Type |
|-----------|----------|
| `null` | `nil` |
| `true` / `false` | `boolean` |
| number (integer) | `number` (integer) |
| number (float) | `number` (float) |
| `"string"` | `string` |
| `[array]` | table with 1-based integer keys |
| `{object}` | table with string keys |

### Examples

```lua
-- Parse JSON object
local data = json.decode('{"name": "test", "value": 42}')
print(data.name)   -- "test"
print(data.value)  -- 42

-- Parse JSON array (becomes 1-indexed Lua table)
local arr = json.decode('[10, 20, 30]')
print(arr[1])  -- 10
print(arr[2])  -- 20
print(#arr)    -- 3

-- Nested structures
local nested = json.decode('{"users": [{"name": "Alice"}, {"name": "Bob"}]}')
print(nested.users[1].name)  -- "Alice"

-- Error handling
local result, err = json.decode("not valid json")
if not result then
    print("Parse error: " .. err)
end

-- Handle potentially invalid input
local function safeParse(str)
    local data, err = json.decode(str)
    if data then
        return data
    else
        print("JSON error: " .. err)
        return nil
    end
end
```

## Common Use Cases

### Configuration Files

```lua
-- Read config
local configFile = io.open("config.json", "r")
if configFile then
    local content = configFile:read("*a")
    configFile:close()

    local config, err = json.decode(content)
    if config then
        print("Loaded setting: " .. config.someSetting)
    end
end

-- Write config
local settings = { volume = 0.8, fullscreen = true }
local configFile = io.open("config.json", "w")
if configFile then
    configFile:write(json.encode(settings, true))
    configFile:close()
end
```

### Data Serialization with AsyncCompute

```lua
-- JSON works well with AsyncCompute for passing complex data
AsyncCompute("my_task", {
    json_payload = json.encode({ query = "test", limit = 100 })
}, function(result)
    if result.success then
        local response = json.decode(result.result.json_response)
        -- Process response
    end
end)
```

### Debug Logging

```lua
-- Pretty-print complex tables for debugging
local complexData = GetSomeGameData()
print("Debug data:\n" .. json.encode(complexData, true))
```

## Notes

- Both functions use the return pattern `value, error` where only one is non-nil
- JSON object key order is not guaranteed to match insertion order
- Very large numbers may lose precision when round-tripping through JSON (JavaScript number limitations)
- Binary data in strings should be base64 encoded before JSON encoding
