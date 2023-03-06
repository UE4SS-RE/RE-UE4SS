# Mod

The `Mod` class is responsible for interacting with the local mod object.

## Inheritance
[RemoteObject](./remoteobject.md)

## Methods

### SetSharedVariable(string VariableName, any Value)

- Sets a variable that can be accessed by any mod.
- The second parameter `Value` can only be one of the following types: `nil`, `string`, `number`, `bool`, `UObject` (+derivatives), `lightuserdata`.
- These variables do not get reset when hot-reloading.

### GetSharedVariable(string VariableName)

- **Return type:** `any`
- **Returns:** a variable that could've been set from another mod.
- The return value can only be one of the following types: `nil`, `string`, `number`, `bool`, `UObject`(+derivatives), `lightuserdata`.

### type()

- **Return type:** `string`
- **Returns:** "ModRef"