#pragma once

#include "game_thread_scheduler.hpp"
#include "ue4ss/inline_hook.hpp"
#include "unreal_readonly.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ue4ss::linux::core
{
    // Delivers FUObjectArray allocation notifications on the validated game
    // thread. The engine allocation hook never enters Lua directly.
    class ObjectNotificationManager
    {
      public:
        using Callback = std::function<bool(const ReadOnlyUObjectHandle&)>; // true cancels the notification

        ObjectNotificationManager() = default;
        ~ObjectNotificationManager();

        ObjectNotificationManager(const ObjectNotificationManager&) = delete;
        ObjectNotificationManager& operator=(const ObjectNotificationManager&) = delete;

        [[nodiscard]] bool start(ReadOnlyUnrealRuntime& runtime,
                                 GameThreadScheduler& scheduler,
                                 std::uint64_t static_construct_object,
                                 std::string& error) noexcept;
        void stop() noexcept;
        [[nodiscard]] bool is_ready() const noexcept;

        [[nodiscard]] std::uint64_t register_notification(const ReadOnlyUObjectHandle& requested_class,
                                                          Callback callback,
                                                          std::string& error) noexcept;
        [[nodiscard]] bool unregister_notification(std::uint64_t id) noexcept;

      private:
        using StaticConstructObjectFunction = void* (*)(const void*);

        struct CallbackRecord
        {
            std::uint64_t id{};
            ReadOnlyUObjectHandle requested_class;
            Callback callback;
            std::atomic<bool> active{true};
            std::mutex execution_mutex;
        };

        struct SharedState
        {
            ReadOnlyUnrealRuntime* runtime{};
            GameThreadScheduler* scheduler{};
            std::atomic<bool> active{};
            std::mutex callbacks_mutex;
            std::vector<std::shared_ptr<CallbackRecord>> callbacks;
        };

        static void* detour(const void* parameters) noexcept;
        void* dispatch(const void* parameters) noexcept;
        static bool deliver(const std::shared_ptr<SharedState>& state,
                            const ReadOnlyUObjectHandle& object) noexcept;

        static std::atomic<ObjectNotificationManager*> s_active;
        static std::atomic<StaticConstructObjectFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ue4ss::linux::InlineHook m_hook;
        std::atomic<StaticConstructObjectFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        std::shared_ptr<SharedState> m_state;
        std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };
} // namespace ue4ss::linux::core
