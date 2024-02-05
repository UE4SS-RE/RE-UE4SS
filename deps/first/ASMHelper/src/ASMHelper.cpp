#include <bit>

#include <ASMHelper/ASMHelper.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Zydis/Zydis.h>

#include <array>

namespace RC::ASM
{
    auto get_first_instruction_at_address(void* in_instruction_ptr) -> std::pair<Instruction, std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT>>
    {
        auto instruction_ptr = static_cast<uint8_t*>(in_instruction_ptr);
        ZydisDecoder decoder{};
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
        ZyanUSize offset = 0;
        ZydisDecodedInstruction instruction{};
        ZydisDecodedOperand operands[10]{};
        while (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, instruction_ptr + offset, 16 - offset, &instruction, operands)))
        {
            break;
        }
        return {in_instruction_ptr, instruction, operands};
    }

    auto resolve_absolute_address(void* in_instruction_ptr) -> void*
    {
        auto [instruction, operands] = get_first_instruction_at_address(in_instruction_ptr);
        ZyanU64 resolved_address{};
        if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&instruction.raw, &operands[0], std::bit_cast<ZyanU64>(in_instruction_ptr), &resolved_address)))
        {
            return std::bit_cast<void*>(resolved_address);
        }
        else
        {
            return nullptr;
        }
    }

    auto resolve_jmp(void* in_instruction_ptr) -> void*
    {
        return resolve_absolute_address(in_instruction_ptr);
    }

    auto resolve_call(void* in_instruction_ptr) -> void*
    {
        return resolve_absolute_address(in_instruction_ptr);
    }

    auto resolve_function_address_from_potential_jmp(void* function_ptr) -> void*
    {
        auto [instruction, _] = get_first_instruction_at_address(function_ptr);
        if (instruction.raw.mnemonic == ZYDIS_MNEMONIC_JMP || instruction.raw.mnemonic == ZYDIS_MNEMONIC_CALL)
        {
            if (auto resolved_address = resolve_jmp(instruction.address); resolved_address)
            {
                return resolve_function_address_from_potential_jmp(resolved_address);
            }
            else
            {
                Output::send<LogLevel::Warning>(SYSSTR("Was unable to resolve JMP instruction @ {}\n"), instruction.address);
                return nullptr;
            }
        }
        else
        {
            return function_ptr;
        }
    }
} // namespace RC::ASM
