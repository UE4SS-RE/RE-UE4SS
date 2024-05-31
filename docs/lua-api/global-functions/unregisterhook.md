# UnregisterHook

The `UnregisterHook` unregisters a callback for a `UFunction`.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | Full name of the UFunction to hook. Type prefix has no effect. |
| 2 | integer  | The PreId of the hook |
| 3 | integer  | The PostId of the hook |

## Example
```lua
local preId, postId = RegisterHook("/Script/Engine.PlayerController:ClientRestart", function()
    print("PlayerController restarted\n")
end)

UnregisterHook("/Script/Engine.PlayerController:ClientRestart", preId, postId)
```