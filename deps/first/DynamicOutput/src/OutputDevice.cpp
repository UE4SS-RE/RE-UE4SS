#include <chrono>
#include <format>

#include <DynamicOutput/OutputDevice.hpp>

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
        auto now = std::chrono::system_clock::now();
        const File::StringType when_as_string = std::format(STR("{:%Y-%m-%d %X}"), now);
        return when_as_string;
    }

    auto OutputDevice::default_format_string(File::StringViewType string_to_format) -> File::StringType
    {
        return std::format(STR("[{}] {}"), get_now_as_string(), string_to_format);
    }
} // namespace RC::Output
