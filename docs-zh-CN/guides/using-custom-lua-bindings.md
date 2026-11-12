# 使用自定义 Lua 绑定

为了让 Lua mod 的开发更轻松，我们加入了从游戏中转储自定义 Lua 绑定的能力。我们还提供一个共享的类型文件，其中包含默认的 UE 类型以及你可用的 API 函数/类/对象。

## 转储自定义 Lua 绑定

只需打开 GUI 控制台窗口中的 Dumpers 标签页，然后点击 "Dump Lua Bindings" 按钮。

生成器会把文件放到 `Mods/shared/types` 文件夹中。

> 警告：不要在你的 Lua 脚本中包含任何生成的文件。如果包含它们，UE4SS 设置的全局变量会被覆盖，一切都会出错。

## 使用绑定

我推荐使用 Visual Studio Code 进行 Lua 开发。你可以安装 sumneko 的 "Lua" 扩展。

把 `Mods` 文件夹作为一个工作区打开。你也可以保存该工作区，这样就不必每次打开 VS Code 都重新操作。

开发 Lua mod 时，语言服务器应该会自动解析所有类型文件，并为你提供智能提示。

> 警告：对许多游戏而言，类型数量非常多，语言服务器可能无法全部解析。这种情况下，你可以在工作区根目录添加一个 `.luarc.json` 文件，并写入以下内容：

```json
{
    "$schema": "https://raw.githubusercontent.com/sumneko/vscode-lua/master/setting/schema.json",
    "workspace.maxPreload": 50000,
    "workspace.preloadFileSize": 5000
}
```

### 如何把 mod 目录作为工作区

另一种做法是只把你 mod 的根目录打开为工作区。
此时你需要添加一个 `.luarc.json`，其中包含 `"workspace.library"` 条目，指向 "shared" 文件夹与 mod 的 "Scripts" 目录的路径。
这两个路径都可以是相对路径。

**`.luarc.json` 示例：**

```json
{
    "$schema": "https://raw.githubusercontent.com/sumneko/vscode-lua/master/setting/schema.json",
    "workspace.maxPreload": 50000,
    "workspace.preloadFileSize": 5000,
    "workspace.library": ["../shared", "Scripts"]
}
```

## 添加注解

要获得关于自定义游戏类型的上下文相关信息，你需要[为代码添加注解](https://emmylua.github.io/annotation.html)（[另一份文档](https://luals.github.io/wiki/annotations/)）。做法是在你想要注解的函数/类/对象上方添加一行注释。

## 示例

```lua
---@class ITM_MisSel_Biome_C
local biome = FindFirstOf("ITM_MisSel_Biome_C")

---@type int
local numMissions = biome.NumMissions

---@type FVector
local soundCoords = { 420.5, 69.0, 3.1 }
biome:SetSoundCoordinate(soundCoords)
```
