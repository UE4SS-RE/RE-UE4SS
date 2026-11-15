# LoadAsset

`LoadAsset` 函数按名称加载资源。

> 它只能在游戏线程中调用。例如在 `UFunction` 钩子或 `RegisterConsoleCommandHandler` 回调内部调用。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 资源的路径与名称 |

## 示例

```lua
RegisterConsoleCommandHandler("summon", function(FullCommand, Parameters)
    if #Parameters < 1 then return false end

    -- Parameters[1] 示例：/Game/LevelElements/Refinery/Pipeline/BP_Pipeline_Start
    LoadAsset(Parameters[1])

    return false
end)
```
