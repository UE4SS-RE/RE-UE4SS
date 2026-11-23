# LoopAsync

> **Deprecated.** For a short repeating callback that can run on the game thread, use [`LoopInGameThreadWithDelay`](./delayedactions.md#loopingamethreadwithdelay) and stop it with `CancelDelayedAction(handle)`. See the [migration guide](../../upgrade-guide.md#executeasync-executewithdelay-and-loopasync).

Runs the callback repeatedly on the mod's async worker after the specified delay. Returning `true` stops the loop.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | integer | The number of milliseconds to sleep |
| 2 | function | The callback function |

## Example

```lua
LoopAsync(1000, function()
    print("Hello World!")
    return false -- Loops forever
end)
```

## Recommended Alternative

```lua
local remaining = 3
local loopHandle
loopHandle = LoopInGameThreadWithDelay(1000, function()
    print("Hello World!")
    remaining = remaining - 1
    if remaining == 0 then
        CancelDelayedAction(loopHandle)
    end
end)
```
