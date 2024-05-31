# UFunction

## Inheritance
[UObject](./uobject.md)

## Metamethods

### __call
- **Usage:** `UFunction(UFunctionParams...)`
- **Return type:** `auto`
- Attempts to call the `UFunction` and returns the result, if any.
- If the `UFunction` is obtained without a context (e.g. from `StaticFindObject`), a `UObject` context must be passed as the first parameter.

## Methods

### GetFunctionFlags()

- **Return type:** `integer`
- **Returns:** the flags for the `UFunction`.

### SetFunctionFlags(integer Flags)
- Sets the flags for the `UFunction`.
