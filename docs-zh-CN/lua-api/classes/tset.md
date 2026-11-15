# TSet

## 继承
[RemoteObject](./remoteobject.md)

## 元方法

### __len

- **用法：** `#TSet`
- **返回类型：** `integer`
- 返回当前集合中元素的数量。

## 方法

### Add(element)

- 向集合中添加一个元素。
- 若该元素已存在于集合中则什么也不做（集合不允许重复）。

### Contains(element)

- **返回类型：** `bool`
- **返回：** 该元素是否存在于集合中。

### Remove(element)

- 从集合中移除指定元素。
- 若该元素不存在则什么也不做。

### Empty()

- 从集合中移除所有元素。

### ForEach(function Callback)

- 遍历集合中的所有元素，并对每个元素调用回调函数。
- 回调参数为：`RemoteUnrealParam element` | `LocalUnrealParam element`。
- 使用 `element:get()` 与 `element:set()` 访问/修改该元素。
- 回调可以返回 `true` 以提前跳出循环。
- 示例：

```lua
mySet:ForEach(function(element)
    local value = element:get()
    print(string.format("Element: %s", tostring(value)))
    -- 返回 true 以停止遍历
    if value == targetValue then
        return true
    end
end)
```

## 说明

- TSet 是无序集合，因此遍历顺序不保证。
- TSet 中的元素必须唯一——添加重复元素不会产生任何效果。
