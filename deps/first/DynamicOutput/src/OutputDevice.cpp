#include <chrono>
#include <format>
#include <fmt/xchar.h>
#include <fmt/chrono.h>
#include <DynamicOutput/OutputDevice.hpp>

#if _WIN32
#define NOMINMAX
#include <Windows.h>
#ifdef TEXT
#undef TEXT
#endif
#endif

namespace RC::Output
{
    auto OutputDevice::has_optional_arg() const -> bool
    {
        return false;
    }

    auto OutputDevice::receive_with_optional_arg([[maybe_unused]] File::StringViewType fmt, [[maybe_unused]] int32_t optional_arg) const -> void
    {
        // This only exists to make it not required to implement
        // Most devices probably won't use this
        throw std::runtime_error{"OutputDevice::receive_with_optional_arg called but no derived implementation found"};
    }

    auto OutputDevice::set_formatter(Formatter new_formatter) -> void
    {
        m_formatter = new_formatter;
    }

    auto OutputDevice::get_now_as_string() -> const File::StringType
    {
        File::StringType when_as_string{};
        bool use_local_time = true;
#ifdef _WIN32
        if (auto module = GetModuleHandleW(L"ntdll.dll"); module && GetProcAddress(module, "wine_get_version"))
        {
            use_local_time = false;
        }
#endif
        if (use_local_time)
        {
            static const auto timezone = std::chrono::current_zone();
            auto now = std::chrono::time_point_cast<std::chrono::system_clock::duration>(timezone->to_local(std::chrono::system_clock::now()));
            when_as_string = fmt::format(STR("{:%Y-%m-%d %X}"), now);
        }
        else
        {
            auto now = std::chrono::system_clock::now();
            when_as_string = fmt::format(STR("{:%Y-%m-%d %X}"), now);
        }
        return when_as_string;
    }

    auto OutputDevice::default_format_string(File::StringViewType string_to_format) -> File::StringType
    {
        return fmt::format(STR("[{}] {}"), get_now_as_string(), string_to_format);
    }
} // namespace RC::Output
