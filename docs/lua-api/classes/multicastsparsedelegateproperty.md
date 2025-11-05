# MulticastSparseDelegateProperty

Represents a multicast sparse delegate property in Unreal Engine.

## Inheritance
[Property](./property.md)

## Description

`MulticastSparseDelegateProperty` wraps Unreal Engine's `FMulticastSparseDelegateProperty`, which represents a multicast delegate that uses sparse storage. This is memory-efficient for delegates that are frequently unbound, as it stores only a 1-byte flag in the object and uses global storage for the actual invocation list.

Despite the different storage mechanism, the API is identical to [MulticastDelegateProperty](./multicastdelegateproperty.md).

## Accessing Delegate Properties

When you access a sparse multicast delegate property on a UObject, it returns a property wrapper object with methods to modify the delegate (identical API to inline multicast delegates):

```lua
local PlayerPawn = GetPlayerController().Pawn

-- Access delegate property (returns property wrapper)
local delegateProp = PlayerPawn.OnEndPlay

-- The wrapper has Add, Remove, and Clear methods
delegateProp:Add(MyCustomObject, FName("OnActorEndPlay"))
```

## Methods

### Add(targetObject, functionName)

Adds a delegate binding to the sparse multicast delegate's invocation list.

- **Parameters:**
  - `targetObject` (UObject): The object to bind the delegate to
  - `functionName` (FName | string): The name of the function to call on the target object

```lua
-- Access the delegate property wrapper
local prop = PlayerPawn.OnEndPlay

-- Add a delegate binding using FName
prop:Add(MyCustomObject, FName("OnActorEndPlay"))

-- Or use a string directly
prop:Add(MyCustomObject, "OnActorEndPlay")
```

### Remove(targetObject, functionName)

Removes a specific delegate binding from the invocation list.

- **Parameters:**
  - `targetObject` (UObject): The object the delegate is bound to
  - `functionName` (FName | string): The name of the bound function

```lua
-- Remove a specific binding
prop:Remove(MyCustomObject, FName("OnActorEndPlay"))

-- Or use a string
prop:Remove(MyCustomObject, "OnActorEndPlay")
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
-- For a delegate with no parameters (e.g., Actor.OnEndPlay)
prop:Broadcast()

-- For a delegate with parameters
prop:Broadcast(25.0, CauserActor)
```

## Implementation Details

`FMulticastSparseDelegateProperty` inherits from `FMulticastDelegateProperty` in the engine, so all the same vtable functions (`AddDelegate`, `RemoveDelegate`, `ClearDelegate`) are available and work identically.

## Notes

- Accessing `Object.PropertyName` for a sparse delegate property returns a **property wrapper** object, not the delegate values
- The container object is implicitly stored in the wrapper, so you don't need to pass it to methods
- Both `FName` and string literals are accepted for function names
- Storage mechanism is transparent to Lua - the API is identical to inline multicast delegates
- Common example: `Actor.OnEndPlay` is a `MulticastSparseDelegateProperty`
