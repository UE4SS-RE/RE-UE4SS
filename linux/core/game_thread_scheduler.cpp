#include "game_thread_scheduler.hpp"

#include "unreal_readonly.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <optional>

namespace
{
    constexpr std::size_t k_max_callbacks_per_tick = 1024u;
}

namespace ue4ss::linux::core
{
    std::atomic<GameThreadScheduler*> GameThreadScheduler::s_active{};
    std::mutex GameThreadScheduler::s_dispatch_mutex;

    GameThreadScheduler::~GameThreadScheduler()
    {
        stop();
    }

    bool GameThreadScheduler::start(ReadOnlyUnrealRuntime& runtime,
                                    std::uint64_t expected_tick_address,
                                    std::string& error) noexcept
    {
        void** slot{};
        if (!runtime.find_game_engine_tick_slot(expected_tick_address, slot, error))
        {
            return false;
        }
        return start_on_slot(slot,
                             std::bit_cast<TickFunction>(static_cast<std::uintptr_t>(expected_tick_address)),
                             error);
    }

    bool GameThreadScheduler::start_on_slot(void** slot,
                                            TickFunction expected_tick,
                                            std::string& error) noexcept
    {
        try
        {
            if (slot == nullptr || expected_tick == nullptr)
            {
                error = "game-thread tick slot and expected function are required";
                return false;
            }
            if (is_ready())
            {
                return m_original.load(std::memory_order_acquire) == expected_tick;
            }

            void* current{};
            std::memcpy(&current, slot, sizeof(current));
            if (current != std::bit_cast<void*>(expected_tick))
            {
                error = "UGameEngine::Tick vtable slot does not equal the validated resolver address";
                return false;
            }

            GameThreadScheduler* expected_active{};
            if (!s_active.compare_exchange_strong(expected_active, this, std::memory_order_acq_rel))
            {
                error = "another game-thread scheduler is already active";
                return false;
            }

            m_original.store(expected_tick, std::memory_order_release);
            m_tick_count.store(0u, std::memory_order_release);
            m_accepting.store(true, std::memory_order_release);
            if (!m_hook.install(slot, std::bit_cast<void*>(&GameThreadScheduler::tick_detour), error))
            {
                m_accepting.store(false, std::memory_order_release);
                m_original.store(nullptr, std::memory_order_release);
                GameThreadScheduler* active = this;
                static_cast<void>(s_active.compare_exchange_strong(active, nullptr, std::memory_order_acq_rel));
                return false;
            }
            error.clear();
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
            error = "unknown exception while installing the game-thread scheduler";
            stop();
            return false;
        }
    }

    void GameThreadScheduler::stop() noexcept
    {
        m_accepting.store(false, std::memory_order_release);
        std::vector<Cleanup> cleanups;
        {
            std::lock_guard lock{m_task_mutex};
            cleanups.reserve(m_tasks.size());
            for (auto& task : m_tasks)
            {
                if (task.cleanup)
                {
                    cleanups.push_back(std::move(task.cleanup));
                }
            }
            m_tasks.clear();
        }
        run_cleanups(cleanups);

        std::string ignored;
        static_cast<void>(m_hook.remove(ignored));
        {
            // Serialize removal of the global raw pointer with detour entry.
            // Once this lock is released, a future detour cannot acquire a
            // reference to this object and every previous one is represented
            // by m_in_flight.
            std::lock_guard dispatch_lock{s_dispatch_mutex};
            GameThreadScheduler* active = this;
            static_cast<void>(s_active.compare_exchange_strong(active, nullptr, std::memory_order_acq_rel));
        }

        // Destruction while a native callback still owns `this` would be a
        // use-after-free. Lua callbacks are protected but not interruptible,
        // so shutdown deliberately waits for one which is already running.
        std::unique_lock tick_lock{m_tick_mutex};
        m_tick_wakeup.wait(tick_lock, [this] { return m_in_flight.load(std::memory_order_acquire) == 0u; });
        m_original.store(nullptr, std::memory_order_release);
        {
            std::lock_guard game_thread_lock{m_game_thread_mutex};
            m_game_thread_id = {};
        }
    }

