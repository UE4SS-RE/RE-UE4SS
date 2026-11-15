# MulticastDelegateProperty

表示虚幻引擎中的内联多播委托属性。

## 继承
[Property](./property.md)

## 说明

`MulticastDelegateProperty` 封装了虚幻引擎的 `FMulticastDelegateProperty`，它表示可以绑定多个对象与函数对的多播委托。该委托的调用列表使用内联存储。

## 访问委托属性

当你访问 UObject 上的多播委托属性时，返回的是一个属性包装器对象，其中带有修改委托的方法：

```lua
local PlayerController = GetPlayerController()

-- 访问委托属性（返回属性包装器）
local delegateProp = PlayerController.OnPossessedPawnChanged

-- 包装器带有 Add、Remove 与 Clear 方法
delegateProp:Add(MyCustomObject, FName("OnPawnChanged"))
```

## 方法

### Add(targetObject, functionName)

向多播委托的调用列表中添加一个委托绑定。

- **参数：**
  - `targetObject`（UObject）：要绑定委托的目标对象
  - `functionName`（FName | string）：要在目标对象上调用的函数名

```lua
-- 访问委托属性包装器
local prop = PlayerController.OnPossessedPawnChanged

-- 使用 FName 添加委托绑定
prop:Add(MyCustomObject, FName("OnPawnChanged"))

-- 也可以直接使用字符串
prop:Add(MyCustomObject, "OnPawnChanged")
```

### Remove(targetObject, functionName)

从调用列表中移除指定的委托绑定。

- **参数：**
  - `targetObject`（UObject）：委托所绑定的对象
  - `functionName`（FName | string）：已绑定函数的名字

```lua
-- 移除指定的绑定
prop:Remove(MyCustomObject, FName("OnPawnChanged"))

-- 也可以使用字符串
prop:Remove(MyCustomObject, "OnPawnChanged")
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
-- 用于没有参数的委托
prop:Broadcast()

-- 用于带参数的委托（例如 OnDamage(float Amount, AActor* Causer)）
prop:Broadcast(25.0, CauserActor)
```

## 说明

- 对委托属性访问 `Object.PropertyName` 返回的是**属性包装器**对象，而不是委托值
- 容器对象被隐式保存在包装器中，因此无需再把它传给各方法
- 函数名同时接受 `FName` 与字符串字面量
- 另见：[MulticastSparseDelegateProperty](./multicastsparsedelegateproperty.md)（稀疏存储变体）
