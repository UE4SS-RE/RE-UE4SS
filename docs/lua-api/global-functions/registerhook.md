# RegisterHook

The `RegisterHook` function is used to hook a UFunction.

> Any `UFunction` that you attempt to register with `RegisterHook` must already exist in memory when you register it.  

## Parameters

| # | Type     | Sub Type    | Information |
|---|----------|-------------|-------------|
| 1 | string   |             | Full name of the UFunction to hook. Type prefix has no effect. |
| 2 | function |             | Callback to execute when the UFunction is executed |

## Example
```lua
RegisterHook("/Script/Engine.PlayerController:ClientRestart", function()
    print("PlayerController restarted\n")
end)
```