    bool GameThreadScheduler::is_ready() const noexcept
    {
        return m_accepting.load(std::memory_order_acquire) && m_hook.is_installed() &&
               m_original.load(std::memory_order_acquire) != nullptr;
    }

    bool GameThreadScheduler::wait_for_first_tick(std::chrono::milliseconds timeout) noexcept
    {
        try
        {
            if (tick_count() != 0u)
            {
                return true;
            }
            std::unique_lock lock{m_tick_mutex};
            return m_tick_wakeup.wait_for(lock, timeout, [this] {
                return tick_count() != 0u || !m_accepting.load(std::memory_order_acquire);
            }) && tick_count() != 0u;
        }
        catch (...)
        {
            return false;
        }
    }

    bool GameThreadScheduler::execute_sync(SynchronousCallback callback,
                                           std::chrono::milliseconds timeout,
                                           std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!callback || timeout.count() <= 0)
            {
                error = "a synchronous callback and positive timeout are required";
                return false;
            }
            if (!is_ready())
            {
                error = "the game-thread scheduler is not ready";
                return false;
            }
            if (is_game_thread())
            {
                return callback(error);
            }

            struct SynchronousState
            {
                std::mutex mutex;
                std::condition_variable wakeup;
                SynchronousCallback callback;
                bool running{};
                bool completed{};
                bool result{};
                std::string error;
            };
            auto state = std::make_shared<SynchronousState>();
            state->callback = std::move(callback);

            const auto task = [state] {
                SynchronousCallback operation;
                {
                    std::lock_guard lock{state->mutex};
                    if (state->completed)
                    {
                        return true;
                    }
                    state->running = true;
                    operation = std::move(state->callback);
                }

                bool result{};
                std::string operation_error;
                try
                {
                    result = operation(operation_error);
                }
                catch (const std::exception& exception)
                {
                    operation_error = exception.what();
                }
                catch (...)
                {
                    operation_error = "unknown exception in a synchronous game-thread operation";
                }
                {
                    std::lock_guard lock{state->mutex};
                    state->result = result;
                    state->error = std::move(operation_error);
                    state->running = false;
                    state->completed = true;
                }
                state->wakeup.notify_all();
                return true;
            };
            const auto cleanup = [state] {
                {
                    std::lock_guard lock{state->mutex};
                    if (state->completed || state->running)
                    {
                        return;
                    }
                    state->error = "synchronous game-thread operation was cancelled";
                    state->completed = true;
                }
                state->wakeup.notify_all();
            };

            const std::uint64_t handle = enqueue_owned(
                    0u, task, std::chrono::milliseconds{}, {}, cleanup);
            if (handle == 0u)
            {
                error = "synchronous game-thread operation could not be queued";
                return false;
            }

