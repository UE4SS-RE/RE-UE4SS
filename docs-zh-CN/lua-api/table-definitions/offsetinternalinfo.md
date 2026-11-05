# OffsetInternalInfo

`OffsetInternalInfo` 表包含与自定义属性相关的偏移信息。

**使用此表时你必须自行提供数据。**

## 结构

| 键 | 值类型 | 说明 |
|----|--------|------|
| Property | string | 作为相对起点的属性名，用于代替基址 |
| RelativeOffset | integer | 从相对起点到该属性的偏移 |

## 示例

```lua
local PropertyInfo = {
    ["Property"] = "HistoryBuffer",
    ["RelativeOffset"] = 0x10
}
```
