# Installation Guide

## Core structure concept

There are four concepts you need to know about.

1. The `root directory`.
    - This directory contains the ue4ss dll file.
2. The `working directory`.
    - This directory contains configuration and mod files and is located inside the `root directory`.
3. The `game directory`.
    - This directory usually contains a small executable with the name of your game and a folder with the same name.  
    - This executable is _not_ your actual game but instead it's just a small wrapper that starts any 3rd party launcher such as Steam or if there is none then it launches the real executable.
    - Example of a `game directory`: `D:\Games\Epic Games\SatisfactoryEarlyAccess\`
4. The `game executable directory`.
    - This directory contains the real executable file for your game and is not part of the UE4SS directory structure.
    - Example of a `game executable directory`: 
    `D:\Games\Epic Games\SatisfactoryEarlyAccess\FactoryGame\Binaries\Win64\`

## Choosing an installation directory

You can install UE4SS in a couple of different ways.

### Method #1 (Preferred)

> Using method #1 will mean that the `root directory` and `working directory` are treated as one single directory that happens to also be the same directory as your `game working directory`.

The preferred and most straightforward way to install UE4SS is to choose the `ue4ss_xinput` download and then just drag & drop all the necessary files in `<game executable directory>`.

Now all you need to do is start your game and UE4SS will automatically be injected.

### Method #2

> Using method #2 will mean that the `root directory` and `working directory` are treated as one single directory that happens to also be the same directory as your `game working directory`.

Same as using method #1 except you choose the "ue4ss_standard" download and any instances of `xinput1_3.dll` becomes `ue4ss.dll`.

This method requires that you manually inject `ue4ss.dll` after the game is launched.

### Method #3

This method is a way to install UE4SS in one place for all your games. Simply extract the zip file in any directory _outside_ the `game directory`, this is what's known as the `root directory`.  

You will then create a folder inside with the name of your game and drag `UE4SS-settings.ini` in to it, this is what's known as the `working directory`.

If the path to your game executable is

```
D:\Games\Epic Games\SatisfactoryEarlyAccess\FactoryGame\Binaries\Win64\FactoryGame-Win64-Shipping.exe
```

Then the name of your `working directory` should be `SatisfactoryEarlyAccess`.  
This directory will be automatically found and used by UE4SS if it exists.

As of UE4SS 1.1, the following files & folders exist inside the `working directory`:

- Mods
    - Mod folders
    - mods.txt
- UE4SS-settings.ini
- UE4SS.log
- UE4SS-ObjectDump.txt

Now all you need to do is start your game and point your injector of choice to `<root directory>/ue4ss.dll`.

## Last but not least...

If you use method #3, if you keep a copy of `UE4SS-settings.ini` inside the `root directory` then this file will act as a default for all the games that don't have a `working directory` as long as you still point your injector to the `root directory`.  

This way you can use method #3 for most of your games and at the same time you can use method #2 for other games if method #3 is problematic for you.