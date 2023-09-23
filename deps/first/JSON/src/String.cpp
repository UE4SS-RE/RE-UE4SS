#include <JSON/String.hpp>

namespace RC::JSON
{
    String::String(StringViewType string): m_data(string) {}

    auto String::serialize([[maybe_unused]]ShouldFormat should_format, [[maybe_unused]]int32_t* indent_level) -> StringType
    {
        return std::format(STR("\"{}\""), m_data);
    }
}
