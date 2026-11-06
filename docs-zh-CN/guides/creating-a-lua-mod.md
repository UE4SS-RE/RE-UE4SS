# 创建 Lua Mod

## 开始之前

要在 UE4SS 中创建 Lua mod，你首先应该：

* 了解[如何安装 UE4SS](../installation-guide.md) 到你的目标游戏，并确认它正常运行；
* 能够编写基本的 Lua 代码（参见官方书籍 [Programming in Lua](https://www.lua.org/pil/contents.html) 及其后续版本，或网上其他推荐的教程）；
* 了解虚幻引擎的对象模型以及游戏 mod 开发的基础知识。

## 最小的 Lua mod 是什么样子

UE4SS 中的 Lua mod，就是放在 UE4SS 安装目录 `Mods/` 下某个文件夹中的一组 Lua 脚本。
本例中我们称它为 `MyLuaMod`。

为了能被加载并执行：

1) mod 文件夹中必须有一个 `scripts` 子文件夹，并在其中包含 `main.lua` 文件，结构如下：

```
Mods\
    ...
    MyLuaMod\
        scripts\
            main.lua
    ...
```

2) `Mods\MyLuaMod\scripts\main.lua` 文件中包含一些 Lua 代码，例如：

```lua
print("[MyLuaMod] Mod loaded\n")
```

3) 必须在 `Mods\mods.txt` 中新增一行，包含你的 mod 文件夹名（即 mod 名称）以及 `1` 表示启用、`0` 表示禁用：

```
...
MyLuaMod : 1
...
```

你的自定义功能都写在 `main.lua` 中，如有需要可以在其中引入其他 Lua 文件，包括创建自己的 Lua 模块或导入各种库。

## 在 Lua mod 中能做什么

UE4SS 提供并在 Lua 中可用的 API，记录在本章 ["Lua API"](../lua-api.md) 的各个小节中。使用这些函数与类，你可以找到并操作内存中的虚幻引擎对象实例，创建新对象或修改已有对象，调用它们的方法并访问它们的字段。

基本上，你做的事情与虚幻引擎游戏开发者在代码中所做的完全相同，只不过你使用 UE4SS 来定位所需对象并且需要一些猜测，而开发者本来就知道对象在哪里、是什么（因为他们有源代码）。

### 创建简单的数据类型

如果你需要创建一个类似结构体的类的对象（例如 `FVector`）以便传给某个虚幻引擎函数，UE4SS 允许你改为传入一个包含该类字段的 Lua 表，例如 `{X=1.0, Y=2.0, Z=3.0}`。

### 使用 Lua C 库

如果你需要加载带原生代码（例如 Windows 上的 DLL）的 Lua C 库，
可以直接把这些 DLL 放进同一个 `\scripts\` 文件夹中。

# 搭建 Lua mod 开发环境

如果你的代码编辑器或 IDE 已针对 Lua 开发正确配置，并且了解 UE4SS API，编写 mod 会容易得多。

1) 配置你的代码编辑器/IDE，使其支持 Lua 语法高亮与代码补全。如果你使用 VSCode，请参见[使用自定义 Lua 绑定](./using-custom-lua-bindings)。

2) 确认你使用的 UE4SS 构建中包含 `Mods\shared\Types.lua`（Github releases 中的开发版构建包含它）。它会在你的 IDE 中加载 UE4SS API 定义。

3) （可选）从 UE4SS 的 GUI 控制台[转储 Lua 绑定](./using-custom-lua-bindings.md#转储自定义-lua-绑定)，并[按照建议加载它们](./using-custom-lua-bindings.md#使用绑定)。

然后在 IDE 中打开 UE4SS 安装目录的 `Mods/` 文件夹，并在其中创建或修改你的 mod。

# 应用代码改动

开发 Lua mod 的主要好处是，你可以快速编辑 Lua 源码而无需像 C++ mod 那样重新编译 C++ mod 库，并且无需重启游戏即可重试。

你可以：

* 在 UE4SS GUI 控制台的 "Console" 标签页点击 "Restart All Mods" 按钮重新加载所有 mod，或者
* 在 `UE4SS-settings.ini` 中启用 "Hot reload"，并使用指定的热键（默认为 `Ctrl+R`）完成同样的操作。

# 你的第一个 mod

在 mod 的 `main.lua` 文件中写一些代码，尝试访问目标游戏中虚幻引擎的对象，并做一件你能在 UE4SS 控制台中观察到的事情。

你可以先试试：

```lua
print("[MyLuaMod] Mod loaded\n")
```

确认它能正常运行之后，就可以开始实现真正的功能了。

下面的示例代码相当通用，应该能适用于 UE4SS 支持的许多游戏。

它注册了一个热键 `Ctrl+F1`，按下时读取玩家坐标，并计算自上次按下热键以来玩家移动了多远。

> 注意，日志 `print` 调用中在方括号内包含了 mod 名称，这有助于你在控制台的众多日志字符串中找到自己 mod 的输出。

玩家坐标按以下方式获取：

1) 使用 [UE4SS `UEHelpers` 类](https://github.com/UE4SS-RE/RE-UE4SS/blob/main/assets/Mods/shared/UEHelpers/UEHelpers.lua) 获取[玩家控制器](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/APlayerController/)。
2) 获取[`Pawn`](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/APawn/)，它代表玩家在虚幻引擎中可以控制的真正"物理"实体。
3) 调用合适的虚幻引擎[方法 `K2_GetActorLocation`](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/AActor/K2_GetActorLocation/)，它返回 `Pawn` 的位置（通过访问其父类 `Actor` 类）。
4) 该位置是虚幻引擎类型 `FVector` 的三分量向量，字段为 `X`、`Y`、`Z`。

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

当你加载游戏直到可以移动角色，按下热键、移动玩家、再按下热键，mod 会输出以下内容或非常类似的内容：

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
