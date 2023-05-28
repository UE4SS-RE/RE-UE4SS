# StaticFindObject

The `StaticFindObject` function is used to find any object that inherits from `UObject` that currently exists in memory.

This function is the recommended way of retrieving non-instance objects such as objects of type UClass or UFunction.  

## Parameters (overload #1)

| # | Type    | Information |
|---|---------|-------------|
| 1 | string  | Full name of the object to find, without the type prefix |

## Parameters (overload #2)

The parameters for this overload mimics the `StaticFindObject` function from UE4.  
For more information see: [Unreal Engine API -> StaticFindObject](https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/StaticFindObject/)

| # | Type    | Information |
|---|---------|-------------|
| 1 | UClass  | The class of the object to find, can be nil. |
| 2 | UObject | The outer to look inside. All packages are searched if nil. |
| 3 | string  | Name of the object to find |
| 4 | bool    | Whether to require an exact match with the UClass parameter |

## Return Value (overload #1 & #2)

| # | Type          | Information |
|---|---------------|-------------|
| 1 | UObject, UClass, or AActor | Object is only valid if an instance was found |

## Example (overload #1)

```lua
local CharacterInstance = StaticFindObject("/Script/Engine.Character")
if not CharacterInstance:IsValid() then
    print("No instance of class 'Character' was found.")
end
```