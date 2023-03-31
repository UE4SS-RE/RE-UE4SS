#ifndef RC_ASM_HPP
#define RC_ASM_HPP

#include <array>
#include <cstdint>

namespace RC::Helper::ASM
{
    enum Instruction : int32_t
    {
        INSTR_LEA,
        INSTR_MOV,
        INSTR_CALL,
        INSTR_DIRECT_ADDR_CALL,
        INSTR_JMP,
        INSTR_INDIRECT_OFFSET_JMP,

        MAX_INSTRUCTIONS,
    };

    constexpr inline static int32_t rex_prefix = 0x48;

    struct OpCodeInfo
    {
        // Whether is_instr has successfully identified this opcode
        bool is_verified{};
        bool has_rex_prefix{};
    };

    inline static std::array<std::array<int32_t, 5>, MAX_INSTRUCTIONS> asm_instructions{{
            // LEA
            {0x8D},

            // MOV
            {0x8B},

            // CALL
            {0xE8},

            // INSTR_DIRECT_ADDR_CALL
            {0xFF},

            // JMP
            {0xE9, 0xEB},

            // INSTR_INDIRECT_OFFSET_JMP
            {0xFF}
    }};

    inline auto is_instr(const uint8_t* instr_ptr, Instruction instr) -> OpCodeInfo
    {
        OpCodeInfo opi{};

        if (!instr_ptr) { return opi; }
        if (instr > asm_instructions.size()) { return opi; }

        for (int i : asm_instructions[instr])
        {
            if (*instr_ptr == i || (*instr_ptr == rex_prefix && *(instr_ptr + 1) == i))
            {
                opi.is_verified = true;
                if (*instr_ptr == rex_prefix) { opi.has_rex_prefix = true; }
                return opi;
            }
        }

        return opi;
    }


    // Returns a pointer to the destination of a call instruction
    inline auto follow_call(uint8_t* instr_ptr) -> uint8_t*
    {
        if (auto opi = is_instr(instr_ptr, INSTR_CALL); opi.is_verified)
        {
            constexpr int8_t instr_size = 0x5;
            if (opi.has_rex_prefix) { ++instr_ptr; }

            uint8_t* next_instr = instr_ptr + instr_size;
            int32_t offset = *static_cast<int32_t*>(static_cast<void*>(instr_ptr + 0x1));
            return next_instr + offset;
        }

        if (auto opi = is_instr(instr_ptr, INSTR_DIRECT_ADDR_CALL); opi.is_verified)
        {
            constexpr int8_t instr_size = 0x6;
            if (opi.has_rex_prefix) { ++instr_ptr; }

            uint8_t* next_instr = instr_ptr + instr_size;
            int32_t offset = *static_cast<int32_t*>(static_cast<void*>(instr_ptr + 0x2));
            return *static_cast<uint8_t**>(static_cast<void*>(next_instr + offset));
        }
        else
        {
            return nullptr;
        }
    }

    inline auto follow_jmp(uint8_t* instr_ptr) -> uint8_t*
    {
        if (auto opi = is_instr(instr_ptr, INSTR_JMP); opi.is_verified)
        {
            constexpr int8_t instr_size = 0x5;
            if (opi.has_rex_prefix) { ++instr_ptr; }

            int32_t offset = *static_cast<int32_t*>(static_cast<void*>(instr_ptr + 0x1));
            return instr_ptr + offset + instr_size;
        }

        if (auto opi = is_instr(instr_ptr, INSTR_INDIRECT_OFFSET_JMP); opi.is_verified)
        {
            // Example w/ REX:
            // 48 FF 25 41 BF 23 00

            constexpr int8_t instr_size = 0x6;
            if (opi.has_rex_prefix) { ++instr_ptr; }

            int32_t offset = *static_cast<int32_t*>(static_cast<void*>(instr_ptr + 0x2));
            return *static_cast<uint8_t**>(static_cast<void*>(instr_ptr + offset + instr_size));
        }
        else
        {
            return nullptr;
        }
    }

    template<typename LoadedType>
    inline auto resolve_lea(uint8_t* instr_ptr) -> LoadedType
    {
        if (auto opi = is_instr(instr_ptr, INSTR_LEA); opi.is_verified)
        {
            constexpr int8_t instr_size = 0x6;
            if (opi.has_rex_prefix) { ++instr_ptr; }

            uint8_t* next_instr = instr_ptr + instr_size;
            int32_t offset = *static_cast<int32_t*>(static_cast<void*>(instr_ptr + 0x2));

            return static_cast<LoadedType>(static_cast<void*>(next_instr + offset));
        }
        else
        {
            return nullptr;
        }
    }

    template<typename LoadedType>
    inline auto resolve_mov(uint8_t* instr_ptr) -> LoadedType
    {
        if (auto opi = is_instr(instr_ptr, INSTR_MOV); opi.is_verified)
        {
            // Size without the REX prefix
            constexpr int8_t instr_size = 0x6;
            if (opi.has_rex_prefix) { ++instr_ptr; }

            uint8_t* next_instr = instr_ptr + instr_size;
            int32_t offset = *static_cast<int32_t*>(static_cast<void*>(instr_ptr + 0x2));

            return static_cast<LoadedType>(static_cast<void*>(next_instr + offset));
        }
        else
        {
            return nullptr;
        }
    }
}

#endif //RC_ASM_HPP
