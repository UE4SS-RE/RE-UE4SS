# FindObject

`FindObject` is a function that finds an object. 

Overload #1 finds by either class name or short object name.

Overload #2 works the same way as `FindObject` in the [UE source](https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/FindObject/).

## Parameters (overload #1)

| # | Type | Information |
|---|------|-------------|
| 1 | string\|FName\|nil | The class name |
| 2 | string\|FName\|nil | The short object name |
| 3 | EObjectFlags | The required object flags |
| 4 | EObjectFlags | The excluded object flags |

> Param 1 or Param 2 can be nil, but not both.

## Parameters (overload #2)

| # | Type | Information |
|---|------|-------------|
| 1 | UClass | The class to find |
| 2 | UObject | The outer to look inside. If this is null then param 3 should start with a package name |
| 3 | string | The object path to search for an object, relative to param 2 |
| 4 | bool | Whether to require an exact match with the UClass parameter |

## Return Value (overload #1 & #2)

| # | Type | Information |
|---|------|-------------|
| 1 | UObject | The derivative of the UObject |

## Example (overload #1)

```lua
local Object = FindObject("/Script/Engine.Character", "Engine.World", EObjectFlags.RF_ClassDefaultObject, EObjectFlags.RF_ClassDefaultObject)


if not Object:IsValid() then
    print("No instance of class 'Character' was found.")
end
```

## Example (overload #2)

```lua
local Object = FindObject(UClass, World, "Character", true)
```