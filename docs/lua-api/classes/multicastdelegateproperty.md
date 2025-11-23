# MulticastDelegateProperty

Represents a multicast inline delegate property in Unreal Engine.

## Inheritance
[Property](./property.md)

## Description

`MulticastDelegateProperty` wraps Unreal Engine's `FMulticastDelegateProperty`, which represents a multicast delegate that can bind multiple object and function pairs. The delegate uses inline storage for its invocation list.

## Accessing Delegate Properties

When you access a multicast delegate property on a UObject, it returns a property wrapper object with methods to modify the delegate:

```lua
local PlayerController = GetPlayerController()

-- Access delegate property (returns property wrapper)
local delegateProp = PlayerController.OnPossessedPawnChanged

-- The wrapper has Add, Remove, and Clear methods
delegateProp:Add(MyCustomObject, FName("OnPawnChanged"))
```

## Methods

### Add(targetObject, functionName)

Adds a delegate binding to the multicast delegate's invocation list.

- **Parameters:**
  - `targetObject` (UObject): The object to bind the delegate to
  - `functionName` (FName | string): The name of the function to call on the target object

```lua
-- Access the delegate property wrapper
local prop = PlayerController.OnPossessedPawnChanged

-- Add a delegate binding using FName
prop:Add(MyCustomObject, FName("OnPawnChanged"))

-- Or use a string directly
prop:Add(MyCustomObject, "OnPawnChanged")
```

### Remove(targetObject, functionName)

Removes a specific delegate binding from the invocation list.

- **Parameters:**
  - `targetObject` (UObject): The object the delegate is bound to
  - `functionName` (FName | string): The name of the bound function

```lua
-- Remove a specific binding
prop:Remove(MyCustomObject, FName("OnPawnChanged"))

-- Or use a string
prop:Remove(MyCustomObject, "OnPawnChanged")
```

### Clear()

Removes all delegate bindings from the invocation list.

```lua
-- Clear all bindings
prop:Clear()
```

### GetBindings()

Returns an array of all current delegate bindings.

- **Returns:** Array of tables, where each table has `Object` (UObject) and `FunctionName` (FName) fields, or `nil` if no bindings exist

```lua
-- Get all current bindings
local bindings = prop:GetBindings()
if bindings then
    print("Delegate has", #bindings, "bindings")
    for i, binding in ipairs(bindings) do
        print(string.format("[%d] %s::%s",
            i,
            binding.Object:GetFullName(),
            binding.FunctionName:ToString()))
    end
end
```

### Broadcast(...)

Fires the delegate, calling all bound functions with the provided parameters.

- **Parameters:** Variadic - depends on the delegate's signature function
- **Note:** Parameters must match the delegate signature's expected types and order

```lua
-- For a delegate with no parameters
prop:Broadcast()

-- For a delegate with parameters (e.g., OnDamage(float Amount, AActor* Causer))
prop:Broadcast(25.0, CauserActor)
```

## Notes

- Accessing `Object.PropertyName` for a delegate property returns a **property wrapper** object, not the delegate values
- The container object is implicitly stored in the wrapper, so you don't need to pass it to methods
- Both `FName` and string literals are accepted for function names
- See also: [MulticastSparseDelegateProperty](./multicastsparsedelegateproperty.md) for sparse storage variant
