# RegisterKeyBind

`RegisterKeyBind` 函数用于把键盘上的某个按键绑定到一个 Lua 函数。

> 通过该函数注册的回调，只有在游戏窗口或调试控制台处于焦点时才会执行。

## 参数（重载 #1）

| # | 类型 | 子类型 | 说明 |
|---|------|--------|------|
| 1 | table | Key | 要绑定的按键 |
| 2 | function | | 按下该按键时要执行的回调 |

## 参数（重载 #2）

| # | 类型 | 子类型 | 说明 |
|---|------|--------|------|
| 1 | integer | | 要绑定的按键，请使用 `Key` 表 |
| 2 | table | ModifierKeys | 与 `Key` 参数一起需要的修饰键 |
| 3 | function | | 按下该按键时要执行的回调 |

## 示例（重载 #1）

```lua
RegisterKeyBind(Key.O, function()
    print("Key 'O' hit.\n")
end)
```

## 示例（重载 #2）

```lua
RegisterKeyBind(Key.O, {ModifierKey.CONTROL, ModifierKey.ALT}, function()
    print("Key 'CTRL + ALT + O' hit.\n")
end)
```

## 进阶示例（重载 #1）

下面的示例注册了一个按键绑定，其回调只在当前没有打开任何控件时才会执行。

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
