#include "load_map_hooks.hpp"

#include <algorithm>
#include <array>
#include <bit>
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
    constexpr std::uint64_t k_ue51_linux_engine_tick_vtable_offset = 0x2f8u;
    constexpr std::uint64_t k_ue51_linux_engine_load_map_vtable_offset = 0x4c8u;
    constexpr std::uint32_t k_rf_class_default_object = 0x10u;

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
}

namespace ue4ss::linux::core
{
    std::atomic<LoadMapHookManager*> LoadMapHookManager::s_active{};
    std::atomic<LoadMapHookManager::LoadMapFunction> LoadMapHookManager::s_original_fallback{};
    std::mutex LoadMapHookManager::s_dispatch_mutex;

    LoadMapHookManager::~LoadMapHookManager()
    {
        stop();
    }

    bool LoadMapHookManager::start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (is_ready()) return m_runtime == &runtime;
            if (!runtime.is_ready())
            {
                error = "validated Unreal 5.1 runtime is required for UEngine::LoadMap";
                return false;
            }

            ReadOnlyUObjectHandle engine;
            const auto live_query = runtime.find_objects(
                    1u,
                    std::optional<std::string_view>{"GameEngine"},
                    std::nullopt,
                    0u,
                    k_rf_class_default_object,
                    false);
            if (live_query.success && !live_query.objects.empty())
            {
                engine = live_query.objects.front();
            }
            else
            {
                const auto cdo_query = runtime.find_by_path("/Script/Engine.Default__GameEngine", 1u);
                if (!cdo_query.success || cdo_query.objects.empty())
                {
                    error = "UGameEngine instance/CDO is unavailable: " +
                            (live_query.detail.empty() ? cdo_query.detail : live_query.detail);
                    return false;
                }
                engine = cdo_query.objects.front();
            }
            std::string class_error;
            if (!runtime.is_a(engine, "GameEngine", class_error))
            {
                error = class_error.empty() ? "engine candidate failed the UGameEngine identity gate"
                                            : class_error;
                return false;
            }

            const auto package_query = runtime.find_all_of("Package", 1u);
            std::uint64_t package_vtable{};
            std::uint64_t process_event_slot{};
            if (!runtime.process_event_available() || !package_query.success || package_query.objects.empty() ||
                !safe_read(package_query.objects.front().address, package_vtable) || package_vtable == 0u ||
                !safe_read(package_vtable + k_ue51_linux_process_event_vtable_offset, process_event_slot) ||
                process_event_slot != std::bit_cast<std::uintptr_t>(runtime.process_event_address()))
            {
                error = "UPackage failed the UE 5.1 Linux Itanium vtable shift gate at ProcessEvent +0x268";
                return false;
            }

            std::uint64_t vtable{};
            std::uint64_t tick_target{};
            std::uint64_t target{};
            if (!safe_read(engine.address, vtable) || vtable == 0u)
            {
                error = "UGameEngine candidate has no readable Linux vtable";
                return false;
            }
            if (!safe_read(vtable + k_ue51_linux_engine_tick_vtable_offset, tick_target) ||
                tick_target == 0u || !executable_address(tick_target))
            {
                error = "UGameEngine::Tick slot +0x2f8 failed the LoadMap ABI gate";
                return false;
            }
            if (!safe_read(vtable + k_ue51_linux_engine_load_map_vtable_offset, target) || target == 0u)
            {
                error = "UGameEngine::LoadMap Linux vtable slot +0x4c8 is null or unreadable";
                return false;
            }
            if (target == tick_target || !executable_address(target))
            {
                error = "UGameEngine::LoadMap candidate is not a distinct executable vtable target";
                return false;
            }

            LoadMapHookManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another UEngine::LoadMap hook manager is already active";
                return false;
            }
            m_runtime = &runtime;
            m_target = target;
            m_accepting.store(true, std::memory_order_release);
            if (!m_hook.install(
                        std::bit_cast<void*>(static_cast<std::uintptr_t>(target)),
                        std::bit_cast<void*>(&LoadMapHookManager::detour),
                        [this](void* trampoline) {
                            const auto original = std::bit_cast<LoadMapFunction>(trampoline);
                            m_original.store(original, std::memory_order_release);
                            s_original_fallback.store(original, std::memory_order_release);
                        },
                        error))
            {
                std::ostringstream contextual_error;
                contextual_error << "UGameEngine vtable=0x" << std::hex << vtable
                                 << " candidate(+0x4c8)=0x" << target << ": " << error;
                error = contextual_error.str();
                m_accepting.store(false, std::memory_order_release);
                m_runtime = nullptr;
                m_target = 0u;
                LoadMapHookManager* active = this;
                static_cast<void>(s_active.compare_exchange_strong(active, nullptr, std::memory_order_acq_rel));
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
            error = "unknown exception while starting UEngine::LoadMap hooks";
            stop();
            return false;
        }
    }

    void LoadMapHookManager::stop() noexcept
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
                        std::bit_cast<LoadMapFunction>(static_cast<std::uintptr_t>(m_target)),
                        std::memory_order_release);
            }
            LoadMapHookManager* active = this;
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
                             "[UE4SS Linux] UEngine::LoadMap trampoline release failed: %s\n",
                             release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] UEngine::LoadMap restore failed; retaining trampoline: %s\n",
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

    bool LoadMapHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr && m_target != 0u &&
               m_hook.is_installed() && m_original.load(std::memory_order_acquire) != nullptr;
    }

    std::uint64_t LoadMapHookManager::target_address() const noexcept
    {
        return m_target;
    }

    std::pair<std::uint64_t, std::uint64_t> LoadMapHookManager::register_hook(
            Callback pre, Callback post, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || (!pre && !post))
            {
                error = "active UEngine::LoadMap manager and at least one callback are required";
                return {};
            }
            const std::uint64_t pre_id = pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const std::uint64_t post_id = post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "UEngine::LoadMap callback id space is exhausted";
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
                error = "UEngine::LoadMap manager stopped during callback registration";
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
            error = "unknown exception while registering a UEngine::LoadMap callback";
            return {};
        }
    }

    bool LoadMapHookManager::unregister_hook(std::uint64_t pre_id,
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
            if (m_callbacks.size() == old_size)
            {
                error = "UEngine::LoadMap callback ids were not found";
                return false;
            }
            return true;
        }
        catch (...)
        {
            error = "unknown exception while unregistering a UEngine::LoadMap callback";
            return false;
        }
    }

    bool LoadMapHookManager::snapshot_invocation(
            void* engine,
            void* world_context,
            void* url,
            void* pending_game,
            void* error_string,
            LoadMapHookInvocation& output,
            std::string& error) const noexcept
    {
        output = {};
        error.clear();
        if (m_runtime == nullptr || engine == nullptr || world_context == nullptr ||
            url == nullptr || error_string == nullptr ||
            !m_runtime->handle_from_address(std::bit_cast<std::uintptr_t>(engine), output.engine))
        {
            error = "LoadMap invocation contains an invalid UEngine or native argument pointer";
            return false;
        }
        std::string class_error;
        if (!m_runtime->is_a(output.engine, "Engine", class_error))
        {
            error = class_error.empty() ? "LoadMap context failed the UEngine identity gate"
                                        : std::move(class_error);
            return false;
        }
        if (!m_runtime->world_from_native_context(
                    std::bit_cast<std::uintptr_t>(world_context), output.world, error) ||
            !m_runtime->snapshot_native_furl(
                    std::bit_cast<std::uintptr_t>(url), output.url, error) ||
            !m_runtime->snapshot_native_fstring(
                    std::bit_cast<std::uintptr_t>(error_string), output.error, error))
        {
            return false;
        }
        if (pending_game != nullptr &&
            !m_runtime->handle_from_address(
                    std::bit_cast<std::uintptr_t>(pending_game), output.pending_game))
        {
            error = "LoadMap PendingGame points outside the validated GUObjectArray";
            return false;
        }
        return true;
    }

    bool LoadMapHookManager::detour(void* engine,
                                    void* world_context,
                                    void* url,
                                    void* pending_game,
                                    void* error_string) noexcept
    {
        LoadMapHookManager* active{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            active = s_active.load(std::memory_order_acquire);
            if (active != nullptr) active->m_in_flight.fetch_add(1u, std::memory_order_acq_rel);
        }
        if (active != nullptr)
        {
            return active->dispatch(engine, world_context, url, pending_game, error_string);
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire); original != nullptr)
        {
            return original(engine, world_context, url, pending_game, error_string);
        }
        return false;
    }

    bool LoadMapHookManager::dispatch(void* engine,
                                      void* world_context,
                                      void* url,
                                      void* pending_game,
                                      void* error_string) noexcept
    {
        const LoadMapFunction original = m_original.load(std::memory_order_acquire);
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
            LoadMapHookInvocation invocation;
            std::string snapshot_error;
            if (m_accepting.load(std::memory_order_acquire) &&
                snapshot_invocation(engine, world_context, url, pending_game, error_string,
                                    invocation, snapshot_error))
            {
                for (const auto& callback : callbacks)
                {
                    if (!callback->active.load(std::memory_order_acquire) || !callback->pre) continue;
                    try
                    {
                        const auto candidate = callback->pre(invocation);
                        if (!override.has_value() && candidate.has_value()) override = candidate;
                    }
                    catch (...) {}
                }
            }
            if (original != nullptr)
            {
                original_called = true;
                original_result = original(engine, world_context, url, pending_game, error_string);
            }
            if (m_accepting.load(std::memory_order_acquire) &&
                snapshot_invocation(engine, world_context, url, pending_game, error_string,
                                    invocation, snapshot_error))
            {
                for (const auto& callback : callbacks)
                {
                    if (!callback->active.load(std::memory_order_acquire) || !callback->post) continue;
                    try
                    {
                        const auto candidate = callback->post(invocation);
                        if (!override.has_value() && candidate.has_value()) override = candidate;
                    }
                    catch (...) {}
                }
            }
        }
        catch (...)
        {
            if (!original_called && original != nullptr)
            {
                try { original_result = original(engine, world_context, url, pending_game, error_string); }
                catch (...) { original_result = false; }
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
        return override.value_or(original_result);
    }
} // namespace ue4ss::linux::core
