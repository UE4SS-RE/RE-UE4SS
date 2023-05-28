# RegisterULocalPlayerExecPreHook

This registers a callback that will get called before `ULocalPlayer::Exec` is called.

Parameters (except strings & bools & `FOutputDevice`) must be retrieved via `Param:Get()` and set via `Param:Set()`.

The callback can have two return values.
- If the first return value is nothing (or nil), the original return value of Exec will be used.
- If the first return value is true or false, the supplied value will override the original return value of Exec.
- The second return value controls whether the original Exec will execute.
- If the second return value is nil or true, the orginal Exec will execute.
- If the second return value is false, the original Exec will not execute.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | function | The callback to register |

## Callback Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | ULocalPlayer | The local player context |
| 2 | UWorld | The world |
| 3 | string | The command |
| 4 | FOutputDevice | The AR |

## Callback Return Values

| # | Type | Information |
|---|------|-------------|
| 1 | bool | Whether to override the original return value of Exec |
| 2 | bool | Whether to execute the original Exec |

## Example

```lua
local function MyCallback(Context, InWorld, Command, Ar)
    -- Do something with the parameters
    -- Return true or false to override the original return value of Exec
    -- Return false to prevent the original Exec from executing

    return nil, true
end

RegisterULocalPlayerExecPreHook(MyCallback)
```
