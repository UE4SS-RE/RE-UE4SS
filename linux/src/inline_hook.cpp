#include "ue4ss/inline_hook.hpp"

#include <Zydis/Zydis.h>

#include <array>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

namespace
{
    constexpr std::size_t k_absolute_jump_size = 14u;
    constexpr std::size_t k_trampoline_page_count = 1u;

    struct Protection
    {
        int flags{};
        std::uintptr_t start{};
        std::uintptr_t end{};
    };

    struct DecodedInstruction
    {
        ZydisDecodedInstruction instruction{};
        std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
        std::uintptr_t original_address{};
        std::size_t original_offset{};
    };

    [[nodiscard]] std::size_t page_size()
    {
        static const auto value = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
        return value;
    }

    [[nodiscard]] std::uintptr_t page_floor(std::uintptr_t address)
    {
        return address & ~(static_cast<std::uintptr_t>(page_size()) - 1u);
    }

    [[nodiscard]] std::uintptr_t page_ceil(std::uintptr_t address)
    {
        return (address + page_size() - 1u) & ~(static_cast<std::uintptr_t>(page_size()) - 1u);
    }

    [[nodiscard]] Protection query_protection(std::uintptr_t address, std::size_t length)
    {
        std::ifstream maps{"/proc/self/maps"};
        std::string line;
        while (std::getline(maps, line))
        {
            std::uintptr_t start{};
            std::uintptr_t end{};
            char permissions[5]{};
            if (std::sscanf(line.c_str(), "%lx-%lx %4s", &start, &end, permissions) != 3)
            {
                continue;
            }
            if (address < start || address > std::numeric_limits<std::uintptr_t>::max() - length || address + length > end)
            {
                continue;
            }
            int flags = 0;
            flags |= permissions[0] == 'r' ? PROT_READ : 0;
            flags |= permissions[1] == 'w' ? PROT_WRITE : 0;
            flags |= permissions[2] == 'x' ? PROT_EXEC : 0;
            return {.flags = flags, .start = start, .end = end};
        }
        throw std::runtime_error("target memory range is not present in /proc/self/maps");
    }

    void write_memory_transaction(void* address, const void* replacement, const void* rollback, std::size_t length, bool executable_code)
    {
        const auto numeric_address = reinterpret_cast<std::uintptr_t>(address);
        const Protection original = query_protection(numeric_address, length);
        const std::uintptr_t start = page_floor(numeric_address);
        const std::size_t page_length = page_ceil(numeric_address + length) - start;
        // Retain an existing executable permission while patching. Removing
        // PROT_EXEC from the entire page can fault an unrelated thread which
        // is currently executing a different function that happens to share
        // the same 4 KiB text page. The RWX window is restricted to this
        // transaction and the exact original protection is restored below.
        const int writable_flags = original.flags | PROT_READ | PROT_WRITE;
        if (mprotect(reinterpret_cast<void*>(start), page_length, writable_flags) != 0)
        {
            throw std::runtime_error(std::string{"mprotect(write): "} + std::strerror(errno));
        }

        std::memcpy(address, replacement, length);
        if (executable_code)
        {
            __builtin___clear_cache(static_cast<char*>(address), static_cast<char*>(address) + length);
        }
        if (mprotect(reinterpret_cast<void*>(start), page_length, original.flags) == 0)
        {
            return;
        }

        const int restore_error = errno;
        std::memcpy(address, rollback, length);
        if (executable_code)
        {
            __builtin___clear_cache(static_cast<char*>(address), static_cast<char*>(address) + length);
        }
        const int rollback_protection_error = mprotect(reinterpret_cast<void*>(start), page_length, original.flags);
        std::string message = std::string{"mprotect(restore): "} + std::strerror(restore_error) + "; replacement bytes rolled back";
        if (rollback_protection_error != 0)
        {
            message += std::string{" but page protection rollback also failed: "} + std::strerror(errno);
        }
        throw std::runtime_error(message);
    }

