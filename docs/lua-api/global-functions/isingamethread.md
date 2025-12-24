# IsInGameThread

`IsInGameThread` is a function that allows you to check whether the thread of the current execution context is the thread usually used by the game to execute game logic.

## Return Value

| # | Type    | Information |
|---|---------|-------------|
| 1 | boolean |             |

## Example

```lua
-- Output: false
print(string.format("IsInAsyncThread: %s\n", IsInGameThread()))
ExecuteInGameThread(function()
    -- Output: true
    print(string.format("IsInGameThread: %s\n", IsInGameThread()))
end)
ExecuteAsync(function()
    -- Output: false
    print(string.format("IsInGameThread: %s\n", IsInGameThread()))
end)
```
