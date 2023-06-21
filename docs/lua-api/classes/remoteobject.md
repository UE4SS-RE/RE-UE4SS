# RemoteObject

The `RemoteObject` class is the first of two base objects that all other objects inherits from, the other one being [LocalObject](./localobject.md).

It contains a pointer to a C++ object that is typically owned by the game.

## Inheritance
None

## Methods

### IsValid()

- **Return type:** `bool`
- **Returns:** whether this object is valid or not

**Example**
```lua
-- 'StaticFindObject' returns a UObject which inherits from RemoteObject.
local Object = StaticFindObject("/Script/CoreUObject.Object")
if Object:IsValid() then
    print("Object is valid\n")
else
    print("Object is NOT valid\n")
end
```
