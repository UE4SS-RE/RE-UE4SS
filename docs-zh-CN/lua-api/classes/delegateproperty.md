# DelegateProperty

表示虚幻引擎中的单播委托属性。

## 继承
[Property](./property.md)

## 说明

`DelegateProperty` 封装了虚幻引擎的 `FDelegateProperty`，它表示只能绑定一个对象与函数对的单播委托。作为元数据操作，委托属性通过其属性包装器访问（使用 `GetPropertyByName`）；而委托**值**则直接通过 `Object.PropertyName` 访问，并返回普通表。

## 读取委托值

委托值直接在 UObject 上访问，返回一个包含 `Object` 与 `FunctionName` 字段的表：

```lua
local PlayerPawn = GetPlayerController().Pawn

-- 读取单播委托的值
local delegate = PlayerPawn.OnSomeDelegate
if delegate then
    print("Bound to:", delegate.Object:GetFullName())
    print("Function:", tostring(delegate.FunctionName))
end
```

## 设置委托值

单播委托可以通过赋一个包含 `Object` 与 `FunctionName` 字段的表来设置，或赋 `nil` 来清除：

```lua
-- 绑定委托
PlayerPawn.OnSomeDelegate = {
    Object = SomeObject,
    FunctionName = FName("MyCallbackFunction")
}

-- 清除委托
PlayerPawn.OnSomeDelegate = nil
```

## 说明

- 单播委托没有 `Add`、`Remove` 或 `Clear` 方法（这些操作请使用多播委托）
- 属性包装器本身除继承自 [Property](./property.md) 的方法外不提供额外方法
- 委托值是普通 Lua 表，而不是 userdata 对象
