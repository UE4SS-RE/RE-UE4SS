#include "blueprint_hooks.hpp"

#include <Zydis/Zydis.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <exception>
#include <fstream>
#include <limits>
#include <sys/uio.h>
#include <unistd.h>

namespace
{
    constexpr std::uint64_t k_ue51_fframe_node_offset = 0x10u;
    constexpr std::uint64_t k_ue51_fframe_code_offset = 0x20u;
    constexpr std::uint64_t k_ue51_fframe_current_native_function_offset = 0x90u;
    constexpr std::size_t k_process_internal_probe_bytes = 164u;

    [[nodiscard]] bool ascii_case_insensitive_equal(
            std::string_view left,
            std::string_view right) noexcept
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < left.size(); ++index)
        {
            const auto lower = [](unsigned char character) {
                return character >= 'A' && character <= 'Z'
                               ? static_cast<unsigned char>(character + ('a' - 'A'))
                               : character;
            };
            if (lower(static_cast<unsigned char>(left[index])) !=
                lower(static_cast<unsigned char>(right[index])))
            {
                return false;
            }
        }
        return true;
    }

    template <typename T>
    [[nodiscard]] bool safe_read(std::uint64_t address, T& value) noexcept
    {
        if (address == 0u || address > std::numeric_limits<std::uintptr_t>::max() - sizeof(T))
        {
            return false;
        }
        iovec local{&value, sizeof(value)};
        iovec remote{reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), sizeof(value)};
        return process_vm_readv(getpid(), &local, 1, &remote, 1, 0) == static_cast<ssize_t>(sizeof(value));
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

    [[nodiscard]] bool decode_branch_target(ZydisDecoder& decoder,
                                            std::uint64_t instruction_address,
                                            ZydisMnemonic required_mnemonic,
                                            std::uint64_t& output) noexcept
    {
        std::array<std::uint8_t, ZYDIS_MAX_INSTRUCTION_LENGTH> bytes{};
        if (!safe_read(instruction_address, bytes))
        {
            return false;
        }
        ZydisDecodedInstruction instruction{};
        std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
        if (ZYAN_FAILED(ZydisDecoderDecodeFull(
                    &decoder, bytes.data(), bytes.size(), &instruction, operands.data())) ||
            instruction.mnemonic != required_mnemonic || instruction.operand_count_visible == 0u)
        {
            return false;
        }
        ZyanU64 absolute{};
        if (ZYAN_FAILED(ZydisCalcAbsoluteAddress(&instruction, &operands[0], instruction_address, &absolute)))
        {
            return false;
        }
        output = absolute;
        if (operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            std::uint64_t indirect{};
            if (!safe_read(output, indirect))
            {
                return false;
            }
            output = indirect;
        }
        return output != 0u;
    }

    [[nodiscard]] std::uint64_t resolve_jump_chain(ZydisDecoder& decoder, std::uint64_t address) noexcept
    {
        for (std::size_t depth = 0; depth < 4u; ++depth)
        {
            std::uint64_t target{};
            if (!decode_branch_target(decoder, address, ZYDIS_MNEMONIC_JMP, target))
            {
                break;
            }
            address = target;
        }
        return address;
    }

    [[nodiscard]] bool resolve_blueprint_addresses(ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                                   ue4ss::linux::core::BlueprintExecutionAddresses& output,
                                                   std::string& error) noexcept
    {
        output = {};
        const auto query = runtime.find_by_path("/Script/CoreUObject.Object:ExecuteUbergraph", 1u);
        if (!query.success || query.objects.empty())
        {
            error = "ExecuteUbergraph UFunction is unavailable: " + query.detail;
            return false;
        }
        std::uint64_t process_internal{};
        if (!runtime.function_native_address(query.objects.front(), process_internal, error))
        {
            return false;
        }
        ZydisDecoder decoder{};
        if (ZYAN_FAILED(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            error = "cannot initialize Zydis for Blueprint resolver validation";
            return false;
        }
        process_internal = resolve_jump_chain(decoder, process_internal);
        if (!executable_address(process_internal))
        {
            error = "ExecuteUbergraph UFunction::Func does not resolve to executable ProcessInternal code";
            return false;
        }

        std::size_t offset{};
        std::size_t call_count{};
        std::uint64_t process_local{};
        while (offset < k_process_internal_probe_bytes)
        {
            std::array<std::uint8_t, ZYDIS_MAX_INSTRUCTION_LENGTH> bytes{};
            const std::uint64_t instruction_address = process_internal + offset;
            if (!safe_read(instruction_address, bytes))
            {
                error = "ProcessInternal instruction window became unreadable";
                return false;
            }
            ZydisDecodedInstruction instruction{};
            std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
            if (ZYAN_FAILED(ZydisDecoderDecodeFull(
                        &decoder, bytes.data(), bytes.size(), &instruction, operands.data())) ||
                instruction.length == 0u)
            {
                error = "ProcessInternal instruction window could not be decoded";
                return false;
            }
            if (instruction.mnemonic == ZYDIS_MNEMONIC_CALL)
            {
                ++call_count;
                if (call_count == 3u)
                {
                    if (!decode_branch_target(decoder, instruction_address, ZYDIS_MNEMONIC_CALL, process_local))
                    {
                        error = "the third ProcessInternal call has no statically validated target";
                        return false;
                    }
                    process_local = resolve_jump_chain(decoder, process_local);
                    break;
                }
            }
            offset += instruction.length;
        }
        if (call_count != 3u || process_local == 0u || process_local == process_internal ||
            !executable_address(process_local))
        {
            error = "ProcessLocalScriptFunction failed the UE5.1 third-call structural gate";
            return false;
        }
        output.process_internal = process_internal;
        output.process_local_script_function = process_local;
        std::array<char, 240> detail{};
        std::snprintf(detail.data(),
                      detail.size(),
                      "ExecuteUbergraph resolved ProcessInternal=0x%llx; its third direct call resolved ProcessLocalScriptFunction=0x%llx",
                      static_cast<unsigned long long>(process_internal),
                      static_cast<unsigned long long>(process_local));
        output.detail = detail.data();
        error.clear();
        return true;
    }
}

