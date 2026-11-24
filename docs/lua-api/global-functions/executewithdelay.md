# ExecuteWithDelay

Calling `ExecuteWithDelay` now raises a Lua error. Schedule a short callback after a delay in milliseconds with [`ExecuteInGameThreadWithDelay`](./delayedactions.md#executeingamethreadwithdelay). See the [migration guide](../../upgrade-guide.md#executeasync-executewithdelay-and-loopasync).

```lua
ExecuteInGameThreadWithDelay(2000, function()
    print("Executed on the game thread after a 2 second delay\n")
end)
```
