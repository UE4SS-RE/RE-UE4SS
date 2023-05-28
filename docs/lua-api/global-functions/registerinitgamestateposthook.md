# RegisterInitGameStatePostHook

This registers a callback that will get called after `AGameModeBase::InitGameState` is called.

Parameters (except strings & bools & `FOutputDevice`) must be retrieved via `Param:Get()` and set via `Param:Set()`.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | function | The callback to register |

## Callback Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | AGameStateBase | The game state context |

## Example

```lua
RegisterInitGameStatePostHook(function(GameState)
    print("InitGameStatePostHook")
end)
```