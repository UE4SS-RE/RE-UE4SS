#include <JSON/Null.hpp>

namespace RC::JSON
{
    auto Null::serialize(ShouldFormat should_format, int32_t* indent_level) -> SystemStringType
    {
        (void)should_format;
        (void)indent_level;
        return SYSSTR("null");
    }
} // namespace RC::JSON
