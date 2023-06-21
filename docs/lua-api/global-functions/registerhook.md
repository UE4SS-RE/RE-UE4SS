# RegisterHook

The `RegisterHook` registers a callback for a `UFunction`

Callbacks are triggered when a `UFunction` is executed.

The callback params are: `UObject self`, `UFunctionParams..`.

Returns two ids, both of which must be passed to `UnregisterHook` if you want to unregister the hook.

> Any `UFunction` that you attempt to register with `RegisterHook` must already exist in memory when you register it.  

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | Full name of the UFunction to hook. Type prefix has no effect. |
| 2 | function | Callback to execute when the UFunction is executed |

## Return Values

| # | Type   | Information |
|---|--------|-------------|
| 1 | integer | The PreId of the hook |
| 2 | integer | The PostId of the hook |

## Example
```lua
RegisterHook("/Script/Engine.PlayerController:ClientRestart", function()
    print("PlayerController restarted\n")
end)
```