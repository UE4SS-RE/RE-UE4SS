#include <Sync.hpp>

namespace RC
{
    auto sync_and_execute(ThreadState& state_manager, const SyncExecutionFunc& callable) -> void
    {
        state_manager.underlying.store(ThreadSyncStateValue::WaitForMain);
        state_manager.underlying.wait(ThreadSyncStateValue::WaitForMain);
        callable();
        state_manager.underlying.store(ThreadSyncStateValue::Complete);
    }

    auto process_sync_request(ThreadState& state_manager) -> void
    {
        if (state_manager.underlying.load() == ThreadSyncStateValue::WaitForMain)
        {
            state_manager.underlying.store(ThreadSyncStateValue::WaitForOther);
            state_manager.underlying.notify_one();
            state_manager.underlying.wait(ThreadSyncStateValue::WaitForOther);
            state_manager.underlying.store(ThreadSyncStateValue::Idle);
        }
    }
} // namespace RC
