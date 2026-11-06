# Unreal Engine 4/5 脚本系统

适用于 UE4/5 游戏的 Lua 脚本系统平台、C++ Modding API、SDK 生成器、蓝图 mod 加载器、实时属性编辑器以及其他转储工具。

## 主要功能

- [Lua 脚本 API](https://docs.ue4ss.com/dev/lua-api.html)：基于 UE 对象系统编写 Lua mod
- [蓝图 Mod 加载](https://docs.ue4ss.com/dev/feature-overview/blueprint-modloader.html)：无需编辑/替换游戏文件即可自动生成蓝图 mod
- [C++ Modding API](https://docs.ue4ss.com/dev/guides/creating-a-c%2B%2B-mod.html)：基于 UE 对象系统编写 C++ mod
- [实时属性查看与编辑器](https://docs.ue4ss.com/dev/feature-overview/live-view.html)：搜索、查看、编辑并监视每个已加载对象的属性，非常适合调试 mod 或分析数值在运行时如何变化
- [UHT 转储器](https://docs.ue4ss.com/dev/feature-overview/dumpers.html#unreal-header-tool-uht-dumper)：生成与 Unreal Header Tool 兼容的 C++ 头文件，为你的游戏创建镜像 .uproject
- [C++ 头文件转储器](https://docs.ue4ss.com/dev/feature-overview/dumpers.html#c-header-generator)：根据反射类与蓝图生成带偏移的标准 C++ 头文件
- [通用 UE Mod](https://docs.ue4ss.com/dev/feature-overview/universal-mods.html)：解锁游戏控制台以及其他通用 mod
- [用于文件解析的转储器](https://docs.ue4ss.com/dev/feature-overview/dumpers.html#usmap-dumper)：为未版本化属性生成 `.usmap` 映射文件
- [UMAP 重建转储器](https://docs.ue4ss.com/dev/feature-overview/dumpers.html#umap-recreation-dumper)：把所有已加载的 actor 转储到文件，以便在编辑器内生成 `.umaps`
- 其他功能，包括偶尔出现的[实验性](https://docs.ue4ss.com/dev/feature-overview/experimental.html)特性

## 目标 UE 版本：从 4.7 到 5.8

UE4SS 的目标并不是做一个永远适用于所有游戏的即插即用方案。
它的目标是提供一个能适用于大多数游戏的底层系统。
你可能需要自行更新 AOB，下面的指南介绍了具体做法。

## 基础安装

最简单的安装方式是从 [Releases](https://github.com/UE4SS-RE/RE-UE4SS/releases/latest) 下载最新非实验性构建的非 dev 版本，并把 zip 内容解压到 `{游戏目录}/GameName/Binaries/Win64/`。

如果你的游戏在自定义配置列表中，也请把相应文件夹中的内容解压到 `Win64` 中。

如果你打算使用 UE4SS 进行 mod 开发，可以按上面同样的方式操作，但改为下载 zDEV 版本。

### 命令行选项

如果 RE-UE4SS 是通过代理 DLL 安装的，则可以使用以下命令行选项：

- `--disable-ue4ss` - 通过带该参数启动游戏来临时禁用 UE4SS，而无需卸载。
- `--ue4ss-path <path>` - 指定 UE4SS.dll 的自定义路径。支持绝对路径（例如 `C:\custom\UE4SS.dll`）与相对路径（例如相对于游戏可执行文件目录的 `dev\builds\UE4SS.dll`）。适用于在不修改安装文件的情况下测试不同的 UE4SS 构建。

### 环境变量

RE-UE4SS 支持以下环境变量：

- `UE4SS_MODS_PATHS` - 以分号分隔的额外 mods 目录列表。路径按相反顺序处理（第一条优先级最高），类似 `PATH` 变量。示例：`C:\SharedMods;D:\GameMods;E:\TestMods`。

## 链接

  [完整安装指南](https://docs.ue4ss.com/dev/installation-guide.html)

  [修复兼容性问题](https://docs.ue4ss.com/dev/guides/fixing-compatibility-problems.html)

  [Lua API - 概览](https://docs.ue4ss.com/dev/lua-api.html)

  [生成与 UHT 兼容的头文件](https://docs.ue4ss.com/dev/guides/generating-uht-compatible-headers.html)

  [自定义游戏配置](https://docs.ue4ss.com/dev/custom-game-configs.html)

  [创建兼容的蓝图 Mod](https://www.youtube.com/watch?v=fB3yT85XhVA)

  [UE4SS Discord 服务器邀请](https://discord.gg/7qhRGHF9Tt)

  [Unreal Engine Modding Discord 服务器邀请](https://discord.gg/unreal-engine-modding-876613187204685934)

## 构建要求

- 原生构建需要 Windows。目前不支持原生 Linux 构建，但可以使用下文单独的前提条件与说明从 Linux [交叉编译 Windows 二进制](#在-linux-上交叉编译-windows-二进制)。
- 原生 Windows 构建需要一个支持 C++23 的 MSVC 版本：
  - MSVC 工具集版本 >= 14.43.0
  - MSVC 版本 >= 19.43
  - Visual Studio 版本 >= 17.13
  - 希望将来能支持更多编译器。
- [Rust 工具链 >= 1.73.0](https://www.rust-lang.org/tools/install)
- [CMake >= 3.22](https://cmake.org/download/)
- 一个构建系统：[Ninja](https://ninja-build.org/) 或 MSVC（随 Visual Studio 提供）

## 构建说明

1. 克隆仓库。
2. 执行命令：`git submodule update --init --recursive`
   请确保你的 Github 账号已与 Epic Games 账号关联，以便访问 UE 源码。
   不要使用 `--remote` 选项，因为它会强制第三方依赖更新到最新提交，可能导致问题。
   拉取 Unreal 伪代码子模块需要你的 github 账号与 Epic games 账号关联。

构建 UE4SS 有几种不同方式。

## 从命令行构建

### 构建模式

构建模式的结构如下：`<Target>__<Config>__<Platform>`

目前支持的选项包括：

* `Target`
  * `Game` - 用于 UE 版本高于 4.21 的常规游戏
  * `LessEqual421` - 用于 UE 版本小于等于 4.21 的常规游戏
  * `CasePreserving` - 用于启用了大小写保留（case preserving）构建的游戏

* `Config`
  * `Dev` - 开发版构建
  * `Debug` - 调试版构建
  * `Shipping` - 发布版构建
  * `Test` - 用于测试的构建

* `Platform`
  * `Win64` - 64 位 Windows

### 基本构建命令

要使用 CMake 构建 UE4SS，请运行以下命令：

```bash
# 使用 Ninja 配置（推荐，构建更快，单配置）
cmake -B build_cmake_Game__Shipping__Win64 -G Ninja -DCMAKE_BUILD_TYPE=Game__Shipping__Win64

# 使用 Ninja 构建
cmake --build build_cmake_Game__Shipping__Win64

# 或使用 MSVC 配置（多配置，无需重新配置即可切换配置）
cmake -B build_cmake_Game__Shipping__Win64 -G "Visual Studio 17 2022"

# 使用 MSVC 构建（需要 --config 参数）
cmake --build build_cmake_Game__Shipping__Win64 --config Game__Shipping__Win64
```

### 配置选项

CMake 允许你配置多种构建选项。以下是一些有用的选项：

#### 代理路径

默认情况下，UE4SS 基于 `C:\Windows\System32\dwmapi.dll` 生成代理。要修改这一点，请设置 CMake 变量：

```bash
cmake -B build -DUE4SS_PROXY_PATH="<path to proxy dll>" -DCMAKE_BUILD_TYPE=Game__Shipping__Win64
```

#### 性能分析器类型

默认情况下，UE4SS 的 profiling 是禁用的（`None`）。要启用 profiling，你既需要指定 profiler 类型，也需要一个包含 STATS 的构建配置：

```bash
# Dev 与 Test 构建默认启用 STATS
cmake -B build -DPROFILER_FLAVOR=<Tracy|Superluminal|None> -DCMAKE_BUILD_TYPE=Game__Dev__Win64
```

> [!NOTE]
> Profiling 需要 STATS 支持。默认情况下，`Dev` 与 `Test` 配置包含 STATS，而 `Shipping` 与 `Debug` 不包含。你可以通过添加编译定义手动为任意配置启用 STATS：
> ```bash
> cmake -B build -DPROFILER_FLAVOR=Tracy -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 -DCMAKE_CXX_FLAGS="-DSTATS"
> ```

### 常用的 CMake 命令

| 命令 | 说明 |
| --- | --- |
| `cmake -B <build_dir> -G <generator>` | 使用指定生成器（Ninja 或 "Visual Studio 17 2022"）配置项目 |
| `cmake --build <build_dir>` | 使用 Ninja 构建（单配置生成器） |
| `cmake --build <build_dir> --config <mode>` | 使用 MSVC 构建（多配置生成器，必须提供 `--config`） |
| `cmake --build <build_dir> --clean-first` | 清理后重新构建（MSVC 需追加 `--config <mode>`） |
| `cmake --build <build_dir> --target <target>` | 构建指定目标（MSVC 需追加 `--config <mode>`） |
| `cmake --build <build_dir> --verbose` | 输出详细构建日志（MSVC 需追加 `--config <mode>`） |

### 在 IDE 中打开

#### Visual Studio

CMake 内置支持生成 Visual Studio 解决方案：

```bash
cmake -B build -G "Visual Studio 17 2022"
```

然后在 `build` 目录中打开生成的 `.sln` 文件。

另外，Visual Studio 2022 原生支持 CMake——你可以直接在 Visual Studio 中打开该文件夹，它会自动检测 CMakeLists.txt 文件。

#### CLion / 其他 CMake IDE

大多数现代 IDE（CLion、带 CMake Tools 的 Visual Studio Code 等）都原生支持 CMake。只需打开项目文件夹，IDE 就会自动检测并配置该 CMake 项目。

注意，如果你更新子模块的原因并不是别人推送了更新、你只是在跟进，那么你也应该提交并推送你所更新的子模块。

### 在 Linux 上交叉编译 Windows 二进制

UE4SS 支持两种从 Linux 交叉编译到 Windows 的方式：**xwin**（推荐）或 **msvc-wine**。

#### 所有交叉编译方式的前提条件

- [Rust 工具链 >= 1.73.0](https://www.rust-lang.org/tools/install)，并安装 `x86_64-pc-windows-msvc` 目标：
  ```bash
  rustup target add x86_64-pc-windows-msvc
  ```
  对于不使用 rustup 的 Rust 安装，请单独安装 Windows 目标标准库，它必须与已安装的 Rust 编译器构建版本匹配。
- [CMake >= 3.22](https://cmake.org/download/)
- [Ninja 构建系统](https://ninja-build.org/)

#### 方式 1：使用 xwin 交叉编译（推荐）

**xwin** 会下载并打包交叉编译所需的 Microsoft CRT 头文件/库以及 Windows SDK 头文件/库，无需安装 Windows。

##### 前提条件

- 支持 Windows 目标的 LLVM/Clang，包括 `clang-cl`、`lld-link` 以及与 MASM 兼容的 `llvm-ml` 汇编器：
  ```bash
  # Ubuntu/Debian
  sudo apt install clang lld llvm

  # Arch Linux
  sudo pacman -S clang lld llvm

  # Enterprise Linux 10（例如 RHEL、CentOS Stream、AlmaLinux）
  sudo dnf install clang lld llvm
  ```
- [xwin](https://github.com/Jake-Shadle/xwin)，使用 Cargo 安装：
  ```bash
  cargo install xwin
  ```
- 完整的交叉构建还需要 Wine 以及 Linux 的 `binfmt_misc` 支持。UE4SS 会直接运行 `proxy_generator.exe` 等 Windows 构建期工具。宿主机必须能够在不显式调用 `wine` 命令的情况下执行 Windows PE 二进制文件。无图形界面的 Wine 安装即可满足构建 UE4SS 的需要。

##### 配置

1. 如果尚未做过，请按[构建说明](#构建说明)所述克隆仓库并初始化其子模块。切换到所克隆仓库的根目录（通常是 `RE-UE4SS/`，其中包含 `CMakeLists.txt`），并在该目录下执行以下命令。

2. 把 Microsoft SDK 与 CRT 下载并解包到仓库的 `xwin/` 目录（仅安装 `xwin` 可执行文件并不会安装 SDK）：

   ```bash
   xwin --accept-license splat --output ./xwin
   ```

   xwin 的 CMake 工具链与 `build.sh` 都会自动在该处找到 SDK 与 CRT；如果你希望把它们存放在别处，请在配置或构建前把 `XWIN_DIR` 设为该目录，例如 `export XWIN_DIR="$HOME/.xwin"`。一旦设置，`XWIN_DIR` 会覆盖仓库内的默认位置。

3. Wine 必须注册到 Linux 的 `binfmt_misc` 机制中，以便在构建期间直接执行 Windows `.exe` 文件。你的发行版可能已经配置好了这一点。

   使用以下命令检查：

   ```bash
   cat /proc/sys/fs/binfmt_misc/status
   grep -il '^interpreter .*wine' /proc/sys/fs/binfmt_misc/* 2>/dev/null
   ```

   你应该看到包含类似内容的简短列表：

   ```text
   enabled
   /proc/sys/fs/binfmt_misc/DOSWin
   ```

   注册的处理器名称在不同发行版中可能不同。如果 `binfmt_misc` 未启用，或没有列出 Wine 解释器，请按下面的说明进行配置。

   在大多数 Linux 发行版上，可以使用以下步骤让 Wine 注册在重启后依然有效。这些说明使用 `systemd`，它包含在大多数主流 Linux 发行版中。如果你的发行版使用其他 init 系统，请查阅其文档以持久化 `binfmt_misc` 注册。

   ```bash
   sudo mkdir -p /etc/binfmt.d
   printf '%s\n' ':DOSWin:M::MZ::/usr/bin/wine:' | sudo tee /etc/binfmt.d/wine.conf
   sudo systemctl restart systemd-binfmt.service
   ```

   如果 Wine 不在 `/usr/bin/wine`，请使用实际的绝对路径。不同发行版的打包与注册方式可能不同。可直接执行一个 Windows 控制台 `.exe`（命令前不加 `wine`）来检查配置是否正确。

##### 可选：抑制 Wine 诊断输出

在运行 Windows 构建期工具时（尤其是在无图形界面的系统上），Wine 会产生大量诊断输出。要在当前 shell 会话中抑制它：

```bash
export WINEDEBUG=-all
```

你也可以在单条命令前加前缀（例如 `WINEDEBUG=-all cmake --build build_xwin`），或把该 export 加到 shell 启动配置中。该设置是可选的；需要 Wine 诊断信息时请取消设置。`build.sh` 的 xwin 模式会自动抑制 Wine 诊断输出。

##### 使用 CMake 手动构建

请在仓库根目录执行以下命令。选择一种工具链并使用它自己的构建目录；CMake 会缓存编译器选择。两种 xwin 工具链都需要用 `llvm-ml` 加 `-m64` 来编译 Windows x64 代理汇编。

```bash
# 使用 xwin-clang-cl 配置（MSVC 兼容的编译器接口）
cmake -S . -B build_xwin -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xwin-clang-cl-toolchain.cmake \
  -DCMAKE_ASM_MASM_COMPILER:FILEPATH="$(command -v llvm-ml)" \
  -DCMAKE_ASM_MASM_FLAGS:STRING=-m64

# 构建
cmake --build build_xwin
```

或者，在单独的构建目录中使用 xwin-clang 工具链（GNU 风格的编译器接口）：

```bash
cmake -S . -B build_xwin_clang -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/xwin-clang-toolchain.cmake \
  -DCMAKE_ASM_MASM_COMPILER:FILEPATH="$(command -v llvm-ml)" \
  -DCMAKE_ASM_MASM_FLAGS:STRING=-m64

cmake --build build_xwin_clang
```

##### 使用 build.sh 脚本构建

通过 `build.sh` 构建时，xwin 工具链会自动使用 `llvm-ml -m64`。请用 `bash` 调用该脚本；不需要可执行文件权限。默认只构建 `UE4SS` 目标；使用 `--target all` 进行完整构建（包含代理 DLL）。

```bash
# 使用 xwin-clang-cl 构建完整项目
bash tools/buildscripts/build.sh --toolchain xwin-clang-cl --target all

# 或使用 xwin-clang 构建完整项目
bash tools/buildscripts/build.sh --toolchain xwin-clang --target all

# 构建指定配置（默认目标为 UE4SS）
bash tools/buildscripts/build.sh --toolchain xwin-clang-cl --build-config Game__Debug__Win64

# 清理后完整构建并输出详细日志
bash tools/buildscripts/build.sh --toolchain xwin-clang-cl --target all --clean --verbose
```

#### 方式 2：使用 msvc-wine 交叉编译

**msvc-wine** 使用在 Wine 下运行的真实 MSVC 工具。兼容性最好，但配置更复杂。

##### 前提条件

- Wine：
  ```bash
  # Ubuntu/Debian
  sudo apt install wine wine64 winbind

  # Arch Linux
  sudo pacman -S wine samba
  ```
- [msvc-wine](https://github.com/mstorsjo/msvc-wine) - 按照其安装指南安装 MSVC 工具
- Clang（用于 wine-clang-cl 模式），或使用 MSVC 的 cl.exe（用于 wine-msvc 模式）

##### 配置

1. 按照[官方说明](https://github.com/mstorsjo/msvc-wine#installation)安装 msvc-wine。
   默认安装到 `~/my_msvc/opt/msvc`。

2. 确保 msvc-wine 工具已加入 PATH：
   ```bash
   export PATH="$HOME/my_msvc/opt/msvc/bin/x64:$PATH"
   ```

3. 设置 Wine 前缀（可选）：
   ```bash
   export WINE_PREFIX=~/.wine
   ```

##### 使用 CMake 手动构建

```bash
# 使用 wine-clang-cl 工具链配置（wine 下的 clang-cl）
cmake -B build_wine \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wine-clang-cl-toolchain.cmake

# 或使用 wine-msvc 工具链（wine 下的 MSVC cl.exe）
cmake -B build_wine \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Game__Shipping__Win64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wine-msvc-toolchain.cmake

# 构建
cmake --build build_wine
```

##### 使用 build.sh 脚本构建

```bash
# 使用 wine-clang-cl 构建
bash tools/buildscripts/build.sh --toolchain wine-clang-cl

# 或使用 wine-msvc 构建
bash tools/buildscripts/build.sh --toolchain wine-msvc

# 构建指定配置
bash tools/buildscripts/build.sh --toolchain wine-clang-cl --build-config Game__Debug__Win64
```

#### 构建产物

交叉编译的二进制文件位于所选构建目录下的 `<BuildMode>/bin/`。例如，手动 `xwin-clang-cl` 构建会生成：

```
build_xwin/
└── Game__Shipping__Win64/
    └── bin/
        ├── UE4SS.dll
        ├── dwmapi.dll  (proxy DLL)
        └── ... (其他文件)
```

而 `build.sh --toolchain xwin-clang-cl` 示例使用的是 `build_xwin-clang-cl_Game__Shipping__Win64/`。

### 在 Wine 下调试

使用基于 Wine 的工具链时，可以用 Wine 的调试器调试崩溃与问题。

#### 使用 winedbg

```bash
# 调试正在运行的程序
winedbg ./path/to/game.exe

# 调试崩溃转储
winedbg crash_2024_12_26_07_39_15.dmp
```

#### 调试的重要说明

- 调试符号（.pdb 文件）**不会**存储在 minidump 文件中
- 你**必须**拥有与崩溃的 .dll 完全对应的同一个 .pdb 文件
- 确保符号匹配的最简单做法：
  1. 记录你构建 UE4SS 时的 git 提交哈希
  2. 调试崩溃时，检出该确切的提交
  3. 重新构建以生成匹配的 .pdb 文件
  4. 然后调试该 minidump

#### 提示

- 如果遇到 "access denied" 错误，请确保已安装 `winbind` 或 `samba`
- PDB 文件生成在 DLL 文件所在的同一目录中

## 更新 git 子模块

如果你想更新 git 子模块，可以通过以下三种方式之一：
1. 执行 `git submodule update --init --recursive` 更新所有子模块。
2. 也可以逐个更新子模块，执行 `git submodule update --init --recursive deps/<first-or-third>/<Repo>`。
   除非你确实想更新到最新提交，否则不要使用 `--remote` 选项。
3. 如果你想为某个子模块指定特定的提交或分支，请 `cd` 进入该依赖的子模块目录并执行 `git checkout <分支名或提交>`。
你可能需要时常更新的主要依赖是 `deps/first/Unreal`。

## 致谢

项目开源以来的所有贡献者：https://github.com/UE4SS-RE/RE-UE4SS/graphs/contributors

- **原始作者** 原始作者已不再希望参与本项目或与其产生关联。请尊重其意愿，避免在涉及本项目时使用其过去的用户名。
- [**Archengius**](https://github.com/Archengius/)
  - 与 UHT 兼容的头文件生成器
- **CasualGamer**
  - 注入器代码与 AOB 扫描器大量基于其工作，其中 90% 的代码出自他手。
- **SunBeam**
  - 为 UE4.25 的函数 'GetFullName' 提供了额外特征码。
  - 用于检查从 ini 加载时特征码格式是否正确的正则表达式。
  - 大量与特征码相关的工作
- **tomsa**
  - const char* 到 vector\<int> 的转换器
    - tomsa：思路与大部分代码
    - 原始作者：nibble 支持
- **boop** / **usize**
  - 新的 UFunction 钩子方法
- [**RussellJ**](https://github.com/RussellJerome)
  - 蓝图 Mod 加载器的灵感来源
- [**Narknon**](https://github.com/narknon)
  - 部分功能以及项目的维护/重托管
- **DeadMor0z**
  - 部分功能以及 Lua 的更新/维护
- [**OutTheShade**](https://github.com/OutTheShade/UnrealMappingsDumper)
  - Unreal Mappings（USMAP）生成器
- **DmgVol**
  - 地图转储器的灵感来源
- [**Buckminsterfullerene**](https://github.com/Buckminsterfullerene02/)
  - 重写文档、各种修复
- **trumank**
  - Lua 绑定生成器、各种修复、自动化与改进
- **localcc**
  - C++ API

## 感谢所有协助测试的人

- GreenHouse
- Otis_Inf
- SunBeam
- Motoson
- hooter
- Synopis
- Buckminsterfullerene

### 支持者

[![JetBrains logo.](https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg)](https://jb.gg/OpenSource)
