# RegisterCallFunctionByNameWithArgumentsPostHook

该函数注册一个回调，它会在 `UObject::CallFunctionByNameWithArguments` 被调用之后执行。

参数（字符串、bool 以及 `FOutputDevice` 除外）必须通过 `Param:Get()` 获取，并通过 `Param:Set()` 设置。

若回调不返回任何值（或返回 nil），则使用 `CallFunctionByNameWithArguments` 原本的返回值。

若回调返回 true 或 false，则用该值覆盖 `CallFunctionByNameWithArguments` 原本的返回值。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | function | 要注册的回调 |

## 回调参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject | 对象上下文 |
| 2 | string | 字符串 |
| 3 | FOutputDevice | AR |
| 4 | UObject | 执行者 |
| 5 | bool | bForceCallWithNonExec 的值 |

## 回调返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | bool | 是否覆盖 `CallFunctionByNameWithArguments` 原本的返回值 |

## 示例

```lua
local function MyCallback(Context, Str, Ar, Executor, bForceCallWithNonExec)
    -- 对参数做一些处理
    -- 返回 nil 表示使用 CallFunctionByNameWithArguments 原本的返回值
    -- 返回 true 或 false 表示覆盖 CallFunctionByNameWithArguments 原本的返回值

    return nil
end

RegisterCallFunctionByNameWithArgumentsPreHook(MyCallback)
```
