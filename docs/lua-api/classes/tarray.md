# TArray

## Inheritance
[RemoteObject](./remoteobject.md)

## Metamethods

### __index
- **Usage:** `TArray[ArrayIndex]`
- **Return type:** `auto`
- Attempts to retrieve the value at the specified integer offset `ArrayIndex` in the array.
- Can return any type, you can use the `type()` function on the returned value to figure out what Lua class it's using (if non-trivial type).

### __newindex
- **Usage:** `TArray[ArrayIndex] = NewValue`
- Attempts to set the value at the specified integer offset `ArrayIndex` in the array to `NewValue`.

### __len
- **Usage:** `#TArray`
- **Return type:** `integer`
- Returns the number of current elements in the array.

## Methods

### GetArrayAddress()
- **Return type:** `integer`
- **Returns:** the address in memory where the `TArray` struct is located.

### GetArrayNum()
- **Return type:** `integer`
- **Returns:** the number of current elements in the array.

### GetArrayMax()
- **Return type:** `integer`
- **Returns:** the maximum number of elements allowed in this array (aka capacity).

### GetArrayDataAddress()
- **Return type:** `integer`
- **Returns:** the address in memory where the data for this array is stored.

### Empty()
- Clears the array.

### ForEach(function Callback)
- Iterates the entire `TArray` and calls the callback function for each element in the array.
- The callback params are: `integer index`, `RemoteUnrealParam elem` | `LocalUnrealParam elem`.
- Use `elem:get()` and `elem:set()` to access/mutate an array element.
