# CustomPropertyInfo

`CustomPropertyInfo` 表包含自定义属性的相关信息。

**使用此表时你必须自行提供数据。**

## 结构

| 键 | 值类型 | 子类型 | 说明 |
|----|--------|--------|------|
| Name | string | | 与 `__index` 元方法配合使用的名称 |
| Type | table | [PropertyTypes](./propertytypes.md) | |
| BelongsToClass | string | | 该属性所属的类完整名称（不含类型前缀） |
| OffsetInternal | integer 或 table | [OffsetInternalInfo](./offsetinternalinfo.md) | 仅当 Type 为 table 时子类型有效 |
| ArrayProperty | table | [ArrayPropertyInfo](./arraypropertyinfo.md) | 仅当 `Type` 为 PropertyTypes.ArrayProperty 时使用 |

## 简单示例

创建一个名为 `MySimpleCustomProperty` 的自定义属性，它把 `Character` 类任意实例中偏移 `0xF40` 处的数据当作 `IntProperty` 来访问。

```lua
local CustomPropertyInfo = {
    ["Name"] = "MySimpleCustomProperty",
    ["Type"] = PropertyTypes.IntProperty,
    ["BelongsToClass"] = "/Script/Engine.Character"
    ["OffsetInternal"] = 0xF40
}
```

## 进阶示例

创建一个名为 `MyAdvancedCustomProperty` 的自定义属性，它把 `Character` 类任意实例中偏移 `0xF48` 处的数据当作内部类型为 `IntProperty` 的 `ArrayProperty` 来访问。

```lua
local CustomPropertyInfo = {
    ["Name"] = "MyAdvancedCustomProperty",
    ["Type"] = PropertyTypes.ArrayProperty,
    ["BelongsToClass"] = "/Script/Engine.Character"
    ["OffsetInternal"] = 0xF48,
    ["ArrayProperty"] = {
        ["Type"] = PropertyTypes.IntProperty
    }
}
```
