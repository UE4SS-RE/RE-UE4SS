#include <Helpers/Debug.hpp>

#if defined(WIN32) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>
#include <chrono>

auto RC::await_debugger() -> void
{
    while (!IsDebuggerPresent())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

#else

#warning "await_debugger not supported on non-Windows platforms!"
auto RC::await_debugger() -> void {}

#endif
