#pragma once

namespace RC
{
    /**
     * @brief Waits for a debugger on the current thread. Currently, windows only.
     * @details Invokes `IsDebuggerPresent` in a loop. You should call this before any breakpoints you set. Note that this makes no stability guarantees;
     * Unreal is multithreaded, so there could be issues if you hold the game thread for too long. In most cases, you'll at least be able to step through
     * the code you want to debug. Setting your IDE to automatically attach itself to the game process can alleviate/eliminate these issues.
     * @note This function is not actually deprecated; it's marked as such to yell at you for calling it in order to make it harder to accidentally use it
     * in a release/public build.
     */
    [[deprecated("await_debugger called! This should only be used for development.")]]
    auto await_debugger() -> void;
}