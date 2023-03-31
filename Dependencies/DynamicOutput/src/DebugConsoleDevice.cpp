#include <chrono>
#include <locale>

#include <DynamicOutput/DebugConsoleDevice.hpp>
#include <DynamicOutput/Output.hpp>

// TODO: Abstract the colors system
#ifdef UE4SS_CONSOLE_COLORS_ENABLED
#define NOMINMAX
#include <Windows.h>
#ifdef TEXT
#undef TEXT
#endif
#endif

namespace RC::Output
{
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
#ifdef UE4SS_CONSOLE_COLORS_ENABLED
#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
        printf_s("DebugConsoleDevice received: %S", m_formatter(fmt).c_str());
#else
        Color::Color color_enum = static_cast<Color::Color>(optional_arg);
        WORD text_attributes{FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE};

        if (color_enum == Color::Default)
        {
            color_enum = static_cast<Color::Color>(DefaultTargets::get_default_log_level());
        }

        switch (color_enum)
        {
            case Color::Default:
            case Color::NoColor:
                break;
            case Color::Cyan:
                text_attributes = FOREGROUND_BLUE | FOREGROUND_GREEN;
                break;
            case Color::Yellow:
                text_attributes = FOREGROUND_RED | FOREGROUND_GREEN;
                break;
            case Color::Red:
                text_attributes = FOREGROUND_RED;
                break;
            case Color::Green:
                text_attributes = FOREGROUND_GREEN;
                break;
            case Color::Blue:
                text_attributes = 9;
                break;
            case Color::Magenta:
                text_attributes = FOREGROUND_BLUE | FOREGROUND_RED;
                break;
        }

        if (color_enum != Color::NoColor)
        {
            text_attributes |= FOREGROUND_INTENSITY;
        }

        Lock output_lock{this};
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text_attributes);
        printf_s("%S", m_formatter(fmt).c_str());

#endif
#else
#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
        printf_s("DebugConsoleDevice received: %S", m_formatter(fmt).c_str());
#else
        printf_s("%S", m_formatter(fmt).c_str());
#endif
#endif
    }

    auto DebugConsoleDevice::lock() const -> void
    {
#ifdef UE4SS_CONSOLE_COLORS_ENABLED
        m_receive_mutex.lock();
#endif
    }

    auto DebugConsoleDevice::unlock() const -> void
    {
#ifdef UE4SS_CONSOLE_COLORS_ENABLED
        m_receive_mutex.unlock();
#endif
    }
}
