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
| `AsyncCompute` | Worker pool | CPU-intensive computation |

Use `AsyncCompute` when you have work that:
- Takes significant CPU time
- Doesn't need UObject access during computation
- Can have its result serialized as primitives/tables
