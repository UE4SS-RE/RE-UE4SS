# FindObjects

Finds the first specified number of objects by class name or short object name.

To find all objects that match your criteria, set param 1 to `0` or `nil`.

## Parameters

| # | Type | Information |
|---|------|-------------|
| 1 | integer | The number of objects to find |
| 2 | string\|FName\|nil | The class name to find |
| 3 | string\|FName\|nil | The short object name to find |
| 4 | EObjectFlags | The required object flags |
| 5 | EObjectFlags | The excluded object flags |
| 6 | bool | Whether to require an exact match with the UClass parameter |

## Return Value

| # | Type | Sub Type | Information |
|---|------|----------|-------------|
| 1 | table | UObject | The derivative of the UObject |

## Example

```lua
local Objects = FindObjects(0, "/Script/Engine.Character", "Engine.World", EObjectFlags.RF_ClassDefaultObject, EObjectFlags.RF_ClassDefaultObject, true)

for _, Object in pairs(Objects) do
    if not Object:IsValid() then
        print("No instance of class 'Character' was found.")
    end
end
```