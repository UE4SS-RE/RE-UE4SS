# GetGameThreadId

`GetGameThreadId` is a function that allows you to get the id for the thread usually used by the game to execute game logic.

This thread is also usually used for callbacks registered via RegisterHook, and other functions used for hooking.

Note that callbacks registered via RegisterHook are not guaranteed to be execute the game thread, they execute wherever the game (or engine) designed them to execute.

## Return Value

| # | Type                               | Information |
|---|------------------------------------|-------------|
| 1 | [ThreadId](../classes/threadid.md) |             |

## Example

```lua
ExecuteInGameThread(function()
    print(string.format("Game Thread Id: %s\n", GetGameThreadId():ToString()))
end)
```
