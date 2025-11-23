# RegisterProcessConsoleExecPreHook
This registers a callback that will get called before `UObject::ProcessConsoleExec` is called.

Parameters (except strings & bools & `FOutputDevice`) must be retrieved via `Param:Get()` and set via `Param:Set()`.

If the callback returns nothing (or nil), the original return value of `ProcessConsoleExec` will be used.

If the callback returns true or false, the supplied value will override the original return value of `ProcessConsoleExec`.

## Parameters
| # | Type | Information |
|---|------|-------------|
| 1 | function | The callback to register |

## Callback Parameters
| # | Type | Information |
|---|------|-------------|
| 1 | UObject | The object context |
| 2 | string | Full command string |
| 3 | table | All command parts, including the command name  |
| 4 | FOutputDevice | The AR |
| 5 | UObject | The executor |

## Callback Return Value
| # | Type | Information |
|---|------|-------------|
| 1 | bool | Whether to override the original return value of `ProcessConsoleExec` |

## Example
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

-- Entered into console: CommandExample param1 "param 2" 3
-- Output
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