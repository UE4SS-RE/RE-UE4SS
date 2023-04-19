# Creating a C++ mod

This guide will help you create a C++ mod using UE4SS.  
It's split up into three parts.  
Part one goes over the prerequisites.  
Part two goes over creating the most basic C++ mod possible.  
Part three will show you how to interact with UE4SS and UE itself (via UE4SS).
## Part #1
1. Make an Epic account and link it to your GitHub account.
2. Clone the UE4SS repo.
3. In the command prompt or bash while in the UE4SS repo directory, execute: `git submodule update --init --recursive`
3. Make a new directory in the UE4SS repo directory, this is where the code for all your mods will live, and I called my directory `MyCppMods`.
4. Make another directory inside the directory that you just created, this will be for your new mod, which I will call `MyAwesomeMod`.
## Part #2
1. Create a file called `CMakeLists.txt` and put this inside it:
```cmake
cmake_minimum_required(VERSION 3.18)

set(TARGET MyAwesomeMod)
project(${TARGET})

add_library(${TARGET} SHARED "dllmain.cpp")
target_include_directories(${TARGET} PRIVATE .)
target_link_libraries(${TARGET} PUBLIC cppsdk_xinput)
```
2. Make a file called `dllmain.cpp` and put this inside it:
```c++
#include <stdio.h>
#include <Mod/CppUserModBase.hpp>

class MyAwesomeMod : public RC::CppUserModBase
{
public:
    MyAwesomeMod() : CppUserModBase()
    {
        printf("MyAwesomeMod says hello\n");
    }

    ~MyAwesomeMod()
    {
    }

    auto update() -> void override
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
3. Open the `CMakeLists.txt` file that's located in the root directory of the repo.
4. Find `add_subdirectory("Dependencies/ArgsParser")` and add `add_subdirectory("MyCppMods/MyAwesomeMod")` on the next line.
5. In the command prompt, cd to `VS_Solution` and then execute `generate_vs_solution.bat`.  
It's important that this bat file is executed inside the `VS_Solution` directory, the `cd` part is not optional.  
You can also double click that file instead of using the command prompt but you won't get notified if anything went wrong.
4. Open VS_Solution\ue4ss.sln.
5. Find your project (in my case: MyAwesomeMod) in the solution explorer and right click it and hit `Build`.
6. From this point, follow the C++ mod installation guide.
## Part #3
In this part, we're going to learn how to output messages to the GUI console, find a UObject by name, and output that name to the GUI console.
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
It's longer than a call to `printf`, but in return the message gets propagated to both the regular console and the GUI console.  
We also get some support for colors via the `LogLevel` enum.
4. Add this below the DynamicOutput include:
```c++
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>
```
5. Let's again utilize the `using namespace` shortcut by adding this below the first one: `using namespace RC::Unreal;`
6. Add this below the call to `Output::send` in the `MyAwesomeMod` constructor:
```c++
auto Object = UObjectGlobals::StaticFindObject(nullptr, nullptr, STR("/Script/CoreUObject.Object"));
Output::send(STR("Object Name: {}\n"), Object->GetFullName());
```
Note that `Output::send` doesn't require a `LogLevel` and that we're using `{}` in the format string instead of `%s`.  
The `Output::send` function uses `std::format` in the back-end so you should do some research around std::format or libfmt if you want to know more about it.  
7. Right click your project and hit `Build`.  
8. From this point, follow the C++ mod installation guide.
