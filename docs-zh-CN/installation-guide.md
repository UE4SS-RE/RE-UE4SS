# 安装

## 核心目录结构概念

你需要了解四个概念。

1. `根目录`（root directory）。
   - 该目录包含 UE4SS 的 dll 文件。
2. `工作目录`（working directory）。
   - 该目录包含配置文件和 mod 文件，位于`根目录`之内。
3. `游戏目录`（game directory）。
   - 该目录通常包含一个以你的游戏命名的小型可执行文件，以及一个同名的文件夹。
   - 这个可执行文件**并不是**你真正的游戏本体，它只是一个小的包装程序，用于启动第三方启动器（例如 Steam）；如果不存在启动器，它则启动真正的游戏可执行文件。
   - `游戏目录`示例：`D:\Games\Epic Games\SatisfactoryEarlyAccess\`
4. `游戏可执行文件目录`（game executable directory）。
   - 该目录包含你游戏的真正可执行文件，它不属于 UE4SS 的目录结构。
   - 你也可以这样识别它：位于其中的游戏可执行文件通常是最大的文件（远大于上面的包装程序），并且是游戏运行时作为包装程序子进程运行的那个进程。
   - `游戏可执行文件目录`示例：
     `D:\Games\Epic Games\SatisfactoryEarlyAccess\FactoryGame\Binaries\Win64\`

## 选择安装方式

你可以通过几种不同的方式安装 UE4SS。

目标是以某种方式让目标游戏加载 UE4SS 的 `*.dll`，并把配置文件和 `\Mods\` 目录放到 UE4SS 能够找到的正确位置。

### 方式 #1 - 基础安装

> 使用方式 #1 意味着`根目录`和`工作目录`会被当作同一个目录，而这个目录恰好也是你的`游戏可执行文件目录`。

基础安装面向那些使用 UE4SS 来运行依赖它的 mod、且不需要做任何开发工作的终端用户。使用这种方式时，不应该出现任何额外的窗口。

最简单、最推荐的安装方式是：选择 `UE4SS_v{版本号}` 形式的下载包（例如 `UE4SS_v3.0.0`），然后把所有必要的文件拖放到`游戏可执行文件目录`中。

现在你只需要启动游戏，UE4SS 就会自动被注入。

### 方式 #2 - 开发者安装

> 使用方式 #2 意味着`根目录`和`工作目录`会被当作同一个目录，而这个目录恰好也是你的`游戏可执行文件目录`。

开发者安装面向 mod 开发者，或希望使用 UE4SS 进行调试、转储等功能的用户。该版本与基础安装版本的区别在于：它包含了一些额外的文件，并且 `UE4SS-settings.ini` 中的默认设置略有不同，例如控制台与 GUI 控制台是可见的。

最简单、最推荐的安装方式是：选择 `zDEV-UE4SS_v{版本号}` 形式的下载包（例如 `zDEV-UE4SS_v3.0.0`），然后把所有必要的文件拖放到`游戏可执行文件目录`中。

现在你只需要启动游戏，UE4SS 就会自动被注入。

## 实验性安装

如果你想体验最新、最全的功能，并且不介意可能出现比正式版更多的 bug，可以访问 [releases 中的 experimental 部分](https://github.com/UE4SS-RE/RE-UE4SS/releases/tag/experimental-latest)，该发布页会在每次 main 分支提交后自动更新。

实验性发布中包含大量旧文件，因此你需要寻找最新的下载包。你可以通过查看发布包的日期来判断哪些是最新的。

你需要寻找两种主要的包：基础版和开发版。它们的格式与上文类似，只是在版本号末尾追加了 `-提交数-提交哈希`。例如，基础安装包可能形如 `UE4SS_v3.0.0-5-ga5e818e.zip`，开发安装包可能形如 `zDEV-UE4SS_v3.0.0-5-ga5e818e.zip`。

> **注意：** 如果你使用实验性版本进行开发，那么你也应该使用 dev 版本文档，在 docs.ue4ss.com 后加上 `/dev` 即可访问（例如本页面的 dev 地址为 `https://docs.ue4ss.com/dev/installation-guide`）。

## 覆盖安装位置

这种方式允许你在代理注入仍然可用的情况下，覆盖`根目录`的位置。

在你的`游戏可执行文件目录`中与 `dwmapi.dll` 同级的目录里，创建一个名为 `override.txt` 的文件，在其中写入指向新的 `UE4SS.dll` 的绝对路径或相对路径。

例如，可能的路径包括：

- `C:/ue4ss/`
- `../../Content/Paks`

### 通过命令行覆盖路径

你也可以通过命令行参数指定 `UE4SS.dll` 的自定义路径。该参数的优先级高于 `override.txt` 文件。

在启动游戏时使用 `--ue4ss-path` 参数：

```
game.exe --ue4ss-path "C:\custom\path\to\UE4SS.dll"
```

绝对路径与相对路径均受支持。相对路径以`游戏可执行文件目录`为基准解析：

```
game.exe --ue4ss-path "dev\builds\UE4SS.dll"
```

这在以下场景中特别有用：

- 开发者在不修改文件的情况下测试不同的 RE-UE4SS 构建
- 在不同 RE-UE4SS 版本之间快速切换
- 使用不同配置进行自动化测试

**加载优先级顺序：**

