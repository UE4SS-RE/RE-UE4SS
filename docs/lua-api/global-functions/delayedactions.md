# Delayed Actions

The Delayed Actions API provides a comprehensive timer system for executing code after delays, with full control over timing, pausing, cancellation, and looping. This system is similar to Unreal Engine's timer system.

## Overview

All delayed actions execute on the game thread and return handles that can be used to control them. Actions are owned by the mod that created them, preventing cross-mod interference.

## Creating Delayed Actions

### ExecuteInGameThreadWithDelay

Executes a callback after a specified delay.

**Overload 1: Auto-generated handle**
```lua
local handle = ExecuteInGameThreadWithDelay(delayMs, callback)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Delay in milliseconds before executing |
| 2 | function | Callback to execute |
| Return | integer | Handle for the created action |

**Overload 2: User-provided handle (UE Delay-style)**
```lua
ExecuteInGameThreadWithDelay(handle, delayMs, callback)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Handle (from `MakeActionHandle()`) |
| 2 | integer | Delay in milliseconds |
| 3 | function | Callback to execute |

This overload only creates the action if the handle is not already active. This mirrors Unreal Engine's `Delay` node behavior.

### RetriggerableExecuteInGameThreadWithDelay

Executes a callback after a delay, but resets the timer if called again with the same handle.

```lua
RetriggerableExecuteInGameThreadWithDelay(handle, delayMs, callback)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Handle (from `MakeActionHandle()`) |
| 2 | integer | Delay in milliseconds |
| 3 | function | Callback to execute |

This is useful for debouncing - each call resets the timer, so the callback only fires after the delay with no new calls.

### LoopInGameThreadWithDelay

Creates a repeating timer that executes the callback at regular intervals.

```lua
local handle = LoopInGameThreadWithDelay(delayMs, callback)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Delay in milliseconds between executions |
| 2 | function | Callback to execute |
| Return | integer | Handle for the created loop |

The loop continues until cancelled with `CancelDelayedAction(handle)`.

### ExecuteInGameThreadAfterFrames

Executes a callback after a specified number of frames.

```lua
local handle = ExecuteInGameThreadAfterFrames(frames, callback)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Number of frames to wait |
| 2 | function | Callback to execute |
| Return | integer | Handle for the created action |

**Note:** Requires EngineTick hook. Check `EngineTickAvailable` before using.

### LoopInGameThreadAfterFrames

Creates a repeating timer that executes every N frames.

```lua
local handle = LoopInGameThreadAfterFrames(frames, callback)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Number of frames between executions |
| 2 | function | Callback to execute |
| Return | integer | Handle for the created loop |

**Note:** Requires EngineTick hook. Check `EngineTickAvailable` before using.

### MakeActionHandle

Generates a unique handle for use with delay functions.

```lua
local handle = MakeActionHandle()
```

| Return | integer | A unique handle that can be used with delay functions |

## Controlling Delayed Actions

### CancelDelayedAction

Cancels a pending or active delayed action.

```lua
local success = CancelDelayedAction(handle)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Handle of the action to cancel |
| Return | boolean | True if the action was found and cancelled |

### PauseDelayedAction

Pauses a delayed action, preserving remaining time.

```lua
local success = PauseDelayedAction(handle)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Handle of the action to pause |
| Return | boolean | True if the action was found and paused |

### UnpauseDelayedAction

Resumes a paused delayed action.

```lua
local success = UnpauseDelayedAction(handle)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Handle of the action to unpause |
| Return | boolean | True if the action was found and unpaused |

### ResetDelayedActionTimer

Restarts the timer with its original delay.

```lua
local success = ResetDelayedActionTimer(handle)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Handle of the action to reset |
| Return | boolean | True if the action was found and reset |

### SetDelayedActionTimer

Changes the delay and restarts the timer.

```lua
local success = SetDelayedActionTimer(handle, newDelayMs)
```

| # | Type | Information |
|---|------|-------------|
| 1 | integer | Handle of the action to modify |
| 2 | integer | New delay in milliseconds |
| Return | boolean | True if the action was found and modified |

### ClearAllDelayedActions

Cancels all delayed actions belonging to the current mod.

```lua
local count = ClearAllDelayedActions()
```

| Return | integer | Number of actions that were cancelled |

**Note:** This only affects actions created by the calling mod. Other mods' actions are not affected.

## Querying Delayed Actions

