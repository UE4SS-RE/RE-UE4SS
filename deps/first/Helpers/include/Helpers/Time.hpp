#pragma once

#include <String/StringType.hpp>

namespace RC
{
    auto get_now_as_string(StringViewType format = STR("{:%Y_%m_%d_%H_%M_%S}")) -> StringType;
} // namespace RC