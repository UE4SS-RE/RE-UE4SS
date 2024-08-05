# Installation

## Core structure concept

There are four concepts you need to know about.

1. The `root directory`.
    - This directory contains the UE4SS dll file.
2. The `working directory`.
    - This directory contains configuration and mod files and is located inside the `root directory`.
3. The `game directory`.
    - This directory usually contains a small executable with the name of your game and a folder with the same name.  
    - This executable is _not_ your actual game but instead it's just a small wrapper that starts any 3rd party launcher such as Steam or if there is none then it launches the real executable.
    - Example of a `game directory`: `D:\Games\Epic Games\SatisfactoryEarlyAccess\`
4. The `game executable directory`.
    - This directory contains the real executable file for your game and is not part of the UE4SS directory structure.
    - You can also recognize it as the game executable located there is usually the largest (much larger than the wrapper above) and is the one running as the child process of the wrapper when the game is running.
    - Example of a `game executable directory`: 
    `D:\Games\Epic Games\SatisfactoryEarlyAccess\FactoryGame\Binaries\Win64\`

## Choosing an installation method

You can install UE4SS in a couple of different ways.

The goal is to have the *.dll of UE4SS to be loaded by the target game one way or another, and have the configuration files and `\Mods\` directory in the correct place for UE4SS to find them.

### Method #1 - Basic Install

> Using method #1 will mean that the `root directory` and `working directory` are treated as one single directory that happens to also be the same directory as your `game executable directory`.

The basic install is intended for end-users who are using UE4SS for mods that use it and don't need to do any development work. There should be no extra windows visible when using this method.

The preferred and most straightforward way to install UE4SS is to choose the `UE4SS_v{version_number}` download (e.g. `UE4SS_v3.0.0`) and then just drag & drop all the necessary files into the `game executable directory`.

Now all you need to do is start your game and UE4SS will automatically be injected.

### Method #2 - Developer Install

> Using method #2 will mean that the `root directory` and `working directory` are treated as one single directory that happens to also be the same directory as your `game executable directory`.

The developer install is intended for mod developers or users who wish to use UE4SS for debugging, dumping or other utilities. The difference between this version and the basic install version is that there are some extra files included, and slightly different default settings in the `UE4SS-settings.ini` file, such as the console and GUI console being visible.

The preferred and most straightforward way to install UE4SS is to choose the `zDEV-UE4SS_v{version_number}` download (e.g. `zDEV-UE4SS_v3.0.0`) and then just drag & drop all the necessary files into the `game executable directory`.

Now all you need to do is start your game and UE4SS will automatically be injected.

## Experimental Install

If you want the latest and greatest features and don't mind the potential for more bugs than the main release, you can visit the [experimental part of releases](https://github.com/UE4SS-RE/RE-UE4SS/releases/tag/experimental) which is automatically updated for each commit to the main branch.

There are a lot of older files in the experimental releases, so you will need to look for the latest downloads. You can tell which are the latest by looking at the date of the release.

There are two main packages you need to look for: basic, and dev. They are in a similar formats as above, but with `-commit number-commit hash` appended to the end of the version number. For example, a basic install might look like `UE4SS_v3.0.0-5-ga5e818e.zip` and a dev install might look like `zDEV-UE4SS_v3.0.0-5-ga5e818e.zip`.

> **Note:** If you are using the experimental version for development, you should be using the dev version of the docs, which you can get to by appending docs.ue4ss.com with `/dev` (e.g. this page would be `https://docs.ue4ss.com/dev/installation-guide`).

## Overriding Install Location

This method allows you to override the location of the `root directory` while proxy injection still works. 

In your `game executable directory` alongside the `dwmapi.dll`, create a file called `override.txt` and inside it you can write either an absolute path or a relative path to your new `UE4SS.dll`. 

For example, possible paths could be:
- `C:/ue4ss/`
- `../../Content/Paks`

## Manual Injection

> Using manual injection will mean that the `root directory` and `working directory` are treated as one single directory that happens to also be the same directory as your `game executable directory`, but any directory may be used.

Following the download of basic or dev methods (stable or experimental) and delete `dwmapi.dll`.  Afterwards, launch the game and manually inject `UE4SS.dll` using your injector of choice.

### Central Install Location via Manual Injection

This method is a way to install UE4SS in one place for all your games. Simply extract the zip file of your choice (basic or dev) in any directory _outside_ the `game directory`, this is what's known as the `root directory`.  

You will then create a folder inside with the name of your game and drag `UE4SS-settings.ini` in to it, this is what's known as the `working directory`.

If the path to your game executable is

```
D:\Games\Epic Games\SatisfactoryEarlyAccess\FactoryGame\Binaries\Win64\FactoryGame-Win64-Shipping.exe
```

Then the name of your `working directory` should be `SatisfactoryEarlyAccess`.  
This directory will be automatically found and used by UE4SS if it exists.

The following files & folders exist inside the `working directory`:

- Mods
    - Mod folders
    - mods.txt
- UE4SS-settings.ini
- UE4SS.log
- UE4SS.dll
- ...and some other auxiliary, optional files that are not required for UE4SS to function.

While `dwmapi.dll` (can have a name of any DLL that is loaded by the game engine or by its dependencies) is in the `game executable directory`.

Now all you need to do is start your game and point your injector of choice to `<root directory>/UE4SS.dll`.

If you use this method, if you keep a copy of `UE4SS-settings.ini` inside the `root directory` then this file will act as a default for all the games that don't have a `working directory` as long as you still point your injector to the `root directory`.  

This way you can use this method for most of your games and at the same time you can use method #1 or method #2 for other games.


## How to verify that UE4SS is running successfully?

Try any of the following:
* Press any of the default keyboard shortcuts, such as `@` or `F10` that open the in-game console (using built-in `ConsoleEnablerMod`)
* Check that the log file `UE4SS.log` is created in the same folder as the UE4SS main DLL, and that the log file contains fresh timestamps and no errors.
* Enable the GUI console in `UE4SS-settings.ini` and check that it appears as a separate window (rendered with OpenGL by default).
* (For developers, if the game is confirmed to be safely debuggable) Check that the UE4SS library is being loaded in a debugger and has its threads spawned in the target game's process and in a reasonable state.