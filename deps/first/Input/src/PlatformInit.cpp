#include <Input/PlatformInputSource.hpp>
#include <Input/Platform/NcursesInputSource.hpp>
#include <Input/Platform/Win32AsyncInputSource.hpp>
#include <Input/Platform/GLFW3InputSource.hpp>

namespace RC::Input
{
    auto Handler::init() -> void
    {
#ifdef WIN32
        register_input_source(std::make_shared<Win32AsyncInputSource>(L"ConsoleWindowClass", L"UnrealWindow"));
#endif
#ifdef LINUX
#ifdef HAS_TUI
        register_input_source(std::make_shared<NcursesInputSource>());
#endif
#ifdef HAS_GLFW
        register_input_source(std::make_shared<GLFW3InputSource>());
#endif
#endif
    }
} // namespace RC::Input