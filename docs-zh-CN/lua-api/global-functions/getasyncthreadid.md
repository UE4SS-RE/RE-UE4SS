# GetAsyncThreadId

`GetAsyncThreadId` 函数可以获取执行 `ExecuteAsync`、`LoopAsync` 等异步函数所注册回调的线程 id。

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | [ThreadId](../classes/threadid.md) | |

## 示例

```lua
ExecuteAsync(function()
    print(string.format("Async Thread Id: %s\n", GetAsyncThreadId():ToString()))
end)
```
