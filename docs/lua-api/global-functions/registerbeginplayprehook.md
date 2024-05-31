# RegisterBeginPlayPreHook

This registers a callback that will get called before `AActor::BeginPlay` is called.

Parameters (except strings & bools & `FOutputDevice`) must be retrieved via `Param:Get()` and set via `Param:Set()`.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | function | The callback to register |

## Callback Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | AActor | The actor context |

## Example

```lua
RegisterBeginPlayPreHook(function(Actor)
    print("BeginPlayPreHook")
end)
```