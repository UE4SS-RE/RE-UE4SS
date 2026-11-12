# 调试 C++ Mod

仅靠日志能做的事有限。有时你可能想设置断点，直接检查调用栈与变量。本指南将展示如何做到这一点。

请注意，本指南同样完全适用于调试 UE4SS 本身。

## 前提条件

本指南假定你已经知道如何使用调试器。如果还不会，请查看你所使用 IDE 的文档；以下是几个链接：

* [Visual Studio](https://learn.microsoft.com/en-us/visualstudio/debugger/debugger-feature-tour?view=visualstudio)
* [CLion](https://www.jetbrains.com/help/clion/debugging-code.html#useful-debugger-shortcuts)
* [VS Code](https://code.visualstudio.com/docs/cpp/cpp-debug)

## 操作步骤

1. 在你要调试的文件顶部附近添加 `#include <Helpers/Debug.hpp>`。
2. 在断点被命中之前的某个时刻调用 `RC::await_debugger()`。这会让当前线程停止执行，直到有调试器连接为止。
3. 把你的调试器附加到游戏进程上。具体做法请参考你的 IDE 文档，该选项通常位于 `Run` 或 `Debug` 菜单下。你要调试的可执行文件通常是游戏目录中与 `ue4ss` 文件夹并列的那个。
   * 另一种做法是，把 IDE 的调试器设置为在可执行文件启动时自动附加。
4. 断点就会被命中！

## 重要注意事项

* 没有任何稳定性保证。虚幻引擎是多线程的，长时间占住游戏线程可能会出问题。多数情况下，你至少可以单步调试你想调试的代码。把 IDE 设置为自动附加到游戏进程可以缓解甚至消除这些问题。
* `RC::await_debugger()` 被标记为已弃用，因此编译器会因为你使用它而报警告。这是有意为之：在你的 mod 或 UE4SS 的发布/公开构建中绝不应调用它。*它实际上并没有被弃用。*
