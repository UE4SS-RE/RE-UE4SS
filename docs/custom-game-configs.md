# Custom Game Configs

> [!IMPORTANT]
> Some of these files may be out of date as the games/UE4SS updates. If you find that a game's custom game config is out of date, please open an issue on the UE4SS-RE/RE-UE4SS repository. Make sure that you first test if the game works without the custom game config, as it may have been fixed in the latest version of UE4SS.

These settings are for games that have altered the engine in ways that make UE4SS not work out of the box.  

You need to download the files from each folder for your game and place them in the same folder in your UE4SS installation. For example, downloading the configs for Kingdom Hearts 3 should result in your files being in the following structure:

```
Binaries/Win64/
├── CustomGameConfigs/
│   └── Kingdom Hearts 3/
│       ├── UE4SS_Signatures/
│       │   ├── FName_Constructor.lua
│       │   ├── FName_ToString.lua
│       │   ├── StaticConstructObject.lua
│       ├── MemberVariableLayout.ini
│       ├── UE4SS-settings.ini
│       ├── VTableLayout.ini
```

... but obviously the file structure will change depending on the game's configs.

If you download the zDEV version, all these files are already included in the zip file.

[You can find them here](https://github.com/UE4SS-RE/RE-UE4SS/tree/main/assets/CustomGameConfigs)