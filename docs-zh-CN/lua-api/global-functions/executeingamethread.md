# ExecuteInGameThread

`ExecuteInGameThread` 函数使用 ProcessEvent 钩子或 EngineTick 钩子，在游戏线程上执行代码。

游戏一旦有空闲，它就会立即执行。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | function | 游戏有空闲时要执行的回调 |
| 2 | [EGameThreadMethod](../table-definitions/egamethreadmethod.md) | （可选）要使用的执行方式。默认为配置的默认方式。 |

## 示例

```lua
-- 基本用法（使用配置中的默认方式）
ExecuteInGameThread(function()
    print("Hello from the game thread!\n")
end)

-- 显式使用 EngineTick 方式（每帧一次）
if EngineTickAvailable then
    ExecuteInGameThread(function()
        print("Hello from EngineTick!\n")
    end, EGameThreadMethod.EngineTick)
end

-- 显式使用 ProcessEvent 方式
if ProcessEventAvailable then
    ExecuteInGameThread(function()
        print("Hello from ProcessEvent!\n")
    end, EGameThreadMethod.ProcessEvent)
end
```

## 相关的全局变量

- `EngineTickAvailable` - 布尔值，指示 EngineTick 钩子是否可用
- `ProcessEventAvailable` - 布尔值，指示 ProcessEvent 钩子是否可用

## 说明

- 若指定的方式不可用，函数会回退到另一种可用的方式
- 默认方式可以在 `UE4SS-settings.ini` 中通过 `DefaultGameThreadExecutionMethod` 配置
- `ProcessEvent` 调用频繁（每帧多次），而 `EngineTick` 每帧只调用一次