    void write_absolute_jump(std::uint8_t* destination, const void* target)
    {
        constexpr std::array<std::uint8_t, 6> prefix{0xffu, 0x25u, 0x00u, 0x00u, 0x00u, 0x00u};
        std::memcpy(destination, prefix.data(), prefix.size());
        const std::uintptr_t numeric_target = reinterpret_cast<std::uintptr_t>(target);
        std::memcpy(destination + prefix.size(), &numeric_target, sizeof(numeric_target));
    }

    [[nodiscard]] bool within_relative_range(std::uintptr_t left, std::uintptr_t right)
    {
        const std::uint64_t distance = left > right ? left - right : right - left;
        return distance <= static_cast<std::uint64_t>(INT32_MAX);
    }

    [[nodiscard]] void* allocate_near(std::uintptr_t target, std::size_t length)
    {
        const std::size_t page_length = page_ceil(length);
#ifdef MAP_FIXED_NOREPLACE
        constexpr std::uintptr_t stride = 64u * 1024u;
        const std::uintptr_t aligned_target = target & ~(stride - 1u);
        constexpr std::uintptr_t maximum_distance = static_cast<std::uintptr_t>(INT32_MAX) - 16u * 1024u * 1024u;
        for (std::uintptr_t distance = stride; distance < maximum_distance; distance += stride)
        {
            const std::array<std::uintptr_t, 2> candidates{
                    aligned_target > distance ? aligned_target - distance : 0u,
                    aligned_target < std::numeric_limits<std::uintptr_t>::max() - distance ? aligned_target + distance : 0u,
            };
            for (const std::uintptr_t candidate : candidates)
            {
                if (candidate < page_size())
                {
                    continue;
                }
                void* mapping =
                        mmap(reinterpret_cast<void*>(candidate), page_length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
                if (mapping != MAP_FAILED)
                {
                    return mapping;
                }
                if (errno != EEXIST && errno != EINVAL && errno != ENOMEM)
                {
                    break;
                }
            }
        }
#endif
        void* mapping = mmap(reinterpret_cast<void*>(page_floor(target)), page_length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mapping == MAP_FAILED)
        {
            return nullptr;
        }
        if (!within_relative_range(target, reinterpret_cast<std::uintptr_t>(mapping)))
        {
            munmap(mapping, page_length);
            return nullptr;
        }
        return mapping;
    }

    [[nodiscard]] std::vector<DecodedInstruction> decode_prologue(void* target, std::size_t& consumed)
    {
        ZydisDecoder decoder{};
        if (ZYAN_FAILED(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            throw std::runtime_error("cannot initialize Zydis decoder");
        }

        std::vector<DecodedInstruction> instructions;
        consumed = 0;
        const auto base = reinterpret_cast<std::uintptr_t>(target);
        while (consumed < k_absolute_jump_size)
        {
            DecodedInstruction decoded;
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(target) + consumed;
            const ZyanStatus status = ZydisDecoderDecodeFull(&decoder, bytes, ZYDIS_MAX_INSTRUCTION_LENGTH, &decoded.instruction, decoded.operands.data());
            if (ZYAN_FAILED(status) || decoded.instruction.length == 0u)
            {
                throw std::runtime_error("cannot decode enough complete instructions for an inline hook");
            }
            decoded.original_address = base + consumed;
            decoded.original_offset = consumed;
            consumed += decoded.instruction.length;
            if (consumed > 128u)
            {
                throw std::runtime_error("inline hook prologue exceeds the safety limit");
            }
            instructions.emplace_back(std::move(decoded));
        }
        return instructions;
    }

    void make_request_absolute(DecodedInstruction& decoded, ZydisEncoderRequest& request, std::uintptr_t block_start, std::size_t block_size)
    {
        if (ZYAN_FAILED(ZydisEncoderDecodedInstructionToEncoderRequest(&decoded.instruction, decoded.operands.data(), decoded.instruction.operand_count_visible, &request)))
        {
            throw std::runtime_error("Zydis cannot convert a decoded instruction for relocation");
        }

        for (ZyanU8 index = 0; index < decoded.instruction.operand_count_visible; ++index)
        {
            const auto& operand = decoded.operands[index];
            const bool relative_immediate = operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE && operand.imm.is_relative;
            const bool relative_memory =
                    operand.type == ZYDIS_OPERAND_TYPE_MEMORY && (operand.mem.base == ZYDIS_REGISTER_RIP || operand.mem.base == ZYDIS_REGISTER_EIP);
            if (!relative_immediate && !relative_memory)
            {
                continue;
            }
            ZyanU64 absolute{};
            if (ZYAN_FAILED(ZydisCalcAbsoluteAddress(&decoded.instruction, &operand, decoded.original_address, &absolute)))
            {
                throw std::runtime_error("Zydis cannot resolve a relative operand during relocation");
            }
            if (relative_immediate && absolute >= block_start && absolute < block_start + block_size)
            {
                throw std::runtime_error("relative branch into the overwritten prologue is not safe to relocate");
            }
            if (relative_immediate)
            {
                request.operands[index].imm.u = absolute;
                // A short branch in the original prologue will almost never
                // reach its original target from a separately mapped
                // trampoline. Promote encodings which have an x86 rel32 form;
                // LOOP*/JCXZ remain fail-closed because they cannot be widened.
                const bool short_only = decoded.instruction.mnemonic == ZYDIS_MNEMONIC_JCXZ ||
                                        decoded.instruction.mnemonic == ZYDIS_MNEMONIC_JECXZ ||
                                        decoded.instruction.mnemonic == ZYDIS_MNEMONIC_JRCXZ ||
                                        decoded.instruction.mnemonic == ZYDIS_MNEMONIC_LOOP ||
                                        decoded.instruction.mnemonic == ZYDIS_MNEMONIC_LOOPE ||
                                        decoded.instruction.mnemonic == ZYDIS_MNEMONIC_LOOPNE;
                if (request.branch_type == ZYDIS_BRANCH_TYPE_SHORT && !short_only)
                {
                    request.branch_type = ZYDIS_BRANCH_TYPE_NEAR;
                    request.branch_width = ZYDIS_BRANCH_WIDTH_32;
                }
            }
            else
            {
                request.operands[index].mem.displacement = static_cast<ZyanI64>(absolute);
            }
        }
    }

    [[nodiscard]] std::size_t build_trampoline(void* trampoline, const std::vector<DecodedInstruction>& instructions, std::uintptr_t target, std::size_t overwritten_size)
    {
        auto* output = static_cast<std::uint8_t*>(trampoline);
        std::size_t cursor = 0;
        for (auto decoded : instructions)
        {
            ZydisEncoderRequest request{};
            make_request_absolute(decoded, request, target, overwritten_size);
            ZyanUSize encoded_length = ZYDIS_MAX_INSTRUCTION_LENGTH;
            if (cursor + encoded_length + k_absolute_jump_size > page_size() * k_trampoline_page_count)
            {
                throw std::runtime_error("relocated trampoline exceeds its executable allocation");
            }
            if (ZYAN_FAILED(ZydisEncoderEncodeInstructionAbsolute(&request, output + cursor, &encoded_length, reinterpret_cast<std::uintptr_t>(output + cursor))))
            {
                std::array<char, 256> formatted{};
                ZydisFormatter formatter{};
                if (ZYAN_SUCCESS(ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL)))
                {
                    static_cast<void>(ZydisFormatterFormatInstruction(
                            &formatter,
                            &decoded.instruction,
                            decoded.operands.data(),
                            decoded.instruction.operand_count_visible,
                            formatted.data(),
                            formatted.size(),
                            decoded.original_address,
                            nullptr));
                }
                std::ostringstream message;
                message << "Zydis cannot encode relocated "
                        << (formatted[0] != '\0' ? formatted.data()
                                                : ZydisMnemonicGetString(decoded.instruction.mnemonic))
                        << " at 0x" << std::hex << decoded.original_address << " (bytes";
                const auto* original = reinterpret_cast<const std::uint8_t*>(decoded.original_address);
                for (std::size_t index = 0; index < decoded.instruction.length; ++index)
                {
                    message << ' ' << std::setw(2) << std::setfill('0')
                            << static_cast<unsigned int>(original[index]);
                }
                message << ')';
                throw std::runtime_error(message.str());
            }
            cursor += encoded_length;
        }
        write_absolute_jump(output + cursor, reinterpret_cast<void*>(target + overwritten_size));
        cursor += k_absolute_jump_size;
        return cursor;
    }
} // namespace

namespace ue4ss::linux
{
    InlineHook::~InlineHook()
    {
        std::string ignored;
        static_cast<void>(remove(ignored));
    }

