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
local function MyCallback(Context, Command, CommandParts, Ar, Executor)
    -- Do something with the parameters
    -- Return nil to use the original return value of ProcessConsoleExec
    -- Return true or false to override the original return value of ProcessConsoleExec

    return nil
end

RegisterProcessConsoleExecPreHook(MyCallback)
```