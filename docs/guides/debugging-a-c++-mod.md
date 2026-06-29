# Debugging a C++ Mod

Logging can only get you so far. Sometimes, you may want to set a breakpoint and inspect the callstack and variables directly. This guide will show you how.

Note that this guide is also a completely valid way to debug UE4SS itself.

## Prerequisites
This guide assumes you already know how to use a debugger. If you don't, check your IDE's guides; here's a few:
* [Visual Studio](https://learn.microsoft.com/en-us/visualstudio/debugger/debugger-feature-tour?view=visualstudio)
* [CLion](https://www.jetbrains.com/help/clion/debugging-code.html#useful-debugger-shortcuts)
* [VS Code](https://code.visualstudio.com/docs/cpp/cpp-debug)

## The Process
1. Add `#include <Helpers/Debug.hpp>` near the top of the file you want to debug.
2. At some point before your breakpoint is hit, call `RC::await_debugger()`. This will stop execution on the current thread until a debugger is connected. 
3. Attach your debugger to the game's process. Refer to your IDE's guides on how exactly to do this, though this option is usually under a `Run` or `Debug` menu. The executable you want to debug is usually the one that's next to the `ue4ss` folder in your game directory.
   * Alternatively, set your IDE's debugger to automatically attach itself to an executable when it launches.
4. The breakpoint will be hit!

## Important Caveats
* There are no stability guarantees. Unreal is multithreaded, so there could be issues if you hold the game thread for too long. In most cases, you'll at least be able to step through the code you want to debug. Setting your IDE to automatically attach itself to the game process can alleviate/eliminate these issues.
* `RC::await_debugger()` is marked as deprecated, so the compiler will scream at you for using it. This is intentional; this should never be called in a release/public build of your mod or UE4SS. *It's not actually deprecated.*