    bool InlineHook::install(void* target, void* replacement, std::string& error)
    {
        return install(target, replacement, {}, error);
    }

    bool InlineHook::install(void* target,
                             void* replacement,
                             const std::function<void(void*)>& before_commit,
                             std::string& error)
    {
        std::lock_guard lock{m_mutex};
        if (m_installed)
        {
            if (target == m_target && replacement == m_replacement)
            {
                return true;
            }
            error = "hook is already installed for a different target";
            return false;
        }
        if (m_trampoline != nullptr)
        {
            error = "restored hook still owns its trampoline; release it before reinstalling";
            return false;
        }
        if (target == nullptr || replacement == nullptr)
        {
            error = "hook target and replacement must be non-null";
            return false;
        }

        try
        {
            const Protection protection = query_protection(reinterpret_cast<std::uintptr_t>(target), k_absolute_jump_size);
            if ((protection.flags & PROT_EXEC) == 0)
            {
                throw std::runtime_error("hook target is not executable memory");
            }

            std::size_t overwritten_size{};
            const auto instructions = decode_prologue(target, overwritten_size);
            void* trampoline = allocate_near(reinterpret_cast<std::uintptr_t>(target), page_size() * k_trampoline_page_count);
            if (trampoline == nullptr)
            {
                throw std::runtime_error("cannot allocate an x86_64 trampoline within relative branch range");
            }

            try
            {
                const std::size_t used = build_trampoline(trampoline, instructions, reinterpret_cast<std::uintptr_t>(target), overwritten_size);
                __builtin___clear_cache(static_cast<char*>(trampoline), static_cast<char*>(trampoline) + used);
                if (mprotect(trampoline, page_size() * k_trampoline_page_count, PROT_READ | PROT_EXEC) != 0)
                {
                    throw std::runtime_error(std::string{"mprotect(trampoline): "} + std::strerror(errno));
                }

                m_original_bytes.assign(static_cast<const std::uint8_t*>(target), static_cast<const std::uint8_t*>(target) + overwritten_size);
                std::vector<std::uint8_t> patch(overwritten_size);
                if (ZYAN_FAILED(ZydisEncoderNopFill(patch.data(), patch.size())))
                {
                    throw std::runtime_error("Zydis cannot construct the inline-hook patch");
                }
                write_absolute_jump(patch.data(), replacement);
                if (before_commit)
                {
                    // Publish the already executable trampoline immediately
                    // before the target becomes reachable through the
                    // replacement. This closes the detour/original race for
                    // hooks installed in a live multithreaded process.
                    before_commit(trampoline);
                }
                write_memory_transaction(target, patch.data(), m_original_bytes.data(), overwritten_size, true);
                m_patch_bytes = std::move(patch);
            }
            catch (...)
            {
                munmap(trampoline, page_size() * k_trampoline_page_count);
                throw;
            }

            m_target = target;
            m_replacement = replacement;
            m_trampoline = trampoline;
            m_trampoline_size = page_size() * k_trampoline_page_count;
            m_installed = true;
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            m_original_bytes.clear();
            m_patch_bytes.clear();
            return false;
        }
    }

