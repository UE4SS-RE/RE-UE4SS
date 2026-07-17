#pragma once

#include <string>
#include <string_view>

namespace ue4ss::linux
{
    [[nodiscard]] std::u16string utf8_to_unreal(std::string_view input);
    [[nodiscard]] std::string unreal_to_utf8(std::u16string_view input);
} // namespace ue4ss::linux