### IsValidDelayedActionHandle

Checks if a handle refers to an existing, non-cancelled action.

```lua
local valid = IsValidDelayedActionHandle(handle)
```

### IsDelayedActionActive

Checks if an action is currently active (not paused, not pending removal).

```lua
local active = IsDelayedActionActive(handle)
```

### IsDelayedActionPaused

Checks if an action is currently paused.

```lua
local paused = IsDelayedActionPaused(handle)
```

### GetDelayedActionRate

Gets the configured delay for an action.

```lua
local delayMs = GetDelayedActionRate(handle)
```

Returns configured delay in milliseconds for time-based actions, or frames for frame-based actions. Returns -1 if handle is invalid.

### GetDelayedActionTimeRemaining

Gets the remaining time until the action fires.

```lua
local remainingMs = GetDelayedActionTimeRemaining(handle)
```

Returns remaining milliseconds for time-based actions, or remaining frames for frame-based actions. Returns -1 if handle is invalid.

### GetDelayedActionTimeElapsed

Gets the time elapsed since the action was started/reset.

```lua
local elapsedMs = GetDelayedActionTimeElapsed(handle)
```

Returns elapsed milliseconds for time-based actions, or elapsed frames for frame-based actions. Returns -1 if handle is invalid.

## Global Variables

### EngineTickAvailable

Boolean indicating if the EngineTick hook is available.

```lua
if EngineTickAvailable then
    -- Frame-based delays are available
end
```

### ProcessEventAvailable

Boolean indicating if the ProcessEvent hook is available.

```lua
if ProcessEventAvailable then
    -- ProcessEvent-based execution is available
end
```

## Examples

### Simple Delay

```lua
ExecuteInGameThreadWithDelay(2000, function()
    print("This prints after 2 seconds\n")
end)
```

### Self-Cancelling Loop

```lua
local counter = 0
local loopHandle  -- Declare first for closure capture
loopHandle = LoopInGameThreadWithDelay(1000, function()
    counter = counter + 1
    print(string.format("Tick %d\n", counter))
    if counter >= 5 then
        CancelDelayedAction(loopHandle)
        print("Loop stopped\n")
    end
end)
```

### Debounced Action

```lua
local debounceHandle = MakeActionHandle()

RegisterKeyBind(Key.F, function()
    -- Only fires 500ms after the last key press
    RetriggerableExecuteInGameThreadWithDelay(debounceHandle, 500, function()
        print("Debounced action fired\n")
    end)
end)
```

### Pausable Timer

```lua
local timerHandle = ExecuteInGameThreadWithDelay(10000, function()
    print("Timer completed\n")
end)

-- Pause after 2 seconds
ExecuteInGameThreadWithDelay(2000, function()
    PauseDelayedAction(timerHandle)
    print("Timer paused\n")
end)

-- Resume after 5 seconds
ExecuteInGameThreadWithDelay(5000, function()
    UnpauseDelayedAction(timerHandle)
    print("Timer resumed\n")
end)
```

### UE-Style Delay (Only Create If Not Exists)

```lua
local cooldownHandle = MakeActionHandle()

RegisterKeyBind(Key.E, function()
    -- Only creates the delay if not already active
    ExecuteInGameThreadWithDelay(cooldownHandle, 1000, function()
        print("Ability ready!\n")
    end)
    print("Ability used (1s cooldown)\n")
end)
```

### Frame-Based Delay

```lua
if EngineTickAvailable then
    ExecuteInGameThreadAfterFrames(60, function()
        print("Fired after 60 frames\n")
    end)
end
```

## Important Notes

1. **Closure Capture**: When using loop handles inside their own callbacks, declare the variable before assignment:
   ```lua
   local loopHandle  -- Declare first
   loopHandle = LoopInGameThreadWithDelay(1000, function()
       CancelDelayedAction(loopHandle)  -- Now correctly captured
   end)
   ```

2. **Mod Ownership**: Each mod can only control its own delayed actions. `CancelDelayedAction`, `PauseDelayedAction`, etc. will fail if called with a handle from another mod.

3. **Frame-Based Delays**: Require the EngineTick hook. Always check `EngineTickAvailable` before using `ExecuteInGameThreadAfterFrames` or `LoopInGameThreadAfterFrames`.

4. **Fallback Behavior**: If the default execution method is unavailable, the system will automatically fall back to the other method if available.
