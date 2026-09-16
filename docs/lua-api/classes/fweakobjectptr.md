# FWeakObjectPtr

## Inheritance
[LocalObject](./localobject.md)

## Methods

### Get() / get()

- **Return type:** `UObjectDerivative`
- **Returns:** the pointed to `UObject` or `UObject` derivative.
> The return can be invalid, so call `UObject:IsValid` after calling this function.

## Property Assignment

Reflected `FWeakObjectProperty` fields on `UObject` instances can be assigned directly from Lua:
- Assigning a `UObject`: sets the weak pointer to reference the target object.
- Assigning an `FWeakObjectPtr`: copies the weak pointer reference.
- Assigning `nil`: clears the weak pointer reference.

```lua
-- Assign an actor to a weak object property
TargetActor.WeakTarget = CurrentEnemy

-- Clear a weak object property
TargetActor.WeakTarget = nil
```