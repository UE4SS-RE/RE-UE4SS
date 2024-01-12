# Creating a Lua mod

## Before you start
To create a Lua mod in UE4SS, you should first: 
* know [how to install UE4SS](../installation-guide.md) in your target game and make sure it is running OK;
* be able to write basic Lua code (see the official book [Programming in Lua](https://www.lua.org/pil/contents.html) and its later editions, or any other recommended tutorial online);
* have an understanding of the object model of the Unreal Engine and the basics of game modding.

## How does a minimal Lua mod look like

A Lua mod in UE4SS is a set of Lua scripts placed in a folder inside the `Mods/` folder of UE4SS installation. 
Let's call it `MyLuaMod` for the purpose of this example.

In order to be loaded and executed:
1) The mod folder must have a `scripts` subfolder and a `main.lua` file inside, so it looks like:
```
Mods\
    ...
    MyLuaMod\
        scripts\
            main.lua
    ...
```

2) The `Mods\MyLuaMod\scripts\main.lua` file has some Lua code inside it, e.g.:
```lua
print("[MyLuaMod] Mod loaded\n")
```

3) The mod must be added and enabled in `Mods\mods.txt` with a new line containing the name of your mod folder (name of your mod) and `1` for enabling or `0` for disabling the mod:
```
...
MyLuaMod : 1
...
```

Your custom functionality goes inside `main.lua`, from which you can include other Lua files if needed, including creating your own Lua modules or importing various libraries.

## What can you do in a Lua mod

The API provided by UE4SS and available to you in Lua is documented in sub-sections of chapter ["Lua API"](../lua-api.md) here. Using those functions and classes, you find and manipulate the instances of Unreal Engine objects in memory, creating new objects or modifying existing ones, calling their methods and accessing their fields.

Basically, you are doing the exact same thing that an Unreal Engine game developer does in their code, but using UE4SS to locate the necessary objects and guessing a bit, while the developers already knew where and what they are (because they have their source code).

### Creating simple data types
If you need to create an object of a structure-like class, e.g. `FVector`, in order to pass it into a Unreal Engine function, UE4SS allows you to pass a Lua table with the fields of the class like `{X=1.0, Y=2.0, Z=3.0}` instead.

### Using Lua C libraries
If you ever need to load Lua C libraries, that have native code (i.e. with DLLs on Windows), 
you can place these DLLs directly inside the same `\scripts\` folder.

# Setting up a Lua mod development environment

It is much easier to write mods if your code editor or IDE is properly configured for Lua development and knows about UE4SS API. 

1) Configure your code editor/IDE to support Lua syntax highlighting and code completion. If you use VSCode, see here in [Using Custom Lua Bindings](./using-custom-lua-bindings).

2) Make sure that your build of UE4SS contains `Mods\shared\Types.lua` (a development build from Github releases contains it). This will load the UE4SS API definitions in your IDE.

3) (Optional) [Dump the Lua Bindings](./using-custom-lua-bindings.md#dumping-custom-lua-bindings) fromm UE4SS Gui console, and [follow the recommendations to load them](./using-custom-lua-bindings.md#to-use-bindings) here.

Then open the `Mods/` folder of your UE4SS installation in your IDE, and create or modify your mod inside it.

# Applying code changes

The main benefit of developing Lua mods is that you can quickly edit Lua sources without recompiling/rebuilding the C++ mod library as is always the case with C++ mods, and retry without restarting the game.

You can either:
* reload all mods from the UE4SS GUI Console with the "Restart All Mods" button on the "Console" tab, or,
* enable "Hot reload" in `UE4SS-settings.ini` and use the assigned hotkey (`Ctrl+R` by default) to do the same.

# Your first mod

In the `main.lua` file of your mod, write some code that will try to access the objects of Unreal Engine inside your target game and do something that you can observe in the UE4SS console.

You can start by trying just 
```lua
print("[MyLuaMod] Mod loaded\n")
```
and once you have verified that it runs OK, you can start implementing some actual functionality.

The example code below is fairly generic and should work for many games supported by UE4SS.

It registers a hotkey `Ctrl+F1` and when pressed, it reads the player coordinates 
and calculates how far the player has moved since the last time the hotkey was pressed. 

> Note that the logging `print` calls include the name of the mod in square brackets, as it helps you
find your mod's output among other log strings in the console.

The player coordinates are retrieved in the following way:

1) Gets the [player controller](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/APlayerController/) using [UE4SS `UEHelpers` class](https://github.com/UE4SS-RE/RE-UE4SS/blob/main/assets/Mods/shared/UEHelpers/UEHelpers.lua).
2) Get [the `Pawn`](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/APawn/), which represents the actual "physical" entity that the player can control in Unreal Engine.
3) Call the appropriate Unreal Engine [method `K2_GetActorLocation`](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/AActor/K2_GetActorLocation/) that returns a `Pawn`'s location (by accessing its parent `Actor` class).
4) The location is a 3-component vector of Unreal Engine type `FVector`, having `X`, `Y` and `Z` as its fields.


```lua
local UEHelpers = require("UEHelpers")

print("[MyLuaMod] Mod loaded\n")

local lastLocation = nil

function ReadPlayerLocation()
    local FirstPlayerController = UEHelpers:GetPlayerController()
    local Pawn = FirstPlayerController.Pawn
    local Location = Pawn:K2_GetActorLocation()
    print(string.format("[MyLuaMod] Player location: {X=%.3f, Y=%.3f, Z=%.3f}\n", Location.X, Location.Y, Location.Z))
    if lastLocation then
        print(string.format("[MyLuaMod] Player moved: {delta_X=%.3f, delta_Y=%.3f, delta_Z=%.3f}\n",
            Location.X - lastLocation.X,
            Location.Y - lastLocation.Y,
            Location.Z - lastLocation.Z)
        )
    end
    lastLocation = Location
end

RegisterKeyBind(Key.F1, { ModifierKey.CONTROL }, function()
    print("[MyLuaMod] Key pressed\n")
    ExecuteInGameThread(function()
        ReadPlayerLocation()
    end)
end)

```

When you load the game until you can move the character, press the hotkey, move the player, press it again, the mod will generate a following output or something very similar:
```
...
[2024-01-09 19:37:27] Starting Lua mod 'MyLuaMod'
[2024-01-09 19:37:27] [Lua] [MyLuaMod] Mod loaded
...
[2024-01-09 19:37:32] [Lua] [MyLuaMod] Key pressed
[2024-01-09 19:37:32] [Lua] [MyLuaMod] Player location: {X=-63.133, Y=4.372, Z=90.000}
[2024-01-09 19:37:39] [Lua] [MyLuaMod] Key pressed
[2024-01-09 19:37:39] [Lua] [MyLuaMod] Player location: {X=788.232, Y=-639.627, Z=90.000}
[2024-01-09 19:37:39] [Lua] [MyLuaMod] Player moved: {delta_X=851.364, delta_Y=-643.999, delta_Z=0.000}
...
```