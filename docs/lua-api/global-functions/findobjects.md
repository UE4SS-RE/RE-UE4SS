# FindObjects

Finds the first specified number of objects by class name or short object name.

To find all objects that match your criteria, set param 1 to `0` or `nil`.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | integer | The number of objects to find |
| 1 | string\|FName\|nil | The short name of the class of the object |
| 2 | string\|FName\|nil | The short name of the object itself |
| 3 | EObjectFlags | Any flags that the object cannot have. Uses \| as a seperator |
| 4 | EObjectFlags | Any flags that the object must have. Uses \| as a seperator |
| 6 | bool | Whether to require an exact match with the UClass parameter |

## Return Value

| # | Type | Sub Type | Information |
|---|------|----------|-------------|
| 1 | table | UObject | The derivative of the UObject |

## Example

```lua
local Object = FindObjects(4, "SceneComponent", "TransformComponent0", EObjectFlags.RF_NoFlags, EObjectFlags.RF_ClassDefaultObject, true)

for _, Object in pairs(Objects) do
    -- Do something with Object
end
```