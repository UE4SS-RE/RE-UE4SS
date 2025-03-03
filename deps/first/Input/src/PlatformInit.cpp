#include <Input/PlatformInputSource.hpp>
#include <Input/Platform/Win32AsyncInputSource.hpp>
#include <Input/Platform/GLFW3InputSource.hpp>

namespace RC::Input
{
    auto Handler::init() -> void
    {
        register_input_source(std::make_shared<Win32AsyncInputSource>(L"ConsoleWindowClass", L"UnrealWindow"));
        register_input_source(std::make_shared<GLFW3InputSource>());
    }
} // namespace RC::Input