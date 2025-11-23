# RegisterHook

The `RegisterHook` registers a callback for a `UFunction`

Callbacks are triggered when a `UFunction` is executed.

The callback params are: `UObject self`, `UFunctionParams..`.

Returns two ids, both of which must be passed to `UnregisterHook` if you want to unregister the hook.

> Any `UFunction` that you attempt to register with `RegisterHook` must already exist in memory when you register it.  

> `RegisterHook` doesn't support delegate functions!

## RegisterHook Parameters

| # | Type     | Information                                                                                                                                                         |
|---|----------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | string   | Full name of the UFunction to hook. Type prefix has no effect.                                                                                                      |
| 2 | function | If UFunction path starts with `/Script/`: Callback to execute before the UFunction is executed.<br/>Otherwise: Callback to execute after the UFunction is executed. |
| 3 | function | (optional)<br/>If UFunction path starts with `/Script/`: Callback to execute after the UFunction is executed<br/>Otherwise: Param does nothing.                     |

## RegisterHook Return Values

| # | Type   | Information |
|---|--------|-------------|
| 1 | integer | The PreId of the hook |
| 2 | integer | The PostId of the hook |

## Callback Parameters
| # | Type   | Information |
|---|--------|-------------|
| 1 | [RemoteUnrealParam](../classes/remoteunrealparam.md) | Object representation of the "this"-pointer ("self" in lua) of the function, also known as "Context". It contains the object wrapped as RemoteUnrealParam that called the function.  |
| 2..N | RemoteUnrealParam... | All function parameters wrapped as RemoteUnrealParam |

## Callback Return Values

| # | Type | Information                                                                                                                                                                                                                             |
|---|------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | any  | An attempt will be made to automatically convert this value to a UE value, and the value will override the original function return value.<br>A value of `nil` (or no return statement) will leave the original return value unchanged. |

## Example
```lua
PreId, PostId = RegisterHook("/Script/Engine.PlayerController:ClientRestart", function(Context, NewPawn)
    local PlayerController = Context:get()
    local Pawn = NewPawn:get()

    print(string.format("PlayerController FullName: %s\n", PlayerController:GetFullName()))
    if Pawn:IsValid() then
        print(string.format("NewPawn FullName: %s\n", Pawn:GetFullName()))
    end

    if PreId then
        -- Unhook once the function has been called
        UnregisterHook("/Script/Engine.PlayerController:ClientRestart", PreId, PostId)
    end
end)
```