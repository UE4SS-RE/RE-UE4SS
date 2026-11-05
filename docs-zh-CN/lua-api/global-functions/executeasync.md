# ExecuteAsync

> **已弃用：** 该函数已被 [`ExecuteInGameThreadWithDelay`](./delayedactions.md#executeingamethreadwithdelay) 取代。新函数提供了取消、暂停/恢复以及状态查询能力。详情参见[迁移指南](../../upgrade-guide.md#executeasync-and-loopasync)。

`ExecuteAsync` 函数会异步执行所给的回调。

它的工作方式与 [ExecuteWithDelay](./executewithdelay.md) 类似，区别在于除了注册回调的开销之外没有额外延迟。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | function | 要执行的回调 |

## 示例

```lua
ExecuteAsync(function()
    print("Executed asynchronously\n")
end)
```

## 推荐的替代方案

```lua
-- 在下一帧/下一次 ProcessEvent 时于游戏线程执行
local handle = ExecuteInGameThreadWithDelay(0, function()
    print("Executed on game thread\n")
end)
-- 现在可以用 CancelDelayedAction(handle) 取消
```
