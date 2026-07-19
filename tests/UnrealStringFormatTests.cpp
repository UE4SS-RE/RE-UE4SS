#include <string_view>

#include <Unreal/Core/Misc/CString.hpp>

int main()
{
    RC::Unreal::TCHAR formatted[128]{};
    const auto length = RC::Unreal::FCString::Snprintf(
            formatted, std::size(formatted), STR("Test|%-8s|%08x|%d|%.2f|"), STR("left"), 0x2a, -7, 3.5);
    constexpr std::basic_string_view<RC::Unreal::TCHAR> expected{STR("Test|left    |0000002a|-7|3.50|")};
    return length == static_cast<RC::Unreal::int32>(expected.size()) && std::basic_string_view<RC::Unreal::TCHAR>{formatted} == expected ? 0 : 1;
}
