# LocalObject

`LocalObject` 是所有其他对象继承的两个基类对象之一，另一个是 [RemoteObject](./remoteobject.md)。

它包含一个由 Lua 持有的内联 C++ 对象。

## 继承
无

## 方法

### IsValid()

- **返回类型：** `bool`
- **返回：** 由于历史兼容原因，始终返回 `true`

**示例**

```lua
-- 'FText' 继承自 LocalObject。
local Text = FText("SomeText")
if Text:IsValid() then
    print("This should always happen\n")
else
    print("This should never happen\n")
end
```
