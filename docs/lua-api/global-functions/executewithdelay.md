# ExecuteWithDelay

> **Deprecated.** For a short callback that can run on the game thread after a delay, use [`ExecuteInGameThreadWithDelay`](./delayedactions.md#executeingamethreadwithdelay). See the [migration guide](../../upgrade-guide.md#executeasync-executewithdelay-and-loopasync).

`ExecuteWithDelay` runs the callback on the mod's async worker after the delay elapses.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | integer  | Delay, in milliseconds, to wait before executing the supplied callback |
| 2 | function | The callback to execute after the supplied delay is over |

## Example
```lua
ExecuteWithDelay(2000, function()
    print("Executed asynchronously after a 2 second delay\n")
end)
```

## Recommended Alternative

```lua
ExecuteInGameThreadWithDelay(2000, function()
    print("Executed on the game thread after a 2 second delay\n")
end)
```
