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
| 2 | string | The command |
| 3 | string | The rest of the command |
| 4 | FOutputDevice | The AR |
| 5 | UObject | The executor |

## Callback Return Value

| # | Type | Information |
|---|------|-------------|
| 1 | bool | Whether to override the original return value of `ProcessConsoleExec` |

## Example

```lua
RegisterProcessConsoleExecPreHook(function(Context, Command, CommandParts, Ar, Executor)
    print("RegisterProcessConsoleExecPreHook:\n")
    print(string.format("Context FullName: %s\n", Context:get():GetFullName()))
    if Executor:get():IsValid() then
        print(string.format("Executor FullName: %s\n", Executor:get():GetFullName()))
    end
    print(string.format("Command: %s\n", Command))
    print(string.format("Number of parameters: %i\n", #CommandParts))
    
    for ParameterNumber, Parameter in ipairs(CommandParts) do
        print(string.format("Parameter #%i -> '%s'\n", ParameterNumber, Parameter))
    end

    return true
end)
```