#include "begin_play_hooks.hpp"

#include <array>
#include <algorithm>
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
    constexpr std::uint64_t k_ue51_linux_actor_begin_play_vtable_offset = 0x388u;
    constexpr std::uint64_t k_ue51_linux_actor_end_play_vtable_offset = 0x390u;
    constexpr std::uint64_t k_ue51_linux_game_mode_init_game_state_vtable_offset = 0x740u;
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
    std::atomic<BeginPlayHookManager*> BeginPlayHookManager::s_active{};
    std::atomic<BeginPlayHookManager::BeginPlayFunction> BeginPlayHookManager::s_original_fallback{};
    std::mutex BeginPlayHookManager::s_dispatch_mutex;
    std::atomic<EndPlayHookManager*> EndPlayHookManager::s_active{};
    std::atomic<EndPlayHookManager::EndPlayFunction> EndPlayHookManager::s_original_fallback{};
    std::mutex EndPlayHookManager::s_dispatch_mutex;
    std::atomic<InitGameStateHookManager*> InitGameStateHookManager::s_active{};
    std::atomic<InitGameStateHookManager::InitGameStateFunction>
            InitGameStateHookManager::s_original_fallback{};
    std::mutex InitGameStateHookManager::s_dispatch_mutex;

    BeginPlayHookManager::~BeginPlayHookManager()
    {
        stop();
    }

    bool BeginPlayHookManager::start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept
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
                error = "validated Unreal 5.1 runtime is required for AActor::BeginPlay";
                return false;
            }

            const auto actor_cdo_query = runtime.find_by_path("/Script/Engine.Default__Actor", 1u);
            if (!actor_cdo_query.success || actor_cdo_query.objects.empty())
            {
                error = "AActor CDO is unavailable: " + actor_cdo_query.detail;
                return false;
            }
            const ReadOnlyUObjectHandle& actor_cdo = actor_cdo_query.objects.front();
            std::string class_error;
            if (!runtime.is_a(actor_cdo, "Actor", class_error))
            {
                error = class_error.empty() ? "Default__Actor failed the AActor identity gate" : class_error;
                return false;
            }

            std::uint64_t vtable{};
            std::uint64_t target{};
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
            if (!safe_read(actor_cdo.address, vtable) || vtable == 0u)
            {
                error = "AActor CDO has no readable Linux vtable";
                return false;
            }
            if (!safe_read(vtable + k_ue51_linux_actor_begin_play_vtable_offset, target) || target == 0u)
            {
                error = "AActor::BeginPlay Linux vtable slot +0x388 is null or unreadable";
                return false;
            }
            if (!executable_address(target))
            {
                error = "AActor::BeginPlay vtable target is not in a readable executable mapping";
                return false;
            }

            BeginPlayHookManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another AActor::BeginPlay hook manager is already active";
                return false;
            }

            m_runtime = &runtime;
            m_target = target;
            m_accepting.store(true, std::memory_order_release);
            if (!m_hook.install(
                        std::bit_cast<void*>(static_cast<std::uintptr_t>(target)),
                        std::bit_cast<void*>(&BeginPlayHookManager::detour),
                        [this](void* trampoline) {
                            const auto original = std::bit_cast<BeginPlayFunction>(trampoline);
                            m_original.store(original, std::memory_order_release);
                            s_original_fallback.store(original, std::memory_order_release);
                        },
                        error))
            {
                std::ostringstream contextual_error;
                contextual_error << "AActor CDO vtable=0x" << std::hex << vtable
                                 << " candidate(+0x388)=0x" << target << ": " << error;
                error = contextual_error.str();
                m_accepting.store(false, std::memory_order_release);
                m_runtime = nullptr;
                m_target = 0u;
                BeginPlayHookManager* active = this;
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
            error = "unknown exception while starting AActor::BeginPlay hooks";
            stop();
            return false;
        }
    }

    void BeginPlayHookManager::stop() noexcept
    {
        m_accepting.store(false, std::memory_order_release);
        std::string hook_error;
        bool restored{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            // Keep the trampoline executable until every detour which already
            // acquired this manager has returned. A thread which fetched the
            // old entry jump but reaches detour() after this critical section
            // uses the restored native target through the static fallback.
            restored = m_hook.restore(hook_error);
            if (restored && m_target != 0u)
            {
                s_original_fallback.store(
                        std::bit_cast<BeginPlayFunction>(static_cast<std::uintptr_t>(m_target)),
                        std::memory_order_release);
            }
            BeginPlayHookManager* active = this;
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
                             "[UE4SS Linux] AActor::BeginPlay trampoline release failed: %s\n",
                             release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            // Failing closed here means retaining the executable trampoline.
            // The core is RTLD_NODELETE, so a late detour remains valid even
            // when a third party changed the target bytes behind us.
            std::fprintf(stderr,
                         "[UE4SS Linux] AActor::BeginPlay restore failed; retaining trampoline: %s\n",
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

    bool BeginPlayHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr && m_target != 0u &&
               m_hook.is_installed() && m_original.load(std::memory_order_acquire) != nullptr;
    }

    std::uint64_t BeginPlayHookManager::target_address() const noexcept
    {
        return m_target;
    }

    std::pair<std::uint64_t, std::uint64_t> BeginPlayHookManager::register_hook(
            Callback pre, Callback post, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || (!pre && !post))
            {
                error = "active AActor::BeginPlay manager and at least one callback are required";
                return {};
            }
            const std::uint64_t pre_id = pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const std::uint64_t post_id = post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "AActor::BeginPlay callback id space is exhausted";
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
                error = "AActor::BeginPlay manager stopped during callback registration";
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
            error = "unknown exception while registering an AActor::BeginPlay callback";
            return {};
        }
    }

    bool BeginPlayHookManager::unregister_hook(std::uint64_t pre_id,
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
                error = "AActor::BeginPlay callback ids were not found";
                return false;
            }
            return true;
        }
        catch (...)
        {
            error = "unknown exception while unregistering an AActor::BeginPlay callback";
            return false;
        }
    }

    void BeginPlayHookManager::detour(void* actor) noexcept
    {
        BeginPlayHookManager* active{};
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
            active->dispatch(actor);
            return;
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire); original != nullptr)
        {
            original(actor);
        }
    }

    void BeginPlayHookManager::dispatch(void* actor) noexcept
    {
        const BeginPlayFunction original = m_original.load(std::memory_order_acquire);
        std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        ReadOnlyUObjectHandle actor_handle;
        bool original_called{};
        try
        {
            if (m_runtime != nullptr && actor != nullptr)
            {
                static_cast<void>(m_runtime->handle_from_address(
                        std::bit_cast<std::uintptr_t>(actor), actor_handle));
            }
            {
                std::lock_guard lock{m_callbacks_mutex};
                callbacks = m_callbacks;
            }
            if (m_accepting.load(std::memory_order_acquire) && actor_handle.address != 0u)
            {
                for (const auto& callback : callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->pre)
                    {
                        try { callback->pre(actor_handle); } catch (...) {}
                    }
                }
            }
            if (original != nullptr)
            {
                original_called = true;
                original(actor);
            }
            if (m_accepting.load(std::memory_order_acquire) && actor_handle.address != 0u)
            {
                for (const auto& callback : callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->post)
                    {
                        try { callback->post(actor_handle); } catch (...) {}
                    }
                }
            }
        }
        catch (...)
        {
            if (!original_called && original != nullptr)
            {
                try { original(actor); } catch (...) {}
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
    }

    EndPlayHookManager::~EndPlayHookManager()
    {
        stop();
    }

    bool EndPlayHookManager::start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept
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
                error = "validated Unreal 5.1 runtime is required for AActor::EndPlay";
                return false;
            }

            const auto actor_cdo_query = runtime.find_by_path("/Script/Engine.Default__Actor", 1u);
            if (!actor_cdo_query.success || actor_cdo_query.objects.empty())
            {
                error = "AActor CDO is unavailable: " + actor_cdo_query.detail;
                return false;
            }
            const ReadOnlyUObjectHandle& actor_cdo = actor_cdo_query.objects.front();
            std::string class_error;
            if (!runtime.is_a(actor_cdo, "Actor", class_error))
            {
                error = class_error.empty() ? "Default__Actor failed the AActor identity gate" : class_error;
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
            std::uint64_t begin_play_target{};
            std::uint64_t target{};
            if (!safe_read(actor_cdo.address, vtable) || vtable == 0u)
            {
                error = "AActor CDO has no readable Linux vtable";
                return false;
            }
            if (!safe_read(vtable + k_ue51_linux_actor_begin_play_vtable_offset, begin_play_target) ||
                begin_play_target == 0u || !executable_address(begin_play_target))
            {
                error = "AActor::BeginPlay predecessor slot +0x388 failed the EndPlay ABI gate";
                return false;
            }
            if (!safe_read(vtable + k_ue51_linux_actor_end_play_vtable_offset, target) || target == 0u)
            {
                error = "AActor::EndPlay Linux vtable slot +0x390 is null or unreadable";
                return false;
            }
            if (target == begin_play_target || !executable_address(target))
            {
                error = "AActor::EndPlay candidate is not a distinct executable vtable target";
                return false;
            }

            EndPlayHookManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another AActor::EndPlay hook manager is already active";
                return false;
            }

            m_runtime = &runtime;
            m_target = target;
            m_accepting.store(true, std::memory_order_release);
            if (!m_hook.install(
                        std::bit_cast<void*>(static_cast<std::uintptr_t>(target)),
                        std::bit_cast<void*>(&EndPlayHookManager::detour),
                        [this](void* trampoline) {
                            const auto original = std::bit_cast<EndPlayFunction>(trampoline);
                            m_original.store(original, std::memory_order_release);
                            s_original_fallback.store(original, std::memory_order_release);
                        },
                        error))
            {
                std::ostringstream contextual_error;
                contextual_error << "AActor CDO vtable=0x" << std::hex << vtable
                                 << " candidate(+0x390)=0x" << target << ": " << error;
                error = contextual_error.str();
                m_accepting.store(false, std::memory_order_release);
                m_runtime = nullptr;
                m_target = 0u;
                EndPlayHookManager* active = this;
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
            error = "unknown exception while starting AActor::EndPlay hooks";
            stop();
            return false;
        }
    }

    void EndPlayHookManager::stop() noexcept
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
                        std::bit_cast<EndPlayFunction>(static_cast<std::uintptr_t>(m_target)),
                        std::memory_order_release);
            }
            EndPlayHookManager* active = this;
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
                             "[UE4SS Linux] AActor::EndPlay trampoline release failed: %s\n",
                             release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] AActor::EndPlay restore failed; retaining trampoline: %s\n",
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

    bool EndPlayHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr && m_target != 0u &&
               m_hook.is_installed() && m_original.load(std::memory_order_acquire) != nullptr;
    }

    std::uint64_t EndPlayHookManager::target_address() const noexcept
    {
        return m_target;
    }

    std::pair<std::uint64_t, std::uint64_t> EndPlayHookManager::register_hook(
            Callback pre, Callback post, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || (!pre && !post))
            {
                error = "active AActor::EndPlay manager and at least one callback are required";
                return {};
            }
            const std::uint64_t pre_id = pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const std::uint64_t post_id = post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "AActor::EndPlay callback id space is exhausted";
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
                error = "AActor::EndPlay manager stopped during callback registration";
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
            error = "unknown exception while registering an AActor::EndPlay callback";
            return {};
        }
    }

    bool EndPlayHookManager::unregister_hook(std::uint64_t pre_id,
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
                error = "AActor::EndPlay callback ids were not found";
                return false;
            }
            return true;
        }
        catch (...)
        {
            error = "unknown exception while unregistering an AActor::EndPlay callback";
            return false;
        }
    }

    void EndPlayHookManager::detour(void* actor, std::int32_t reason) noexcept
    {
        EndPlayHookManager* active{};
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
            active->dispatch(actor, reason);
            return;
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire); original != nullptr)
        {
            original(actor, reason);
        }
    }

    void EndPlayHookManager::dispatch(void* actor, std::int32_t reason) noexcept
    {
        const EndPlayFunction original = m_original.load(std::memory_order_acquire);
        std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        ReadOnlyUObjectHandle actor_handle;
        bool original_called{};
        try
        {
            if (m_runtime != nullptr && actor != nullptr)
            {
                static_cast<void>(m_runtime->handle_from_address(
                        std::bit_cast<std::uintptr_t>(actor), actor_handle));
            }
            {
                std::lock_guard lock{m_callbacks_mutex};
                callbacks = m_callbacks;
            }
            if (m_accepting.load(std::memory_order_acquire) && actor_handle.address != 0u)
            {
                for (const auto& callback : callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->pre)
                    {
                        try { callback->pre(actor_handle, reason); } catch (...) {}
                    }
                }
            }
            if (original != nullptr)
            {
                original_called = true;
                original(actor, reason);
            }
            if (m_accepting.load(std::memory_order_acquire) && actor_handle.address != 0u)
            {
                for (const auto& callback : callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->post)
                    {
                        try { callback->post(actor_handle, reason); } catch (...) {}
                    }
                }
            }
        }
        catch (...)
        {
            if (!original_called && original != nullptr)
            {
                try { original(actor, reason); } catch (...) {}
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
    }

    InitGameStateHookManager::~InitGameStateHookManager()
    {
        stop();
    }

    bool InitGameStateHookManager::start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept
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
                error = "validated Unreal 5.1 runtime is required for AGameModeBase::InitGameState";
                return false;
            }

            ReadOnlyUObjectHandle game_mode;
            const auto live_query = runtime.find_objects(
                    1u,
                    std::optional<std::string_view>{"GameModeBase"},
                    std::nullopt,
                    0u,
                    k_rf_class_default_object,
                    false);
            if (live_query.success && !live_query.objects.empty())
            {
                game_mode = live_query.objects.front();
            }
            else
            {
                auto cdo_query = runtime.find_by_path("/Script/Engine.Default__GameModeBase", 1u);
                if (!cdo_query.success || cdo_query.objects.empty())
                {
                    cdo_query = runtime.find_by_path("/Script/Engine.Default__GameMode", 1u);
                }
                if (!cdo_query.success || cdo_query.objects.empty())
                {
                    error = "AGameModeBase instance/CDO is unavailable: " +
                            (live_query.detail.empty() ? cdo_query.detail : live_query.detail);
                    return false;
                }
                game_mode = cdo_query.objects.front();
            }
            std::string class_error;
            if (!runtime.is_a(game_mode, "GameModeBase", class_error))
            {
                error = class_error.empty() ? "GameMode candidate failed the AGameModeBase identity gate" : class_error;
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
            std::uint64_t inherited_begin_play{};
            std::uint64_t inherited_end_play{};
            std::uint64_t target{};
            if (!safe_read(game_mode.address, vtable) || vtable == 0u)
            {
                error = "AGameModeBase candidate has no readable Linux vtable";
                return false;
            }
            if (!safe_read(vtable + k_ue51_linux_actor_begin_play_vtable_offset, inherited_begin_play) ||
                !safe_read(vtable + k_ue51_linux_actor_end_play_vtable_offset, inherited_end_play) ||
                inherited_begin_play == 0u || inherited_end_play == 0u ||
                !executable_address(inherited_begin_play) || !executable_address(inherited_end_play))
            {
                error = "AGameModeBase candidate failed its inherited AActor vtable layout gate";
                return false;
            }
            if (!safe_read(vtable + k_ue51_linux_game_mode_init_game_state_vtable_offset, target) || target == 0u)
            {
                error = "AGameModeBase::InitGameState Linux vtable slot +0x740 is null or unreadable";
                return false;
            }
            if (target == inherited_begin_play || target == inherited_end_play || !executable_address(target))
            {
                error = "AGameModeBase::InitGameState candidate is not a distinct executable vtable target";
                return false;
            }

            InitGameStateHookManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another AGameModeBase::InitGameState hook manager is already active";
                return false;
            }

            m_runtime = &runtime;
            m_target = target;
            m_accepting.store(true, std::memory_order_release);
            if (!m_hook.install(
                        std::bit_cast<void*>(static_cast<std::uintptr_t>(target)),
                        std::bit_cast<void*>(&InitGameStateHookManager::detour),
                        [this](void* trampoline) {
                            const auto original = std::bit_cast<InitGameStateFunction>(trampoline);
                            m_original.store(original, std::memory_order_release);
                            s_original_fallback.store(original, std::memory_order_release);
                        },
                        error))
            {
                std::ostringstream contextual_error;
                contextual_error << "AGameModeBase vtable=0x" << std::hex << vtable
                                 << " candidate(+0x740)=0x" << target << ": " << error;
                error = contextual_error.str();
                m_accepting.store(false, std::memory_order_release);
                m_runtime = nullptr;
                m_target = 0u;
                InitGameStateHookManager* active = this;
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
            error = "unknown exception while starting AGameModeBase::InitGameState hooks";
            stop();
            return false;
        }
    }

    void InitGameStateHookManager::stop() noexcept
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
                        std::bit_cast<InitGameStateFunction>(static_cast<std::uintptr_t>(m_target)),
                        std::memory_order_release);
            }
            InitGameStateHookManager* active = this;
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
                             "[UE4SS Linux] AGameModeBase::InitGameState trampoline release failed: %s\n",
                             release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] AGameModeBase::InitGameState restore failed; retaining trampoline: %s\n",
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

    bool InitGameStateHookManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_runtime != nullptr && m_target != 0u &&
               m_hook.is_installed() && m_original.load(std::memory_order_acquire) != nullptr;
    }

    std::uint64_t InitGameStateHookManager::target_address() const noexcept
    {
        return m_target;
    }

    std::pair<std::uint64_t, std::uint64_t> InitGameStateHookManager::register_hook(
            Callback pre, Callback post, std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!is_ready() || (!pre && !post))
            {
                error = "active AGameModeBase::InitGameState manager and at least one callback are required";
                return {};
            }
            const std::uint64_t pre_id = pre ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            const std::uint64_t post_id = post ? m_next_id.fetch_add(1u, std::memory_order_relaxed) : 0u;
            if ((pre && pre_id == 0u) || (post && post_id == 0u))
            {
                error = "AGameModeBase::InitGameState callback id space is exhausted";
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
                error = "AGameModeBase::InitGameState manager stopped during callback registration";
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
            error = "unknown exception while registering an AGameModeBase::InitGameState callback";
            return {};
        }
    }

    bool InitGameStateHookManager::unregister_hook(std::uint64_t pre_id,
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
                error = "AGameModeBase::InitGameState callback ids were not found";
                return false;
            }
            return true;
        }
        catch (...)
        {
            error = "unknown exception while unregistering an AGameModeBase::InitGameState callback";
            return false;
        }
    }

    void InitGameStateHookManager::detour(void* game_mode,
                                          void* argument_1,
                                          void* argument_2,
                                          void* argument_3,
                                          void* argument_4,
                                          void* argument_5) noexcept
    {
        InitGameStateHookManager* active{};
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
            active->dispatch(game_mode, argument_1, argument_2, argument_3, argument_4, argument_5);
            return;
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire); original != nullptr)
        {
            original(game_mode, argument_1, argument_2, argument_3, argument_4, argument_5);
        }
    }

    void InitGameStateHookManager::dispatch(void* game_mode,
                                            void* argument_1,
                                            void* argument_2,
                                            void* argument_3,
                                            void* argument_4,
                                            void* argument_5) noexcept
    {
        const InitGameStateFunction original = m_original.load(std::memory_order_acquire);
        std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        ReadOnlyUObjectHandle game_mode_handle;
        bool original_called{};
        try
        {
            if (m_runtime != nullptr && game_mode != nullptr)
            {
                static_cast<void>(m_runtime->handle_from_address(
                        std::bit_cast<std::uintptr_t>(game_mode), game_mode_handle));
            }
            {
                std::lock_guard lock{m_callbacks_mutex};
                callbacks = m_callbacks;
            }
            if (m_accepting.load(std::memory_order_acquire) && game_mode_handle.address != 0u)
            {
                for (const auto& callback : callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->pre)
                    {
                        try { callback->pre(game_mode_handle); } catch (...) {}
                    }
                }
            }
            if (original != nullptr)
            {
                original_called = true;
                original(game_mode, argument_1, argument_2, argument_3, argument_4, argument_5);
            }
            if (m_accepting.load(std::memory_order_acquire) && game_mode_handle.address != 0u)
            {
                for (const auto& callback : callbacks)
                {
                    if (callback->active.load(std::memory_order_acquire) && callback->post)
                    {
                        try { callback->post(game_mode_handle); } catch (...) {}
                    }
                }
            }
        }
        catch (...)
        {
            if (!original_called && original != nullptr)
            {
                try { original(game_mode, argument_1, argument_2, argument_3, argument_4, argument_5); }
                catch (...) {}
            }
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
    }
} // namespace ue4ss::linux::core
