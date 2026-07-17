#pragma once

#include "ue4ss/inline_hook.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace ue4ss::linux::core
{
    class ReadOnlyUnrealRuntime;

    // Executes bounded callbacks from UGameEngine::Tick. The hook is placed in
    // the validated UGameEngine vtable slot, so enabling the scheduler never
    // rewrites live executable instructions.
    class GameThreadScheduler
    {
      public:
        using TickFunction = void (*)(void*, float, bool);
        // true completes the task; false retries it on a future tick. Repeating
        // tasks additionally use false to continue at their configured rate.
        using Callback = std::function<bool()>;
        using Cleanup = std::function<void()>;
        using SynchronousCallback = std::function<bool(std::string&)>;
        using OwnerId = std::uintptr_t;

        enum class ActionState
        {
            active,
            paused,
            executing,
        };

        struct ActionSnapshot
        {
            std::uint64_t handle{};
            OwnerId owner{};
            ActionState state{ActionState::active};
            bool frame_based{};
            bool repeating{};
            std::int64_t rate{};
            std::int64_t remaining{};
            std::int64_t elapsed{};
        };

        GameThreadScheduler() = default;
        ~GameThreadScheduler();

        GameThreadScheduler(const GameThreadScheduler&) = delete;
        GameThreadScheduler& operator=(const GameThreadScheduler&) = delete;
        GameThreadScheduler(GameThreadScheduler&&) = delete;
        GameThreadScheduler& operator=(GameThreadScheduler&&) = delete;

        [[nodiscard]] bool start(ReadOnlyUnrealRuntime& runtime,
                                 std::uint64_t expected_tick_address,
                                 std::string& error) noexcept;
        [[nodiscard]] bool start_on_slot(void** slot,
                                         TickFunction expected_tick,
                                         std::string& error) noexcept;
        void stop() noexcept;

        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] bool wait_for_first_tick(std::chrono::milliseconds timeout) noexcept;
        [[nodiscard]] bool execute_sync(SynchronousCallback callback,
                                        std::chrono::milliseconds timeout,
                                        std::string& error) noexcept;
        [[nodiscard]] std::uint64_t enqueue(Callback callback,
                                            std::chrono::milliseconds delay = {},
                                            std::chrono::milliseconds repeat = {}) noexcept;
        [[nodiscard]] std::uint64_t reserve_handle() noexcept;
        [[nodiscard]] std::uint64_t enqueue_owned(OwnerId owner,
                                                  Callback callback,
                                                  std::chrono::milliseconds delay,
                                                  std::chrono::milliseconds repeat = {},
                                                  Cleanup cleanup = {},
                                                  std::uint64_t requested_handle = 0u,
                                                  bool force_repeating = false) noexcept;
        [[nodiscard]] std::uint64_t enqueue_frames_owned(OwnerId owner,
                                                         Callback callback,
                                                         std::int64_t frames,
                                                         bool repeating = false,
                                                         Cleanup cleanup = {},
                                                         std::uint64_t requested_handle = 0u) noexcept;
        [[nodiscard]] bool retrigger_owned(OwnerId owner,
                                           std::uint64_t handle,
                                           std::int64_t rate) noexcept;
        [[nodiscard]] bool reset_owned(OwnerId owner, std::uint64_t handle) noexcept;
        [[nodiscard]] bool set_rate_owned(OwnerId owner,
                                          std::uint64_t handle,
                                          std::int64_t rate) noexcept;
        [[nodiscard]] bool pause_owned(OwnerId owner, std::uint64_t handle) noexcept;
        [[nodiscard]] bool unpause_owned(OwnerId owner, std::uint64_t handle) noexcept;
        [[nodiscard]] bool cancel_owned(OwnerId owner, std::uint64_t handle) noexcept;
        [[nodiscard]] std::size_t clear_owner(OwnerId owner) noexcept;
        [[nodiscard]] bool is_valid_handle(std::uint64_t handle) const noexcept;
        [[nodiscard]] std::optional<ActionSnapshot> action_snapshot(
                std::uint64_t handle) const noexcept;
        [[nodiscard]] std::uint64_t tick_count() const noexcept;
        [[nodiscard]] bool is_game_thread() const noexcept;
        [[nodiscard]] std::optional<std::thread::id> game_thread_id() const noexcept;

      private:
        enum class DelayKind
        {
            time,
            frames,
        };

        struct Task
        {
            std::uint64_t id{};
            OwnerId owner{};
            DelayKind kind{DelayKind::time};
            ActionState state{ActionState::active};
            std::chrono::steady_clock::time_point due;
            std::chrono::milliseconds rate{};
            std::chrono::milliseconds repeat{};
            std::chrono::milliseconds paused_remaining{};
            std::int64_t frame_rate{};
            std::int64_t frames_remaining{};
            bool repeating{};
            bool pause_after_execution{};
            Callback callback;
            Cleanup cleanup;
        };

        static void tick_detour(void* context, float delta_seconds, bool idle_mode) noexcept;
        void dispatch_tick(void* context, float delta_seconds, bool idle_mode) noexcept;
        void execute_due_tasks() noexcept;
        [[nodiscard]] std::uint64_t allocate_handle_locked() noexcept;
        static void run_cleanups(std::vector<Cleanup>& cleanups) noexcept;

        static std::atomic<GameThreadScheduler*> s_active;
        static std::mutex s_dispatch_mutex;

        ue4ss::linux::PointerHook m_hook;
        std::atomic<TickFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint64_t> m_tick_count{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_task_id{1u};
        mutable std::mutex m_task_mutex;
        std::vector<Task> m_tasks;
        mutable std::mutex m_tick_mutex;
        std::condition_variable m_tick_wakeup;
        mutable std::mutex m_game_thread_mutex;
        std::thread::id m_game_thread_id;
    };
} // namespace ue4ss::linux::core
