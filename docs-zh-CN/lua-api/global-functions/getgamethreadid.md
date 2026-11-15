# GetGameThreadId

`GetGameThreadId` 函数可以获取游戏通常用于执行游戏逻辑的线程 id。

该线程通常也用于执行通过 `RegisterHook` 注册的回调，以及其他用于挂钩的函数。

请注意，通过 `RegisterHook` 注册的回调并不保证在游戏线程上执行，它们会在游戏（或引擎）设计它们执行的线程上运行。

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | [ThreadId](../classes/threadid.md) | |

## 示例

```lua
ExecuteInGameThread(function()
    print(string.format("Game Thread Id: %s\n", GetGameThreadId():ToString()))
end)
```
