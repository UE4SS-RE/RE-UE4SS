#include <chrono>
#include <format>

#include <DynamicOutput/OutputDevice.hpp>

namespace RC::Output
{
    auto OutputDevice::has_optional_arg() const -> bool
    {
        return false;
    }

    auto OutputDevice::receive_with_optional_arg([[maybe_unused]] SystemStringViewType fmt, [[maybe_unused]] int32_t optional_arg) const -> void
    {
        // This only exists to make it not required to implement
        // Most devices probably won't use this
        throw std::runtime_error{"OutputDevice::receive_with_optional_arg called but no derived implementation found"};
    }

    auto OutputDevice::set_formatter(Formatter new_formatter) -> void
    {
        m_formatter = new_formatter;
    }

    auto OutputDevice::get_now_as_string() -> const SystemStringType
    {
        auto now = std::chrono::system_clock::now();
        const SystemStringType when_as_string = std::format(SYSSTR("{:%Y-%m-%d %X}"), now);
        return when_as_string;
    }

    auto OutputDevice::default_format_string(SystemStringViewType string_to_format) -> SystemStringType
    {
        return std::format(SYSSTR("[{}] {}"), get_now_as_string(), string_to_format);
    }
} // namespace RC::Output
