# TMap

## Inheritance
[RemoteObject](./remoteobject.md)

## Metamethods

### __len
- **Usage:** `#TMap`
- **Return type:** `integer`
- Returns the number of current pairs in the map.

## Methods

### Find(key)
- **Return type:** `RemoteUnrealParam` | `LocalUnrealParam`
- **Returns:** the element found in the map
- **Throws:** if an element was not found in the map
- Finds the specified key in the map

### Add(key, value)
- Inserts a key/value pair into the map. If the key already exists in the map, replaces the value.

### Contains(key)
- **Return type:** `bool`
- **Returns:** if the element exists in the map.
- Checks if a key exists inside of the map.

### Remove(key)
- Removes an element from the map. If an element doesn't exist, does nothing.

### Empty()
- Clears the map.

### ForEach(function Callback)
- Iterates the entire `TMap` and calls the callback function for each element in the array.
- The callback params are: `RemoteUnrealParam key`, `RemoteUnrealParam value` | `LocalUnrealParam value`.
- Use `elem:get()` and `elem:set()` to access/mutate the value.
- Mutating the key is undefined behavior.
