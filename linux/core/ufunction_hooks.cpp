#include "ufunction_hooks.hpp"

#include <algorithm>
#include <bit>
#include <exception>
#include <limits>
#include <stdexcept>
#include <sys/uio.h>
#include <unistd.h>

namespace
{
    constexpr std::uint64_t k_ue51_ufunction_func_offset = 0xd8u;
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
}

namespace ue4ss::linux::core
{
    std::atomic<UFunctionHookManager*> UFunctionHookManager::s_active{};
    std::mutex UFunctionHookManager::s_dispatch_mutex;
    std::array<std::atomic<UFunctionHookManager::NativeFunction>, UFunctionHookManager::k_max_hooked_functions>
            UFunctionHookManager::s_originals{};

    UFunctionHookManager::~UFunctionHookManager()
    {
        stop();
    }

    bool UFunctionHookManager::start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (is_ready())
            {
                return m_runtime == &runtime;
            }
            if (!runtime.is_ready())
            {
                error = "validated UObject runtime gate is required before UFunction hooks";
                return false;
            }
            UFunctionHookManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another UFunction hook manager is already active";
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
            error = "unknown exception while starting the UFunction hook manager";
            stop();
            return false;
        }
    }

    void UFunctionHookManager::stop() noexcept
    {
        m_accepting.store(false, std::memory_order_release);
        {
            std::lock_guard lock{m_records_mutex};
            for (auto& record : m_records)
            {
                std::string ignored;
                static_cast<void>(record->hook.remove(ignored));
            }
        }
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            UFunctionHookManager* active = this;
            static_cast<void>(s_active.compare_exchange_strong(active, nullptr, std::memory_order_acq_rel));
        }
        std::unique_lock wait_lock{m_wait_mutex};
        m_wait_wakeup.wait(wait_lock, [this] { return m_in_flight.load(std::memory_order_acquire) == 0u; });
        wait_lock.unlock();
        {
            std::lock_guard lock{m_records_mutex};
            m_records.clear();
        }
        m_runtime = nullptr;
    }

    bool UFunctionHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr;
    }

    std::pair<std::uint64_t, std::uint64_t> UFunctionHookManager::register_hook(
            const ReadOnlyUObjectHandle& function,
            Callback pre,
            Callback post,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || m_runtime == nullptr || !m_runtime->is_valid(function))
            {
                error = "active UFunction hook manager and live function are required";
                return {};
            }
            std::string class_error;
            if (!m_runtime->is_a(function, "Function", class_error))
            {
                error = class_error.empty() ? "hook target is not a UFunction" : class_error;
                return {};
            }
            if (!pre && !post)
            {
                error = "at least one UFunction callback is required";
                return {};
            }
            const std::uint64_t pre_id = pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const std::uint64_t post_id = post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "UFunction callback id space is exhausted";
                return {};
            }

            std::lock_guard lock{m_records_mutex};
            if (!m_accepting.load(std::memory_order_acquire))
            {
                error = "UFunction hook manager is stopping";
                return {};
            }
            const auto existing = std::find_if(m_records.begin(), m_records.end(), [&function](const auto& record) {
                return record->function.address == function.address;
            });
            if (existing != m_records.end())
            {
                auto callback = std::make_shared<CallbackRecord>();
                callback->pre_id = pre_id;
                callback->post_id = post_id;
                callback->pre = std::move(pre);
                callback->post = std::move(post);
                (*existing)->callbacks.push_back(std::move(callback));
                return {pre_id, post_id};
            }

            const std::uint64_t slot_address = function.address + k_ue51_ufunction_func_offset;
            std::uint64_t original_address{};
            if (slot_address < function.address || !safe_read(slot_address, original_address) || original_address == 0u)
            {
                error = "UFunction::Func slot is unreadable or null";
                return {};
            }
            auto record = std::make_unique<FunctionRecord>();
            record->function = function;
            record->original = std::bit_cast<NativeFunction>(static_cast<std::uintptr_t>(original_address));
            record->slot_index = m_records.size();
            if (record->slot_index >= k_max_hooked_functions)
            {
                error = "the native UFunction hook limit of " + std::to_string(k_max_hooked_functions) + " was reached";
                return {};
            }
            auto callback = std::make_shared<CallbackRecord>();
            callback->pre_id = pre_id;
            callback->post_id = post_id;
            callback->pre = std::move(pre);
            callback->post = std::move(post);
            record->callbacks.push_back(std::move(callback));
            FunctionRecord* published = record.get();
            m_records.push_back(std::move(record));
            s_originals[published->slot_index].store(published->original, std::memory_order_release);
            if (!published->hook.install(
                        reinterpret_cast<void**>(static_cast<std::uintptr_t>(slot_address)),
                        std::bit_cast<void*>(detour_table()[published->slot_index]),
                        error))
            {
                s_originals[published->slot_index].store(nullptr, std::memory_order_release);
                m_records.pop_back();
                return {};
            }
            return {pre_id, post_id};
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return {};
        }
        catch (...)
        {
            error = "unknown exception while registering a UFunction hook";
            return {};
        }
    }

    bool UFunctionHookManager::unregister_hook(const ReadOnlyUObjectHandle& function,
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
                error = "UFunction has no registered callbacks";
                return false;
            }
            auto& callbacks = (*record)->callbacks;
            const auto old_size = callbacks.size();
            std::erase_if(callbacks, [pre_id, post_id](const auto& callback) {
                if (callback->pre_id == pre_id && callback->post_id == post_id)
                {
                    callback->active.store(false, std::memory_order_release);
                    return true;
                }
                return false;
            });
            if (callbacks.size() == old_size)
            {
                error = "UFunction callback ids were not found";
                return false;
            }
            // Keep the atomic pointer hook installed until manager shutdown.
            // This avoids racing a copied callback snapshot during a Lua-side
            // UnregisterHook call.
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while unregistering a UFunction hook";
            return false;
        }
    }

    std::size_t UFunctionHookManager::hooked_function_count() const noexcept
    {
        try
        {
            std::lock_guard lock{m_records_mutex};
            return m_records.size();
        }
        catch (...)
        {
            return 0u;
        }
    }

    template <std::size_t SlotIndex>
    void UFunctionHookManager::detour_slot(void* context, void* stack, void* result) noexcept
    {
        UFunctionHookManager* active{};
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
            active->dispatch(SlotIndex, context, stack, result);
            return;
        }
        const NativeFunction original = s_originals[SlotIndex].load(std::memory_order_acquire);
        if (original != nullptr)
        {
            original(context, stack, result);
        }
    }

    const std::array<UFunctionHookManager::NativeFunction, UFunctionHookManager::k_max_hooked_functions>&
    UFunctionHookManager::detour_table() noexcept
    {
        static const auto table = []<std::size_t... SlotIndices>(std::index_sequence<SlotIndices...>) {
            return std::array<NativeFunction, k_max_hooked_functions>{&UFunctionHookManager::detour_slot<SlotIndices>...};
        }(std::make_index_sequence<k_max_hooked_functions>{});
        return table;
    }

    void UFunctionHookManager::dispatch(
            std::size_t slot_index, void* context, void* stack, void* result) noexcept
    {
        bool original_called{};
        DispatchSnapshot snapshot;
        try
        {
            {
                std::lock_guard lock{m_records_mutex};
                if (slot_index >= m_records.size() || m_records[slot_index] == nullptr)
                {
                    throw std::runtime_error{"detoured UFunction slot has no published hook record"};
                }
                const auto& record = m_records[slot_index];
                snapshot = {record->function, record->original, record->callbacks};
            }

            UFunctionHookInvocation invocation{
                    .function = snapshot.function,
                    .stack_address = std::bit_cast<std::uintptr_t>(stack),
                    .result_address = std::bit_cast<std::uintptr_t>(result),
            };
            if (m_runtime != nullptr && context != nullptr)
            {
                static_cast<void>(m_runtime->handle_from_address(std::bit_cast<std::uintptr_t>(context), invocation.context));
            }
            if (m_accepting.load(std::memory_order_acquire))
            {
                for (auto& callback : snapshot.callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->pre)
                    {
                        try
                        {
                            callback->pre(invocation);
                        }
                        catch (...)
                        {
                        }
                    }
                }
            }
            if (snapshot.original != nullptr)
            {
                original_called = true;
                snapshot.original(context, stack, result);
            }
            if (m_accepting.load(std::memory_order_acquire))
            {
                for (auto& callback : snapshot.callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->post)
                    {
                        try
                        {
                            callback->post(invocation);
                        }
                        catch (...)
                        {
                        }
                    }
                }
            }
        }
        catch (...)
        {
            if (!original_called && snapshot.original != nullptr)
            {
                try
                {
                    snapshot.original(context, stack, result);
                }
                catch (...)
                {
                }
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
    }
}
