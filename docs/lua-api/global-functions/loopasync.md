# LoopAsync

Calling `LoopAsync` now raises a Lua error. Schedule repeating game-thread callbacks with [`LoopInGameThreadWithDelay`](./delayedactions.md#loopingamethreadwithdelay). To stop the loop, pass its handle to `CancelDelayedAction`. See the [migration guide](../../upgrade-guide.md#executeasync-executewithdelay-and-loopasync).

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
