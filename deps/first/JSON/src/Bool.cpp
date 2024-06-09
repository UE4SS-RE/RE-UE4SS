#include <JSON/Bool.hpp>

namespace RC::JSON
{
    Bool::Bool(bool value) : m_underlying_value(value)
    {
    }

    auto Bool::serialize(ShouldFormat should_format, int32_t* indent_level) -> SystemStringType
    {
        (void)should_format;
        (void)indent_level;
        return m_underlying_value ? SYSSTR("true") : SYSSTR("false");
    }
} // namespace RC::JSON
