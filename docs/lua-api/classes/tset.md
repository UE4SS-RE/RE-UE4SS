# TSet

## Inheritance
[RemoteObject](./remoteobject.md)

## Metamethods

### __len
- **Usage:** `#TSet`
- **Return type:** `integer`
- Returns the number of elements currently in the set.

## Methods

### Add(element)
- Adds an element to the set.
- If the element already exists in the set, does nothing (sets don't allow duplicates).

### Contains(element)
- **Return type:** `bool`
- **Returns:** whether the element exists in the set.

### Remove(element)
- Removes the specified element from the set.
- If the element doesn't exist, does nothing.

### Empty()
- Removes all elements from the set.

### ForEach(function Callback)
- Iterates through all elements in the set and calls the callback function for each element.
- The callback param is: `RemoteUnrealParam element` | `LocalUnrealParam element`.
- Use `element:get()` and `element:set()` to access/mutate the element.
- The callback can optionally return `true` to break out of the loop early.
- Example:
```lua
mySet:ForEach(function(element)
    local value = element:get()
    print(string.format("Element: %s", tostring(value)))
    -- Return true to stop iterating
    if value == targetValue then
        return true
    end
end)
```

## Notes
- TSet is an unordered collection, so the iteration order is not guaranteed.
- Elements in a TSet must be unique - adding a duplicate has no effect.
