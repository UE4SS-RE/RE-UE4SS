# Mod 管理函数

这些函数允许 Lua mod 以编程方式重启或卸载自身或其他 mod。

## RestartCurrentMod

把当前正在运行的 mod 排队，在下一个更新周期重启。

```lua
RegisterKeyBind(Key.F5, function()
    print("Restarting mod...\n")
    RestartCurrentMod()
end)
```

## UninstallCurrentMod

把当前正在运行的 mod 排队，在下一个更新周期卸载。这会销毁 Lua 状态并移除所有钩子与按键绑定。

```lua
RegisterKeyBind(Key.F6, function()
    print("Uninstalling mod...\n")
    UninstallCurrentMod()
end)
```

## RestartMod

按名称重启另一个 mod。

### 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要重启的 mod 名称 |

### 示例

```lua
RegisterKeyBind(Key.F7, function()
    print("Restarting MyOtherMod...\n")
    RestartMod("MyOtherMod")
end)
```

## UninstallMod

按名称卸载另一个 mod。

### 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | string | 要卸载的 mod 名称 |

### 示例

```lua
RegisterKeyBind(Key.F8, function()
    print("Uninstalling MyOtherMod...\n")
    UninstallMod("MyOtherMod")
end)
```

## 进阶示例

下面的示例创建了一个 mod，为重启多个其他 mod 提供按键绑定：

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
