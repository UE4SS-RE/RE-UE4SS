#include <JSON/Null.hpp>

namespace RC::JSON
{
    auto Null::serialize(ShouldFormat should_format, int32_t* indent_level) -> StringType
    {
        (void)should_format;
        (void)indent_level;
        return STR("null");
    }
}
