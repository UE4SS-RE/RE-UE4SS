#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, // handle to DLL module
                    DWORD fdwReason, // reason for calling function
                    LPVOID lpvReserved) // reserved
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH: break;

        case DLL_THREAD_ATTACH: break;

        case DLL_THREAD_DETACH: break;

        case DLL_PROCESS_DETACH: break;
    }
    return TRUE;
}