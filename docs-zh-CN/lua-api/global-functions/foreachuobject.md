# ForEachUObject

`ForEachUObject` 函数会遍历当前存在于 `GUObjectArray` 中的每一个 UObject。

UE4 中的 `GUObjectArray` 变量是一个存放 UObject 的大型分块数组。

该数组的结构多年来曾有变化，而 `ForEachUObject` 函数被设计为在所有引擎版本上行为一致。

## 参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | function | 对 GUObjectArray 中每个 UObject 执行的回调 |

## 回调参数

| # | 类型 | 说明 |
|---|------|------|
| 1 | UObject | 该 UObject |
| 2 | integer | 该 UObject 所在的分块索引 |
| 3 | integer | 该 UObject 的对象索引 |

## 示例

```lua
-- 警告：由于大量的 'print' 调用，这会需要相当长的时间才能执行完
ForEachUObject(function(Object, ChunkIndex, ObjectIndex)
    print(string.format("Chunk: %X | Object: %X | Name: %s\n", ChunkIndex, ObjectIndex, Object:GetFullName()))
end)
```
