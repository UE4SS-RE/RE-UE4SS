#include "object_notifications.hpp"

#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <limits>

namespace ue4ss::linux::core
{
    namespace
    {
        [[nodiscard]] bool trace_notifications() noexcept
        {
            const char* value = std::getenv("UE4SS_TRACE_OBJECT_NOTIFICATIONS");
            return value != nullptr && (value[0] == '1' || value[0] == 't' || value[0] == 'y');
        }
    }

    std::atomic<ObjectNotificationManager*> ObjectNotificationManager::s_active{};
    std::atomic<ObjectNotificationManager::StaticConstructObjectFunction>
            ObjectNotificationManager::s_original_fallback{};
    std::mutex ObjectNotificationManager::s_dispatch_mutex;

    ObjectNotificationManager::~ObjectNotificationManager()
    {
        stop();
    }

    bool ObjectNotificationManager::start(ReadOnlyUnrealRuntime& runtime,
                                          GameThreadScheduler& scheduler,
                                          std::uint64_t static_construct_object,
                                          std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (is_ready())
            {
                return true;
            }
            if (!runtime.is_ready() || !scheduler.is_ready() || static_construct_object == 0u ||
                static_construct_object > std::numeric_limits<std::uintptr_t>::max())
            {
                error = "validated UObject runtime, game-thread scheduler, and StaticConstructObject resolver are required";
                return false;
            }
            ObjectNotificationManager* expected{};
            if (!s_active.compare_exchange_strong(expected, this, std::memory_order_acq_rel))
            {
                error = "another object notification manager is already active";
                return false;
            }

            auto state = std::make_shared<SharedState>();
            state->runtime = &runtime;
            state->scheduler = &scheduler;
            state->active.store(true, std::memory_order_release);
            m_state = std::move(state);
            m_accepting.store(true, std::memory_order_release);
            const auto target = std::bit_cast<void*>(static_cast<std::uintptr_t>(static_construct_object));
            if (!m_hook.install(
                        target,
                        std::bit_cast<void*>(&ObjectNotificationManager::detour),
                        [this](void* trampoline) {
                            const auto original = std::bit_cast<StaticConstructObjectFunction>(trampoline);
                            m_original.store(original, std::memory_order_release);
                            s_original_fallback.store(original, std::memory_order_release);
                        },
                        error))
            {
                stop();
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
            error = "unknown exception while installing object notifications";
            stop();
            return false;
        }
    }

