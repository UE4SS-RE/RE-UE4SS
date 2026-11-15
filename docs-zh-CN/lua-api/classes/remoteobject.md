# RemoteObject

`RemoteObject` 是所有其他对象继承的两个基类对象之一，另一个是 [LocalObject](./localobject.md)。

它包含一个指向 C++ 对象的指针，该对象通常由游戏持有。

## 继承
无

## 方法

### IsValid()

- **返回类型：** `bool`
- **返回：** 该对象是否有效

**示例**

```lua
-- 'StaticFindObject' 返回的 UObject 继承自 RemoteObject。
local Object = StaticFindObject("/Script/CoreUObject.Object")
if Object:IsValid() then
    print("Object is valid\n")
else
    print("Object is NOT valid\n")
end
```
