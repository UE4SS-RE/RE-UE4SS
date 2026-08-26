# OnUnload

The `OnUnload` function is used for cleaning up your code / variable change you have done in your script to restore original game state. Useful for toggle function for exemple.

> You can have multiple OnUnload in your code. I wouldn't suggest it as you don't know the execution order of the OnUnload method.

## Parameters

|  #  | Type  | Information         |
|-----|-------|---------------------|
| 1 | function | The callback to use |

## Example
```lua
OnUnload(function()
    print("[MyMod] Unloaded\n")
end)
```
