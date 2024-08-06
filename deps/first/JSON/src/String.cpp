#include <JSON/String.hpp>

#include <fmt/core.h>
#include <fmt/xchar.h>

namespace RC::JSON
{
    String::String(StringViewType string) : m_data(string)
    {
    }

    auto String::serialize([[maybe_unused]] ShouldFormat should_format, [[maybe_unused]] int32_t* indent_level) -> StringType
    {
        return fmt::format(STR("\"{}\""), m_data);
    }
} // namespace RC::JSON
