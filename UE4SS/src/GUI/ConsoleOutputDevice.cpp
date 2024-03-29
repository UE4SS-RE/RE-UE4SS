#include <chrono>
#include <locale>

#include <GUI/ConsoleOutputDevice.hpp>
#include <UE4SSProgram.hpp>

namespace RC::Output
{
    auto ConsoleDevice::has_optional_arg() const -> bool
    {
        return true;
    }

    auto ConsoleDevice::receive(File::StringViewType fmt) const -> void
    {
        receive_with_optional_arg(fmt, Color::NoColor);
    }

    auto ConsoleDevice::receive_with_optional_arg(File::StringViewType fmt, [[maybe_unused]] int32_t optional_arg) const -> void
    {
#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
        printf_s("ConsoleDevice received: %S", m_formatter(fmt).c_str());
#else
        auto fmt_copy = File::StringType{fmt};
        if (fmt_copy.ends_with(STR('\n')))
        {
            fmt_copy.pop_back();
        }
        auto color = static_cast<Color::Color>(optional_arg);
        UE4SSProgram::get_program().get_debugging_ui().get_console().add_line(m_formatter(fmt_copy), color);
#endif
    }
} // namespace RC::Output
