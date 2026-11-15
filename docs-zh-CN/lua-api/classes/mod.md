# Mod

`Mod` 类负责与本地 mod 对象交互。

## 继承
[RemoteObject](./remoteobject.md)

## 方法

### SetSharedVariable(string VariableName, any Value)

- 设置一个可以被任意 mod 访问的变量。
- 第二个参数 `Value` 只能是以下类型之一：`nil`、`string`、`number`、`bool`、`UObject`（及其派生类）、`lightuserdata`。

> 警告：热重载时这些变量不会被重置。

**示例**

```lua
-- 共享 UObject 时，务必确认该 UObject 在被再次使用之前不会消失。
-- 共享 actor 这类临时对象是非常糟糕的做法，因为它们可能被销毁而不复存在。
local StaticObject = StaticFindObject("/Script/Engine.Default__GameplayStatics")

-- 'ModRef' 是一个自动创建的全局变量，即当前 mod 的实例。
ModRef:SetSharedVariable("MyVariable", StaticObject)
```

### GetSharedVariable(string VariableName)

- **返回类型：** `any`
- **返回：** 一个可能由其他 mod 设置的变量。
- 返回值只能是以下类型之一：`nil`、`string`、`number`、`bool`、`UObject`（及其派生类）、`lightuserdata`。

**示例**

```lua
-- 假设 'SetSharedVariable' 的示例脚本已经执行过。
local SharedObject = ModRef:GetSharedVariable("MyVariable")

-- 'GetSharedVariable' 可能返回它能够存储的任意内容。
-- 任何 mod 都可以覆盖任意共享变量的值。
if SharedObject and type(SharedObject) == "userdata" and SharedObject:type() == "UObject" and SharedObject:IsValid() then
    print(string.format("SharedObject '%s' is valid.\n", SharedObject:GetFullName()))
else
    print("SharedObject was nil, not userdata, not a UObject, or an invalid UObject")
end
```

### OnUnload(function Callback)

- 在该 mod 被卸载之前调用回调。
- 调用回调前总会等待异步操作完成。
- 该函数默认不存在，必须由 mod 自己提供，参见示例。

**示例**

```lua
ModRef.OnUnload = function()
    print("Mod is unloading.\n")
end
```

### type()

- **返回类型：** `string`
- **返回：** "ModRef"
