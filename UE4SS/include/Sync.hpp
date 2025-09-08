#pragma once

#include <functional>
#include <atomic>

namespace RC
{
    enum class ThreadSyncStateValue
    {
        Idle = -1,
        WaitForMain = 0,
        WaitForOther = 1,
        Complete = 2,
    };

    struct ThreadState
    {
        std::atomic<ThreadSyncStateValue> underlying{ThreadSyncStateValue::Idle};

        ThreadState() = default;
        ThreadState(ThreadState&& other) noexcept : underlying(other.underlying.load())
        {
        }
        ThreadState& operator=(ThreadState&& other) noexcept
        {
            if (this == &other) return *this;
            underlying = other.underlying.load();
            return *this;
        }
    };

    // Force the current thread to block until the main thread replies, and then execute the provided function.
    using SyncExecutionFunc = std::function<void()>;
    auto sync_and_execute(ThreadState& state_manager, const SyncExecutionFunc& callable) -> void;

    // Block the current thread if another thread has requested it.
    auto process_sync_request(ThreadState& state_manager) -> void;

    // Same as 'sync_and_execute', but using RAII instead of providing a function.
    class ScopedThreadSynchronizer
    {
      private:
        ThreadState& m_state_manager;

      public:
        explicit ScopedThreadSynchronizer(ThreadState& state_manager) : m_state_manager{state_manager}
        {
            m_state_manager.underlying.store(ThreadSyncStateValue::WaitForMain);
            m_state_manager.underlying.wait(ThreadSyncStateValue::WaitForMain);
        }

        ~ScopedThreadSynchronizer()
        {
            m_state_manager.underlying.store(ThreadSyncStateValue::Complete);
            m_state_manager.underlying.notify_one();
        }
    };
} // namespace RC
