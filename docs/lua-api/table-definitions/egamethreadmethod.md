# EGameThreadMethod

`EGameThreadMethod` is an enum that specifies which hook to use for game thread execution.

## Values

| Key | Information |
|-----|-------------|
| ProcessEvent | Execute via the ProcessEvent hook. Called frequently (multiple times per frame). |
| EngineTick | Execute via the EngineTick hook. Called once per frame. |

## Usage

```lua
-- Check availability before using a specific method
if EngineTickAvailable then
    ExecuteInGameThread(function()
        print("Runs once per frame\n")
    end, EGameThreadMethod.EngineTick)
end

if ProcessEventAvailable then
    ExecuteInGameThread(function()
        print("Runs via ProcessEvent\n")
    end, EGameThreadMethod.ProcessEvent)
end
```

## Related Global Variables

- `EngineTickAvailable` - Boolean, true if EngineTick hook is available
- `ProcessEventAvailable` - Boolean, true if ProcessEvent hook is available

## Notes

- If neither method is available, game thread execution functions will throw an error
- The default method can be configured in `UE4SS-settings.ini` via `DefaultGameThreadExecutionMethod`
- When a specific method is requested but unavailable, functions will fall back to the other method if available
- `ProcessEvent` is generally more reliable but fires very frequently
- `EngineTick` is better for frame-synchronized operations and is required for frame-based delays

## See Also

- [ExecuteInGameThread](../global-functions/executeingamethread.md)
- [Delayed Actions](../global-functions/delayedactions.md)
