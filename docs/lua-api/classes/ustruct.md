# UStruct

## Inheritance
[UObject](./uobject.md)

## Methods

### GetSuperStruct()

- **Return type:** `UClass`
- **Returns:** the `SuperStruct` of this struct (can be invalid).

## ForEachFunction(function Callback)

- Iterates every `UFunction` that belongs to this struct.
- The callback has one param: `UFunction Function`.
- Return `true` in the callback to stop iterating.

## ForEachProperty(function Callback)

- Iterates every `Property` that belongs to this struct.
- The callback has one param: `Property Property`.
- Return `true` in the callback to stop iterating.