#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <tlhelp32.h>

#include "UE4SSProgram.hpp"
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

#define WIN_API_FUNCTION_NAME DllMain

using namespace RC;

// We're outside DllMain here
auto thread_dll_start(UE4SSProgram* program) -> unsigned long
{
    // Wrapper for entire program
    // Everything must be channeled through MProgram
    // One of the purposes for this is to forward any errors to main so that we can close or keep the window/console open
    // There is atleast one more purpose but I forgot what it was...
    program->init();

    if (auto e = program->get_error_object(); e->has_error())
    {
        // If the output system errored out then use printf_s as a fallback
        // Logging will only happen to the debug console but it's something at least
        if (!Output::has_internal_error())
        {
            Output::send<LogLevel::Error>(STR("Fatal Error: {}\n"), ensure_str(e->get_message()));
        }
        else
        {
            printf_s("Error: %s\n", e->get_message());
        }
    }

    return 0;
}

auto process_initialized(HMODULE moduleHandle) -> void
{
    wchar_t moduleFilenameBuffer[1024]{'\0'};
    GetModuleFileNameW(moduleHandle, moduleFilenameBuffer, sizeof(moduleFilenameBuffer) / sizeof(wchar_t));

    auto program = new UE4SSProgram(moduleFilenameBuffer, {});
    if (HANDLE handle = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(thread_dll_start), (LPVOID)program, 0, nullptr); handle)
    {
        CloseHandle(handle);
    }
}

auto get_main_thread_id() -> DWORD
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    DWORD currentPid = GetCurrentProcessId();

    THREADENTRY32 th32;
    th32.dwSize = sizeof(THREADENTRY32);

    uint64_t earliestCreationTime = std::numeric_limits<uint64_t>::max();
    DWORD mainThreadId = 0;

    for (Thread32First(snapshot, &th32); Thread32Next(snapshot, &th32);)
    {
        if (th32.th32OwnerProcessID != currentPid)
        {
            continue;
        }

        HANDLE thread = OpenThread(THREAD_QUERY_INFORMATION, false, th32.th32ThreadID);
        if (!thread)
        {
            continue;
        }

        FILETIME threadTimes[4];
        if (!GetThreadTimes(thread, &threadTimes[0], &threadTimes[1], &threadTimes[2], &threadTimes[3]))
        {
            CloseHandle(thread);
            continue;
        }

        uint64_t creationTime = ((uint64_t)threadTimes[0].dwHighDateTime << 32) | threadTimes[0].dwLowDateTime;
        if (creationTime < earliestCreationTime)
        {
            earliestCreationTime = creationTime;
            mainThreadId = th32.th32ThreadID;
        }

        CloseHandle(thread);
    }

    return mainThreadId;
}

// We're still inside DllMain so be careful what you do here
auto dll_process_attached(HMODULE moduleHandle) -> void
{
    // injected through proxy
    if (GetCurrentThreadId() == get_main_thread_id())
    {
        QueueUserAPC((PAPCFUNC)process_initialized, GetCurrentThread(), (ULONG_PTR)moduleHandle);
    }
    else // injected manually -> thread id different from main
    {
        process_initialized(moduleHandle);
    }
}

auto WIN_API_FUNCTION_NAME(HMODULE hModule, DWORD ul_reason_for_call, [[maybe_unused]] LPVOID lpReserved) -> BOOL
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        dll_process_attached(hModule);
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        UE4SSProgram::static_cleanup();
        break;
    }
    return TRUE;
}