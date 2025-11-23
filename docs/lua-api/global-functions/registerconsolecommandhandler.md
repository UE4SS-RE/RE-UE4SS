# RegisterConsoleCommandHandler
The `RegisterConsoleCommandHandler` function executes the provided Lua function whenever the supplied custom command is entered into the UE console.

## Parameters
| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | The name of the custom command |
| 2 | function | The callback to execute when the custom command is entered into the UE console |

## Callback Parameters
| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | Full command string |
| 2 | table    | Table with parameters (without the command name) |
| 3 | FOutputDevice | The output device to write to |

## Callback Return Value
| # | Type  | Information |
|---|-------|-------------|
| 1 | bool  | Whether to prevent other handlers from handling this command |

## Example
```lua
RegisterConsoleCommandHandler("CommandExample", function(FullCommand, Parameters, OutputDevice)
    print("RegisterConsoleCommandHandler:\n")

    print(string.format("Command: %s\n", FullCommand))
    print(string.format("Number of parameters: %i\n", #Parameters))
    
    for ParameterNumber, Parameter in ipairs(Parameters) do
        print(string.format("Parameter #%i -> '%s'\n", ParameterNumber, Parameter))
    end

    OutputDevice:Log("Write something to game console")

    return false
end)

-- Entered into console: CommandExample param1 "param 2" 3
-- Output
--[[
RegisterConsoleCommandHandler:
Command: CommandExample param1 "param 2" 3
Number of parameters: 3
Parameter #1 -> 'param1'
Parameter #2 -> 'param 2'
Parameter #3 -> '3'
--]]
```