// Main file that initializes all tests
// TODO: Consider using a proper testing framework and maybe making this a separate DLL file

#include <Windows.h>
#include <iostream>
#include "UE4SSProgramTest.hpp"
#include <DynamicOutput/DynamicOutput.hpp>

using namespace RC;

// We're outside DllMain here
auto WINAPI thread_dll_start([[maybe_unused]]LPVOID thread_param) -> unsigned long
{
    // Wrapper for entire program
    // Everything must be channeled through MProgram
    // One of the purposes for this is to forward any errors to main so that we can close or keep the window/console open
    // There is atleast one more purpose but I forgot what it was...
    HMODULE moduleHandle = reinterpret_cast<HMODULE>(thread_param);
    wchar_t moduleFilenameBuffer[1024] {'\0'};
    GetModuleFileNameW(moduleHandle, moduleFilenameBuffer, sizeof(moduleFilenameBuffer) / sizeof(wchar_t));

    UE4SSProgramTest program = UE4SSProgramTest(moduleFilenameBuffer, {});

    if (auto e = program.get_error_object(); e->has_error())
    {
        // If the output system errored out then use printf_s as a fallback
        // Logging will only happen to the debug console but it's something at least
        if (!Output::has_internal_error())
        {
            Output::send(STR("Error: {}\n"), RC::fmt(L"%S", e->get_message()));
        }
        else
        {
            printf_s("Error: %s\n", e->get_message());
        }
    }

    return 0;
}

// We're still inside DllMain so be careful what you do here
auto dll_process_attached(HMODULE moduleHandle) -> void
{
    if (HANDLE handle = CreateThread(nullptr, 0, thread_dll_start, moduleHandle, 0, nullptr); handle)
    {
        CloseHandle(handle);
    }

    std::cin.get();
}

auto APIENTRY DllMain([[maybe_unused]] HMODULE hModule,
                      DWORD ul_reason_for_call,
                      [[maybe_unused]] LPVOID lpReserved
) -> BOOL
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