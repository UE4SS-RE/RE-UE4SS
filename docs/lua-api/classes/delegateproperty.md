# DelegateProperty

Represents a single-cast delegate property in Unreal Engine.

## Inheritance
[Property](./property.md)

## Description

`DelegateProperty` wraps Unreal Engine's `FDelegateProperty`, which represents a single-cast delegate that can bind one object and function pair. Delegate properties are accessed via their property wrapper (using `GetPropertyByName`) for metadata operations, while delegate **values** are accessed directly via `Object.PropertyName` and return simple tables.

## Reading Delegate Values

Delegate values are accessed directly on UObjects and return a table with `Object` and `FunctionName` fields:

```lua
local PlayerPawn = GetPlayerController().Pawn

-- Read single-cast delegate value
local delegate = PlayerPawn.OnSomeDelegate
if delegate then
    print("Bound to:", delegate.Object:GetFullName())
    print("Function:", tostring(delegate.FunctionName))
end
```

## Setting Delegate Values

Single-cast delegates can be set by assigning a table with `Object` and `FunctionName` fields, or `nil` to clear:

```lua
-- Bind a delegate
PlayerPawn.OnSomeDelegate = {
    Object = SomeObject,
    FunctionName = FName("MyCallbackFunction")
}

-- Clear a delegate
PlayerPawn.OnSomeDelegate = nil
```

## Notes

- Single-cast delegates do not have `Add`, `Remove`, or `Clear` methods (use multicast delegates for those operations)
- The property wrapper itself provides no additional methods beyond those inherited from [Property](./property.md)
- Delegate values are plain Lua tables, not userdata objects