    void ObjectNotificationManager::stop() noexcept
    {
        m_accepting.store(false, std::memory_order_release);
        std::string hook_error;
        bool restored{};
        {
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            const auto target = std::bit_cast<StaticConstructObjectFunction>(m_hook.target());
            restored = m_hook.restore(hook_error);
            if (restored && target != nullptr)
            {
                s_original_fallback.store(target, std::memory_order_release);
            }
            ObjectNotificationManager* active = this;
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
                             "[UE4SS Linux] StaticConstructObject trampoline release failed: %s\n",
                             release_error.c_str());
            }
        }
        else if (!hook_error.empty())
        {
            std::fprintf(stderr,
                         "[UE4SS Linux] StaticConstructObject restore failed; retaining trampoline: %s\n",
                         hook_error.c_str());
        }

        auto state = std::move(m_state);
        if (state != nullptr)
        {
            state->active.store(false, std::memory_order_release);
            std::vector<std::shared_ptr<CallbackRecord>> callbacks;
            {
                std::lock_guard lock{state->callbacks_mutex};
                callbacks = std::move(state->callbacks);
                state->callbacks.clear();
            }
            for (const auto& callback : callbacks)
            {
                std::lock_guard execution_lock{callback->execution_mutex};
                callback->active.store(false, std::memory_order_release);
            }
        }
        m_original.store(nullptr, std::memory_order_release);
    }

    bool ObjectNotificationManager::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_hook.is_installed() &&
               m_original.load(std::memory_order_acquire) != nullptr && m_state != nullptr;
    }

    std::uint64_t ObjectNotificationManager::register_notification(
            const ReadOnlyUObjectHandle& requested_class, Callback callback, std::string& error) noexcept
    {
        error.clear();
        try
        {
            const auto state = m_state;
            std::string class_error;
            if (!is_ready() || state == nullptr || state->runtime == nullptr || !callback ||
                !state->runtime->is_a(requested_class, "Class", class_error))
            {
                error = class_error.empty()
                                ? "active object notifications, a live UClass, and callback are required"
                                : class_error;
                return 0u;
            }
            const std::uint64_t id = m_next_id.fetch_add(1u, std::memory_order_relaxed);
            if (id == 0u || id == std::numeric_limits<std::uint64_t>::max())
            {
                error = "object notification id space is exhausted";
                return 0u;
            }
            auto record = std::make_shared<CallbackRecord>();
            record->id = id;
            record->requested_class = requested_class;
            record->callback = std::move(callback);
            std::lock_guard lock{state->callbacks_mutex};
            if (!state->active.load(std::memory_order_acquire))
            {
                error = "object notification manager is stopping";
                return 0u;
            }
            state->callbacks.push_back(std::move(record));
            return id;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return 0u;
        }
        catch (...)
        {
            error = "unknown exception while registering an object notification";
            return 0u;
        }
    }

    bool ObjectNotificationManager::unregister_notification(std::uint64_t id) noexcept
    {
        try
        {
            const auto state = m_state;
            if (state == nullptr || id == 0u)
            {
                return false;
            }
            std::shared_ptr<CallbackRecord> removed;
            {
                std::lock_guard lock{state->callbacks_mutex};
                const auto iterator = std::find_if(state->callbacks.begin(), state->callbacks.end(), [id](const auto& record) {
                    return record->id == id;
                });
                if (iterator == state->callbacks.end())
                {
                    return false;
                }
                removed = *iterator;
                state->callbacks.erase(iterator);
            }
            std::lock_guard execution_lock{removed->execution_mutex};
            removed->active.store(false, std::memory_order_release);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    void* ObjectNotificationManager::detour(const void* parameters) noexcept
    {
        ObjectNotificationManager* active{};
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
            return active->dispatch(parameters);
        }
        if (const auto original = s_original_fallback.load(std::memory_order_acquire); original != nullptr)
        {
            return original(parameters);
        }
        return nullptr;
    }

    void* ObjectNotificationManager::dispatch(const void* parameters) noexcept
    {
        void* object{};
        try
        {
            if (const auto original = m_original.load(std::memory_order_acquire); original != nullptr)
            {
                object = original(parameters);
            }
            const auto state = m_state;
            ReadOnlyUObjectHandle handle;
            const bool captured = m_accepting.load(std::memory_order_acquire) && state != nullptr &&
                                  state->runtime != nullptr && state->scheduler != nullptr && object != nullptr &&
                                  state->runtime->handle_from_address(std::bit_cast<std::uintptr_t>(object), handle);
            if (captured)
            {
                const std::uint64_t task = state->scheduler->enqueue([state, handle] { return deliver(state, handle); });
                if (trace_notifications())
                {
                    std::fprintf(stderr,
                                 "[UE4SS Linux] object notification captured index=%d serial=%d task=%llu\n",
                                 handle.internal_index,
                                 handle.serial_number,
                                 static_cast<unsigned long long>(task));
                }
            }
            else if (trace_notifications())
            {
                std::fprintf(stderr,
                             "[UE4SS Linux] object allocation observed but no stable registry handle was available: object=%p\n",
                             object);
            }
        }
        catch (...)
        {
            // No exception may cross the StaticConstructObject boundary.
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_wait_wakeup.notify_all();
        }
        return object;
    }

    bool ObjectNotificationManager::deliver(
            const std::shared_ptr<SharedState>& state, const ReadOnlyUObjectHandle& object) noexcept
    {
        try
        {
            if (state == nullptr || !state->active.load(std::memory_order_acquire) || state->runtime == nullptr ||
                !state->runtime->is_valid(object))
            {
                return true;
            }
            std::vector<std::shared_ptr<CallbackRecord>> callbacks;
            {
                std::lock_guard lock{state->callbacks_mutex};
                callbacks = state->callbacks;
            }
            std::vector<std::uint64_t> cancelled;
            if (trace_notifications())
            {
                std::fprintf(stderr,
                             "[UE4SS Linux] delivering object notification index=%d callbacks=%zu\n",
                             object.internal_index,
                             callbacks.size());
            }
            for (const auto& callback : callbacks)
            {
                std::string ignored;
                const bool matches = callback->active.load(std::memory_order_acquire) &&
                                     state->runtime->is_a(object, callback->requested_class, ignored);
                if (trace_notifications())
                {
                    std::fprintf(stderr,
                                 "[UE4SS Linux] object notification callback=%llu match=%s detail=%s\n",
                                 static_cast<unsigned long long>(callback->id),
                                 matches ? "yes" : "no",
                                 ignored.c_str());
                }
                if (!matches)
                {
                    continue;
                }
                std::lock_guard execution_lock{callback->execution_mutex};
                if (!callback->active.load(std::memory_order_acquire))
                {
                    continue;
                }
                bool cancel = true;
                try
                {
                    cancel = callback->callback(object);
                }
                catch (...)
                {
                    cancel = true;
                }
                if (cancel)
                {
                    callback->active.store(false, std::memory_order_release);
                    cancelled.push_back(callback->id);
                }
            }
            if (!cancelled.empty())
            {
                std::lock_guard lock{state->callbacks_mutex};
                std::erase_if(state->callbacks, [&cancelled](const auto& callback) {
                    return std::find(cancelled.begin(), cancelled.end(), callback->id) != cancelled.end();
                });
            }
        }
        catch (...)
        {
        }
        return true;
    }
} // namespace ue4ss::linux::core
