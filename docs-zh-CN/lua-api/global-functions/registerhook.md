# RegisterHook

`RegisterHook` 为某个 `UFunction` 注册回调。

当该 `UFunction` 被执行时，回调会被触发。

回调参数为：`UObject self`、`UFunctionParams..`。

该函数返回两个 id，若要取消该钩子，必须把这两个 id 都传给 `UnregisterHook`。

> 你尝试用 `RegisterHook` 注册的任意 `UFunction`，在注册时必须已经存在于内存中。

> `RegisterHook` 不支持委托函数！

## RegisterHook 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要挂钩的 UFunction 完整名称。类型前缀不起作用。 |
| 2 | function | 若 UFunction 路径以 `/Script/` 开头：在该 UFunction 执行前调用的回调。<br/>否则：在该 UFunction 执行后调用的回调。 |
| 3 | function | （可选）<br/>若 UFunction 路径以 `/Script/` 开头：在该 UFunction 执行后调用的回调。<br/>否则：该参数不起作用。 |

## RegisterHook 返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | integer | 该钩子的 PreId |
| 2 | integer | 该钩子的 PostId |

## 回调参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | [RemoteUnrealParam](../classes/remoteunrealparam.md) | 该函数"this"指针（Lua 中的 "self"）对应的对象表示，也称 "Context"。它包含以 RemoteUnrealParam 包装的、调用该函数的对象。 |
| 2..N | RemoteUnrealParam... | 所有函数参数，均以 RemoteUnrealParam 包装 |

## 回调返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | any | 系统会尝试把该值自动转换为 UE 值，并用它覆盖原函数的返回值。<br>返回 `nil`（或不写 return 语句）则保持原返回值不变。 |

## 示例

```lua
PreId, PostId = RegisterHook("/Script/Engine.PlayerController:ClientRestart", function(Context, NewPawn)
    local PlayerController = Context:get()
    local Pawn = NewPawn:get()

    print(string.format("PlayerController FullName: %s\n", PlayerController:GetFullName()))
    if Pawn:IsValid() then
        print(string.format("NewPawn FullName: %s\n", Pawn:GetFullName()))
    end

    if PreId then
        -- 函数被调用一次后就取消钩子
        UnregisterHook("/Script/Engine.PlayerController:ClientRestart", PreId, PostId)
    end
end)
```