namespace ue4ss::linux::core
{
    std::atomic<BlueprintHookManager*> BlueprintHookManager::s_active{};
    std::atomic<BlueprintHookManager::ScriptFunction> BlueprintHookManager::s_original_fallback{};
    std::mutex BlueprintHookManager::s_dispatch_mutex;

    BlueprintHookManager::~BlueprintHookManager()
    {
        stop();
    }

    bool BlueprintHookManager::start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (is_ready())
            {
                return m_runtime == &runtime;
            }
            if (!runtime.is_ready() || !resolve_blueprint_addresses(runtime, m_addresses, error))
            {
                return false;
            }
            BlueprintHookManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another Blueprint hook manager is already active";
                return false;
            }
            m_runtime = &runtime;
            m_accepting.store(true, std::memory_order_release);
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
            error = "unknown exception while starting Blueprint hooks";
            stop();
            return false;
        }
    }

#if defined(UE4SS_LINUX_TESTING)
    bool BlueprintHookManager::start_for_testing(ReadOnlyUnrealRuntime& runtime,
                                                 BlueprintExecutionAddresses addresses,
                                                 std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (is_ready())
            {
                return m_runtime == &runtime;
            }
            if (!runtime.is_ready() || addresses.process_internal == 0u ||
                addresses.process_local_script_function == 0u ||
                !executable_address(addresses.process_internal) ||
                !executable_address(addresses.process_local_script_function))
            {
                error = "test Blueprint addresses must be non-null executable functions";
                return false;
            }
            BlueprintHookManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another Blueprint hook manager is already active";
                return false;
            }
            m_addresses = std::move(addresses);
            m_runtime = &runtime;
            m_accepting.store(true, std::memory_order_release);
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
            error = "unknown exception while starting test Blueprint hooks";
            stop();
            return false;
        }
    }
