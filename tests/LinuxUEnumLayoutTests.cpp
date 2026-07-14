#include <array>
#include <utility>

#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/UnrealVersion.hpp>

int main()
{
    using RC::Unreal::UEnum;
    using RC::Unreal::Version;

    Version::Major = 5;
    Version::Minor = 1;

    UEnum::MemberOffsets.clear();
    UEnum::MemberOffsets.emplace(STR("CppType"), 0x30);
    UEnum::MemberOffsets.emplace(STR("Names"), 0x40);
    UEnum::MemberOffsets.emplace(STR("CppForm"), 0x50);
    UEnum::MemberOffsets.emplace(STR("EnumFlags_Internal"), 0x54);
    UEnum::MemberOffsets.emplace(STR("EnumDisplayNameFn"), 0x58);
    UEnum::MemberOffsets.emplace(STR("EnumPackage"), 0x60);
    UEnum::MemberOffsets.emplace(STR("UEP_TotalSize"), 0x68);

    RC::Unreal::UnrealInitializer::ApplyPlatformMemberOffsets();

    constexpr std::array expected_offsets{
            std::pair{STR("CppType"), 0x30},
            std::pair{STR("CppForm"), 0x40},
            std::pair{STR("Names"), 0x48},
            std::pair{STR("EnumFlags_Internal"), 0x58},
            std::pair{STR("EnumDisplayNameFn"), 0x60},
            std::pair{STR("EnumPackage"), 0x68},
            std::pair{STR("UEP_TotalSize"), 0x70},
    };

    for (const auto& [name, expected_offset] : expected_offsets)
    {
        const auto offset = UEnum::MemberOffsets.find(name);
        if (offset == UEnum::MemberOffsets.end() || offset->second != expected_offset)
        {
            return 1;
        }
    }

    return 0;
}
