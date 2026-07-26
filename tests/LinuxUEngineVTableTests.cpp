#include <cstdint>
#include <unordered_map>

#include <File/File.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtualBaseVC.hpp>

int main()
{
    std::unordered_map<RC::File::StringType, uint32_t> layout{
            {STR("__vecDelDtor"), 0x0},
            {STR("Browse"), 0x4B0},
            {STR("TickWorldTravel"), 0x4B8},
            {STR("LoadMap"), 0x4C0},
            {STR("RedrawViewports"), 0x4C8},
    };

    const auto adjusted_offset = [&](const RC::File::StringType& function_name) {
        return RC::Unreal::adjust_unreal_virtual_function_offset<RC::Unreal::UEngine>(layout, layout.find(function_name));
    };

    if (adjusted_offset(STR("Browse")) != 0x4B8)
    {
        return 1;
    }
    if (adjusted_offset(STR("LoadMap")) != 0x4C0)
    {
        return 2;
    }
    if (adjusted_offset(STR("TickWorldTravel")) != 0x4C8)
    {
        return 3;
    }
    return adjusted_offset(STR("RedrawViewports")) == 0x4D0 ? 0 : 4;
}
