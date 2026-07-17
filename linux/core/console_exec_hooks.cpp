#include "console_exec_hooks.hpp"

#include "ue4ss/utf.hpp"

#include <Zydis/Zydis.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstdio>
#include <exception>
#include <fstream>
#include <limits>
#include <sstream>
#include <sys/uio.h>
#include <unistd.h>

namespace
{
    constexpr std::uint64_t k_ue51_linux_process_event_vtable_offset = 0x268u;
    constexpr std::uint64_t k_ue51_linux_process_console_exec_vtable_offset = 0x280u;
    constexpr std::uint64_t k_ue51_linux_output_device_serialize_vtable_offset = 0x10u;

    template <typename T>
    [[nodiscard]] bool safe_read(std::uint64_t address, T& value) noexcept
    {
        if (address == 0u || address > std::numeric_limits<std::uintptr_t>::max() - sizeof(T))
        {
            return false;
        }
        iovec local{&value, sizeof(value)};
        iovec remote{reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), sizeof(value)};
        return process_vm_readv(getpid(), &local, 1, &remote, 1, 0) ==
               static_cast<ssize_t>(sizeof(value));
    }

    [[nodiscard]] bool executable_address(std::uint64_t address) noexcept
    {
        std::ifstream maps{"/proc/self/maps"};
        std::string line;
        while (std::getline(maps, line))
        {
            unsigned long long start{};
            unsigned long long end{};
            std::array<char, 5> permissions{};
            if (std::sscanf(line.c_str(), "%llx-%llx %4s", &start, &end, permissions.data()) == 3 &&
                address >= start && address < end)
            {
                return permissions[0] == 'r' && permissions[2] == 'x';
            }
        }
        return false;
    }

    [[nodiscard]] std::vector<std::string> split_command(std::string_view command)
    {
        std::vector<std::string> parts;
        std::string current;
        bool quoted{};
        char quote{};
        bool escaped{};
        for (const char byte : command)
        {
            if (escaped)
            {
                current.push_back(byte);
                escaped = false;
                continue;
            }
            if (byte == '\\' && quoted)
            {
                escaped = true;
                continue;
            }
            if ((byte == '"' || byte == '\'') && (!quoted || quote == byte))
            {
                if (quoted)
                {
                    quoted = false;
                    quote = 0;
                }
                else
                {
                    quoted = true;
                    quote = byte;
                }
                continue;
            }
            if (!quoted && std::isspace(static_cast<unsigned char>(byte)) != 0)
            {
                if (!current.empty())
                {
                    parts.push_back(std::move(current));
                    current.clear();
                }
                continue;
            }
            current.push_back(byte);
        }
        if (escaped) current.push_back('\\');
        if (!current.empty()) parts.push_back(std::move(current));
        return parts;
    }

    [[nodiscard]] std::vector<std::uint64_t> direct_call_targets(
            std::uint64_t function_address)
    {
        constexpr std::size_t scan_size = 256u;
        std::array<std::uint8_t, scan_size> bytes{};
        iovec local{bytes.data(), bytes.size()};
        iovec remote{
                reinterpret_cast<void*>(static_cast<std::uintptr_t>(function_address)),
                bytes.size()};
        if (process_vm_readv(getpid(), &local, 1, &remote, 1, 0) !=
            static_cast<ssize_t>(bytes.size()))
        {
            return {};
        }
        ZydisDecoder decoder{};
        if (ZYAN_FAILED(ZydisDecoderInit(
                    &decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return {};
        }
        std::vector<std::uint64_t> targets;
        std::size_t offset{};
        while (offset < bytes.size())
        {
            ZydisDecodedInstruction instruction{};
            std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
            if (ZYAN_FAILED(ZydisDecoderDecodeFull(
                        &decoder,
                        bytes.data() + offset,
                        bytes.size() - offset,
                        &instruction,
                        operands.data())) ||
                instruction.length == 0u)
            {
                break;
            }
            if (instruction.mnemonic == ZYDIS_MNEMONIC_CALL &&
                instruction.operand_count_visible > 0u &&
                operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                operands[0].imm.is_relative)
            {
                ZyanU64 absolute{};
                if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(
                            &instruction,
                            &operands[0],
                            function_address + offset,
                            &absolute)) &&
                    executable_address(absolute) &&
                    std::find(targets.begin(), targets.end(), absolute) == targets.end())
                {
                    targets.push_back(absolute);
                }
            }
            offset += instruction.length;
            if (instruction.mnemonic == ZYDIS_MNEMONIC_RET) break;
        }
        return targets;
    }

    [[nodiscard]] bool looks_like_call_function_by_name_target(
            std::uint64_t function_address) noexcept
    {
        constexpr std::size_t scan_size = 64u;
        std::array<std::uint8_t, scan_size> bytes{};
        iovec local{bytes.data(), bytes.size()};
        iovec remote{
                reinterpret_cast<void*>(static_cast<std::uintptr_t>(function_address)),
                bytes.size()};
        if (process_vm_readv(getpid(), &local, 1, &remote, 1, 0) !=
            static_cast<ssize_t>(bytes.size()))
        {
            return false;
        }

        ZydisDecoder decoder{};
        if (ZYAN_FAILED(ZydisDecoderInit(
                    &decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return false;
        }
        std::size_t offset{};
        bool establishes_frame{};
        bool preserves_fifth_argument{};
        while (offset < bytes.size())
        {
            ZydisDecodedInstruction instruction{};
            std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
            if (ZYAN_FAILED(ZydisDecoderDecodeFull(
                        &decoder,
                        bytes.data() + offset,
                        bytes.size() - offset,
                        &instruction,
                        operands.data())) ||
                instruction.length == 0u)
            {
                return false;
            }
            if (instruction.mnemonic == ZYDIS_MNEMONIC_MOV &&
                instruction.operand_count_visible >= 2u &&
                operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                if (operands[0].reg.value == ZYDIS_REGISTER_RBP &&
                    operands[1].reg.value == ZYDIS_REGISTER_RSP)
                {
                    establishes_frame = true;
                }
                if (operands[1].reg.value == ZYDIS_REGISTER_R8D)
                {
                    preserves_fifth_argument = true;
                }
            }
            offset += instruction.length;
            if (establishes_frame && preserves_fifth_argument) return true;
            if (instruction.mnemonic == ZYDIS_MNEMONIC_RET ||
                instruction.mnemonic == ZYDIS_MNEMONIC_JMP)
            {
                break;
            }
        }
        return false;
    }

    [[nodiscard]] bool local_player_exec_target_uses_sysv_arguments(
            std::uint64_t function_address) noexcept
    {
        constexpr std::size_t scan_size = 64u;
        std::array<std::uint8_t, scan_size> bytes{};
        iovec local{bytes.data(), bytes.size()};
        iovec remote{
                reinterpret_cast<void*>(static_cast<std::uintptr_t>(function_address)),
                bytes.size()};
        if (process_vm_readv(getpid(), &local, 1, &remote, 1, 0) !=
            static_cast<ssize_t>(bytes.size()))
        {
            return false;
        }
        ZydisDecoder decoder{};
        if (ZYAN_FAILED(ZydisDecoderInit(
                    &decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return false;
        }
        bool uses_context{};
        bool uses_world{};
        bool uses_command{};
        bool uses_output{};
        std::size_t offset{};
        while (offset < bytes.size())
        {
            ZydisDecodedInstruction instruction{};
            std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
            if (ZYAN_FAILED(ZydisDecoderDecodeFull(
                        &decoder,
                        bytes.data() + offset,
                        bytes.size() - offset,
                        &instruction,
                        operands.data())) ||
                instruction.length == 0u)
            {
                return false;
            }
            for (ZyanU8 index = 1u; index < instruction.operand_count_visible; ++index)
            {
                if (operands[index].type != ZYDIS_OPERAND_TYPE_REGISTER) continue;
                uses_context = uses_context ||
                               operands[index].reg.value == ZYDIS_REGISTER_RDI;
                uses_world = uses_world ||
                             operands[index].reg.value == ZYDIS_REGISTER_RSI;
                uses_command = uses_command ||
                               operands[index].reg.value == ZYDIS_REGISTER_RDX;
                uses_output = uses_output ||
                              operands[index].reg.value == ZYDIS_REGISTER_RCX;
            }
            if (uses_context && uses_world && uses_command && uses_output) return true;
            offset += instruction.length;
            if (instruction.mnemonic == ZYDIS_MNEMONIC_RET ||
                instruction.mnemonic == ZYDIS_MNEMONIC_JMP)
            {
                break;
            }
        }
        return false;
    }

    [[nodiscard]] std::optional<std::uint64_t> resolve_local_player_exec_thunk(
            std::uint64_t thunk,
            std::uint64_t secondary_base_offset,
            std::string& error) noexcept
    {
        error.clear();
        if (thunk == 0u || !executable_address(thunk) ||
            secondary_base_offset == 0u)
        {
            error = "ULocalPlayer FExec slot is not executable";
            return std::nullopt;
        }
        std::array<std::uint8_t, 32u> bytes{};
        iovec local{bytes.data(), bytes.size()};
        iovec remote{
                reinterpret_cast<void*>(static_cast<std::uintptr_t>(thunk)),
                bytes.size()};
        if (process_vm_readv(getpid(), &local, 1, &remote, 1, 0) !=
            static_cast<ssize_t>(bytes.size()))
        {
            error = "cannot read ULocalPlayer FExec adjustor thunk";
            return std::nullopt;
        }
        ZydisDecoder decoder{};
        if (ZYAN_FAILED(ZydisDecoderInit(
                    &decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            error = "cannot initialize the x86_64 decoder";
            return std::nullopt;
        }
        ZydisDecodedInstruction adjust{};
        std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> adjust_operands{};
        if (ZYAN_FAILED(ZydisDecoderDecodeFull(
                    &decoder,
                    bytes.data(),
                    bytes.size(),
                    &adjust,
                    adjust_operands.data())) ||
            adjust.mnemonic != ZYDIS_MNEMONIC_ADD ||
            adjust.operand_count_visible < 2u ||
            adjust_operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
            adjust_operands[0].reg.value != ZYDIS_REGISTER_RDI ||
            adjust_operands[1].type != ZYDIS_OPERAND_TYPE_IMMEDIATE ||
            adjust_operands[1].imm.value.s !=
                    -static_cast<std::int64_t>(secondary_base_offset))
        {
            error = "ULocalPlayer FExec slot does not adjust rdi by the validated secondary-base offset";
            return std::nullopt;
        }
        const std::size_t jump_offset = adjust.length;
        ZydisDecodedInstruction jump{};
        std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> jump_operands{};
        if (jump_offset >= bytes.size() ||
            ZYAN_FAILED(ZydisDecoderDecodeFull(
                    &decoder,
                    bytes.data() + jump_offset,
                    bytes.size() - jump_offset,
                    &jump,
                    jump_operands.data())) ||
            jump.mnemonic != ZYDIS_MNEMONIC_JMP ||
            jump.operand_count_visible == 0u ||
            jump_operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE ||
            !jump_operands[0].imm.is_relative)
        {
            error = "ULocalPlayer FExec adjustor does not end in a relative tail jump";
            return std::nullopt;
        }
        ZyanU64 target{};
        if (ZYAN_FAILED(ZydisCalcAbsoluteAddress(
                    &jump,
                    &jump_operands[0],
                    thunk + jump_offset,
                    &target)) ||
            target == thunk || !executable_address(target) ||
            !local_player_exec_target_uses_sysv_arguments(target))
        {
            error = "ULocalPlayer::Exec target failed the four-argument SysV prologue gate";
            return std::nullopt;
        }
        return static_cast<std::uint64_t>(target);
    }
} // namespace

namespace ue4ss::linux::core
{
    std::vector<std::string> tokenize_console_command(std::string_view command)
    {
        return split_command(command);
    }

    std::optional<std::uint64_t>
    resolve_call_function_by_name_with_arguments_thunk(
            std::uint64_t process_console_exec,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (process_console_exec == 0u ||
                !executable_address(process_console_exec))
            {
                error = "ProcessConsoleExec is not in an executable mapping";
                return std::nullopt;
            }
            std::array<std::uint8_t, 32u> bytes{};
            iovec local{bytes.data(), bytes.size()};
            iovec remote{
                    reinterpret_cast<void*>(
                            static_cast<std::uintptr_t>(process_console_exec)),
                    bytes.size()};
            if (process_vm_readv(getpid(), &local, 1, &remote, 1, 0) !=
                static_cast<ssize_t>(bytes.size()))
            {
                error = "cannot read the ProcessConsoleExec tail thunk";
                return std::nullopt;
            }

            ZydisDecoder decoder{};
            if (ZYAN_FAILED(ZydisDecoderInit(
                        &decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
            {
                error = "cannot initialize the x86_64 decoder";
                return std::nullopt;
            }
            ZydisDecodedInstruction clear_instruction{};
            std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> clear_operands{};
            if (ZYAN_FAILED(ZydisDecoderDecodeFull(
                        &decoder,
                        bytes.data(),
                        bytes.size(),
                        &clear_instruction,
                        clear_operands.data())) ||
                clear_instruction.mnemonic != ZYDIS_MNEMONIC_XOR ||
                clear_instruction.operand_count_visible < 2u ||
                clear_operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                clear_operands[1].type != ZYDIS_OPERAND_TYPE_REGISTER ||
                clear_operands[0].reg.value != ZYDIS_REGISTER_R8D ||
                clear_operands[1].reg.value != ZYDIS_REGISTER_R8D)
            {
                error = "ProcessConsoleExec does not clear the fifth SysV argument in r8d";
                return std::nullopt;
            }

            const std::size_t jump_offset = clear_instruction.length;
            ZydisDecodedInstruction jump_instruction{};
            std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> jump_operands{};
            if (jump_offset >= bytes.size() ||
                ZYAN_FAILED(ZydisDecoderDecodeFull(
                        &decoder,
                        bytes.data() + jump_offset,
                        bytes.size() - jump_offset,
                        &jump_instruction,
                        jump_operands.data())) ||
                jump_instruction.mnemonic != ZYDIS_MNEMONIC_JMP ||
                jump_instruction.operand_count_visible == 0u ||
                jump_operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE ||
                !jump_operands[0].imm.is_relative)
            {
                error = "ProcessConsoleExec does not tail-jump after clearing r8d";
                return std::nullopt;
            }
            ZyanU64 target{};
            if (ZYAN_FAILED(ZydisCalcAbsoluteAddress(
                        &jump_instruction,
                        &jump_operands[0],
                        process_console_exec + jump_offset,
                        &target)) ||
                target == process_console_exec || !executable_address(target))
            {
                error = "ProcessConsoleExec tail-jump target is not executable";
                return std::nullopt;
            }
            if (!looks_like_call_function_by_name_target(target))
            {
                error = "ProcessConsoleExec tail-jump target failed the five-argument function prologue gate";
                return std::nullopt;
            }
            return static_cast<std::uint64_t>(target);
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return std::nullopt;
        }
        catch (...)
        {
            error = "unknown exception while resolving the ProcessConsoleExec tail thunk";
            return std::nullopt;
        }
    }

    LocalPlayerExecInspection inspect_local_player_exec_abi(
            ReadOnlyUnrealRuntime& runtime) noexcept
    {
        LocalPlayerExecInspection output;
        try
        {
            if (!runtime.is_ready())
            {
                output.detail = "validated Unreal runtime is unavailable";
                return output;
            }
            const auto query = runtime.find_by_path(
                    "/Script/Engine.Default__LocalPlayer", 1u);
            if (!query.success || query.objects.empty())
            {
                output.detail = "ULocalPlayer CDO is unavailable: " + query.detail;
                return output;
            }
            output.class_default_object = query.objects.front();
            std::string class_error;
            if (!runtime.is_a(
                        output.class_default_object, "LocalPlayer", class_error))
            {
                output.detail = class_error.empty()
                                        ? "Default__LocalPlayer failed its class identity gate"
                                        : std::move(class_error);
                return output;
            }

            std::ostringstream diagnostics;
            diagnostics << "ULocalPlayer CDO=0x" << std::hex
                        << output.class_default_object.address;
            for (std::uint64_t offset = 0x28u; offset <= 0x80u; offset += 8u)
            {
                std::uint64_t vtable{};
                if (!safe_read(output.class_default_object.address + offset, vtable) ||
                    vtable < 0x10u)
                {
                    continue;
                }
                std::int64_t offset_to_top{};
                std::uint64_t type_info{};
                if (!safe_read(vtable - 0x10u, offset_to_top) ||
                    !safe_read(vtable - 0x8u, type_info))
                {
                    continue;
                }
                diagnostics << " +0x" << std::hex << offset << "=>vtable=0x"
                            << vtable << ",top=" << std::dec << offset_to_top;
                if (offset_to_top != -static_cast<std::int64_t>(offset))
                {
                    continue;
                }
                std::vector<std::uint64_t> slots;
                for (std::uint64_t slot = 0u; slot < 8u; ++slot)
                {
                    std::uint64_t target{};
                    if (!safe_read(vtable + slot * sizeof(void*), target) ||
                        !executable_address(target))
                    {
                        break;
                    }
                    slots.push_back(target);
                }
                if (slots.size() < 3u) continue;

                output.secondary_vtable = vtable;
                output.offset_to_top = offset_to_top;
                output.type_info = type_info;
                output.secondary_base_offset = offset;
                output.executable_slots = std::move(slots);
                output.exec_adjustor_thunk = output.executable_slots[2u];
                std::string exec_error;
                const auto exec_target = resolve_local_player_exec_thunk(
                        output.exec_adjustor_thunk, offset, exec_error);
                diagnostics << ",typeinfo=0x" << std::hex << type_info
                            << ",slots=" << std::dec
                            << output.executable_slots.size();
                for (const auto target : output.executable_slots)
                {
                    diagnostics << " 0x" << std::hex << target;
                }
                if (!exec_target.has_value())
                {
                    output.detail = diagnostics.str() +
                                    "; Exec slot +0x10 rejected: " + exec_error;
                    return output;
                }
                output.success = true;
                output.exec_target = *exec_target;
                diagnostics << "; Exec adjustor=0x" << std::hex
                            << output.exec_adjustor_thunk << " target=0x"
                            << output.exec_target;
                output.detail = diagnostics.str();
                return output;
            }
            output.detail = diagnostics.str() +
                            "; no secondary Itanium base vtable matched its offset-to-top";
            return output;
        }
        catch (const std::exception& exception)
        {
            output.detail = exception.what();
            return output;
        }
        catch (...)
        {
            output.detail = "unknown exception while inspecting ULocalPlayer::FExec ABI";
            return output;
        }
    }

    std::atomic<ProcessConsoleExecHookManager*> ProcessConsoleExecHookManager::s_active{};
    std::atomic<ProcessConsoleExecHookManager::ProcessConsoleExecFunction>
            ProcessConsoleExecHookManager::s_original_fallback{};
    std::mutex ProcessConsoleExecHookManager::s_dispatch_mutex;

    ProcessConsoleExecHookManager::~ProcessConsoleExecHookManager()
    {
        stop();
    }

    bool ProcessConsoleExecHookManager::start(
            ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (is_ready()) return m_runtime == &runtime;
            if (!runtime.is_ready() || !runtime.process_event_available())
            {
                error = "validated Unreal 5.1 runtime and ProcessEvent are required for ProcessConsoleExec";
                return false;
            }

            const auto object_query =
                    runtime.find_by_path("/Script/CoreUObject.Default__Object", 1u);
            if (!object_query.success || object_query.objects.empty())
            {
                error = "UObject CDO is unavailable: " + object_query.detail;
                return false;
            }
            const auto& object_cdo = object_query.objects.front();
            std::string class_error;
            if (!runtime.is_a(object_cdo, "Object", class_error))
            {
                error = class_error.empty() ? "Default__Object failed the UObject identity gate"
                                            : class_error;
                return false;
            }

            std::uint64_t vtable{};
            std::uint64_t process_event{};
            std::uint64_t target{};
            if (!safe_read(object_cdo.address, vtable) || vtable == 0u ||
                !safe_read(vtable + k_ue51_linux_process_event_vtable_offset, process_event) ||
                process_event != std::bit_cast<std::uintptr_t>(runtime.process_event_address()))
            {
                error = "UObject CDO failed the UE 5.1 Linux ProcessEvent vtable gate at +0x268";
                return false;
            }
            if (!safe_read(vtable + k_ue51_linux_process_console_exec_vtable_offset, target) ||
                target == 0u || target == process_event || !executable_address(target))
            {
                error = "UObject::ProcessConsoleExec Linux vtable slot +0x280 is not a distinct executable target";
                return false;
            }
            m_direct_call_targets = ::direct_call_targets(target);
            std::string call_function_error;
            const auto call_function_target =
                    resolve_call_function_by_name_with_arguments_thunk(
                            target, call_function_error);
            if (!call_function_target.has_value())
            {
                error = "CallFunctionByNameWithArguments tail-thunk gate failed: " +
                        call_function_error;
                return false;
            }
            m_call_function_by_name_target = *call_function_target;

            ProcessConsoleExecHookManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another UObject::ProcessConsoleExec hook manager is already active";
                return false;
            }
            m_runtime = &runtime;
            m_target = target;
            m_accepting.store(true, std::memory_order_release);
            if (!m_hook.install(
                        std::bit_cast<void*>(static_cast<std::uintptr_t>(target)),
                        std::bit_cast<void*>(&ProcessConsoleExecHookManager::detour),
                        [this](void* trampoline) {
                            const auto original =
                                    std::bit_cast<ProcessConsoleExecFunction>(trampoline);
                            m_original.store(original, std::memory_order_release);
                            s_original_fallback.store(original, std::memory_order_release);
                        },
                        error))
            {
                std::ostringstream contextual_error;
                contextual_error << "UObject CDO vtable=0x" << std::hex << vtable
                                 << " candidate(+0x280)=0x" << target << ": " << error;
                error = contextual_error.str();
                m_accepting.store(false, std::memory_order_release);
                m_runtime = nullptr;
                m_target = 0u;
                m_call_function_by_name_target = 0u;
                m_direct_call_targets.clear();
                ProcessConsoleExecHookManager* active = this;
                static_cast<void>(s_active.compare_exchange_strong(
                        active, nullptr, std::memory_order_acq_rel));
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            stop();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while starting UObject::ProcessConsoleExec hooks";
            stop();
            return false;
        }
    }

    void ProcessConsoleExecHookManager::stop() noexcept
    {
        m_accepting.store(false, std::memory_order_release);
        std::string hook_error;
        bool restored{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            restored = m_hook.restore(hook_error);
            if (restored && m_target != 0u)
            {
                s_original_fallback.store(
                        std::bit_cast<ProcessConsoleExecFunction>(
                                static_cast<std::uintptr_t>(m_target)),
                        std::memory_order_release);
            }
            ProcessConsoleExecHookManager* active = this;
            static_cast<void>(s_active.compare_exchange_strong(
                    active, nullptr, std::memory_order_acq_rel));
        }
        std::unique_lock wait_lock{m_wait_mutex};
        m_wait_wakeup.wait(wait_lock, [this] {
            return m_in_flight.load(std::memory_order_acquire) == 0u;
        });
        wait_lock.unlock();
        if (restored)
        {
            std::string release_error;
            if (!m_hook.release(release_error))
            {
                std::fprintf(stderr,
                             "[UE4SS Linux] ProcessConsoleExec trampoline release failed: %s\n",
                             release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] ProcessConsoleExec restore failed; retaining trampoline: %s\n",
                         hook_error.c_str());
        }
        {
            std::lock_guard lock{m_callbacks_mutex};
            m_callbacks.clear();
        }
        m_original.store(nullptr, std::memory_order_release);
        m_runtime = nullptr;
        m_target = 0u;
        m_call_function_by_name_target = 0u;
        m_direct_call_targets.clear();
    }

    bool ProcessConsoleExecHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr &&
               m_target != 0u && m_hook.is_installed() &&
               m_original.load(std::memory_order_acquire) != nullptr;
    }

    std::uint64_t ProcessConsoleExecHookManager::target_address() const noexcept
    {
        return m_target;
    }

    std::uint64_t
    ProcessConsoleExecHookManager::call_function_by_name_target() const noexcept
    {
        return m_call_function_by_name_target;
    }

    const std::vector<std::uint64_t>&
    ProcessConsoleExecHookManager::direct_call_targets() const noexcept
    {
        return m_direct_call_targets;
    }

    std::pair<std::uint64_t, std::uint64_t> ProcessConsoleExecHookManager::register_hook(
            Callback pre, Callback post, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || (!pre && !post))
            {
                error = "active ProcessConsoleExec manager and at least one callback are required";
                return {};
            }
            const auto pre_id = pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const auto post_id = post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "ProcessConsoleExec callback id space is exhausted";
                return {};
            }
            auto callback = std::make_shared<CallbackRecord>();
            callback->pre_id = pre_id;
            callback->post_id = post_id;
            callback->pre = std::move(pre);
            callback->post = std::move(post);
            std::lock_guard lock{m_callbacks_mutex};
            if (!is_ready())
            {
                error = "ProcessConsoleExec manager stopped during callback registration";
                return {};
            }
            m_callbacks.push_back(std::move(callback));
            return {pre_id, post_id};
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return {};
        }
        catch (...)
        {
            error = "unknown exception while registering a ProcessConsoleExec callback";
            return {};
        }
    }

    bool ProcessConsoleExecHookManager::unregister_hook(
            std::uint64_t pre_id, std::uint64_t post_id, std::string& error) noexcept
    {
        error.clear();
        try
        {
            std::lock_guard lock{m_callbacks_mutex};
            const auto old_size = m_callbacks.size();
            std::erase_if(m_callbacks, [pre_id, post_id](const auto& callback) {
                if (callback->pre_id == pre_id && callback->post_id == post_id)
                {
                    callback->active.store(false, std::memory_order_release);
                    return true;
                }
                return false;
            });
            if (old_size == m_callbacks.size())
            {
                error = "ProcessConsoleExec callback ids were not found";
                return false;
            }
            return true;
        }
        catch (...)
        {
            error = "unknown exception while unregistering a ProcessConsoleExec callback";
            return false;
        }
    }

    bool ProcessConsoleExecHookManager::snapshot_invocation(
            void* context,
            const char16_t* command,
            void* output_device,
            void* executor,
            ProcessConsoleExecInvocation& output,
            std::string& error) const noexcept
    {
        output = {};
        error.clear();
        if (m_runtime == nullptr || context == nullptr || command == nullptr ||
            output_device == nullptr ||
            !m_runtime->handle_from_address(
                    std::bit_cast<std::uintptr_t>(context), output.context) ||
            !m_runtime->snapshot_native_tchar_string(
                    std::bit_cast<std::uintptr_t>(command), output.command, error))
        {
            if (error.empty()) error = "ProcessConsoleExec contains an invalid native argument";
            return false;
        }
        if (executor != nullptr &&
            !m_runtime->handle_from_address(
                    std::bit_cast<std::uintptr_t>(executor), output.executor))
        {
            error = "ProcessConsoleExec Executor points outside the validated GUObjectArray";
            return false;
        }
        output.output_device = std::bit_cast<std::uintptr_t>(output_device);
        output.command_parts = tokenize_console_command(output.command);
        return true;
    }

    bool ProcessConsoleExecHookManager::log(
            std::uint64_t output_device, std::string_view message, std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || output_device == 0u)
            {
                error = "active ProcessConsoleExec manager and FOutputDevice are required";
                return false;
            }
            std::uint64_t vtable{};
            std::uint64_t serialize{};
            if (!safe_read(output_device, vtable) || vtable == 0u ||
                !safe_read(vtable + k_ue51_linux_output_device_serialize_vtable_offset,
                           serialize) ||
                serialize == 0u || !executable_address(serialize))
            {
                error = "FOutputDevice Serialize slot +0x10 failed the Linux Itanium ABI gate";
                return false;
            }
            std::u16string converted = ue4ss::linux::utf8_to_unreal(message);
            if (converted.empty()) converted.push_back(u'\0');
            const RawFName category{};
            using SerializeFunction =
                    void (*)(void*, const char16_t*, std::uint8_t, const RawFName*, double);
            std::bit_cast<SerializeFunction>(static_cast<std::uintptr_t>(serialize))(
                    std::bit_cast<void*>(static_cast<std::uintptr_t>(output_device)),
                    converted.c_str(),
                    6u,
                    &category,
                    0.0);
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while writing to FOutputDevice";
            return false;
        }
    }

    bool ProcessConsoleExecHookManager::detour(
            void* context,
            const char16_t* command,
            void* output_device,
            void* executor) noexcept
    {
        ProcessConsoleExecHookManager* active{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            active = s_active.load(std::memory_order_acquire);
            if (active != nullptr)
            {
                active->m_in_flight.fetch_add(1u, std::memory_order_acq_rel);
            }
        }
        if (active != nullptr)
        {
            return active->dispatch(context, command, output_device, executor);
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire);
            original != nullptr)
        {
            return original(context, command, output_device, executor);
        }
        return false;
    }

    bool ProcessConsoleExecHookManager::dispatch(
            void* context,
            const char16_t* command,
            void* output_device,
            void* executor) noexcept
    {
        const auto original = m_original.load(std::memory_order_acquire);
        std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        std::optional<bool> override;
        bool original_result{};
        bool original_called{};
        try
        {
            {
                std::lock_guard lock{m_callbacks_mutex};
                callbacks = m_callbacks;
            }
            ProcessConsoleExecInvocation invocation;
            std::string snapshot_error;
            const bool valid = m_accepting.load(std::memory_order_acquire) &&
                               snapshot_invocation(context,
                                                   command,
                                                   output_device,
                                                   executor,
                                                   invocation,
                                                   snapshot_error);
            if (valid)
            {
                for (const auto& callback : callbacks)
                {
                    if (!callback->active.load(std::memory_order_acquire) || !callback->pre)
                        continue;
                    try
                    {
                        const auto candidate = callback->pre(invocation);
                        if (!override.has_value() && candidate.has_value()) override = candidate;
                    }
                    catch (...)
                    {
                    }
                }
            }
            if (original != nullptr)
            {
                original_called = true;
                original_result = original(context, command, output_device, executor);
            }
            if (valid && m_accepting.load(std::memory_order_acquire))
            {
                for (const auto& callback : callbacks)
                {
                    if (!callback->active.load(std::memory_order_acquire) || !callback->post)
                        continue;
                    try
                    {
                        const auto candidate = callback->post(invocation);
                        if (!override.has_value() && candidate.has_value()) override = candidate;
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
        catch (...)
        {
            if (!original_called && original != nullptr)
            {
                try
                {
                    original_result = original(context, command, output_device, executor);
                }
                catch (...)
                {
                    original_result = false;
                }
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
        return override.value_or(original_result);
    }

    std::atomic<CallFunctionByNameWithArgumentsHookManager*>
            CallFunctionByNameWithArgumentsHookManager::s_active{};
    std::atomic<CallFunctionByNameWithArgumentsHookManager::TargetFunction>
            CallFunctionByNameWithArgumentsHookManager::s_original_fallback{};
    std::mutex CallFunctionByNameWithArgumentsHookManager::s_dispatch_mutex;

    CallFunctionByNameWithArgumentsHookManager::~CallFunctionByNameWithArgumentsHookManager()
    {
        stop();
    }

    bool CallFunctionByNameWithArgumentsHookManager::start(
            ReadOnlyUnrealRuntime& runtime,
            const ProcessConsoleExecHookManager& process_console_exec,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (is_ready()) return m_runtime == &runtime;
            if (!runtime.is_ready() || !process_console_exec.is_ready())
            {
                error = "validated Unreal runtime and ProcessConsoleExec hook manager are required";
                return false;
            }
            const std::uint64_t expected =
                    process_console_exec.call_function_by_name_target();
            // ProcessConsoleExec is already detoured at this point, so its
            // live entry bytes must not be decoded again. The manager only
            // exposes this target after resolving and structurally validating
            // the original thunk before installing its own patch.
            if (expected == 0u || expected == process_console_exec.target_address() ||
                !executable_address(expected))
            {
                error = "validated ProcessConsoleExec tail-thunk target is unavailable";
                return false;
            }

            CallFunctionByNameWithArgumentsHookManager* active{};
            if (!s_active.compare_exchange_strong(
                        active, this, std::memory_order_acq_rel))
            {
                error = "another CallFunctionByNameWithArguments hook manager is already active";
                return false;
            }
            m_runtime = &runtime;
            m_target = expected;
            m_accepting.store(true, std::memory_order_release);
            if (!m_hook.install(
                        std::bit_cast<void*>(static_cast<std::uintptr_t>(expected)),
                        std::bit_cast<void*>(
                                &CallFunctionByNameWithArgumentsHookManager::detour),
                        [this](void* trampoline) {
                            const auto original =
                                    std::bit_cast<TargetFunction>(trampoline);
                            m_original.store(original, std::memory_order_release);
                            s_original_fallback.store(
                                    original, std::memory_order_release);
                        },
                        error))
            {
                std::ostringstream contextual_error;
                contextual_error
                        << "validated ProcessConsoleExec tail target 0x" << std::hex
                        << expected << ": " << error;
                error = contextual_error.str();
                m_accepting.store(false, std::memory_order_release);
                m_runtime = nullptr;
                m_target = 0u;
                active = this;
                static_cast<void>(s_active.compare_exchange_strong(
                        active, nullptr, std::memory_order_acq_rel));
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            stop();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while starting CallFunctionByNameWithArguments hooks";
            stop();
            return false;
        }
    }

    void CallFunctionByNameWithArgumentsHookManager::stop() noexcept
    {
        m_accepting.store(false, std::memory_order_release);
        std::string hook_error;
        bool restored{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            restored = m_hook.restore(hook_error);
            if (restored && m_target != 0u)
            {
                s_original_fallback.store(
                        std::bit_cast<TargetFunction>(
                                static_cast<std::uintptr_t>(m_target)),
                        std::memory_order_release);
            }
            CallFunctionByNameWithArgumentsHookManager* active = this;
            static_cast<void>(s_active.compare_exchange_strong(
                    active, nullptr, std::memory_order_acq_rel));
        }
        std::unique_lock wait_lock{m_wait_mutex};
        m_wait_wakeup.wait(wait_lock, [this] {
            return m_in_flight.load(std::memory_order_acquire) == 0u;
        });
        wait_lock.unlock();
        if (restored)
        {
            std::string release_error;
            if (!m_hook.release(release_error))
            {
                std::fprintf(
                        stderr,
                        "[UE4SS Linux] CallFunctionByNameWithArguments trampoline release failed: %s\n",
                        release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] CallFunctionByNameWithArguments restore failed; retaining trampoline: %s\n",
                    hook_error.c_str());
        }
        {
            std::lock_guard lock{m_callbacks_mutex};
            m_callbacks.clear();
        }
        m_original.store(nullptr, std::memory_order_release);
        m_runtime = nullptr;
        m_target = 0u;
    }

    bool CallFunctionByNameWithArgumentsHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr &&
               m_target != 0u && m_hook.is_installed() &&
               m_original.load(std::memory_order_acquire) != nullptr;
    }

    std::uint64_t
    CallFunctionByNameWithArgumentsHookManager::target_address() const noexcept
    {
        return m_target;
    }

    std::pair<std::uint64_t, std::uint64_t>
    CallFunctionByNameWithArgumentsHookManager::register_hook(
            Callback pre, Callback post, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || (!pre && !post))
            {
                error = "active CallFunctionByNameWithArguments manager and at least one callback are required";
                return {};
            }
            const auto pre_id =
                    pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const auto post_id =
                    post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "CallFunctionByNameWithArguments callback id space is exhausted";
                return {};
            }
            auto callback = std::make_shared<CallbackRecord>();
            callback->pre_id = pre_id;
            callback->post_id = post_id;
            callback->pre = std::move(pre);
            callback->post = std::move(post);
            std::lock_guard lock{m_callbacks_mutex};
            if (!is_ready())
            {
                error = "CallFunctionByNameWithArguments manager stopped during callback registration";
                return {};
            }
            m_callbacks.push_back(std::move(callback));
            return {pre_id, post_id};
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return {};
        }
        catch (...)
        {
            error = "unknown exception while registering a CallFunctionByNameWithArguments callback";
            return {};
        }
    }

    bool CallFunctionByNameWithArgumentsHookManager::unregister_hook(
            std::uint64_t pre_id,
            std::uint64_t post_id,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            std::lock_guard lock{m_callbacks_mutex};
            const auto old_size = m_callbacks.size();
            std::erase_if(m_callbacks, [pre_id, post_id](const auto& callback) {
                if (callback->pre_id == pre_id && callback->post_id == post_id)
                {
                    callback->active.store(false, std::memory_order_release);
                    return true;
                }
                return false;
            });
            if (old_size == m_callbacks.size())
            {
                error = "CallFunctionByNameWithArguments callback ids were not found";
                return false;
            }
            return true;
        }
        catch (...)
        {
            error = "unknown exception while unregistering a CallFunctionByNameWithArguments callback";
            return false;
        }
    }

    bool CallFunctionByNameWithArgumentsHookManager::snapshot_invocation(
            void* context,
            const char16_t* command,
            void* output_device,
            void* executor,
            bool force_call_with_non_exec,
            CallFunctionByNameWithArgumentsInvocation& output,
            std::string& error) const noexcept
    {
        output = {};
        error.clear();
        if (m_runtime == nullptr || context == nullptr || command == nullptr ||
            output_device == nullptr ||
            !m_runtime->handle_from_address(
                    std::bit_cast<std::uintptr_t>(context), output.context) ||
            !m_runtime->snapshot_native_tchar_string(
                    std::bit_cast<std::uintptr_t>(command), output.command, error))
        {
            if (error.empty())
                error = "CallFunctionByNameWithArguments contains an invalid native argument";
            return false;
        }
        if (executor != nullptr &&
            !m_runtime->handle_from_address(
                    std::bit_cast<std::uintptr_t>(executor), output.executor))
        {
            error = "CallFunctionByNameWithArguments Executor points outside the validated GUObjectArray";
            return false;
        }
        output.output_device = std::bit_cast<std::uintptr_t>(output_device);
        output.force_call_with_non_exec = force_call_with_non_exec;
        return true;
    }

    bool CallFunctionByNameWithArgumentsHookManager::detour(
            void* context,
            const char16_t* command,
            void* output_device,
            void* executor,
            bool force_call_with_non_exec) noexcept
    {
        CallFunctionByNameWithArgumentsHookManager* active{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            active = s_active.load(std::memory_order_acquire);
            if (active != nullptr)
            {
                active->m_in_flight.fetch_add(1u, std::memory_order_acq_rel);
            }
        }
        if (active != nullptr)
        {
            return active->dispatch(context,
                                    command,
                                    output_device,
                                    executor,
                                    force_call_with_non_exec);
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire);
            original != nullptr)
        {
            return original(context,
                            command,
                            output_device,
                            executor,
                            force_call_with_non_exec);
        }
        return false;
    }

    bool CallFunctionByNameWithArgumentsHookManager::dispatch(
            void* context,
            const char16_t* command,
            void* output_device,
            void* executor,
            bool force_call_with_non_exec) noexcept
    {
        const auto original = m_original.load(std::memory_order_acquire);
        std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        std::optional<bool> override;
        bool original_result{};
        bool original_called{};
        try
        {
            {
                std::lock_guard lock{m_callbacks_mutex};
                callbacks = m_callbacks;
            }
            CallFunctionByNameWithArgumentsInvocation invocation;
            std::string snapshot_error;
            const bool valid = m_accepting.load(std::memory_order_acquire) &&
                               snapshot_invocation(context,
                                                   command,
                                                   output_device,
                                                   executor,
                                                   force_call_with_non_exec,
                                                   invocation,
                                                   snapshot_error);
            if (valid)
            {
                for (const auto& callback : callbacks)
                {
                    if (!callback->active.load(std::memory_order_acquire) ||
                        !callback->pre)
                    {
                        continue;
                    }
                    try
                    {
                        const auto candidate = callback->pre(invocation);
                        if (candidate.has_value()) override = candidate;
                    }
                    catch (...)
                    {
                    }
                }
            }
            if (original != nullptr)
            {
                original_called = true;
                original_result = original(context,
                                           command,
                                           output_device,
                                           executor,
                                           force_call_with_non_exec);
            }
            if (valid && m_accepting.load(std::memory_order_acquire))
            {
                for (const auto& callback : callbacks)
                {
                    if (!callback->active.load(std::memory_order_acquire) ||
                        !callback->post)
                    {
                        continue;
                    }
                    try
                    {
                        const auto candidate = callback->post(invocation);
                        if (candidate.has_value()) override = candidate;
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
        catch (...)
        {
            if (!original_called && original != nullptr)
            {
                try
                {
                    original_result = original(context,
                                               command,
                                               output_device,
                                               executor,
                                               force_call_with_non_exec);
                }
                catch (...)
                {
                    original_result = false;
                }
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
        return override.value_or(original_result);
    }

    std::atomic<LocalPlayerExecHookManager*>
            LocalPlayerExecHookManager::s_active{};
    std::atomic<LocalPlayerExecHookManager::TargetFunction>
            LocalPlayerExecHookManager::s_original_fallback{};
    std::mutex LocalPlayerExecHookManager::s_dispatch_mutex;

    LocalPlayerExecHookManager::~LocalPlayerExecHookManager()
    {
        stop();
    }

    bool LocalPlayerExecHookManager::start(
            ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (is_ready()) return m_runtime == &runtime;
            const auto inspection = inspect_local_player_exec_abi(runtime);
            if (!inspection.success || inspection.exec_target == 0u)
            {
                error = inspection.detail.empty()
                                ? "ULocalPlayer::Exec ABI inspection failed"
                                : inspection.detail;
                return false;
            }
            LocalPlayerExecHookManager* active{};
            if (!s_active.compare_exchange_strong(
                        active, this, std::memory_order_acq_rel))
            {
                error = "another ULocalPlayer::Exec hook manager is already active";
                return false;
            }
            m_runtime = &runtime;
            m_target = inspection.exec_target;
            m_secondary_base_offset = inspection.secondary_base_offset;
            m_accepting.store(true, std::memory_order_release);
            if (!m_hook.install(
                        std::bit_cast<void*>(
                                static_cast<std::uintptr_t>(m_target)),
                        std::bit_cast<void*>(&LocalPlayerExecHookManager::detour),
                        [this](void* trampoline) {
                            const auto original =
                                    std::bit_cast<TargetFunction>(trampoline);
                            m_original.store(original, std::memory_order_release);
                            s_original_fallback.store(
                                    original, std::memory_order_release);
                        },
                        error))
            {
                std::ostringstream contextual_error;
                contextual_error << "validated ULocalPlayer::Exec target 0x" << std::hex
                                 << m_target << ": " << error;
                error = contextual_error.str();
                m_accepting.store(false, std::memory_order_release);
                m_runtime = nullptr;
                m_target = 0u;
                m_secondary_base_offset = 0u;
                active = this;
                static_cast<void>(s_active.compare_exchange_strong(
                        active, nullptr, std::memory_order_acq_rel));
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            stop();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while starting ULocalPlayer::Exec hooks";
            stop();
            return false;
        }
    }

    void LocalPlayerExecHookManager::stop() noexcept
    {
        m_accepting.store(false, std::memory_order_release);
        std::string hook_error;
        bool restored{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            restored = m_hook.restore(hook_error);
            if (restored && m_target != 0u)
            {
                s_original_fallback.store(
                        std::bit_cast<TargetFunction>(
                                static_cast<std::uintptr_t>(m_target)),
                        std::memory_order_release);
            }
            LocalPlayerExecHookManager* active = this;
            static_cast<void>(s_active.compare_exchange_strong(
                    active, nullptr, std::memory_order_acq_rel));
        }
        std::unique_lock wait_lock{m_wait_mutex};
        m_wait_wakeup.wait(wait_lock, [this] {
            return m_in_flight.load(std::memory_order_acquire) == 0u;
        });
        wait_lock.unlock();
        if (restored)
        {
            std::string release_error;
            if (!m_hook.release(release_error))
            {
                std::fprintf(
                        stderr,
                        "[UE4SS Linux] ULocalPlayer::Exec trampoline release failed: %s\n",
                        release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            std::fprintf(
                    stderr,
                    "[UE4SS Linux] ULocalPlayer::Exec restore failed; retaining trampoline: %s\n",
                    hook_error.c_str());
        }
        {
            std::lock_guard lock{m_callbacks_mutex};
            m_callbacks.clear();
        }
        m_original.store(nullptr, std::memory_order_release);
        m_runtime = nullptr;
        m_target = 0u;
        m_secondary_base_offset = 0u;
    }

    bool LocalPlayerExecHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr &&
               m_target != 0u && m_secondary_base_offset != 0u &&
               m_hook.is_installed() &&
               m_original.load(std::memory_order_acquire) != nullptr;
    }

    std::uint64_t LocalPlayerExecHookManager::target_address() const noexcept
    {
        return m_target;
    }

    std::uint64_t LocalPlayerExecHookManager::secondary_base_offset() const noexcept
    {
        return m_secondary_base_offset;
    }

    std::pair<std::uint64_t, std::uint64_t>
    LocalPlayerExecHookManager::register_hook(
            Callback pre, Callback post, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || (!pre && !post))
            {
                error = "active ULocalPlayer::Exec manager and at least one callback are required";
                return {};
            }
            const auto pre_id =
                    pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const auto post_id =
                    post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "ULocalPlayer::Exec callback id space is exhausted";
                return {};
            }
            auto callback = std::make_shared<CallbackRecord>();
            callback->pre_id = pre_id;
            callback->post_id = post_id;
            callback->pre = std::move(pre);
            callback->post = std::move(post);
            std::lock_guard lock{m_callbacks_mutex};
            if (!is_ready())
            {
                error = "ULocalPlayer::Exec manager stopped during callback registration";
                return {};
            }
            m_callbacks.push_back(std::move(callback));
            return {pre_id, post_id};
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return {};
        }
        catch (...)
        {
            error = "unknown exception while registering a ULocalPlayer::Exec callback";
            return {};
        }
    }

    bool LocalPlayerExecHookManager::unregister_hook(
            std::uint64_t pre_id,
            std::uint64_t post_id,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            std::lock_guard lock{m_callbacks_mutex};
            const auto old_size = m_callbacks.size();
            std::erase_if(m_callbacks, [pre_id, post_id](const auto& callback) {
                if (callback->pre_id == pre_id && callback->post_id == post_id)
                {
                    callback->active.store(false, std::memory_order_release);
                    return true;
                }
                return false;
            });
            if (old_size == m_callbacks.size())
            {
                error = "ULocalPlayer::Exec callback ids were not found";
                return false;
            }
            return true;
        }
        catch (...)
        {
            error = "unknown exception while unregistering a ULocalPlayer::Exec callback";
            return false;
        }
    }

    bool LocalPlayerExecHookManager::snapshot_invocation(
            void* context,
            void* world,
            const char16_t* command,
            void* output_device,
            LocalPlayerExecInvocation& output,
            std::string& error) const noexcept
    {
        output = {};
        error.clear();
        if (m_runtime == nullptr || context == nullptr || command == nullptr ||
            output_device == nullptr ||
            !m_runtime->handle_from_address(
                    std::bit_cast<std::uintptr_t>(context), output.context) ||
            !m_runtime->snapshot_native_tchar_string(
                    std::bit_cast<std::uintptr_t>(command), output.command, error))
        {
            if (error.empty())
                error = "ULocalPlayer::Exec contains an invalid native argument";
            return false;
        }
        std::string class_error;
        if (!m_runtime->is_a(output.context, "LocalPlayer", class_error))
        {
            error = class_error.empty()
                            ? "ULocalPlayer::Exec context failed the LocalPlayer class gate"
                            : std::move(class_error);
            return false;
        }
        if (world != nullptr)
        {
            if (!m_runtime->handle_from_address(
                        std::bit_cast<std::uintptr_t>(world), output.world) ||
                !m_runtime->is_a(output.world, "World", class_error))
            {
                error = "ULocalPlayer::Exec world failed the UWorld object gate";
                return false;
            }
        }
        output.output_device = std::bit_cast<std::uintptr_t>(output_device);
        return true;
    }

    bool LocalPlayerExecHookManager::detour(
            void* context,
            void* world,
            const char16_t* command,
            void* output_device) noexcept
    {
        LocalPlayerExecHookManager* active{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            active = s_active.load(std::memory_order_acquire);
            if (active != nullptr)
            {
                active->m_in_flight.fetch_add(1u, std::memory_order_acq_rel);
            }
        }
        if (active != nullptr)
        {
            return active->dispatch(context, world, command, output_device);
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire);
            original != nullptr)
        {
            return original(context, world, command, output_device);
        }
        return false;
    }

    bool LocalPlayerExecHookManager::dispatch(
            void* context,
            void* world,
            const char16_t* command,
            void* output_device) noexcept
    {
        const auto original = m_original.load(std::memory_order_acquire);
        std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        std::optional<bool> override;
        bool execute_original{true};
        bool original_result{};
        bool original_called{};
        try
        {
            {
                std::lock_guard lock{m_callbacks_mutex};
                callbacks = m_callbacks;
            }
            LocalPlayerExecInvocation invocation;
            std::string snapshot_error;
            const bool valid = m_accepting.load(std::memory_order_acquire) &&
                               snapshot_invocation(context,
                                                   world,
                                                   command,
                                                   output_device,
                                                   invocation,
                                                   snapshot_error);
            if (valid)
            {
                for (const auto& callback : callbacks)
                {
                    if (!callback->active.load(std::memory_order_acquire) ||
                        !callback->pre)
                    {
                        continue;
                    }
                    try
                    {
                        const auto candidate = callback->pre(invocation);
                        if (candidate.return_override.has_value())
                            override = candidate.return_override;
                        if (!candidate.execute_original) execute_original = false;
                    }
                    catch (...)
                    {
                    }
                }
            }
            if (execute_original && original != nullptr)
            {
                original_called = true;
                original_result = original(context, world, command, output_device);
            }
            if (valid && m_accepting.load(std::memory_order_acquire))
            {
                for (const auto& callback : callbacks)
                {
                    if (!callback->active.load(std::memory_order_acquire) ||
                        !callback->post)
                    {
                        continue;
                    }
                    try
                    {
                        const auto candidate = callback->post(invocation);
                        if (candidate.return_override.has_value())
                            override = candidate.return_override;
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
        catch (...)
        {
            if (!original_called && execute_original && original != nullptr)
            {
                try
                {
                    original_result = original(
                            context, world, command, output_device);
                }
                catch (...)
                {
                    original_result = false;
                }
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
        return override.value_or(original_result);
    }
} // namespace ue4ss::linux::core
