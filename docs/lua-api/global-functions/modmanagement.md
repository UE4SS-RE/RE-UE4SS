# Mod Management Functions

These functions allow Lua mods to restart or uninstall themselves or other mods programmatically.

## RestartCurrentMod

Queues the currently running mod for restart on the next update cycle.

```lua
RegisterKeyBind(Key.F5, function()
    print("Restarting mod...\n")
    RestartCurrentMod()
end)
```

## UninstallCurrentMod

Queues the currently running mod for uninstallation on the next update cycle. This destroys the Lua state and removes all hooks and keybinds.

```lua
RegisterKeyBind(Key.F6, function()
    print("Uninstalling mod...\n")
    UninstallCurrentMod()
end)
```

## RestartMod

Restarts another mod by name.

### Parameters

| # | Type   | Information |
|---|--------|-------------|
| 1 | string | The name of the mod to restart |

### Example

```lua
RegisterKeyBind(Key.F7, function()
    print("Restarting MyOtherMod...\n")
    RestartMod("MyOtherMod")
end)
```

## UninstallMod

Uninstalls another mod by name.

### Parameters

| # | Type   | Information |
|---|--------|-------------|
| 1 | string | The name of the mod to uninstall |

### Example

```lua
RegisterKeyBind(Key.F8, function()
    print("Uninstalling MyOtherMod...\n")
    UninstallMod("MyOtherMod")
end)
```

## Advanced Example

This example creates a mod that provides keybinds to restart multiple other mods:

```lua
local ModsToManage = {
    {Key = Key.ONE, ModName = "ActorDumperMod"},
    {Key = Key.TWO, ModName = "LineTraceMod"},
    {Key = Key.THREE, ModName = "ConsoleCommandsMod"},
}

for _, entry in ipairs(ModsToManage) do
    RegisterKeyBind(entry.Key, {ModifierKey.CONTROL, ModifierKey.SHIFT}, function()
        print(string.format("Restarting %s...\n", entry.ModName))
        RestartMod(entry.ModName)
    end)
end
```
