# ExecuteInGameThread

`ExecuteInGameThread` is a function that allows you to execute code using `ProcessEvent`.

It will execute as soon as the game has time to execute it.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | function | Callback to execute when the game has time |

## Example

```lua
ExecuteInGameThread(function()
    print("Hello from the game thread!\n")
end)
```
