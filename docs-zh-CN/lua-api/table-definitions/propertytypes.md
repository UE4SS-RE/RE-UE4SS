# PropertyTypes

`PropertyTypes` 表包含虚幻引擎属性的类型信息。
它主要用于 `RegisterCustomProperty` 这个 Lua 函数。

**此表的数据是自动填充的。
请勿修改表内的数据。**

## 结构

| 键 | 值 |
|----|----|
| ObjectProperty                   | internal_value |
| Int8Property                     | internal_value |
| Int16Property                    | internal_value |
| IntProperty                      | internal_value |
| Int64Property                    | internal_value |
| NameProperty                     | internal_value |
| FloatProperty                    | internal_value |
| StrProperty                      | internal_value |
| ByteProperty                     | internal_value |
| BoolProperty                     | internal_value |
| ArrayProperty                    | internal_value |
| MapProperty                      | internal_value |
| StructProperty                   | internal_value |
| ClassProperty                    | internal_value |
| WeakObjectProperty               | internal_value |
| EnumProperty                     | internal_value |
| TextProperty                     | internal_value |
| DelegateProperty                 | internal_value |
| MulticastDelegateProperty        | internal_value |
| MulticastInlineDelegateProperty  | internal_value |
| MulticastSparseDelegateProperty  | internal_value |

## 示例

```lua
local PropertyType = PropertyTypes.ObjectProperty
```
