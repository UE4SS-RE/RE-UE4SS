# TMap

## 继承
[RemoteObject](./remoteobject.md)

## 元方法

### __len

- **用法：** `#TMap`
- **返回类型：** `integer`
- 返回当前 map 中键值对的数量。

## 方法

### Find(key)

- **返回类型：** `RemoteUnrealParam` | `LocalUnrealParam`
- **返回：** 在 map 中找到的元素
- **抛出异常：** 若 map 中未找到该元素
- 在 map 中查找指定的键

### Add(key, value)

- 向 map 中插入一个键值对。若该键已存在，则替换其值。

### Contains(key)

- **返回类型：** `bool`
- **返回：** 该元素是否存在于 map 中。
- 检查某个键是否存在于 map 内。

### Remove(key)

- 从 map 中移除一个元素。若元素不存在则什么也不做。

### Empty()

- 清空 map。

### ForEach(function Callback)

- 遍历整个 `TMap`，对 map 中的每个元素调用回调函数。
- 回调参数为：`RemoteUnrealParam key`、`RemoteUnrealParam value` | `LocalUnrealParam value`。
- 使用 `elem:get()` 与 `elem:set()` 访问/修改值。
- 修改键属于未定义行为。
- 回调可以返回 `true` 以提前跳出循环。
