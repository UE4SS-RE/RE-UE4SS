# SDK 生成器

该生成器旨在让 C++ mod 能够方便地访问所有已反射的成员变量与函数。
它还会为所有已反射的枚举生成 `enum` 定义。
生成的结构体与类在内存布局上是精确的：前置填充、成员间填充与尾部填充都会被输出，静态数组与位域会被还原，结构体对齐也经过计算而非假定。

当派生类型的成员落在基类型尾部填充区之内时（MSVC 允许非 POD 类型这样做），基类型会以显式 `alignas` 打包输出，从而让派生成员仍落在正确的偏移上。

若某个属性的 C++ 类型无法被生成器表达——例如 `TMap`/`TSet` 成员、Verse 属性，或运行中的引擎定义而生成器尚不认识的属性类——则会以 `UE4SS_SDK/PlaceholderTypes.hpp` 中的不透明占位类型输出，其大小与对齐由引擎报告。该成员仍然保留名称与偏移，其后的所有成员位置依然正确，并且你可以通过 `GetTyped<T>()` 把自己的定义重新解释到这块存储上。而在以前，一个这样的属性就会导致整个转储中止，不产生任何文件。

要针对特定游戏构建验证生成的 SDK，可以包含 `UE4SS_SDK/LayoutAsserts.hpp`。其中包含针对每个生成类型的大小、对齐以及每个成员偏移的 `static_assert`。它默认不会被包含，因此布局问题只会表现为某个具体类型的断言失败，而不会破坏整个 SDK 的构建。`UE4SS_SDK/RuntimeSDKTest.hpp` 则在运行时对实际运行的游戏做同样的检查。

UE4SS 后端被编译进 UE4SS 之中，因此生成器无需任何配置文件即可工作。把一份后端描述放入 `<working_dir>/UE4SS_SDK_Backends` 即可新增一个后端，或者在后端使用相同文件名（`UE4SS.json`）时覆盖内置后端。`assets/UE4SS_SDK_Backends/UE4SS.json` 中随附的副本与内置后端由同一个文件生成，因此它也可以作为自定义的起点。

## 工作原理

该生成器生成类、结构体和枚举。
它的设计目标是让人能在以 UE4SS 作为 UE 后端的 C++ mod 中轻松使用。
如果你不想把 UE4SS 作为后端，那么需要你自己提供所有必需的 UE 类型，例如 `UObject`，以及模板类型，例如 `TArray`、`TObjectPtr` 和 `TWeakObjectPtr`。
你还必须提供 UE 的 `FindObject<T>` 函数模板，因为运行时正是通过它来查找函数的。
你的后端必须支持这样的调用：`FindObject<UFunction>(nullptr, L"/Script/Engine.CharacterMovementComponent:SetMovementMode")`

## 使用方法

以下指南面向 UE4SS 的 C++ mod，但经过调整后也可用于任何 UE 后端。
关于如何配合其他后端使用，请查看 `UE4SS-settings.ini` 中的 `[SDKGenerator]` 小节。

1. 确保已将 `GuiConsoleEnabled` 与 `GuiConsoleVisible` 设置为 1，然后启动游戏。
2. 在 GUI 中切换到 `Dumpers` 标签页，点击 `Generate SDK` 按钮。
   此时 `<Game>/Binaries/Win64` 下应该已经出现了 `UE4SS_SDK` 目录。
3. 把这个目录整体复制到你的 C++ mod 目录中。
4. 在你 C++ mod 的 `CMakeLists.txt` 中加入 `add_subdirectory(UE4SS_SDK)`，并确保把 `UE4SS_SDK` 加入 `target_link_libraries`。

## 在 C++ 中使用

要在代码中使用该 SDK，你需要包含你关心的那部分头文件。

下面是一个示例，无论你选择哪个后端它都是准确的。
以 UE4SS 作为后端与使用其他后端的唯一区别在于基础 UE 类型来自哪里。
如果你使用 UE4SS 作为后端，这个示例并不适用，因为 `AActor` 类型已经由 UE4SS 提供；不过该示例对 UE4SS 未提供的任何类型都适用：

```c++
#include <UE4SS_SDK/Script/Engine/Actor.hpp>

// 本示例经过简化，省略了 'int main()'
// 以及其他无关但必需的代码。

void DoStuff()
{
    // 在本示例中，假设 'Actor' 是一个有效的 AActor。
    AActor* Actor{};

    // 'GetChildren' 函数返回 'TArray<AActor*>'，因此你必须已经提供了 'TArray<T>'。
    auto& Array = AsActor->Children;
    for (const auto& Element : Array)
    {
        printf("Element: %S", Element->GetFullName().c_str());
    }
}
```
