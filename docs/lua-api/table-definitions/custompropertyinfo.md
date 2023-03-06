# CustomPropertyInfo

The `CustomPropertyInfo` table contains information about a custom property.

**You must supply data yourself when using this table.**

## Structure
| Key            | Value Type       | Sub Type           | Information |
|----------------|------------------|--------------------|-------------|
| Name           | string           |                    | Name to use with the __index metamethod |
| Type           | table            | [PropertyTypes](./propertytypes.md)|
| BelongsToClass | string           |                    | Full class name without type, that this property belongs to
| OffsetInternal | integer or table | [OffsetInternalInfo](./offsetinternalinfo.md)| Sub Type only valid if Type is table
| ArrayProperty  | table            | [ArrayPropertyInfo](./arraypropertyinfo.md)| Only use when 'Type' is PropertyTypes.ArrayProperty

## Simple Example
Creates a custom property with the name `MySimpleCustomProperty` that accesses whatever data is at offset `0xF40` in any instance of class `Character` as if it was an `IntProperty`.
```lua
local CustomPropertyInfo = {
    ["Name"] = "MySimpleCustomProperty",
    ["Type"] = PropertyTypes.IntProperty,
    ["BelongsToClass"] = "/Script/Engine.Character"
    ["OffsetInternal"] = 0xF40
}
```

## Advanced Example
Creates a custom property with the name `MyAdvancedCustomProperty` that accesses whatever data is at offset `0xF48` in any instance of class `Character` as if it was an `ArrayProperty` with an inner type of `IntProperty`.
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