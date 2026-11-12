# IsInAsyncThread

`IsInAsyncThread` 函数用于检查当前执行上下文所在线程，是否为执行 `ExecuteAsync`、`LoopAsync` 等异步函数所注册回调的线程。

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | boolean | |

## 示例

```lua
-- 输出：false
print(string.format("IsInAsyncThread: %s\n", IsInAsyncThread()))
ExecuteInGameThread(function()
    -- 输出：false
    print(string.format("IsInAsyncThread: %s\n", IsInAsyncThread()))
end)
ExecuteAsync(function()
    -- 输出：true
    print(string.format("IsInAsyncThread: %s\n", IsInAsyncThread()))
end)
```
