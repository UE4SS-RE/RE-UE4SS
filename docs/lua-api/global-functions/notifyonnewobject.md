# NotifyOnNewObject

The `NotifyOnNewObject` function executes a callback whenever an instance of the supplied class is constructed via `StaticConstructObject_Internal` by UE4.

Inheritance is taken into account, so if you provide `"/Script/Engine.Actor"` as the class then it will execute the callback when any object is constructed that's either an `AActor` or is derived from `AActor`.

> The provided class must exist before this calling this function.

## Parameters

| # | Type     | Information |
|---|----------|-------------|
| 1 | string   | Full name of the class to get instance construction notifications for, without the type prefix |
| 2 | function | The callback to execute when an instance of the supplied class is constructed |

## Return Value

| # | Type    | Information |
|---|---------|-------------|
| 1 | UObject | The constructed object |

## Example
```lua
NotifyOnNewObject("/Script/Engine.Actor", function(ConstructedObject)
    print(string.format("Constructed: %s\n", ConstructedObject:GetFullName()))
end)
```