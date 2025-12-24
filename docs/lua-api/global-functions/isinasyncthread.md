# IsInAsyncThread

`IsInAsyncThread` is a function that allows you to check whether the thread of the current execution context is the thread that executes callbacks registered via ExecuteAsync, LoopAsync, and other async functions.

## Return Value

| # | Type    | Information |
|---|---------|-------------|
| 1 | boolean |             |

## Example

```lua
-- Output: false
print(string.format("IsInAsyncThread: %s\n", IsInAsyncThread()))
ExecuteInGameThread(function()
    -- Output: false
    print(string.format("IsInAsyncThread: %s\n", IsInAsyncThread()))
end)
ExecuteAsync(function()
    -- Output: true
    print(string.format("IsInAsyncThread: %s\n", IsInAsyncThread()))
end)
```
