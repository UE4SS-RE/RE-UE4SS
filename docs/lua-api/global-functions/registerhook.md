# RegisterHook

The `RegisterHook` registers a callback for a `UFunction`

Callbacks are triggered when a `UFunction` is executed.

The callback params are: `UObject self`, `UFunctionParams..`.

Returns two ids, both of which must be passed to `UnregisterHook` if you want to unregister the hook.

> Any `UFunction` that you attempt to register with `RegisterHook` must already exist in memory when you register it.  

> `RegisterHook` doesn't support delegate functions!

## Parameters

| # | Type     | Information                                                                                                                                                         |
|---|----------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 1 | string   | Full name of the UFunction to hook. Type prefix has no effect.                                                                                                      |
| 2 | function | If UFunction path starts with `/Script/`: Callback to execute before the UFunction is executed.<br/>Otherwise: Callback to execute after the UFunction is executed. |
| 3 | function | (optional)<br/>If UFunction path starts with `/Script/`: Callback to execute after the UFunction is executed<br/>Otherwise: Param does nothing.                     |

## Return Values

| # | Type   | Information |
|---|--------|-------------|
| 1 | integer | The PreId of the hook |
| 2 | integer | The PostId of the hook |

## Example
```lua
PreId, PostId = RegisterHook("/Script/Engine.PlayerController:ClientRestart", function(Context, NewPawn)
    local PlayerController = Context:get()
    local Pawn = NewPawn:get()

    print("PayerController FullName: " .. PlayerController:GetFullName())
    if Pawn:IsValid() then
        print("NewPawn FullName: " .. Pawn:GetFullName())
    end

    if PreId then
        -- Unhook once the function has been called
        UnregisterHook("/Script/Engine.PlayerController:ClientRestart", PreId, PostId)
    end
end)
```