# ExecuteAsync

Calling `ExecuteAsync` now raises a Lua error. Schedule short game-thread callbacks with [`ExecuteInGameThread`](./executeingamethread.md). See the [migration guide](../../upgrade-guide.md#executeasync-executewithdelay-and-loopasync).

```lua
ExecuteInGameThread(function()
    print("Executed on game thread\n")
end)
```