    bool InlineHook::restore_locked(std::string& error)
    {
        if (!m_installed)
        {
            return true;
        }
        try
        {
            if (m_patch_bytes.size() != m_original_bytes.size() || std::memcmp(m_target, m_patch_bytes.data(), m_patch_bytes.size()) != 0)
            {
                throw std::runtime_error("hook target bytes changed after installation; refusing to overwrite them");
            }
            write_memory_transaction(m_target, m_original_bytes.data(), m_patch_bytes.data(), m_original_bytes.size(), true);
            m_installed = false;
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
    }

    bool InlineHook::release_locked(std::string& error)
    {
        if (m_installed)
        {
            error = "inline hook must be restored before its trampoline is released";
            return false;
        }
        if (m_trampoline == nullptr)
        {
            return true;
        }
        if (munmap(m_trampoline, m_trampoline_size) != 0)
        {
            error = std::string{"munmap(trampoline): "} + std::strerror(errno);
            return false;
        }
        m_target = nullptr;
        m_replacement = nullptr;
        m_trampoline = nullptr;
        m_trampoline_size = 0;
        m_original_bytes.clear();
        m_patch_bytes.clear();
        return true;
    }

    bool InlineHook::restore(std::string& error)
    {
        std::lock_guard lock{m_mutex};
        error.clear();
        return restore_locked(error);
    }

    bool InlineHook::release(std::string& error)
    {
        std::lock_guard lock{m_mutex};
        error.clear();
        return release_locked(error);
    }

    bool InlineHook::remove(std::string& error)
    {
        std::lock_guard lock{m_mutex};
        error.clear();
        return restore_locked(error) && release_locked(error);
    }

    bool InlineHook::is_installed() const
    {
        std::lock_guard lock{m_mutex};
        return m_installed;
    }

    void* InlineHook::target() const
    {
        std::lock_guard lock{m_mutex};
        return m_target;
    }

    void* InlineHook::trampoline() const
    {
        std::lock_guard lock{m_mutex};
        return m_trampoline;
    }

    PointerHook::~PointerHook()
    {
        std::string ignored;
        static_cast<void>(remove(ignored));
    }

    bool PointerHook::install(void** slot, void* replacement, std::string& error)
    {
        std::lock_guard lock{m_mutex};
        if (m_installed)
        {
            if (slot == m_slot && replacement == m_replacement)
            {
                return true;
            }
            error = "pointer hook is already installed for a different slot";
            return false;
        }
        if (slot == nullptr || replacement == nullptr)
        {
            error = "pointer slot and replacement must be non-null";
            return false;
        }
        if (reinterpret_cast<std::uintptr_t>(slot) % alignof(void*) != 0u)
        {
            error = "pointer slot is not naturally aligned";
            return false;
        }

        try
        {
            void* original{};
            std::memcpy(&original, slot, sizeof(original));
            write_memory_transaction(slot, &replacement, &original, sizeof(replacement), false);
            m_slot = slot;
            m_original = original;
            m_replacement = replacement;
            m_installed = true;
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
    }

    bool PointerHook::remove(std::string& error)
    {
        std::lock_guard lock{m_mutex};
        if (!m_installed)
        {
            return true;
        }
        try
        {
            void* current{};
            std::memcpy(&current, m_slot, sizeof(current));
            if (current != m_replacement)
            {
                throw std::runtime_error("pointer slot changed after installation; refusing to overwrite it");
            }
            write_memory_transaction(m_slot, &m_original, &m_replacement, sizeof(m_original), false);
            m_slot = nullptr;
            m_original = nullptr;
            m_replacement = nullptr;
            m_installed = false;
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
    }

    bool PointerHook::is_installed() const
    {
        std::lock_guard lock{m_mutex};
        return m_installed;
    }

    void* PointerHook::trampoline() const
    {
        std::lock_guard lock{m_mutex};
        return m_original;
    }
} // namespace ue4ss::linux
