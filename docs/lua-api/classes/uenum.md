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

### GetEnumNameByIndex(integer Index)
- **Return types:** `FName`, `Integer`
- **Returns:** the `FName` that coresponds the given `Index`.
- **Returns:** the `Integer` value that coresponds the given `Index`.

### InsertIntoNames(string Name, integer Value, integer Index, boolean ShiftValues = true)
- Inserts a `FName`/`Value` combination into a a `UEnum` at the given `Index`.
- If `ShiftValues = true`, will shift all enum values greater than inserted value by one.

### EditNameAt(integer Index, string NewName)
- At a given `Index`, will modify the found element in the `UEnum` and replace its `Name` with the given `NewName`.

### EditValueAt(integer Index, integer NewValue)
- At a given `Index`, will modify the found element in the `UEnum` and replace its value with the given `NewValue`.

### RemoveFromNamesAt(integer Index, integer Count = 1, boolean AllowShrinking = true)
- Will remove `Count` element(s) at the given `Index` from a `UEnum`.
- If `AllowShrinkning = true`, will shrink the enum array when removing elements.