#include <Input/PlatformInputSource.hpp>
#include <Input/Platform/NcursesInputSource.hpp>
#include <Input/Platform/Win32APIInputSource.hpp>

namespace RC::Input
{
    auto Handler::init() -> void
    {
        #ifdef WIN32
            register_input_source(std::make_shared<Win32APIInputSource>(L"ConsoleWindowClass", L"UnrealWindow"));
        #endif
        #ifdef LINUX
            register_input_source(std::make_shared<NcursesInputSource>());
        #endif
    }
}