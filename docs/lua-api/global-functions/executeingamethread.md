# ExecuteInGameThread

`ExecuteInGameThread` is a function that executes code on the game thread using either the ProcessEvent hook or the EngineTick hook.

It will execute as soon as the game has time to execute it.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | function | Callback to execute when the game has time |
| 2 | [EGameThreadMethod](../table-definitions/egamethreadmethod.md) | (Optional) The execution method to use. Defaults to the configured default method. |

## Example

```lua
-- Basic usage (uses default method from config)
ExecuteInGameThread(function()
    print("Hello from the game thread!\n")
end)

-- Explicit EngineTick method (once per frame)
if EngineTickAvailable then
    ExecuteInGameThread(function()
        print("Hello from EngineTick!\n")
    end, EGameThreadMethod.EngineTick)
end

-- Explicit ProcessEvent method
if ProcessEventAvailable then
    ExecuteInGameThread(function()
        print("Hello from ProcessEvent!\n")
    end, EGameThreadMethod.ProcessEvent)
end
```

## Related Global Variables

- `EngineTickAvailable` - Boolean indicating if the EngineTick hook is available
- `ProcessEventAvailable` - Boolean indicating if the ProcessEvent hook is available

## Notes

- If the specified method is not available, the function will fall back to the other method if available
- The default method can be configured in `UE4SS-settings.ini` via `DefaultGameThreadExecutionMethod`
- `ProcessEvent` is called frequently (multiple times per frame), while `EngineTick` is called once per frame
