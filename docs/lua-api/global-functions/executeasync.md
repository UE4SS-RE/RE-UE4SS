# ExecuteAsync

> **Deprecated:** This function is deprecated in favor of [`ExecuteInGameThreadWithDelay`](./delayedactions.md#executeingamethreadwithdelay). The new function provides cancellation, pause/resume, and state querying capabilities. See the [migration guide](../../upgrade-guide.md#executeasync-and-loopasync) for details.

The `ExecuteAsync` function asynchronously executes the supplied callback.

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
-- Executes on the game thread on the next frame/ProcessEvent
local handle = ExecuteInGameThreadWithDelay(0, function()
    print("Executed on game thread\n")
end)
-- Can now cancel with: CancelDelayedAction(handle)
```
