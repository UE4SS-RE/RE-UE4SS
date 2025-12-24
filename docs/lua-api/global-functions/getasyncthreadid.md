# GetAsyncThreadId

`GetAsyncThreadId` is a function that allows you to get the id for the thread that executes callbacks registered via ExecuteAsync, LoopAsync, and other async functions.

## Return Value

| # | Type                               | Information |
|---|------------------------------------|-------------|
| 1 | [ThreadId](../classes/threadid.md) |             |

## Example

```lua
ExecuteAsync(function()
    print(string.format("Async Thread Id: %s\n", GetAsyncThreadId():ToString()))
end)
```