            std::unique_lock lock{state->mutex};
            if (!state->wakeup.wait_for(
                        lock, timeout, [&state] { return state->completed; }))
            {
                const bool running = state->running;
                lock.unlock();
                if (!running)
                {
                    static_cast<void>(cancel_owned(0u, handle));
                }
                lock.lock();
                if (!state->completed && !state->running)
                {
                    state->error = "synchronous game-thread operation timed out";
                    state->completed = true;
                }
                while (state->running)
                {
                    state->wakeup.wait(lock);
                }
            }
            error = state->error;
            return state->completed && state->result;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while synchronizing with the game thread";
            return false;
        }
    }

    std::uint64_t GameThreadScheduler::enqueue(Callback callback,
                                               std::chrono::milliseconds delay,
                                               std::chrono::milliseconds repeat) noexcept
    {
        return enqueue_owned(0u, std::move(callback), delay, repeat);
    }

    std::uint64_t GameThreadScheduler::reserve_handle() noexcept
    {
        try
        {
            std::lock_guard lock{m_task_mutex};
            return allocate_handle_locked();
        }
        catch (...)
        {
            return 0u;
        }
    }

    std::uint64_t GameThreadScheduler::enqueue_owned(OwnerId owner,
                                                     Callback callback,
                                                     std::chrono::milliseconds delay,
                                                     std::chrono::milliseconds repeat,
                                                     Cleanup cleanup,
                                                     std::uint64_t requested_handle,
                                                     bool force_repeating) noexcept
    {
        try
        {
            if (!is_ready() || !callback || delay.count() < 0 || repeat.count() < 0)
            {
                return 0u;
            }
            std::lock_guard lock{m_task_mutex};
            if (!m_accepting.load(std::memory_order_acquire))
            {
                return 0u;
            }
            const auto handle_exists = [this](std::uint64_t handle) {
                return std::any_of(m_tasks.begin(), m_tasks.end(), [handle](const Task& task) {
                    return task.id == handle;
                });
            };
            const std::uint64_t id = requested_handle != 0u ? requested_handle : allocate_handle_locked();
            if (id == 0u || id > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
                handle_exists(id))
            {
                return 0u;
            }
            Task task{
                    .id = id,
                    .owner = owner,
                    .kind = DelayKind::time,
                    .state = ActionState::active,
                    .due = std::chrono::steady_clock::now() + delay,
                    .rate = delay,
                    .repeat = repeat,
                    .repeating = force_repeating || repeat.count() > 0,
                    .callback = std::move(callback),
                    .cleanup = std::move(cleanup),
            };
            m_tasks.push_back(std::move(task));
            return id;
        }
        catch (...)
        {
            return 0u;
        }
    }

    std::uint64_t GameThreadScheduler::enqueue_frames_owned(OwnerId owner,
                                                            Callback callback,
                                                            std::int64_t frames,
                                                            bool repeating,
                                                            Cleanup cleanup,
                                                            std::uint64_t requested_handle) noexcept
    {
        try
        {
            if (!is_ready() || !callback || frames < 0)
            {
                return 0u;
            }
            std::lock_guard lock{m_task_mutex};
            if (!m_accepting.load(std::memory_order_acquire))
            {
                return 0u;
            }
            const std::uint64_t id = requested_handle != 0u ? requested_handle : allocate_handle_locked();
            if (id == 0u || id > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
                std::any_of(m_tasks.begin(), m_tasks.end(), [id](const Task& task) { return task.id == id; }))
            {
                return 0u;
            }
            m_tasks.push_back(Task{
                    .id = id,
                    .owner = owner,
                    .kind = DelayKind::frames,
                    .state = ActionState::active,
                    .frame_rate = frames,
                    .frames_remaining = frames,
                    .repeating = repeating,
                    .callback = std::move(callback),
                    .cleanup = std::move(cleanup),
            });
            return id;
        }
        catch (...)
        {
            return 0u;
        }
    }

    bool GameThreadScheduler::retrigger_owned(OwnerId owner,
                                              std::uint64_t handle,
                                              std::int64_t rate) noexcept
    {
        return set_rate_owned(owner, handle, rate);
    }

    bool GameThreadScheduler::reset_owned(OwnerId owner, std::uint64_t handle) noexcept
    {
        try
        {
            std::lock_guard lock{m_task_mutex};
            const auto iterator = std::find_if(m_tasks.begin(), m_tasks.end(), [=](const Task& task) {
                return task.id == handle && task.owner == owner;
            });
            if (iterator == m_tasks.end())
            {
                return false;
            }
            if (iterator->kind == DelayKind::frames)
            {
                iterator->frames_remaining = iterator->frame_rate;
            }
            else
            {
                iterator->due = std::chrono::steady_clock::now() + iterator->rate;
                iterator->paused_remaining = {};
            }
            iterator->state = ActionState::active;
            iterator->pause_after_execution = false;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool GameThreadScheduler::set_rate_owned(OwnerId owner,
                                             std::uint64_t handle,
                                             std::int64_t rate) noexcept
    {
        try
        {
            if (rate < 0)
            {
                return false;
            }
            std::lock_guard lock{m_task_mutex};
            const auto iterator = std::find_if(m_tasks.begin(), m_tasks.end(), [=](const Task& task) {
                return task.id == handle && task.owner == owner;
            });
            if (iterator == m_tasks.end())
            {
                return false;
            }
            if (iterator->kind == DelayKind::frames)
            {
                iterator->frame_rate = rate;
                iterator->frames_remaining = rate;
            }
            else
            {
                iterator->rate = std::chrono::milliseconds{rate};
                if (iterator->repeating)
                {
                    iterator->repeat = iterator->rate;
                }
                iterator->due = std::chrono::steady_clock::now() + iterator->rate;
                iterator->paused_remaining = {};
            }
            iterator->state = ActionState::active;
            iterator->pause_after_execution = false;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool GameThreadScheduler::pause_owned(OwnerId owner, std::uint64_t handle) noexcept
    {
        try
        {
            std::lock_guard lock{m_task_mutex};
            const auto iterator = std::find_if(m_tasks.begin(), m_tasks.end(), [=](const Task& task) {
                return task.id == handle && task.owner == owner;
            });
            if (iterator == m_tasks.end())
            {
                return false;
            }
            if (iterator->state == ActionState::executing && iterator->repeating)
            {
                iterator->pause_after_execution = true;
                return true;
            }
            if (iterator->state != ActionState::active)
            {
                return false;
            }
            if (iterator->kind == DelayKind::time)
            {
                const auto now = std::chrono::steady_clock::now();
                iterator->paused_remaining = iterator->due > now
                                                     ? std::chrono::duration_cast<std::chrono::milliseconds>(
                                                               iterator->due - now)
                                                     : std::chrono::milliseconds{};
            }
            iterator->state = ActionState::paused;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool GameThreadScheduler::unpause_owned(OwnerId owner, std::uint64_t handle) noexcept
    {
        try
        {
            std::lock_guard lock{m_task_mutex};
            const auto iterator = std::find_if(m_tasks.begin(), m_tasks.end(), [=](const Task& task) {
                return task.id == handle && task.owner == owner;
            });
            if (iterator == m_tasks.end() || iterator->state != ActionState::paused)
            {
                return false;
            }
            if (iterator->kind == DelayKind::time)
            {
                iterator->due = std::chrono::steady_clock::now() + iterator->paused_remaining;
                iterator->paused_remaining = {};
            }
            iterator->state = ActionState::active;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool GameThreadScheduler::cancel_owned(OwnerId owner, std::uint64_t handle) noexcept
    {
        try
        {
            Cleanup cleanup;
            {
                std::lock_guard lock{m_task_mutex};
                const auto iterator = std::find_if(m_tasks.begin(), m_tasks.end(), [=](const Task& task) {
                    return task.id == handle && task.owner == owner;
                });
                if (iterator == m_tasks.end())
                {
                    return false;
                }
                cleanup = std::move(iterator->cleanup);
                m_tasks.erase(iterator);
            }
            if (cleanup)
            {
                try
                {
                    cleanup();
                }
                catch (...)
                {
                }
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    std::size_t GameThreadScheduler::clear_owner(OwnerId owner) noexcept
    {
        try
        {
            std::vector<Cleanup> cleanups;
            std::size_t count{};
            {
                std::lock_guard lock{m_task_mutex};
                for (auto iterator = m_tasks.begin(); iterator != m_tasks.end();)
                {
                    if (iterator->owner == owner)
                    {
                        if (iterator->cleanup)
                        {
                            cleanups.push_back(std::move(iterator->cleanup));
                        }
                        iterator = m_tasks.erase(iterator);
                        ++count;
                    }
                    else
                    {
                        ++iterator;
                    }
                }
            }
            run_cleanups(cleanups);
            return count;
        }
        catch (...)
        {
            return 0u;
        }
    }

    bool GameThreadScheduler::is_valid_handle(std::uint64_t handle) const noexcept
    {
        try
        {
            std::lock_guard lock{m_task_mutex};
            return std::any_of(m_tasks.begin(), m_tasks.end(), [handle](const Task& task) {
                return task.id == handle;
            });
        }
        catch (...)
        {
            return false;
        }
    }

    std::optional<GameThreadScheduler::ActionSnapshot> GameThreadScheduler::action_snapshot(
            std::uint64_t handle) const noexcept
    {
        try
        {
            std::lock_guard lock{m_task_mutex};
            const auto iterator = std::find_if(m_tasks.begin(), m_tasks.end(), [handle](const Task& task) {
                return task.id == handle;
            });
            if (iterator == m_tasks.end())
            {
                return std::nullopt;
            }
            std::int64_t rate{};
            std::int64_t remaining{};
            if (iterator->kind == DelayKind::frames)
            {
                rate = iterator->frame_rate;
                remaining = std::max<std::int64_t>(0, iterator->frames_remaining);
            }
            else
            {
                rate = iterator->rate.count();
                if (iterator->state == ActionState::paused)
                {
                    remaining = std::max<std::int64_t>(0, iterator->paused_remaining.count());
                }
                else
                {
                    const auto now = std::chrono::steady_clock::now();
                    remaining = iterator->due > now
                                        ? std::chrono::duration_cast<std::chrono::milliseconds>(
                                                  iterator->due - now)
                                                  .count()
                                        : 0;
                }
            }
            return ActionSnapshot{
                    .handle = iterator->id,
                    .owner = iterator->owner,
                    .state = iterator->state,
                    .frame_based = iterator->kind == DelayKind::frames,
                    .repeating = iterator->repeating,
                    .rate = rate,
                    .remaining = remaining,
                    .elapsed = std::clamp<std::int64_t>(rate - remaining, 0, rate),
            };
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::uint64_t GameThreadScheduler::tick_count() const noexcept
    {
        return m_tick_count.load(std::memory_order_acquire);
    }

    bool GameThreadScheduler::is_game_thread() const noexcept
    {
        try
        {
            std::lock_guard lock{m_game_thread_mutex};
            return m_game_thread_id != std::thread::id{} && m_game_thread_id == std::this_thread::get_id();
        }
        catch (...)
        {
            return false;
        }
    }

    std::optional<std::thread::id> GameThreadScheduler::game_thread_id() const noexcept
    {
        try
        {
            std::lock_guard lock{m_game_thread_mutex};
            if (m_game_thread_id == std::thread::id{})
            {
                return std::nullopt;
            }
            return m_game_thread_id;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    void GameThreadScheduler::tick_detour(void* context, float delta_seconds, bool idle_mode) noexcept
    {
        GameThreadScheduler* active{};
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
            active->dispatch_tick(context, delta_seconds, idle_mode);
        }
    }

    void GameThreadScheduler::dispatch_tick(void* context, float delta_seconds, bool idle_mode) noexcept
    {
        try
        {
            {
                std::lock_guard lock{m_game_thread_mutex};
                if (m_game_thread_id == std::thread::id{})
                {
                    m_game_thread_id = std::this_thread::get_id();
                }
            }
            if (m_accepting.load(std::memory_order_acquire))
            {
                execute_due_tasks();
                if (m_tick_count.fetch_add(1u, std::memory_order_acq_rel) == 0u)
                {
                    m_tick_wakeup.notify_all();
                }
            }
            if (const TickFunction original = m_original.load(std::memory_order_acquire); original != nullptr)
            {
                original(context, delta_seconds, idle_mode);
            }
        }
        catch (...)
        {
            // No exception may cross the native Unreal callback boundary.
        }
        if (m_in_flight.fetch_sub(1u, std::memory_order_acq_rel) == 1u)
        {
            m_tick_wakeup.notify_all();
        }
    }

    void GameThreadScheduler::execute_due_tasks() noexcept
    {
        try
        {
            const auto now = std::chrono::steady_clock::now();
            std::vector<std::uint64_t> due;
            due.reserve(16u);
            {
                std::lock_guard lock{m_task_mutex};
                for (auto& task : m_tasks)
                {
                    if (due.size() >= k_max_callbacks_per_tick || task.state != ActionState::active)
                    {
                        continue;
                    }
                    bool ready{};
                    if (task.kind == DelayKind::frames)
                    {
                        if (task.frames_remaining > 0)
                        {
                            --task.frames_remaining;
                        }
                        ready = task.frames_remaining <= 0;
                    }
                    else
                    {
                        ready = task.due <= now;
                    }
                    if (ready)
                    {
                        task.state = ActionState::executing;
                        due.push_back(task.id);
                    }
                }
            }

            for (const auto id : due)
            {
                Callback callback;
                {
                    std::lock_guard lock{m_task_mutex};
                    const auto iterator = std::find_if(m_tasks.begin(), m_tasks.end(), [id](const Task& task) {
                        return task.id == id;
                    });
                    if (iterator == m_tasks.end())
                    {
                        continue;
                    }
                    callback = iterator->callback;
                }
                bool stop = true;
                try
                {
                    stop = callback();
                }
                catch (...)
                {
                    stop = true;
                }
                Cleanup cleanup;
                {
                    std::lock_guard lock{m_task_mutex};
                    const auto iterator = std::find_if(m_tasks.begin(), m_tasks.end(), [id](const Task& task) {
                        return task.id == id;
                    });
                    if (iterator == m_tasks.end())
                    {
                        continue;
                    }
                    if (stop || !m_accepting.load(std::memory_order_acquire))
                    {
                        cleanup = std::move(iterator->cleanup);
                        m_tasks.erase(iterator);
                    }
                    else if (!iterator->repeating)
                    {
                        iterator->state = ActionState::active;
                        if (iterator->kind == DelayKind::frames)
                        {
                            iterator->frames_remaining = 0;
                        }
                        else
                        {
                            iterator->due = std::chrono::steady_clock::now();
                        }
                    }
                    else if (iterator->pause_after_execution)
                    {
                        iterator->pause_after_execution = false;
                        iterator->state = ActionState::paused;
                        if (iterator->kind == DelayKind::frames)
                        {
                            iterator->frames_remaining = iterator->frame_rate;
                        }
                        else
                        {
                            iterator->paused_remaining = iterator->rate;
                        }
                    }
                    else
                    {
                        iterator->state = ActionState::active;
                        if (iterator->kind == DelayKind::frames)
                        {
                            iterator->frames_remaining = iterator->frame_rate;
                        }
                        else
                        {
                            iterator->due = std::chrono::steady_clock::now() + iterator->repeat;
                        }
                    }
                }
                if (cleanup)
                {
                    try
                    {
                        cleanup();
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
        catch (...)
        {
        }
    }

    std::uint64_t GameThreadScheduler::allocate_handle_locked() noexcept
    {
        constexpr auto max_handle = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        for (std::size_t attempt = 0u; attempt < 1024u; ++attempt)
        {
            const std::uint64_t candidate = m_next_task_id.fetch_add(1u, std::memory_order_relaxed);
            if (candidate == 0u || candidate > max_handle)
            {
                if (candidate > max_handle)
                {
                    m_next_task_id.store(1u, std::memory_order_relaxed);
                }
                continue;
            }
            if (std::none_of(m_tasks.begin(), m_tasks.end(), [candidate](const Task& task) {
                    return task.id == candidate;
                }))
            {
                return candidate;
            }
        }
        return 0u;
    }

    void GameThreadScheduler::run_cleanups(std::vector<Cleanup>& cleanups) noexcept
    {
        for (auto& cleanup : cleanups)
        {
            if (!cleanup)
            {
                continue;
            }
            try
            {
                cleanup();
            }
            catch (...)
            {
            }
        }
    }
} // namespace ue4ss::linux::core
