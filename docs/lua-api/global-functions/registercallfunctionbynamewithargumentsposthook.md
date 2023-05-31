# RegisterCallFunctionByNameWithArgumentsPostHook

This registers a callback that will get called after `UObject::CallFunctionByNameWithArguments` is called.

Parameters (except strings & bools & `FOutputDevice`) must be retrieved via `Param:Get()` and set via `Param:Set()`.

If the callback returns nothing (or nil), the original return value of `CallFunctionByNameWithArguments` will be used.

If the callback returns true or false, the supplied value will override the original return value of `CallFunctionByNameWithArguments`.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | function | The callback to register |

## Callback Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | UObject | The object context |
| 2 | string | The string |
| 3 | FOutputDevice | The AR |
| 4 | UObject | The executor |
| 5 | bool | The bForceCallWithNonExec value |

## Callback Return Value

| # | Type | Information |
|---|------|-------------|
| 1 | bool | Whether to override the original return value of `CallFunctionByNameWithArguments` |

## Example

```lua
local function MyCallback(Context, Str, Ar, Executor, bForceCallWithNonExec)
    -- Do something with the parameters
    -- Return nil to use the original return value of CallFunctionByNameWithArguments
    -- Return true or false to override the original return value of CallFunctionByNameWithArguments

    return nil
end

RegisterCallFunctionByNameWithArgumentsPreHook(MyCallback)
```
```