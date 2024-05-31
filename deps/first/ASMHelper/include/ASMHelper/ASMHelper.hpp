#pragma once

#include <ASMHelper/Common.hpp>
#include <Zydis/Zydis.h>

namespace RC::ASM
{
    struct Instruction
    {
        void* address{};
        ZydisDecodedInstruction raw{};
        ZydisDecodedOperand* operands{};
    };

    RC_ASM_API auto resolve_jmp(void* instruction_ptr) -> void*;
    RC_ASM_API auto resolve_call(void* instruction_ptr) -> void*;

    RC_ASM_API auto resolve_function_address_from_potential_jmp(void* function_ptr) -> void*;
} // namespace RC::ASM
