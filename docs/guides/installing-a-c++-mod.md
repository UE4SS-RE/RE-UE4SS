# Installing a C++ Mod

1. This part assumes you have UE4SS installed and working for your game already. If not, refer to the [installation guide](../installation-guide.md). 

2. After building, you will have the following file:
    - `MyAwesomeMod.dll` in `MyMods\Binaries\<Configuration>\MyAwesomeMod`
    
3. Navigate over to your game's executable folder and open the `Mods` folder. Here we'll do a couple things:  
    - Create a folder structure in `Mods` that looks like `MyAwesomeMod\dlls`. 
    - Move `MyAwesomeMod.dll` inside the `dlls` folder and rename it to `main.dll`.

The result should look like:
```
Mods\
    MyAwesomeMod\
        dlls\
            main.dll
```    

4. To enable loading of your mod in-game you will have to edit the `mods.txt` located in the `Mods` folder. By default it looks something like this:
```
CheatManagerEnablerMod : 1
ActorDumperMod : 0
ConsoleCommandsMod : 1
ConsoleEnablerMod : 1
SplitScreenMod : 0
LineTraceMod : 1
BPModLoaderMod : 1
jsbLuaProfilerMod : 0



; Built-in keybinds, do not move up!
Keybinds : 1
```

Here you will want to add the line: 
```
MyAwesomeMod : 1
```
**above** the keybinds to enable `MyAwesomeMod`.

Alternatively, place an empty text file named `enabled.txt` inside of the MyAwesomeMod folder.  This method is not recommended because it does not allow load ordering  
and bypasses mods.txt, but may allow for easier installation by end users.

5. Launch your game and if everything was done correctly, you should see the text "MyAwesomeMod says hello" highlighted in blue somewhere at the top of UE4SS console (before all the scanning occurs), and if you used the `on_unreal_init` function, you should see "Object Name: /Script/CoreUObject.Object" highlighted in blue as well (right after the scanning finishes).

## Automation

Now that you understand how the process works, you can use the [UE4SS CPP Template](https://github.com/UE4SS-RE/UE4SSCPPTemplate) repository that automates the process of creating a mod, building it, and installing it. Be aware that the `new_mod_setup.bat` script will checkout the commit at the latest release so that you can be sure that your mod is being built with the correct ABI as latest release. 

> NOTE: Any changes to the build system that affects the mod template is pushed to the `dev` branch, which is then merged into main when a new UE4SS release is created. This makes sure that the template is always in-sync with the latest UE4SS release.