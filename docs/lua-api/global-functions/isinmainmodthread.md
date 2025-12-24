# IsInMainModThread

`IsInMainModThread` is a function that allows you to check whether the thread of the current execution context is the main mod thread that executes main.lua.

## Return Value

| # | Type    | Information |
|---|---------|-------------|
| 1 | boolean |             |

## Example

```lua
-- Output: true
print(string.format("IsInMainThread: %s\n", IsInMainModThread()))
ExecuteInGameThread(function()
    -- Output: false
    print(string.format("IsInMainThread: %s\n", IsInMainModThread()))
end)
```
