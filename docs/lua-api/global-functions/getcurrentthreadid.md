# GetCurrentThreadId

`GetCurrentThreadId` is a function that allows you to get the id for the thread of the current execution context.

## Return Value

| # | Type                               | Information |
|---|------------------------------------|-------------|
| 1 | [ThreadId](../classes/threadid.md) |             |

## Example

```lua
ExecuteInGameThread(function()
    print(string.format("Current Thread Id: %s\n", GetCurrentThreadId():ToString()))
end)
```
