# RegisterCustomProperty

The `RegisterCustomProperty` function is used to register custom properties for use just as if it were a reflected native or BP property.

This is an advanced function that's used to add support for non-reflected properties in the `__index` metamethod in multiple metatables.

## Parameters

| # | Type    | Sub Type           | Information |
|---|---------|--------------------|-------------|
| 1 | table   | [CustomPropertyInfo](https://github.com/UE4SS/UE4SS/wiki/Table%3A-CustomPropertyInfo) | A table containing all of the required information for registering a custom property |

## Example

Registers a custom property with the name `MySimpleCustomProperty` that accesses whatever data is at offset `0xF40` in any instance of class `Character` as if it was an `IntProperty`.

It then grabs that value of the first instance of the class `Character` as an example of how the system works.
```lua
RegisterCustomProperty({
    ["Name"] = "MySimpleCustomProperty",
    ["Type"] = PropertyTypes.IntProperty,
    ["BelongsToClass"] = "/Script/Engine.Character"
    ["OffsetInternal"] = 0xF40
})

local CharacterInstance = FindFirstOf("Character")
if CharacterInstance:IsValid() then
    local MySimplePropertyValue = CharacterInstance.MySimpleCustomProperty
end
```