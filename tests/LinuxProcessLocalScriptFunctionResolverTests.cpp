#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <Unreal/UnrealInitializer.hpp>

namespace
{
    auto write_relative_branch(std::array<uint8_t, 64>& code, size_t instruction_offset, uint8_t opcode, size_t target_offset)
            -> void
    {
        code[instruction_offset] = opcode;
        const auto displacement = static_cast<int32_t>(target_offset - (instruction_offset + 5));
        std::memcpy(code.data() + instruction_offset + 1, &displacement, sizeof(displacement));
    }
} // namespace

int main()
{
    asm volatile("" : : "r"(&RC::Unreal::UnrealInitializer::StaticStorage::bVersionedContainerIsInitialized) : "memory");

    std::array<uint8_t, 64> process_internal{};
    process_internal.fill(0x90);
    process_internal[0] = 0xFF;
    process_internal[1] = 0xD0;
    process_internal[2] = 0xFF;
    process_internal[3] = 0xD0;
    write_relative_branch(process_internal, 4, 0xE8, 32);
    process_internal[9] = 0x5B;
    write_relative_branch(process_internal, 10, 0xE9, 48);
    process_internal[15] = 0xC3;
    process_internal[32] = 0xC3;
    process_internal[48] = 0x55;
    process_internal[49] = 0xC3;

    const auto resolved = RC::Unreal::UnrealInitializer::ResolveProcessLocalScriptFunctionAddress(
            process_internal.data(), process_internal.size());
    return resolved == process_internal.data() + 48 ? 0 : 2;
}
