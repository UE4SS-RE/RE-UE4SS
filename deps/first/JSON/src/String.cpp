#include <JSON/String.hpp>

namespace RC::JSON
{
    String::String(SystemStringViewType string) : m_data(string)
    {
    }

    auto String::serialize([[maybe_unused]] ShouldFormat should_format, [[maybe_unused]] int32_t* indent_level) -> SystemStringType
    {
        return std::format(SYSSTR("\"{}\""), m_data);
    }
} // namespace RC::JSON
