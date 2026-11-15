# EGameThreadMethod

`EGameThreadMethod` 是一个枚举，用于指定在游戏线程上执行代码时要使用的钩子。

## 取值

| 键 | 说明 |
|----|------|
| ProcessEvent | 通过 ProcessEvent 钩子执行。调用非常频繁（每帧多次）。 |
| EngineTick | 通过 EngineTick 钩子执行。每帧调用一次。 |

## 用法

```lua
-- 在使用特定方式之前先检查其可用性
if EngineTickAvailable then
    ExecuteInGameThread(function()
        print("Runs once per frame\n")
    end, EGameThreadMethod.EngineTick)
end

if ProcessEventAvailable then
    ExecuteInGameThread(function()
        print("Runs via ProcessEvent\n")
    end, EGameThreadMethod.ProcessEvent)
end
```

## 相关的全局变量

- `EngineTickAvailable` - 布尔值，当 EngineTick 钩子可用时为 true
- `ProcessEventAvailable` - 布尔值，当 ProcessEvent 钩子可用时为 true

## 说明

- 如果两种方式都不可用，游戏线程执行函数会抛出错误
- 默认方式可以在 `UE4SS-settings.ini` 中通过 `DefaultGameThreadExecutionMethod` 配置
- 当请求了特定方式但它不可用时，函数会回退到另一种可用的方式
- `ProcessEvent` 通常更可靠，但触发非常频繁
- `EngineTick` 更适合需要与帧同步的操作，并且是基于帧的延迟所必需的

## 另见

- [ExecuteInGameThread](../global-functions/executeingamethread.md)
- [延迟操作](../global-functions/delayedactions.md)
