# TArray

`TArray` 是虚幻引擎的数组实现。
下面描述的接口在若干方面与之不同。

第一个也是最重要的区别是：它从 1 开始索引，以符合 Lua 程序员对表式结构的预期。

第二个区别是：它由 `FScriptArray` 支撑，而不是 `TArray<T>`，因为 Lua 没有模板系统。
在需要时，子类型 `T` 会在运行时自动解析。

## 继承
[RemoteObject](./remoteobject.md)

## 元方法

### __index

- **用法：** `TArray[ArrayIndex]`
- **返回类型：** `auto`
- 尝试获取数组中整数偏移 `ArrayIndex` 处的值。
- 可能返回任意类型；你可以对返回值使用 `type()` 函数来确定它使用的 Lua 类（若为非平凡类型）。

### __newindex

- **用法：** `TArray[ArrayIndex] = NewValue`
- 尝试把数组中整数偏移 `ArrayIndex` 处的值设为 `NewValue`。

### __len

- **用法：** `#TArray`
- **返回类型：** `integer`
- 返回当前数组中元素的数量。

## 方法

### GetArrayAddress()

- **返回类型：** `integer`
- **返回：** `TArray` 结构体所在的内存地址。

### GetArrayNum()

- **返回类型：** `integer`
- **返回：** 当前数组中元素的数量。

### GetArrayMax()

- **返回类型：** `integer`
- **返回：** 该数组允许的最大元素数量（即容量）。

### GetArrayDataAddress()

- **返回类型：** `integer`
- **返回：** 该数组数据所在的内存地址。

### Empty()

- 清空数组。

### ForEach(function Callback)

- 遍历整个 `TArray`，对每个元素调用回调函数。
- 回调参数为：`integer index`、`RemoteUnrealParam elem` | `LocalUnrealParam elem`。
- 使用 `elem:get()` 与 `elem:set()` 访问/修改数组元素。
- 回调可以返回 `true` 以提前跳出循环。
