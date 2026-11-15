# 创建 C++ Mod

本指南将帮助你使用 UE4SS 创建一个 C++ mod。
它分为四个部分。
第一部分介绍前提条件。
第二部分介绍如何创建最基础的 C++ mod。
第三部分展示如何与 UE4SS 以及（通过 UE4SS）与 UE 本身交互。
第四部分介绍 mod 的安装。

> 本指南要求你拥有可用的 C++ 开发环境，并安装 `CMake` 与 `git`，最好与从源码构建 UE4SS 本身所需的环境类似。

## 第 1 部分

**在按以下步骤操作之前，请确保你已安装 [README 中提到的所有构建要求](https://docs.ue4ss.com/#build-requirements)！**

1. 注册 Epic 账号并将其与你的 GitHub 账号关联
2. 检查邮箱并接受 @EpicGames GitHub 组织的邀请，以获取 Unreal 源码访问权限。
3. 在你的 GitHub 账号上配置 SSH 密钥，以便 git 能访问你在第 2、3 步获得的 Unreal 源码。
4. 在电脑上任意位置创建一个目录，名字随意，我的叫 `MyMods`。
5. 克隆 RE-UE4SS 仓库，最终得到 `MyMods/RE-UE4SS`。
6. 打开 CMD，进入 `RE-UE4SS` 并执行：`git submodule update --init --recursive`
7. 回到 `MyMods` 目录，创建一个新目录，用来存放你的 mod 源码文件。
   我把它命名为 `MyAwesomeMod`。
8. 在 `MyMods` 中创建一个名为 `CMakeLists.txt` 的文件，写入以下内容：

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyMods)

add_subdirectory(RE-UE4SS)
add_subdirectory(MyAwesomeMod)
```

## 第 2 部分

1. 在 `MyMods/MyAwesomeMod` 中创建一个名为 `CMakeLists.txt` 的文件，写入以下内容：

```cmake
set(TARGET MyAwesomeMod)

add_library(${TARGET} SHARED
    dllmain.cpp
)

target_include_directories(${TARGET} PRIVATE .)
target_link_libraries(${TARGET} PUBLIC UE4SS)

# 构建完成后把 DLL 复制到游戏的 mod 目录（可选，请按需调整路径）
# add_custom_command(TARGET ${TARGET} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET}> "path/to/game/Binaries/Win64/Mods/${TARGET}/dlls/"
# )
```

2. 在 `MyMods/MyAwesomeMod` 中创建一个名为 `dllmain.cpp` 的文件，写入以下内容：

```c++
#include <stdio.h>
#include <Mod/CppUserModBase.hpp>

class MyAwesomeMod : public RC::CppUserModBase
{
public:
    MyAwesomeMod() : CppUserModBase()
    {
        ModName = STR("MyAwesomeMod");
        ModVersion = STR("1.0");
        ModDescription = STR("This is my awesome mod");
        ModAuthors = STR("UE4SS Team");
        // 除非你想以某种方式针对与当前构建所用的 UE4SS 不同的版本，
        // 否则不要修改这一项。
        //ModIntendedSDKVersion = STR("2.6");

        printf("MyAwesomeMod says hello\n");
    }

    ~MyAwesomeMod() override
    {
    }
};

#define MY_AWESOME_MOD_API __declspec(dllexport)
extern "C"
{
    MY_AWESOME_MOD_API RC::CppUserModBase* start_mod()
    {
        return new MyAwesomeMod();
    }

    MY_AWESOME_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
```

3. 在命令提示符中，进入 `MyMods` 目录，执行以下任一方式：

A. 使用 Ninja 从命令行构建：

```bash
# 配置（Ninja 是单配置生成器）
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Game__Shipping__Win64

# 构建（Ninja 不需要 --config）
cmake --build build
```

或 B. 生成 Visual Studio 解决方案：

```bash
# 配置（MSVC 是多配置生成器）
cmake -B build -G "Visual Studio 17 2022"

# 从命令行构建（MSVC 需要 --config）
cmake --build build --config Game__Shipping__Win64
```

如果你选择方式 `B`，VS 解决方案会位于 `build` 目录中。

4. 在 Visual Studio 中打开 `MyMods/build/MyMods.sln`
5. 除非你要调试，否则请确认当前配置为 `Game__Shipping__Win64`
6. 在解决方案资源管理器中找到你的项目（我的是 MyAwesomeMod），右键点击并选择 `Build`。

## 第 3 部分

本部分我们将学习如何输出日志到文件以及两个控制台，并学习如何按名称查找 UObject 并输出其名称。

1. 在 `#include <Mod/CppUserModBase.hpp>` 下面添加 `#include <DynamicOutput/DynamicOutput.hpp>`。
   现在你也可以删除 `#include <stdio.h>`，因为我们将不再使用 `printf`，而它是唯一需要该头文件的东西。
2. 为了省些时间和麻烦，也让代码更美观一些，在所有 include 下面添加这一行：

```c++
using namespace RC;
```

3. 把 `MyAwesomeMod` 构造函数体中对 printf 的调用替换为：

```c++
Output::send<LogLevel::Verbose>(STR("MyAwesomeMod says hello\n"));
```

它比 `printf` 调用更长，但作为回报，消息会被传播到日志文件以及普通控制台和 GUI 控制台。
我们还通过 `LogLevel` 枚举获得了对颜色的支持。

4. 在 DynamicOutput 的 include 下面添加：

```c++
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>
```

5. 再次利用 `using namespace` 快捷键，在上一行下面添加：`using namespace RC::Unreal;`

6. 在 mod 构造函数中使用 `register_on_unreal_init` 注册 on_unreal_init 回调，使构造函数完整代码如下：

```c++
    MyAwesomeMod() : CppUserModBase()
    {
        ModName = STR("MyAwesomeMod");
        ModVersion = STR("1.0");
        ModDescription = STR("This is my awesome mod");
        ModAuthors = STR("UE4SS Team");
        // 除非你想以某种方式针对与当前构建所用的 UE4SS 不同的版本，
        // 否则不要修改这一项。
        //ModIntendedSDKVersion = STR("2.6");

        register_on_unreal_init(&on_unreal_init);

        printf("MyAwesomeMod says hello\n");
    }
```

7. 像这样定义 `on_unreal_init`：

```c++
static auto on_unreal_init(CppModUserBase* self) -> void
{
    // 在该函数触发之后，你可以在本函数以及任何其他地方使用 'Unreal' 命名空间。
    auto Object = UObjectGlobals::StaticFindObject<UObject*>(nullptr, nullptr, STR("/Script/CoreUObject.Object"));
    Output::send<LogLevel::Verbose>(STR("Object Name: {}\n"), Object->GetFullName());
}
```

注意，`Output::send` 不需要 `LogLevel`，并且我们使用 `{}` 而不是 `%s` 作为格式占位符。
`Output::send` 函数在底层使用 `std::format`，如果想了解更多，你应该去查阅 std::format 或 libfmt 的相关资料。

7. 右键点击你的项目并选择 `Build`。

## 第 4 部分

[点击查看 C++ Mod 安装指南](./installing-a-c++-mod.md)
