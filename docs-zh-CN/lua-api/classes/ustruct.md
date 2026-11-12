# UStruct

## 继承
[UObject](./uobject.md)

## 方法

### GetSuperStruct()

- **返回类型：** `UClass`
- **返回：** 该结构体的 `SuperStruct`（可能无效）。

## ForEachFunction(function Callback)

- 遍历属于该结构体的每一个 `UFunction`。
- 回调有一个参数：`UFunction Function`。
- 在回调中返回 `true` 可停止遍历。

## ForEachProperty(function Callback)

- 遍历属于该结构体的每一个 `Property`。
- 回调有一个参数：`Property Property`。
- 在回调中返回 `true` 可停止遍历。