#endif

    void BlueprintHookManager::stop() noexcept
    {
        m_accepting.store(false, std::memory_order_release);
        std::string hook_error;
        bool restored{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            restored = m_hook.restore(hook_error);
            if (restored && m_addresses.process_local_script_function != 0u)
            {
                s_original_fallback.store(
                        std::bit_cast<ScriptFunction>(
                                static_cast<std::uintptr_t>(m_addresses.process_local_script_function)),
                        std::memory_order_release);
            }
            BlueprintHookManager* active = this;
            static_cast<void>(s_active.compare_exchange_strong(active, nullptr, std::memory_order_acq_rel));
        }
        std::unique_lock wait_lock{m_wait_mutex};
        m_wait_wakeup.wait(wait_lock, [this] { return m_in_flight.load(std::memory_order_acquire) == 0u; });
        wait_lock.unlock();
        if (restored)
        {
            std::string release_error;
            if (!m_hook.release(release_error))
            {
                std::fprintf(stderr,
                             "[UE4SS Linux] Blueprint trampoline release failed: %s\n",
                             release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] Blueprint target restore failed; retaining trampoline: %s\n",
                         hook_error.c_str());
        }
        {
            std::lock_guard lock{m_records_mutex};
            m_records.clear();
            m_named_callbacks.clear();
        }
        m_original.store(nullptr, std::memory_order_release);
        m_runtime = nullptr;
        m_addresses = {};
    }

    bool BlueprintHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr &&
               m_addresses.process_internal != 0u && m_addresses.process_local_script_function != 0u;
    }

    const BlueprintExecutionAddresses& BlueprintHookManager::addresses() const noexcept
    {
        return m_addresses;
    }

    bool BlueprintHookManager::is_script_function(const ReadOnlyUObjectHandle& function) const noexcept
    {
        try
        {
            std::uint64_t native{};
            std::string ignored;
            return is_ready() && m_runtime->function_native_address(function, native, ignored) &&
                   native == m_addresses.process_internal;
        }
        catch (...)
        {
            return false;
        }
    }

    bool BlueprintHookManager::ensure_hook_installed(std::string& error) noexcept
    {
        if (m_hook.is_installed())
        {
            return true;
        }
        return m_hook.install(
                std::bit_cast<void*>(static_cast<std::uintptr_t>(m_addresses.process_local_script_function)),
                std::bit_cast<void*>(&BlueprintHookManager::detour),
                [this](void* trampoline) {
                    const auto original = std::bit_cast<ScriptFunction>(trampoline);
                    m_original.store(original, std::memory_order_release);
                    s_original_fallback.store(original, std::memory_order_release);
                },
                error);
    }

    std::pair<std::uint64_t, std::uint64_t> BlueprintHookManager::register_hook(
            const ReadOnlyUObjectHandle& function, Callback pre, Callback post, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_script_function(function) || (!pre && !post))
            {
                error = "a Blueprint UFunction and at least one callback are required";
                return {};
            }
            const std::uint64_t pre_id = pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const std::uint64_t post_id = post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "Blueprint callback id space is exhausted";
                return {};
            }
            std::lock_guard lock{m_records_mutex};
            if (!ensure_hook_installed(error))
            {
                return {};
            }
            const auto existing = std::find_if(m_records.begin(), m_records.end(), [&function](const auto& record) {
                return record->function.address == function.address;
            });
            FunctionRecord* record{};
            if (existing == m_records.end())
            {
                auto created = std::make_unique<FunctionRecord>();
                created->function = function;
                record = created.get();
                m_records.push_back(std::move(created));
            }
            else
            {
                record = existing->get();
            }
            auto callback = std::make_shared<CallbackRecord>();
            callback->pre_id = pre_id;
            callback->post_id = post_id;
            callback->pre = std::move(pre);
            callback->post = std::move(post);
            record->callbacks.push_back(std::move(callback));
            return {pre_id, post_id};
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return {};
        }
        catch (...)
        {
            error = "unknown exception while registering a Blueprint hook";
            return {};
        }
    }

    bool BlueprintHookManager::unregister_hook(const ReadOnlyUObjectHandle& function,
                                               std::uint64_t pre_id,
                                               std::uint64_t post_id,
                                               std::string& error) noexcept
    {
        error.clear();
        try
        {
            std::lock_guard lock{m_records_mutex};
            const auto record = std::find_if(m_records.begin(), m_records.end(), [&function](const auto& candidate) {
                return candidate->function.address == function.address;
            });
            if (record == m_records.end())
            {
                error = "Blueprint UFunction has no registered callbacks";
                return false;
            }
            const auto old_size = (*record)->callbacks.size();
            std::erase_if((*record)->callbacks, [pre_id, post_id](const auto& callback) {
                if (callback->pre_id == pre_id && callback->post_id == post_id)
                {
                    callback->active.store(false, std::memory_order_release);
                    return true;
                }
                return false;
            });
            if ((*record)->callbacks.size() == old_size)
            {
                error = "Blueprint callback ids were not found";
                return false;
            }
            return true;
        }
        catch (...)
        {
            error = "unknown exception while unregistering a Blueprint hook";
            return false;
        }
    }

    std::uint64_t BlueprintHookManager::register_named_hook(
            std::string event_name,
            Callback callback,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || event_name.empty() || !callback)
            {
                error = "an active Blueprint hook manager, event name, and callback are required";
                return 0u;
            }
            const std::uint64_t id = m_next_id.fetch_add(1u, std::memory_order_relaxed);
            if (id == 0u)
            {
                error = "Blueprint named callback id space is exhausted";
                return 0u;
            }
            std::lock_guard lock{m_records_mutex};
            if (!ensure_hook_installed(error))
            {
                return 0u;
            }
            auto record = std::make_shared<NamedCallbackRecord>();
            record->id = id;
            record->event_name = std::move(event_name);
            record->callback = std::move(callback);
            m_named_callbacks.push_back(std::move(record));
            return id;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return 0u;
        }
        catch (...)
        {
            error = "unknown exception while registering a Blueprint named hook";
            return 0u;
        }
    }

    bool BlueprintHookManager::unregister_named_hook(
            std::uint64_t callback_id,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (callback_id == 0u)
            {
                error = "a non-zero Blueprint named callback id is required";
                return false;
            }
            std::lock_guard lock{m_records_mutex};
            const auto old_size = m_named_callbacks.size();
            std::erase_if(m_named_callbacks, [callback_id](const auto& callback) {
                if (callback->id == callback_id)
                {
                    callback->active.store(false, std::memory_order_release);
                    return true;
                }
                return false;
            });
            if (m_named_callbacks.size() == old_size)
            {
                error = "Blueprint named callback id was not found";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while unregistering a Blueprint named hook";
            return false;
        }
    }

    void BlueprintHookManager::detour(void* context, void* stack, void* result) noexcept
    {
        BlueprintHookManager* active{};
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
            active->dispatch(context, stack, result);
            return;
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire); original != nullptr)
        {
            original(context, stack, result);
        }
    }

    void BlueprintHookManager::dispatch(void* context, void* stack, void* result) noexcept
    {
        const ScriptFunction original = m_original.load(std::memory_order_acquire);
        std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        std::vector<std::shared_ptr<NamedCallbackRecord>> named_callbacks;
        ReadOnlyUObjectHandle function;
        bool original_called{};
        try
        {
            std::uint64_t function_address{};
            const std::uint64_t stack_address = std::bit_cast<std::uintptr_t>(stack);
            if (stack != nullptr)
            {
                // ProcessLocalScriptFunction executes calls nested in the
                // current frame. Node is commonly the outer graph, whereas
                // CurrentNativeFunction (or the UFunction stored immediately
                // before Code) identifies the function actually being called.
                static_cast<void>(safe_read(stack_address + k_ue51_fframe_current_native_function_offset,
                                            function_address));
                if (function_address == 0u)
                {
                    std::uint64_t code{};
                    if (safe_read(stack_address + k_ue51_fframe_code_offset, code) && code >= sizeof(std::uint64_t))
                    {
                        static_cast<void>(safe_read(code - sizeof(std::uint64_t), function_address));
                    }
                }
                if (function_address == 0u)
                {
                    static_cast<void>(safe_read(stack_address + k_ue51_fframe_node_offset, function_address));
                }
            }
            if (function_address != 0u && m_runtime != nullptr &&
                m_runtime->handle_from_address(function_address, function))
            {
                std::string function_name;
                ReadOnlyUObjectDescription description;
                std::string description_error;
                if (m_runtime->describe_object(function, description, description_error))
                {
                    function_name = std::move(description.name);
                }
                std::lock_guard lock{m_records_mutex};
                const auto record = std::find_if(m_records.begin(), m_records.end(), [function_address](const auto& candidate) {
                    return candidate->function.address == function_address;
                });
                if (record != m_records.end())
                {
                    callbacks = (*record)->callbacks;
                }
                if (!function_name.empty())
                {
                    for (const auto& named : m_named_callbacks)
                    {
                        if (ascii_case_insensitive_equal(named->event_name, function_name))
                        {
                            named_callbacks.push_back(named);
                        }
                    }
                }
            }
            UFunctionHookInvocation invocation{
                    .function = function,
                    .stack_address = std::bit_cast<std::uintptr_t>(stack),
                    .result_address = std::bit_cast<std::uintptr_t>(result),
            };
            if (m_runtime != nullptr && context != nullptr)
            {
                static_cast<void>(m_runtime->handle_from_address(std::bit_cast<std::uintptr_t>(context), invocation.context));
            }
            if (m_accepting.load(std::memory_order_acquire))
            {
                for (const auto& callback : callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->pre)
                    {
                        try { callback->pre(invocation); } catch (...) {}
                    }
                }
                for (const auto& callback : named_callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->callback)
                    {
                        try { callback->callback(invocation); } catch (...) {}
                    }
                }
            }
            if (original != nullptr)
            {
                original_called = true;
                original(context, stack, result);
            }
            if (m_accepting.load(std::memory_order_acquire))
            {
                for (const auto& callback : callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->post)
                    {
                        try { callback->post(invocation); } catch (...) {}
                    }
                }
            }
        }
        catch (...)
        {
            if (!original_called && original != nullptr)
            {
                try { original(context, stack, result); } catch (...) {}
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
    }
} // namespace ue4ss::linux::core
