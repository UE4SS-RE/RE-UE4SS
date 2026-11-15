# RegisterULocalPlayerExecPostHook

该函数注册一个回调，它会在 `ULocalPlayer::Exec` 被调用之后执行。

参数（字符串、bool 以及 `FOutputDevice` 除外）必须通过 `Param:Get()` 获取，并通过 `Param:Set()` 设置。

回调可以有两个返回值。

- 若第一个返回值为空（或 nil），则使用 Exec 原本的返回值。
- 若第一个返回值为 true 或 false，则用该值覆盖 Exec 原本的返回值。
- 第二个返回值控制原本的 Exec 是否执行。
- 若第二个返回值为 nil 或 true，则原本的 Exec 会执行。
- 若第二个返回值为 false，则原本的 Exec 不会执行。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | function | 要注册的回调 |

## 回调参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | ULocalPlayer | 本地玩家上下文 |
| 2 | UWorld | 世界 |
| 3 | string | 命令 |
| 4 | FOutputDevice | AR |

## 回调返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | bool | 是否覆盖 Exec 原本的返回值 |
| 2 | bool | 是否执行原本的 Exec |

## 示例

```lua
local function MyCallback(Context, InWorld, Command, Ar)
    -- 对参数做一些处理
    -- 返回 true 或 false 以覆盖 Exec 原本的返回值
    -- 返回 false 以阻止原本的 Exec 执行

    return nil, true
end

RegisterULocalPlayerExecPostHook(MyCallback)
```
