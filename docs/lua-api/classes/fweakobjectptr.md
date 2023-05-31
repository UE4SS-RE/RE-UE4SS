# FWeakObjectPtr

## Inheritance
[LocalObject](./localobject.md)

## Methods

### Get()

- **Return type:** `UObjectDerivative`
- **Returns:** the pointed to `UObject` or `UObject` derivative.
> The return can be invalid, so call `UObject:IsValid` after calling this function.