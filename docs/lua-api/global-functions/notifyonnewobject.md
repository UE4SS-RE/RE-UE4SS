# NotifyOnNewObject

The `NotifyOnNewObject` function executes a callback whenever an instance of the supplied class is constructed via `StaticConstructObject_Internal` by UE4.

Inheritance is taken into account, so if you provide `"/Script/Engine.Actor"` as the class then it will execute the callback when any object is constructed that's either an `AActor` or is derived from `AActor`.

The callback can return `true` to remove (unregister) that specific `NotifyOnNewObject` callback.

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


## Callback Return Value

| # | Type    | Information |
|---|---------|-------------|
| 1 | boolean | If true, remove the current callback from listening to any more notifications |


## Example
```lua
NotifyOnNewObject("/Script/Engine.World", function(ConstructedObject)
    print(string.format("World Constructed: %s\n", ConstructedObject:GetFullName()))
end)

NotifyOnNewObject("/Script/Engine.Actor", function(ConstructedObject)
    print(string.format("Actor Constructed: %s\n", ConstructedObject:GetFullName()))

    -- Unregister this callback after the first actor is found,
    -- meaning this current function will only be called exactly once
    return true
end)
```

## What NOT to do

Please don't duplicate the `NotifyOnNewObject` call for the same class multiple times, as it could cause performance issues if multiple mods are doing it (which has been seen in the wild).

For example, this:
```lua
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.bShowMouseCursor = true
end)
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.bForceFeedbackEnabled = false
end)
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.InputYawScale = 2.5
end)
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.InputPitchScale = -2.5
end)
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.InputRollScale = 1.0
end)
```

should just be this:
```lua
NotifyOnNewObject("/Script/Engine.PlayerController", function(PlayerController)
    PlayerController.bShowMouseCursor = true
    PlayerController.bForceFeedbackEnabled = false
    PlayerController.InputYawScale = 2.5
    PlayerController.InputPitchScale = -2.5
    PlayerController.InputRollScale = 1.0
end)
```
