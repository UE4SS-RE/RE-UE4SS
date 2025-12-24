# AsyncCompute

The `AsyncCompute` function allows you to run computations on a worker thread without blocking the main thread or game thread. Results are safely returned to the main thread where you can access the full UE4SS Lua API.

## Overview

AsyncCompute is designed for CPU-intensive work that doesn't need to interact with game objects. The computation runs on a separate thread, and when complete, the callback is invoked on the main thread with the serialized result.

**Key constraints:**
- Input data must be serializable (primitives and tables only)
- Worker code cannot access UObjects, UE4SS bindings, or the main Lua state
- Results are serialized and passed back to the callback
- Callbacks run on the main thread (safe for full Lua API access)

## AsyncCompute

Submits a computation task to run on a worker thread.

```lua
local handle = AsyncCompute(taskName, input, callback)
```

| # | Type | Information |
|---|------|-------------|
| 1 | string | Name of a registered C++ task handler |
| 2 | table/nil | Input data (serializable primitives and tables only) |
| 3 | function | Callback to receive result |
| Return | integer | Handle to track/cancel the task (0 if task not found) |

The callback receives a result table with the following structure:

```lua
{
    success = boolean,  -- True if computation succeeded
    error = string?,    -- Error message if success is false
    result = any?       -- Computed result if success is true
}
```

## AsyncComputeLua

Runs pure Lua code on a worker thread in an isolated Lua state. Unlike `AsyncCompute`, this allows you to write custom computation logic in Lua without needing C++ task handlers.

```lua
local handle = AsyncComputeLua(sourceCode, input, callback, options?)
```

| # | Type | Information |
|---|------|-------------|
| 1 | string | Lua source code that returns a function |
| 2 | any | Input data (serializable primitives and tables only) |
| 3 | function | Callback to receive result |
| 4 | table? | Optional options table (see below) |
| Return | integer | Handle to track/cancel the task |

**Options table:**
```lua
{
    json = boolean  -- Add json.encode/decode to worker (default: false)
}
```

The source code must return a function that takes input and returns output:

```lua
local handle = AsyncComputeLua([[
    return function(input)
        -- Computation runs in isolated Lua state
        local result = 0
        for i = 1, input.iterations do
            result = result + math.sqrt(i)
        end
        return { sum = result }
    end
]], { iterations = 1000000 }, function(result)
    if result.success then
        print("Sum: " .. result.result.sum .. "\n")
    else
        print("Error: " .. result.error .. "\n")
    end
end)
```

### Isolated Environment

The worker runs in a completely independent Lua state with:

**Available:**
- `math.*` - All math functions
- `string.*` - String manipulation
- `table.*` - Table utilities
- `utf8.*` - UTF-8 support
- `io.*` - File I/O (use with care for threading)
- `os.*` - OS functions (time, clock, execute, etc.)
- `coroutine.*` - Coroutines within the worker
- Basic functions: `tonumber`, `tostring`, `type`, `pairs`, `ipairs`, `next`, `select`, `pcall`, `xpcall`, `error`, `assert`, `rawget`, `rawset`, `rawequal`, `rawlen`, `setmetatable`, `getmetatable`, `unpack`/`table.unpack`

**NOT available (for isolation):**
- `debug.*` - Could break isolation
- `load`, `loadfile`, `dofile` - Dynamic code loading
- `require`, `package.*` - Module system
- All UE4SS bindings - No `FindFirstOf`, `RegisterHook`, etc.
- Main state globals - Worker is completely independent

### JSON Support in Workers

Enable JSON with `{ json = true }`:

```lua
AsyncComputeLua([[
    return function(input)
        -- Parse incoming JSON
        local data = json.decode(input.json_string)

        -- Process...
        data.processed = true

        -- Return as JSON
        return { result_json = json.encode(data) }
    end
]], { json_string = '{"value": 42}' }, callback, { json = true })
```

## CancelAsyncCompute

Cancels a pending async computation.

```lua
local success = CancelAsyncCompute(handle)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Handle returned from AsyncCompute |
| Return | boolean | True if cancelled, false if already completed or invalid |

Note: If the task is already running, it cannot be cancelled (the callback will still fire).

## Built-in Task Handlers

### sleep

Sleeps for a specified duration. Useful for testing or simple delays that don't block.

**Input:**
```lua
{ ms = integer }  -- Milliseconds to sleep (default: 1000)
```

**Result:**
```lua
{ slept_ms = integer }  -- Actual sleep duration
```

### compute

Performs a CPU-intensive calculation (sum of square roots). Useful for testing serialization and thread safety.

**Input:**
```lua
{ iterations = integer }  -- Number of iterations (default: 1000000)
```

**Result:**
```lua
{
    sum = number,        -- Computed sum
    iterations = integer -- Iterations performed
}
```

## Examples

### Simple Async Sleep

```lua
AsyncCompute("sleep", { ms = 2000 }, function(result)
    if result.success then
        print(string.format("Slept for %d ms\n", result.result.slept_ms))
    else
        print("Sleep failed: " .. result.error .. "\n")
    end
end)
print("Sleep started (non-blocking)\n")
```

### CPU-Intensive Computation

```lua
-- Won't block the game while computing
AsyncCompute("compute", { iterations = 10000000 }, function(result)
    if result.success then
        print(string.format("Computed sum: %f (%d iterations)\n",
            result.result.sum, result.result.iterations))
    end
end)
```

### Cancelling a Task

```lua
local handle = AsyncCompute("sleep", { ms = 5000 }, function(result)
    print("This won't print if cancelled\n")
end)