1. `--ue4ss-path` 命令行参数（最高优先级）
2. `override.txt` 文件
3. `<游戏可执行文件目录>/ue4ss/UE4SS.dll`
4. `<游戏可执行文件目录>/UE4SS.dll`（最低优先级）

## 手动注入

> 使用手动注入意味着`根目录`和`工作目录`会被当作同一个目录，而这个目录也恰好是你的`游戏可执行文件目录`，但实际上可以使用任意目录。

按照基础版或开发版（稳定版或实验版）的方式下载后，删除 `dwmapi.dll`。随后启动游戏，并使用你喜欢的注入器手动注入 `UE4SS.dll`。

### 通过手动注入实现集中式安装

这种方式可以让你把所有游戏的 UE4SS 都安装在一个地方。只需将你选择的 zip 文件（基础版或开发版）解压到`游戏目录`**之外**的任意目录，这个目录就是所谓的`根目录`。

然后，在其中创建一个以你的游戏命名的文件夹，并把 `UE4SS-settings.ini` 拖入其中，这个文件夹就是所谓的`工作目录`。

如果你的游戏可执行文件路径是

```
D:\Games\Epic Games\SatisfactoryEarlyAccess\FactoryGame\Binaries\Win64\FactoryGame-Win64-Shipping.exe
```

那么你的`工作目录`名称应该是 `SatisfactoryEarlyAccess`。
如果该目录存在，UE4SS 会自动找到并使用它。

`工作目录`中包含以下文件和文件夹：

- Mods
  - mod 文件夹
  - mods.txt
- UE4SS-settings.ini
- UE4SS.log
- UE4SS.dll
- ……以及一些其他辅助性的、可选的、UE4SS 正常运行并不必需的文件。

而 `dwmapi.dll`（可以取任意一个由游戏引擎或其依赖加载的 DLL 名称）位于`游戏可执行文件目录`中。

现在你只需要启动游戏，并把你的注入器指向 `<根目录>/UE4SS.dll`。

如果你使用这种方式，并且在`根目录`中保留了一份 `UE4SS-settings.ini`，那么只要注入器仍指向`根目录`，该文件就会作为所有没有`工作目录`的游戏的默认配置。

这样，你就可以对大多数游戏使用这种方式，同时对其他游戏使用方式 #1 或方式 #2。

## 如何确认 UE4SS 已成功运行？

可以尝试以下任意一种方式：

- 按下任意默认快捷键，例如 `@` 或 `F10`，以打开游戏内控制台（使用内置的 `ConsoleEnablerMod`）
- 检查与 UE4SS 主 DLL 位于同一文件夹中的日志文件 `UE4SS.log` 是否已生成，并且日志中包含最新的时间戳且没有错误。
- 在 `UE4SS-settings.ini` 中启用 GUI 控制台，并确认它以独立窗口的形式出现（默认使用 OpenGL 渲染）。
- （面向开发者，前提是已确认该游戏可以被安全调试）检查 UE4SS 库是否已被加载到调试器中，其线程是否已在目标游戏进程中创建并处于正常状态。

## 配置

安装之后，你可以通过 `UE4SS-settings.ini` 文件来配置 RE-UE4SS。

### 命令行选项

通过代理 DLL 安装时，RE-UE4SS 支持以下命令行选项：

- `--disable-ue4ss` - 通过带该参数启动游戏来临时禁用 RE-UE4SS，而无需卸载
- `--ue4ss-path <path>` - 指定 UE4SS.dll 的自定义路径（绝对或相对）。适用于在不修改安装文件的情况下测试不同的构建

### 多个 Mods 目录

你可以使用 `UE4SS-settings.ini` 文件从多个目录加载 mod：

```ini
[Overrides]
; 使用 + 前缀添加额外的 mods 目录，使用 - 前缀移除
+ModsFolderPaths = ../SharedMods
+ModsFolderPaths = C:/MyMods
-ModsFolderPaths = ../SharedMods
```

INI 中的路径按出现顺序处理（最后一条优先级最高）。

或者，使用以分号分隔路径的 `UE4SS_MODS_PATHS` 环境变量：

```
set UE4SS_MODS_PATHS=C:\SharedMods;D:\GameMods;E:\TestMods
```

环境变量中的路径按相反顺序处理（第一条优先级最高）。INI 与环境变量中的路径都支持相对路径和绝对路径，也支持符号链接。

### 控制 Mods 列表

默认情况下，UE4SS 会解析每个 mods 目录中的 `mods.txt` 文件。你可以覆盖该行为，改为只使用一个统一的 `mods.txt` 文件：

```ini
[Overrides]
; 用作统一 mod 列表的 mods.txt 文件路径。
; 如果设置了该项，则只会解析该 mods.txt，而不会解析所有 mod 目录中的 mods.txt。
; 可以是相对于工作目录的路径，也可以是绝对路径。
ControllingModsTxt = ../MyModsList/mods.txt
```

这在以下场景中很有用：

- 在多个 mods 目录之间集中管理 mod 加载顺序
- 为多个游戏安装使用共享配置
- 将 mod 列表纳入版本控制，并与 mod 文件分离

> **关于翻译**：本页面为社区中文翻译，内容可能滞后于英文原版。如中英文版本存在差异，请以[英文原版](https://docs.ue4ss.com/dev/installation-guide.html)为准。
