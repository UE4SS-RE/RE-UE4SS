# LoopAsync

> **已弃用：** 该函数已被 [`LoopInGameThreadWithDelay`](./delayedactions.md#loopingamethreadwithdelay) 取代。新函数提供了取消、暂停/恢复以及状态查询能力。详情参见[迁移指南](../../upgrade-guide.md#executeasync-and-loopasync)。

启动一个循环，每次迭代休眠指定的毫秒数，并在回调返回 true 时停止。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 休眠的毫秒数 |
| 2 | function | 回调函数 |

## 示例

```lua
LoopAsync(1000, function()
    print("Hello World!")
    return false -- 无限循环
end)
```

## 推荐的替代方案

```lua
-- 使用 LoopInGameThreadWithDelay 获得更好的控制
local loopHandle
loopHandle = LoopInGameThreadWithDelay(1000, function()
    print("Hello World!")
    -- 可以从任何地方通过 handle 取消
    if shouldStop then
        CancelDelayedAction(loopHandle)
    end
end)

-- 优点：
-- 随时取消：CancelDelayedAction(loopHandle)
-- 暂停：PauseDelayedAction(loopHandle)
-- 恢复：UnpauseDelayedAction(loopHandle)
-- 查询状态：IsDelayedActionActive(loopHandle)
```
