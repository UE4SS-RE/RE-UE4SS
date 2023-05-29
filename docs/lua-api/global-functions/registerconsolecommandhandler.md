# RegisterConsoleCommandHandler

The `RegisterConsoleCommandHandler` function executes the provided Lua function whenever the supplied custom command is entered into the UE console.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | The name of the custom command |
| 2 | function | The callback to execute when the custom command is entered into the UE console|

## Callback Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | The name of the custom command |
| 2 | table    | Table containing all parameters |
| 3 | FOutputDevice | The output device to write to |

## Callback Return Value

| # | Type  | Information |
|---|-------|-------------|
| 1 | bool  | Whether to prevent other handlers from handling this command |

## Example
```lua
RegisterConsoleCommandHandler("CommandExample", function(FullCommand, Parameters, OutputDevice)
    print("Custom command callback for 'CommandExample' command executed.\n")
    print(string.format("Full command: %s\n", FullCommand))
    print(string.format("Number of parameters: %i\n", #Parameters))
    
    for ParameterNumber, Parameter in pairs(Parameters) do
        print(string.format("Parameter #%i -> '%s'\n", ParameterNumber, Parameter))
    end

    return true
end)

-- Entered into console: CommandExample 1 2 3
-- Output
--[[
Custom command callback for 'CommandExample' command executed.
Full command: CommandExample 1 2 3
Number of parameters: 3
Parameter #1 -> '1'
Parameter #2 -> '2'
Parameter #3 -> '3'
Parameter #0 -> 'CommandExample'
--]]
```