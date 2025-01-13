# FindObject

`FindObject` is a function that finds an object. 

Overload #1 finds by either class name or short object name.

Overload #2 works the same way as `FindObject` in the [UE source](https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/FindObject/).

## Parameters (overload #1)

| # | Type | Information |
|---|------|-------------|
| 1 | string\|FName\|UClass\|nil | The short name of the object's class or the UClass itself |
| 2 | string\|FName\|nil | The short name of the object itself |
| 3 | EObjectFlags | Any flags that the object cannot have. Uses \| as a seperator |
| 4 | EObjectFlags | Any flags that the object must have. Uses \| as a seperator |

> Param 1 or Param 2 can be nil, but not both.

## Parameters (overload #2)

| # | Type | Information |
|---|------|-------------|
| 1 | UClass | The class to find |
| 2 | UObject\|UClass | The outer to look inside. If this is null then param 3 should start with a package name |
| 3 | string | The object path to search for an object, relative to param 2 |
| 4 | bool | Whether to require an exact match with the UClass parameter |

## Return Value (overload #1 & #2)

| # | Type | Information |
|---|------|-------------|
| 1 | UObject | The derivative of the UObject |

## Example (overload #1)

```lua
-- SceneComponent instance called TransformComponent0
local Object = FindObject("SceneComponent", "TransformComponent0")
```

```lua
-- FirstPersonCharacter_C instance called FirstPersonCharacter_C_0
local Object = FindObject("FirstPersonCharacter_C", "FirstPersonCharacter_C_0", EObjectFlags.RF_NoFlags, EObjectFlags.RF_ClassDefaultObject)
```

Alternatively, you can use a UClass object:

```lua
local Object = FindObject(SceneComponentClass, "TransformComponent0")
```

## Example (overload #2)

```lua
local Object = FindObject(UClass, World, "Character", true)
```
