# RegisterKeyBind

The `RegisterKeyBind` function is used to bind a key on the keyboard to a Lua function.

> Callbacks registered with this function are only executed when either the game or the debug console is in focus.  

## Parameters (overload #1)

| # | Type     | Sub Type    | Information |
|---|----------|-------------|-------------|
| 1 | table    | Key         | Key to bind |
| 2 | function |             | Callback to execute when the key is hit on the keyboard |

## Parameters (overload #2)

| # | Type     | Sub Type    | Information |
|---|----------|-------------|-------------|
| 1 | integer  |             | Key to bind, use the 'Key' table |
| 2 | table    | ModifierKeys | Modifier keys required alongside the 'Key' parameter |
| 3 | function |             | Callback to execute when the key is hit on the keyboard |

## Example (overload #1)
```lua
RegisterKeyBind(Key.O, function()
    print("Key 'O' hit.\n")
end)
```

## Example (overload #2)
```lua
RegisterKeyBind(Key.O, {ModifierKey.CONTROL, ModifierKey.ALT}, function()
    print("Key 'CTRL + ALT + O' hit.\n")
end)
```

## Advanced Example (overload #1)
This registers a key bind with a callback that does nothing unless there are no widgets currently open
```lua
local AnyWidgetsOpen = false

RegisterHook("/Script/UMG.UserWidget:Construct", function()
    AnyWidgetsOpen = true
end)

RegisterHook("/Script/UMG.UserWidget:Destruct", function()
    AnyWidgetsOpen = false
end)

RegisterKeyBind(Key.B, function()
    if AnyWidgetsOpen then return end
    print("Key 'B' hit, while no widgets are open.\n")
end)
```