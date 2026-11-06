# 自定义游戏配置

> [!IMPORTANT]
> 随着游戏和 UE4SS 的更新，其中一些文件可能已经过时。如果你发现某个游戏的自定义配置已过时，请在 UE4SS-RE/RE-UE4SS 仓库中提交一个 issue。请务必先测试该游戏在不使用自定义配置的情况下能否正常工作，因为在最新版本的 UE4SS 中该问题可能已经被修复。

这些设置用于那些对引擎做了改动、导致 UE4SS 无法开箱即用的游戏。

你需要把对应游戏文件夹中的所有文件下载下来，并放到你的 UE4SS 安装目录中相同的文件夹下。例如，下载 Kingdom Hearts 3 的配置后，你的文件结构应该如下：

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

……当然，具体的文件结构会因游戏配置的不同而有所变化。

如果你下载的是 zDEV 版本，那么所有这些文件都已经包含在 zip 文件中了。

[你可以在这里找到它们](https://github.com/UE4SS-RE/RE-UE4SS/tree/main/assets/CustomGameConfigs)
