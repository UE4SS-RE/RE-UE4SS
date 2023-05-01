#ifndef RC_ASM_HELPER_HPP
#define RC_ASM_HELPER_HPP

#include <ASMHelper/Common.hpp>
#include <Zydis/Zydis.h>

namespace RC::ASM
{
    struct Instruction
    {
        void* address{};
        ZydisDecodedInstruction raw{};
    };

    RC_ASM_API auto is_jmp_instruction(void* instruction_ptr) -> bool;
    RC_ASM_API auto is_call_instruction(void* instruction_ptr) -> bool;

    RC_ASM_API auto resolve_jmp(void* instruction_ptr) -> void*;
    RC_ASM_API auto resolve_call(void* instruction_ptr) -> void*;

    RC_ASM_API auto resolve_function_address_from_potential_jmp(void* function_ptr) -> void*;
}

#endif // RC_ASM_HELPER_HPP
