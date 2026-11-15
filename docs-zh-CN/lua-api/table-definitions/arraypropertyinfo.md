# ArrayPropertyInfo

`ArrayPropertyInfo` 表包含自定义 ArrayProperty 属性的类型信息。

**使用此表时你必须自行提供数据。**

## 结构

| 键 | 值类型 | 子类型 |
|----|--------|--------|
| Type | table | [PropertyTypes](./propertytypes.md) |

## 示例

```lua
local ArrayPropertyInfo = {
    ["Type"] = PropertyTypes.IntProperty
}
```
