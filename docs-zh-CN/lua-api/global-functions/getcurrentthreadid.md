# GetCurrentThreadId

`GetCurrentThreadId` 函数可以获取当前执行上下文所在线程的 id。

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | [ThreadId](../classes/threadid.md) | |

## 示例

```lua
ExecuteInGameThread(function()
    print(string.format("Current Thread Id: %s\n", GetCurrentThreadId():ToString()))
end)
```
