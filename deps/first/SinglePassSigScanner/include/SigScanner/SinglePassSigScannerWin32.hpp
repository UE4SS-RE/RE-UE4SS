#pragma once

#include <SigScanner/Common.hpp>

// Windows.h forward declarations
struct _SYSTEM_INFO;
typedef _SYSTEM_INFO SYSTEM_INFO;
struct _MODULEINFO;
typedef _MODULEINFO MODULEINFO;

namespace RC
{
    // Windows structs, to prevent the need to include Windows.h in this header
    struct WIN_MODULEINFO
    {
        void* lpBaseOfDll;
        unsigned long SizeOfImage;
        void* EntryPoint;
        RC_SPSS_API auto operator=(MODULEINFO) -> WIN_MODULEINFO&;
    };
    using ModuleInfo = WIN_MODULEINFO;
    using ModuleOS = MODULEINFO;
    using SystemInfo = SYSTEM_INFO;
} // namespace RC
