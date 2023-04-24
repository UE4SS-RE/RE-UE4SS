#include <chrono>
#include <locale>

#include <DynamicOutput/DebugConsoleDevice.hpp>
#include <DynamicOutput/Output.hpp>

#define NOMINMAX
#include <Windows.h>
#ifdef TEXT
#undef TEXT
#endif

namespace RC::Output
{
    static auto log_level_to_color(LogLevel::LogLevel log_level) -> std::string
    {
        switch (log_level)
        {
        case LogLevel::Default:
        case LogLevel::Normal:
            return "\033[0;0m";
        case LogLevel::Verbose:
            return "\033[1;36m";
        case LogLevel::Warning:
            return "\033[1;33m";
        case LogLevel::Error:
            return "\033[1;31m";
        }
        
        return "\033[0;0m";
    }

    auto DebugConsoleDevice::set_windows_console_out_mode_if_needed() const -> void
    {
        if (m_windows_console_mode_set) { return; }
        HANDLE current_console_out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (current_console_out_handle != INVALID_HANDLE_VALUE)
        {
            DWORD current_console_out_mode = 0;
            GetConsoleMode(current_console_out_handle, &current_console_out_mode);
            SetConsoleMode(current_console_out_handle, current_console_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
        m_windows_console_mode_set = true;
    }

    auto DebugConsoleDevice::has_optional_arg() const -> bool
    {
        return true;
    }

    auto DebugConsoleDevice::receive(File::StringViewType fmt) const -> void
    {
        receive_with_optional_arg(fmt, Color::NoColor);
    }

    auto DebugConsoleDevice::receive_with_optional_arg(File::StringViewType fmt, [[maybe_unused]]int32_t optional_arg) const -> void
    {
        set_windows_console_out_mode_if_needed();

#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
        printf_s("DebugConsoleDevice received: %S", m_formatter(fmt).c_str());
#else
        printf_s("%s%S\033[0m", log_level_to_color(static_cast<LogLevel::LogLevel>(optional_arg)).c_str(), m_formatter(fmt).c_str());
#endif
    }
}
