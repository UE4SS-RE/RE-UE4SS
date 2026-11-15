# 安装 C++ Mod

1. 本节假定你已经为你的游戏安装并运行好了 UE4SS。如果没有，请参考[安装指南](../installation-guide.md)。

2. 构建完成后，你会得到以下文件：
   - `MyAwesomeMod.dll`，位于 `MyMods\Binaries\<Configuration>\MyAwesomeMod`

3. 打开你游戏的可执行文件目录，进入其中的 `Mods` 文件夹。在这里我们需要做几件事：
   - 在 `Mods` 中创建形如 `MyAwesomeMod\dlls` 的文件夹结构。
   - 把 `MyAwesomeMod.dll` 移动到 `dlls` 文件夹中，并重命名为 `main.dll`。

> 注意：如果不存在 `main.dll`，`MyAwesomeMod.dll` 同样可以工作，UE4SS 会使用它。

最终结果应该像这样：

```
Mods\
    MyAwesomeMod\
        dlls\
            main.dll
```

4. 若要在游戏内启用你的 mod 加载，你需要编辑 `Mods` 文件夹中的 `mods.txt`。默认内容大致如下：

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

你需要在 keybinds **之上**添加一行：

```
MyAwesomeMod : 1
```

以启用 `MyAwesomeMod`。

或者，在 MyAwesomeMod 文件夹中放一个名为 `enabled.txt` 的空文本文件。不推荐这种做法，因为它无法设置加载顺序，并且会绕过 `mods.txt`，但对最终用户来说安装可能更简单。

5. 启动游戏。如果一切操作正确，你应该能在 UE4SS 控制台顶部附近看到以蓝色高亮显示的文本 "MyAwesomeMod says hello"（在扫描开始之前）；如果你注册了 `on_unreal_init` 回调，还应该看到以蓝色高亮显示的 "Object Name: /Script/CoreUObject.Object"（就在扫描结束之后）。

## 自动化

现在你已经了解了整个流程，可以使用 [UE4SS C++ 模板](https://github.com/UE4SS-RE/UE4SSCPPTemplate) 仓库，它会自动完成创建 mod、构建 mod 与安装 mod 的过程。请注意，`new_mod_setup.bat` 脚本会检出最新发布版本对应的提交，这样可以确保你的 mod 使用的是与最新发布版一致的 ABI 进行构建。

> 注意：任何影响 mod 模板的构建系统改动都会被推送到 `dev` 分支，并在新的 UE4SS 发布版本创建时合并到 main。这样可以确保模板始终与最新的 UE4SS 发布版本保持同步。
