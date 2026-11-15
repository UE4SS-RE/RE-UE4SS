# RegisterProcessConsoleExecPreHook

该函数注册一个回调，它会在 `UObject::ProcessConsoleExec` 被调用之前执行。

参数（字符串、bool 以及 `FOutputDevice` 除外）必须通过 `Param:Get()` 获取，并通过 `Param:Set()` 设置。

若回调不返回任何值（或返回 nil），则使用 `ProcessConsoleExec` 原本的返回值。

若回调返回 true 或 false，则用该值覆盖 `ProcessConsoleExec` 原本的返回值。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | function | 要注册的回调 |

## 回调参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject | 对象上下文 |
| 2 | string | 完整命令字符串 |
| 3 | table | 所有命令片段，包括命令名 |
| 4 | FOutputDevice | AR |
| 5 | UObject | 执行者 |

## 回调返回值

| # | 类型 | 说明 |
|---|------|------|
| 1 | bool | 是否覆盖 `ProcessConsoleExec` 原本的返回值 |

## 示例

```lua
RegisterProcessConsoleExecPreHook(function(Context, FullCommand, CommandParts, Ar, Executor)
    print("RegisterProcessConsoleExecPreHook:\n")
    local ContextObject = Context:get()
    local ExecutorObject = Executor:get()

    print(string.format("Context FullName: %s\n", ContextObject:GetFullName()))
    if ExecutorObject:IsValid() then
        print(string.format("Executor FullName: %s\n", ExecutorObject:GetFullName()))
    end
    print(string.format("Command: %s\n", FullCommand))
    print(string.format("Number of command parts: %i\n", #CommandParts))

    if #CommandParts > 0 then
        print(string.format("Command Name: %s\n", CommandParts[1]))
        for PartNumber, CommandPart in ipairs(CommandParts) do
            print(string.format("CommandPart: #%i -> '%s'\n", PartNumber, CommandPart))
        end
    end

    Ar:Log("Write something to game console")

    return true
end)

-- 在控制台输入：CommandExample param1 "param 2" 3
-- 输出
--[[
RegisterProcessConsoleExecPreHook:
Context FullName: GameState /Game/Maps/Map_MainMenu.Map_MainMenu:PersistentLevel.GameState_2147479851
Executor FullName: BP_MainMenuPawn_C /Game/Maps/Map_MainMenu.Map_MainMenu:PersistentLevel.BP_MainMenuPawn_C_2147479061
Command: CommandExample param1 "param 2" 3
Number of command parts: 4
Command Name: CommandExample
CommandPart: #1 -> 'CommandExample'
CommandPart: #2 -> 'param1'
CommandPart: #3 -> 'param 2'
CommandPart: #4 -> '3'
--]]
```
