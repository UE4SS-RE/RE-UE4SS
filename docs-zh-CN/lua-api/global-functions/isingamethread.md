# IsInGameThread

`IsInGameThread` 函数用于检查当前执行上下文所在线程，是否为游戏通常用于执行游戏逻辑的线程。

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | boolean | |

## 示例

```lua
-- 输出：false
print(string.format("IsInAsyncThread: %s\n", IsInGameThread()))
ExecuteInGameThread(function()
    -- 输出：true
    print(string.format("IsInGameThread: %s\n", IsInGameThread()))
end)
ExecuteAsync(function()
    -- 输出：false
    print(string.format("IsInGameThread: %s\n", IsInGameThread()))
end)
```
