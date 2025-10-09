#include <chrono>
#include <format>
#include <fmt/xchar.h>
#include <fmt/chrono.h>
#include <DynamicOutput/OutputDevice.hpp>
#include <Helpers/Time.hpp>

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

    auto OutputDevice::default_format_string(File::StringViewType string_to_format) -> File::StringType
    {
        return fmt::format(STR("[{}] {}"), get_now_as_string(STR("{:%Y-%m-%d %X}")), string_to_format);
    }
} // namespace RC::Output
