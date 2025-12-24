# LoopAsync

> **Deprecated:** This function is deprecated in favor of [`LoopInGameThreadWithDelay`](./delayedactions.md#loopingamethreadwithdelay). The new function provides cancellation, pause/resume, and state querying capabilities. See the [migration guide](../../upgrade-guide.md#executeasync-and-loopasync) for details.

Starts a loop that sleeps for the supplied number of milliseconds and stops when the callback returns true.

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
-- Use LoopInGameThreadWithDelay for better control
local loopHandle
loopHandle = LoopInGameThreadWithDelay(1000, function()
    print("Hello World!")
    -- Can cancel from anywhere using the handle
    if shouldStop then
        CancelDelayedAction(loopHandle)
    end
end)

-- Benefits:
-- Cancel anytime: CancelDelayedAction(loopHandle)
-- Pause: PauseDelayedAction(loopHandle)
-- Resume: UnpauseDelayedAction(loopHandle)
-- Check state: IsDelayedActionActive(loopHandle)
```
