#include <array>
#include <cstddef>
#include <cstring>

#include <Unreal/Hooks/Hooks.hpp>
#include <Unreal/NameTypes.hpp>

namespace
{
    std::array<std::byte, sizeof(RC::Unreal::FName)> s_expected_bytes{};

    extern "C" __attribute__((noinline)) void fname_constructor_fixture(
            RC::Unreal::FName* output,
            const RC::CharType* string,
            RC::Unreal::EFindName find_type)
    {
        asm volatile("nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop");

        if (!output || !string || find_type != RC::Unreal::FNAME_Find)
        {
            return;
        }
        std::memcpy(output, s_expected_bytes.data(), s_expected_bytes.size());
    }
} // namespace

int main()
{
    for (size_t index = 0; index < s_expected_bytes.size(); ++index)
    {
        s_expected_bytes[index] = static_cast<std::byte>(0x40 + index);
    }

    RC::Unreal::FName::ConstructorInternal.assign_address(
            reinterpret_cast<void*>(&fname_constructor_fixture));

    bool callback_called = false;
    const auto callback_id = RC::Unreal::Hook::RegisterFNameConstructorPostCallback(
            [&](auto&, auto&&...) { callback_called = true; },
            {false, false, STR("LinuxFNameHookTests"), STR("ConstructorABI")});
    if (callback_id == RC::Unreal::Hook::ERROR_ID)
    {
        return 1;
    }

    const RC::CharType test_name[] = STR("LinuxFNameConstructorABI");
    const RC::Unreal::FName name{test_name, RC::Unreal::FNAME_Find};

    if (!callback_called)
    {
        return 2;
    }
    return std::memcmp(&name, s_expected_bytes.data(), s_expected_bytes.size()) == 0 ? 0 : 3;
}
