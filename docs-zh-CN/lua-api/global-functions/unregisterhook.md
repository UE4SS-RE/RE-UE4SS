# UnregisterHook

`UnregisterHook` 用于取消为某个 `UFunction` 注册的回调。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要挂钩的 UFunction 完整名称。类型前缀不起作用。 |
| 2 | integer | 该钩子的 PreId |
| 3 | integer | 该钩子的 PostId |

## 示例

```lua
local preId, postId = RegisterHook("/Script/Engine.PlayerController:ClientRestart", function()
    print("PlayerController restarted\n")
end)

UnregisterHook("/Script/Engine.PlayerController:ClientRestart", preId, postId)
```
