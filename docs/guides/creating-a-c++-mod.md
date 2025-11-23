# Creating a C++ mod

This guide will help you create a C++ mod using UE4SS.  
It's split up into four parts.  
Part one goes over the prerequisites.  
Part two goes over creating the most basic C++ mod possible.  
Part three will show you how to interact with UE4SS and UE itself (via UE4SS).  
Part four will cover installation of the mod.

> The guide requires having a working C++ development environment with `CMake` and `git`, preferably similar to the one required to build UE4SS itself from sources.

## Part 1
**Make sure you have downloaded all the [build requirements mentioned in the README](https://docs.ue4ss.com/#build-requirements) before following these steps!**

1. Make an Epic account and link it to your GitHub account
2. Check your email and accept the invitation to the @EpicGames GitHub organization for Unreal source access.
3. Setup SSH keys on your GitHub account which will let git access the Unreal source you got access for in 2 and 3.
4. Make a directory somewhere on your computer, the name doesn't matter but I named mine `MyMods`.
5. Clone the RE-UE4SS repo so that you end up with `MyMods/RE-UE4SS`.
6. Open CMD and cd into `RE-UE4SS` and execute: `git submodule update --init --recursive`
7. Go back to the `MyMods` directory and create a new directory, this directory will contain your mod source files.
I named mine `MyAwesomeMod`.
8. Create a file called `CMakeLists.txt` inside `MyMods` and put this inside it:
```cmake
cmake_minimum_required(VERSION 3.22)
project(MyMods)

add_subdirectory(RE-UE4SS)
add_subdirectory(MyAwesomeMod)
```

## Part #2
1. Create a file called `CMakeLists.txt` inside `MyMods/MyAwesomeMod` and put this inside it:
```cmake
set(TARGET MyAwesomeMod)

add_library(${TARGET} SHARED
    dllmain.cpp
)

target_include_directories(${TARGET} PRIVATE .)
target_link_libraries(${TARGET} PUBLIC UE4SS)

# Copy the DLL to the game's mod directory after building (optional, adjust path as needed)
# add_custom_command(TARGET ${TARGET} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET}> "path/to/game/Binaries/Win64/Mods/${TARGET}/dlls/"
# )
```
2. Make a file called `dllmain.cpp` in `MyMods/MyAwesomeMod` and put this inside it:
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
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        //ModIntendedSDKVersion = STR("2.6");
        
        printf("MyAwesomeMod says hello\n");
    }

    ~MyAwesomeMod() override
    {
    }

    auto on_update() -> void override
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
3. In the command prompt, in the `MyMods` directory, execute either:
A. Build from command line with Ninja:
```bash
# Configure (Ninja is single-configuration)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Game__Shipping__Win64

# Build (no --config needed for Ninja)
cmake --build build
```
or B. Generate Visual Studio solution:
```bash
# Configure (MSVC is multi-configuration)
cmake -B build -G "Visual Studio 17 2022"

# Build from command line (MSVC requires --config)
cmake --build build --config Game__Shipping__Win64
```
If you chose option `B`, the VS solution will be in the `build` directory.

4. Open `MyMods/build/MyMods.sln` in Visual Studio
5. Make sure that you're set to the `Game__Shipping__Win64` configuration unless you want to debug.
6. Find your project (in my case: MyAwesomeMod) in the solution explorer and right click it and hit `Build`.

## Part #3
In this part, we're going to learn how to log to file, and both consoles, as well as find a UObject by name, and log that name.
1. Add `#include <DynamicOutput/DynamicOutput.hpp>` under `#include <Mod/CppUserModBase.hpp>`.  
You can now also remove `#include <stdio.h>` because we'll be removing the use of `printf` which was the only thing that required it.
2. To save some time and annoyance and make the code look a bit better, add this line below all the includes:
```c++
using namespace RC;
```
3. Replace the call to printf in the body of the `MyAwesomeMod` constructor with:
```c++
Output::send<LogLevel::Verbose>(STR("MyAwesomeMod says hello\n"));
```
It's longer than a call to `printf`, but in return the message gets propagated to the log file and both the regular console and the GUI console.  
We also get some support for colors via the `LogLevel` enum.

4. Add this below the DynamicOutput include:
```c++
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>
```
5. Let's again utilize the `using namespace` shortcut by adding this below the first one: `using namespace RC::Unreal;`
6. Add this function in your mod class:
```c++
auto on_unreal_init() -> void override
{
    // You are allowed to use the 'Unreal' namespace in this function and anywhere else after this function has fired.
    auto Object = UObjectGlobals::StaticFindObject<UObject*>(nullptr, nullptr, STR("/Script/CoreUObject.Object"));
    Output::send<LogLevel::Verbose>(STR("Object Name: {}\n"), Object->GetFullName());
}
```
Note that `Output::send` doesn't require a `LogLevel` and that we're using `{}` in the format string instead of `%s`.  
The `Output::send` function uses `std::format` in the back-end so you should do some research around std::format or libfmt if you want to know more about it.

7. Right click your project and hit `Build`.

## Part #4

[Click to go to guide for installing a C++ Mod](./installing-a-c++-mod.md)
