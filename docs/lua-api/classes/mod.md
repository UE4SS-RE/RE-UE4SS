# Mod

The `Mod` class is responsible for interacting with the local mod object.

## Inheritance
[RemoteObject](./remoteobject.md)

## Methods

### SetSharedVariable(string VariableName, any Value)

- Sets a variable that can be accessed by any mod.
- The second parameter `Value` can only be one of the following types: `nil`, `string`, `number`, `bool`, `UObject` (+derivatives), `lightuserdata`.
> Warning: These variables do not get reset when hot-reloading.

**Example**
```lua
-- When sharing a UObject, make absolutely sure that it's a UObject that doesn't cease to exist before it's used again.
-- It's a very bad idea to share transient objects like actors as they might die and stop existing.
local StaticObject = StaticFindObject("/Script/Engine.Default__GameplayStatics")

-- The 'ModRef' variable is a global variable that's automatically created and is the instance of the current mod.
ModRef:SetSharedVariable("MyVariable", StaticObject)
```

### GetSharedVariable(string VariableName)

- **Return type:** `any`
- **Returns:** a variable that could've been set from another mod.
- The return value can only be one of the following types: `nil`, `string`, `number`, `bool`, `UObject`(+derivatives), `lightuserdata`.

**Example**
```lua
-- Assuming that the example script for 'SetSharedVariable' has been executed.
local SharedObject = ModRef:GetSharedVariable("MyVariable")

-- 'GetSharedVariable' may return anything that its able to store.
-- Any mod is able to override the value for any shared variable.
if SharedObject and type(SharedObject) == "userdata" and SharedObject:type() == "UObject" and SharedObject:IsValid() then
    print(string.format("SharedObject '%s' is valid.\n", SharedObject:GetFullName()))
else
    print("SharedObject was nil, not userdata, not a UObject, or an invalid UObject")
end
```

### type()

- **Return type:** `string`
- **Returns:** "ModRef"
