#include <Input/PlatformInputSource.hpp>
#include <Input/Platform/NcursesInputSource.hpp>
#include <Input/Platform/Win32AsyncInputSource.hpp>

namespace RC::Input
{
    auto Handler::init() -> void
    {
#ifdef WIN32
        register_input_source(std::make_shared<Win32AsyncInputSource>(L"ConsoleWindowClass", L"UnrealWindow"));
#endif
#ifdef LINUX
        register_input_source(std::make_shared<NcursesInputSource>());
#endif
    }
} // namespace RC::Input