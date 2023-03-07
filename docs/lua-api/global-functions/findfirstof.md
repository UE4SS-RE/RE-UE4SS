# FindFirstOf

The `FindFirstOf` function will find the first non-default instance of the supplied class name.

> This function cannot be used to find non-instances or default instances.

## Parameters

| # | Type    | Information |
|---|---------|-------------|
| 1 | string  | Short name of the class to find an instance of |

## Return Value

| # | Type          | Information |
|---|---------------|-------------|
| 1 | UObject, UClass, or AActor | Object is only valid if an instance was found |

## Example
```lua
local CharacterInstance = FindFirstOf("Character")
if not CharacterInstance:IsValid() then
    print("No instance of class 'Character' was found.")
end
```