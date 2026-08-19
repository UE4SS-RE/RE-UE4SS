#include <chrono>
#include <locale>
#include <sstream>

#include <GUI/ConsoleOutputDevice.hpp>
#include <UE4SSProgram.hpp>
#include <CallStackDebug/CallStackDebug.hpp>

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
        auto formatted_message = m_formatter(fmt_copy);
        std::wstringstream stream{formatted_message};
        for (File::StringType line; std::getline(stream, line);)
        {
            UE4SSProgram::get_program().get_debugging_ui().get_console().add_line(line, color);
        }
#if RC_STACKTRACE_ENABLED
        if (static_cast<LogLevel::LogLevel>(optional_arg) == LogLevel::Error)
        {
            const auto trace_string = to_string(CallStackDebug::generate_call_stack());
            std::stringstream stack_stream{trace_string};
            for (std::string line; std::getline(stack_stream, line);)
            {
                UE4SSProgram::get_program().get_debugging_ui().get_console().add_line(line, Color::Red);
            }
        }
#endif
#endif
    }
} // namespace RC::Output
