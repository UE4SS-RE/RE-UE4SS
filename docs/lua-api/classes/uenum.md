# UEnum

## Inheritance
[RemoteObject](./remoteobject.md)

## Methods

### GetNameByValue(integer Value)

- **Return type:** `FName`
- **Returns:** the `FName` that corresponds to the specified value.

### ForEachName(LuaFunction Callback)

- Iterates every `FName`/`Value` combination that belongs to this enum.
- The callback has two params: `FName Name`, `integer Value`.
- Return `true` in the callback to stop iterating.