# 延迟操作

延迟操作 API 提供了一套完整的计时器系统，可在延迟之后执行代码，并且能完全控制计时、暂停、取消与循环。该系统与虚幻引擎的计时器系统类似。

## 概览

所有延迟操作都在游戏线程上执行，并返回可用于控制它们的句柄。操作归创建它的 mod 所有，从而避免不同 mod 之间互相干扰。

## 创建延迟操作

### ExecuteInGameThreadWithDelay

在指定延迟之后执行回调。

**重载 1：自动生成句柄**

```lua
local handle = ExecuteInGameThreadWithDelay(delayMs, callback)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 执行前的延迟毫秒数 |
| 2 | function | 要执行的回调 |
| 返回 | integer | 所创建操作的句柄 |

**重载 2：用户提供句柄（UE Delay 风格）**

```lua
ExecuteInGameThreadWithDelay(handle, delayMs, callback)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 句柄（来自 `MakeActionHandle()`） |
| 2 | integer | 延迟毫秒数 |
| 3 | function | 要执行的回调 |

该重载只有在句柄尚未处于活动状态时才会创建操作。这与虚幻引擎 `Delay` 节点的行为一致。

### RetriggerableExecuteInGameThreadWithDelay

在延迟之后执行回调，但若以相同句柄再次调用，则会重置计时器。

```lua
RetriggerableExecuteInGameThreadWithDelay(handle, delayMs, callback)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 句柄（来自 `MakeActionHandle()`） |
| 2 | integer | 延迟毫秒数 |
| 3 | function | 要执行的回调 |

这适合用于防抖（debounce）——每次调用都会重置计时器，因此只有在延迟期间没有新调用时，回调才会触发。

### LoopInGameThreadWithDelay

创建一个按固定间隔重复执行回调的计时器。

```lua
local handle = LoopInGameThreadWithDelay(delayMs, callback)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 每次执行之间的延迟毫秒数 |
| 2 | function | 要执行的回调 |
| 返回 | integer | 所创建循环的句柄 |

循环会一直持续，直到通过 `CancelDelayedAction(handle)` 取消。

### ExecuteInGameThreadAfterFrames

在指定帧数之后执行回调。

```lua
local handle = ExecuteInGameThreadAfterFrames(frames, callback)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 要等待的帧数 |
| 2 | function | 要执行的回调 |
| 返回 | integer | 所创建操作的句柄 |

**注意：** 需要 EngineTick 钩子。使用前请检查 `EngineTickAvailable`。

### LoopInGameThreadAfterFrames

创建一个每隔 N 帧执行一次的重复计时器。

```lua
local handle = LoopInGameThreadAfterFrames(frames, callback)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 执行之间的帧数 |
| 2 | function | 要执行的回调 |
| 返回 | integer | 所创建循环的句柄 |

**注意：** 需要 EngineTick 钩子。使用前请检查 `EngineTickAvailable`。

### MakeActionHandle

生成一个用于延迟函数的唯一句柄。

```lua
local handle = MakeActionHandle()
```

| 返回 | integer | 可用于延迟函数的唯一句柄 |
|------|---------|--------------------------|

## 控制延迟操作

### CancelDelayedAction

取消一个待执行或已激活的延迟操作。

```lua
local success = CancelDelayedAction(handle)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 要取消的操作句柄 |
| 返回 | boolean | 若找到并取消了该操作则为 true |

### PauseDelayedAction

暂停一个延迟操作，并保留剩余时间。

```lua
local success = PauseDelayedAction(handle)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 要暂停的操作句柄 |
| 返回 | boolean | 若找到并暂停了该操作则为 true |

### UnpauseDelayedAction

恢复一个已暂停的延迟操作。

```lua
local success = UnpauseDelayedAction(handle)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 要恢复的操作句柄 |
| 返回 | boolean | 若找到并恢复了该操作则为 true |

### ResetDelayedActionTimer

以原本的延迟重新启动计时器。

```lua
local success = ResetDelayedActionTimer(handle)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 要重置的操作句柄 |
| 返回 | boolean | 若找到并重置了该操作则为 true |

### SetDelayedActionTimer

修改延迟时间并重新启动计时器。

```lua
local success = SetDelayedActionTimer(handle, newDelayMs)
```

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 要修改的操作句柄 |
| 2 | integer | 新的延迟毫秒数 |
| 返回 | boolean | 若找到并修改了该操作则为 true |

### ClearAllDelayedActions

取消属于当前 mod 的所有延迟操作。

```lua
local count = ClearAllDelayedActions()
```

| 返回 | integer | 被取消的操作数量 |
|------|---------|------------------|

