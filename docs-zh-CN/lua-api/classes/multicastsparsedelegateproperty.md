# MulticastSparseDelegateProperty

表示虚幻引擎中的稀疏多播委托属性。

## 继承
[Property](./property.md)

## 说明

`MulticastSparseDelegateProperty` 封装了虚幻引擎的 `FMulticastSparseDelegateProperty`，它表示使用稀疏存储的多播委托。对于经常处于未绑定状态的委托而言，这种存储方式更节省内存：对象中只保存一个 1 字节的标志，真正的调用列表则存放在全局存储中。

尽管存储机制不同，其 API 与 [MulticastDelegateProperty](./multicastdelegateproperty.md) 完全一致。

## 访问委托属性

当你访问 UObject 上的稀疏多播委托属性时，返回的是一个属性包装器对象，其中带有修改委托的方法（API 与内联多播委托相同）：

```lua
local PlayerPawn = GetPlayerController().Pawn

-- 访问委托属性（返回属性包装器）
local delegateProp = PlayerPawn.OnEndPlay

-- 包装器带有 Add、Remove 与 Clear 方法
delegateProp:Add(MyCustomObject, FName("OnActorEndPlay"))
```

## 方法

### Add(targetObject, functionName)

向稀疏多播委托的调用列表中添加一个委托绑定。

- **参数：**
  - `targetObject`（UObject）：要绑定委托的目标对象
  - `functionName`（FName | string）：要在目标对象上调用的函数名

```lua
-- 访问委托属性包装器
local prop = PlayerPawn.OnEndPlay

-- 使用 FName 添加委托绑定
prop:Add(MyCustomObject, FName("OnActorEndPlay"))

-- 也可以直接使用字符串
prop:Add(MyCustomObject, "OnActorEndPlay")
```

### Remove(targetObject, functionName)

从调用列表中移除指定的委托绑定。

- **参数：**
  - `targetObject`（UObject）：委托所绑定的对象
  - `functionName`（FName | string）：已绑定函数的名字

```lua
-- 移除指定的绑定
prop:Remove(MyCustomObject, FName("OnActorEndPlay"))

-- 也可以使用字符串
prop:Remove(MyCustomObject, "OnActorEndPlay")
```

### Clear()

从调用列表中移除所有委托绑定。

```lua
-- 清除所有绑定
prop:Clear()
```

### GetBindings()

返回当前所有委托绑定组成的数组。

- **返回：** 由表组成的数组，每个表包含 `Object`（UObject）与 `FunctionName`（FName）字段；若没有任何绑定则返回 `nil`

```lua
-- 获取当前所有绑定
local bindings = prop:GetBindings()
if bindings then
    print("Delegate has", #bindings, "bindings")
    for i, binding in ipairs(bindings) do
        print(string.format("[%d] %s::%s",
            i,
            binding.Object:GetFullName(),
            binding.FunctionName:ToString()))
    end
end
```

### Broadcast(...)

触发委托，用提供的参数调用所有已绑定的函数。

- **参数：** 可变数量——取决于委托签名中的函数
- **注意：** 参数的类型与顺序必须与委托签名一致

```lua
-- 用于没有参数的委托（例如 Actor.OnEndPlay）
prop:Broadcast()

-- 用于带参数的委托
prop:Broadcast(25.0, CauserActor)
```

## 实现细节

在引擎中 `FMulticastSparseDelegateProperty` 继承自 `FMulticastDelegateProperty`，因此所有相同的虚表函数（`AddDelegate`、`RemoveDelegate`、`ClearDelegate`）都可用且行为一致。

## 说明

- 对稀疏委托属性访问 `Object.PropertyName` 返回的是**属性包装器**对象，而不是委托值
- 容器对象被隐式保存在包装器中，因此无需再把它传给各方法
- 函数名同时接受 `FName` 与字符串字面量
- 存储机制对 Lua 是透明的——API 与内联多播委托完全相同
- 常见示例：`Actor.OnEndPlay` 就是一个 `MulticastSparseDelegateProperty`
