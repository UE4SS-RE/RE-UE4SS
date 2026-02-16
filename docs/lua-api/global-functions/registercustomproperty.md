# RegisterCustomProperty

The `RegisterCustomProperty` function is used to register custom properties for use just as if it were a reflected native or BP property.

This is an advanced function that's used to add support for non-reflected properties in the `__index` metamethod in multiple metatables.

## Parameters

| # | Type    | Sub Type           | Information |
|---|---------|--------------------|-------------|
| 1 | table   | [CustomPropertyInfo](https://github.com/UE4SS/UE4SS/wiki/Table%3A-CustomPropertyInfo) | A table containing all of the required information for registering a custom property |

## Example

### Simple Property

Registers a custom property with the name `MySimpleCustomProperty` that accesses whatever data is at offset `0xF40` in any instance of class `Character` as if it was an `IntProperty`.

It then grabs that value of the first instance of the class `Character` as an example of how the system works.
```lua
RegisterCustomProperty({
    ["Name"] = "MySimpleCustomProperty",
    ["Type"] = PropertyTypes.IntProperty,
    ["BelongsToClass"] = "/Script/Engine.Character",
    ["OffsetInternal"] = 0xF40
})

local CharacterInstance = FindFirstOf("Character")
if CharacterInstance:IsValid() then
    local MySimplePropertyValue = CharacterInstance.MySimpleCustomProperty
end
```

### ArrayProperty

For array properties, you need to specify the inner type using the `ArrayProperty` table:

```lua
RegisterCustomProperty({
    ["Name"] = "MyCustomArray",
    ["Type"] = PropertyTypes.ArrayProperty,
    ["BelongsToClass"] = "/Script/Engine.Character",
    ["OffsetInternal"] = 0x100,
    ["ArrayProperty"] = {
        ["Type"] = PropertyTypes.IntProperty
    }
})

local CharacterInstance = FindFirstOf("Character")
if CharacterInstance:IsValid() then
    local ArrayLength = CharacterInstance.MyCustomArray:GetArrayNum()
    for i = 1, ArrayLength do
        local Value = CharacterInstance.MyCustomArray[i]
        print(string.format("Array[%d] = %d", i, Value))
    end
end
```

### StructProperty

For struct properties, you need to specify the struct type by name using the `StructProperty` table. The struct must be a reflected `UScriptStruct` that exists in the game:

```lua
RegisterCustomProperty({
    ["Name"] = "MyCustomStruct",
    ["Type"] = PropertyTypes.StructProperty,
    ["BelongsToClass"] = "/Script/MyGame.MyClass",
    ["OffsetInternal"] = 0x28,
    ["StructProperty"] = {
        ["Name"] = "/Script/MyGame.MyStructType"
    }
})

local MyObject = FindFirstOf("MyClass")
if MyObject:IsValid() then
    -- Access struct fields directly
    local StructField = MyObject.MyCustomStruct.SomeField
end
```
