# LocalObject

The `LocalObject` class is the second of two base objects that all other objects inherits from, the other one being [RemoteObject](./remoteobject.md).

It contains an inlined C++ object that is owned by Lua.

## Inheritance
None

## Methods

### IsValid()

- **Return type:** `bool`
- **Returns:** always `true` for historic compatibility reasons

**Example**
```lua
-- 'FText' inherits from LocalObject.
local Text = FText("SomeText")
if Text:IsValid() then
    print("This should always happen\n")
else
    print("This should never happen\n")
end
```