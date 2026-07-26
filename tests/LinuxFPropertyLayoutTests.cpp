#include <array>
#include <cstddef>
#include <cstring>

#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/UnrealVersion.hpp>

int main()
{
    RC::Unreal::Version::Major = 5;
    RC::Unreal::Version::Minor = 1;

    RC::Unreal::FProperty::MemberOffsets.clear();
    RC::Unreal::FProperty::MemberOffsets.emplace(STR("ArrayDim"), 0x38);
    RC::Unreal::FProperty::MemberOffsets.emplace(STR("ElementSize"), 0x3C);

    alignas(void*) std::array<std::byte, 0x80> property_storage{};
    const int32_t array_dim = 1;
    const int32_t element_size = 0x18;
    std::memcpy(property_storage.data() + 0x34, &array_dim, sizeof(array_dim));
    std::memcpy(property_storage.data() + 0x38, &element_size, sizeof(element_size));

    const auto* property = reinterpret_cast<const RC::Unreal::FProperty*>(property_storage.data());
    return property->GetSize() == element_size ? 0 : 1;
}