**注意：** 该函数只影响调用它的 mod 所创建的操作，其他 mod 的操作不受影响。

## 查询延迟操作

### IsValidDelayedActionHandle

检查句柄是否指向一个存在且未被取消的操作。

```lua
local valid = IsValidDelayedActionHandle(handle)
```

### IsDelayedActionActive

检查操作当前是否处于活动状态（未暂停、未被标记移除）。

```lua
local active = IsDelayedActionActive(handle)
```

### IsDelayedActionPaused

检查操作当前是否处于暂停状态。

```lua
local paused = IsDelayedActionPaused(handle)
```

### GetDelayedActionRate

获取操作配置的延迟。

```lua
local delayMs = GetDelayedActionRate(handle)
```

对于基于时间的操作返回配置的延迟毫秒数，对于基于帧的操作返回帧数。若句柄无效则返回 -1。

### GetDelayedActionTimeRemaining

获取距离操作触发的剩余时间。

```lua
local remainingMs = GetDelayedActionTimeRemaining(handle)
```

对于基于时间的操作返回剩余毫秒数，对于基于帧的操作返回剩余帧数。若句柄无效则返回 -1。

### GetDelayedActionTimeElapsed

获取自操作启动/重置以来经过的时间。

```lua
local elapsedMs = GetDelayedActionTimeElapsed(handle)
```

对于基于时间的操作返回已经过的毫秒数，对于基于帧的操作返回已经过的帧数。若句柄无效则返回 -1。

## 全局变量

### EngineTickAvailable

布尔值，指示 EngineTick 钩子是否可用。

```lua
if EngineTickAvailable then
    -- 可以使用基于帧的延迟
end
```

### ProcessEventAvailable

布尔值，指示 ProcessEvent 钩子是否可用。

```lua
if ProcessEventAvailable then
    -- 可以使用基于 ProcessEvent 的执行方式
end
```

## 示例

### 简单延迟

```lua
ExecuteInGameThreadWithDelay(2000, function()
    print("This prints after 2 seconds\n")
end)
```

### 自我取消的循环

```lua
local counter = 0
local loopHandle  -- 先声明，以便闭包捕获
loopHandle = LoopInGameThreadWithDelay(1000, function()
    counter = counter + 1
    print(string.format("Tick %d\n", counter))
    if counter >= 5 then
        CancelDelayedAction(loopHandle)
        print("Loop stopped\n")
    end
end)
```

### 防抖操作

```lua
local debounceHandle = MakeActionHandle()

RegisterKeyBind(Key.F, function()
    -- 只在最后一次按键之后 500ms 触发
    RetriggerableExecuteInGameThreadWithDelay(debounceHandle, 500, function()
        print("Debounced action fired\n")
    end)
end)
```

### 可暂停的计时器

```lua
local timerHandle = ExecuteInGameThreadWithDelay(10000, function()
    print("Timer completed\n")
end)

-- 2 秒后暂停
ExecuteInGameThreadWithDelay(2000, function()
    PauseDelayedAction(timerHandle)
    print("Timer paused\n")
end)

-- 5 秒后恢复
ExecuteInGameThreadWithDelay(5000, function()
    UnpauseDelayedAction(timerHandle)
    print("Timer resumed\n")
end)
```

### UE 风格延迟（仅在不存在时创建）

```lua
local cooldownHandle = MakeActionHandle()

RegisterKeyBind(Key.E, function()
    -- 只有在尚未激活时才创建延迟
    ExecuteInGameThreadWithDelay(cooldownHandle, 1000, function()
        print("Ability ready!\n")
    end)
    print("Ability used (1s cooldown)\n")
end)
```

### 基于帧的延迟

```lua
if EngineTickAvailable then
    ExecuteInGameThreadAfterFrames(60, function()
        print("Fired after 60 frames\n")
    end)
end
```

## 重要说明

1. **闭包捕获**：在循环自身的回调中引用循环句柄时，要先声明变量再赋值：

   ```lua
   local loopHandle  -- 先声明
   loopHandle = LoopInGameThreadWithDelay(1000, function()
       CancelDelayedAction(loopHandle)  -- 现在可以被正确捕获
   end)
   ```

2. **Mod 归属**：每个 mod 只能控制自己创建的延迟操作。用其他 mod 的句柄调用 `CancelDelayedAction`、`PauseDelayedAction` 等函数会失败。

3. **基于帧的延迟**：需要 EngineTick 钩子。使用 `ExecuteInGameThreadAfterFrames` 或 `LoopInGameThreadAfterFrames` 之前请务必检查 `EngineTickAvailable`。

4. **回退行为**：若默认执行方式不可用，系统会自动回退到另一种可用的方式。
