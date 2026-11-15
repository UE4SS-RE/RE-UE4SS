# IsInMainModThread

`IsInMainModThread` 函数用于检查当前执行上下文所在线程，是否为执行 `main.lua` 的主 mod 线程。

## 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | boolean | |

## 示例

```lua
-- 输出：true
print(string.format("IsInMainThread: %s\n", IsInMainModThread()))
ExecuteInGameThread(function()
    -- 输出：false
    print(string.format("IsInMainThread: %s\n", IsInMainModThread()))
end)
```
