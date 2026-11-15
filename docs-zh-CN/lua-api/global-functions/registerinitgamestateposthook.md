# RegisterInitGameStatePostHook

该函数注册一个回调，它会在 `AGameModeBase::InitGameState` 被调用之后执行。

参数（字符串、bool 以及 `FOutputDevice` 除外）必须通过 `Param:Get()` 获取，并通过 `Param:Set()` 设置。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | function | 要注册的回调 |

## 回调参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | AGameStateBase | 游戏状态上下文 |

## 示例

```lua
RegisterInitGameStatePostHook(function(GameState)
    print("InitGameStatePostHook")
end)
```
