#pragma once

#include <string>
#include <cstdint>

#include <String/StringType.hpp>

namespace RC::GUI::Filter
{
    class MaxValueSize
    {
      public:
        static inline StringType s_debug_name{STR("MaxValueSize")};
        static inline int32_t s_value{500};
        static inline std::string s_value_buffer{fmt::format("{}", s_value)};
    };
} // namespace RC::GUI::Filter