-- Cancel after 1 second
ExecuteInGameThreadWithDelay(1000, function()
    if CancelAsyncCompute(handle) then
        print("Task cancelled\n")
    end
end)
```

### Using Result in Game Thread

```lua
AsyncCompute("compute", { iterations = 1000000 }, function(result)
    if result.success then
        -- We're on main thread, but to modify UObjects we still need game thread
        ExecuteInGameThread(function()
            local player = FindFirstOf("PlayerController")
            if player:IsValid() then
                -- Safe to access UObjects here
                print("Computed on worker, applied on game thread\n")
            end
        end)
    end
end)
```

### Lua Worker - Custom Computation

```lua
-- Run custom Lua code without a C++ handler
AsyncComputeLua([[
    return function(input)
        local sum = 0
        for i = 1, input.count do
            sum = sum + math.sqrt(i)
        end
        return { sum = sum, count = input.count }
    end
]], { count = 1000000 }, function(result)
    if result.success then
        print(string.format("Sum of %d square roots: %f\n",
            result.result.count, result.result.sum))
    end
end)
```

### Lua Worker - Data Processing with JSON

```lua
-- Parse and transform JSON data off the main thread
local jsonData = '{"users": [{"name": "Alice", "age": 30}, {"name": "Bob", "age": 25}]}'

AsyncComputeLua([[
    return function(input)
        local data = json.decode(input.json)

        -- Process users
        local adults = {}
        for _, user in ipairs(data.users) do
            if user.age >= 18 then
                table.insert(adults, user.name)
            end
        end

        return {
            adult_names = adults,
            count = #adults
        }
    end
]], { json = jsonData }, function(result)
    if result.success then
        print("Adults: " .. table.concat(result.result.adult_names, ", ") .. "\n")
    end
end, { json = true })
```

### Lua Worker - String Processing

```lua
-- Heavy string processing without blocking
AsyncComputeLua([[
    return function(input)
        local text = input.text
        local words = {}

        for word in text:gmatch("%S+") do
            table.insert(words, word:lower())
        end

        table.sort(words)

        return {
            word_count = #words,
            sorted = table.concat(words, " ")
        }
    end
]], { text = "The quick brown fox jumps over the lazy dog" }, function(result)
    if result.success then
        print(result.result.word_count .. " words: " .. result.result.sorted .. "\n")
    end
end)
```

## Input Serialization

AsyncCompute serializes your input data before passing it to the worker thread. Only these types are supported:

| Type | Supported |
|------|-----------|
| nil | Yes |
| boolean | Yes |
| number (integer) | Yes |
| number (float) | Yes |
| string | Yes |
| table (nested) | Yes |
| function | **No** - throws error |
| userdata (UObjects, etc.) | **No** - throws error |
| thread | **No** - throws error |

Tables can be nested, but circular references will throw an error.

## Thread Safety Notes

1. **Worker thread**: The C++ task handler runs on a thread pool. It has no access to Lua state.

2. **Result callback**: Runs on the UE4SS main thread (where `main.lua` executes). You have full access to the Lua API, but not direct UObject access.

3. **Game thread access**: If your callback needs to modify UObjects, wrap that code in `ExecuteInGameThread()`.

## Custom Task Handlers

C++ mods can register custom async task handlers:

```cpp
#include <AsyncCompute.hpp>

// In your mod's startup:
RC::AsyncComputeManager::get().register_task_handler("my_task",
    [](const RC::LuaType::LuaValue& input) -> RC::AsyncComputeResult {
        // This runs on a worker thread
        // Do your computation here

        // Return result
        std::vector<std::pair<RC::LuaType::LuaValue, RC::LuaType::LuaValue>> result;
        result.emplace_back(
            RC::LuaType::LuaValue{std::string("output")},
            RC::LuaType::LuaValue{42}
        );
        return RC::AsyncComputeResult::Success(RC::LuaType::LuaValue{std::move(result)});
    }
);
```

Then in Lua:

```lua
AsyncCompute("my_task", { ... }, function(result)
    print(result.result.output)  -- 42
end)
```

## Comparison with Other Async Functions

| Function | Thread | Use Case |
|----------|--------|----------|
| `ExecuteAsync` (deprecated) | Async thread | Simple delays (unsafe, deprecated) |
| `ExecuteInGameThreadWithDelay` | Game thread | Delayed game logic, UObject access |
| `AsyncCompute` | Worker pool | CPU-intensive computation (C++ handlers) |
| `AsyncComputeLua` | Worker pool | Custom Lua computation (no C++ needed) |

**When to use `AsyncCompute`:**
- You need a built-in task (sleep, compute)
- A C++ mod provides custom task handlers you want to use
- Maximum performance for frequently-used operations

**When to use `AsyncComputeLua`:**
- You want to write custom computation logic in Lua
- You don't want to write or depend on C++ code
- You need JSON parsing/generation in the worker
- One-off computations that don't justify a C++ handler
