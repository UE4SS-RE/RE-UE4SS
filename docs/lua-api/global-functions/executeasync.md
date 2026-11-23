# ExecuteAsync

> **Deprecated.** For a short callback that can run on the game thread, use [`ExecuteInGameThread`](./executeingamethread.md). See the [migration guide](../../upgrade-guide.md#executeasync-executewithdelay-and-loopasync).

The `ExecuteAsync` function queues the supplied callback on the mod's async worker.

It works in a similar manner to [ExecuteWithDelay](./executewithdelay.md), except that there is no delay beyond the cost of registering the callback.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | function | The callback to execute|

## Example
```lua
ExecuteAsync(function()
    print("Executed asynchronously\n")
end)
```

## Recommended Alternative

```lua
ExecuteInGameThread(function()
    print("Executed on game thread\n")
end)
```
