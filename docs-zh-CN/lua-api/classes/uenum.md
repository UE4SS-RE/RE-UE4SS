# UEnum

## 继承
[RemoteObject](./remoteobject.md)

## 方法

### GetNameByValue(integer Value)

- **返回类型：** `FName`
- **返回：** 与指定值对应的 `FName`。

### ForEachName(LuaFunction Callback)

- 遍历属于该枚举的每一个 `FName`/`Value` 组合。
- 回调有两个参数：`FName Name`、`integer Value`。
- 在回调中返回 `true` 可停止遍历。

### GetEnumNameByIndex(integer Index)

- **返回类型：** `FName`、`Integer`
- **返回：** 与给定 `Index` 对应的 `FName`。
- **返回：** 与给定 `Index` 对应的 `Integer` 值。

### InsertIntoNames(string Name, integer Value, integer Index, boolean ShiftValues = true)

- 在给定 `Index` 处向 `UEnum` 插入一个 `FName`/`Value` 组合。
- 若 `ShiftValues = true`，则所有大于插入值的枚举值都会后移一位。

### EditNameAt(integer Index, string NewName)

- 在给定 `Index` 处修改 `UEnum` 中对应的元素，把其 `Name` 替换为给定的 `NewName`。

### EditValueAt(integer Index, integer NewValue)

- 在给定 `Index` 处修改 `UEnum` 中对应的元素，把其值替换为给定的 `NewValue`。

### RemoveFromNamesAt(integer Index, integer Count = 1, boolean AllowShrinking = true)

- 从 `UEnum` 中移除给定 `Index` 处的 `Count` 个元素。
- 若 `AllowShrinkning = true`，移除元素后会收缩枚举数组。
