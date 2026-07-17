#pragma once

#include "ue4ss/inline_hook.hpp"
#include "unreal_readonly.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ue4ss::linux::core
{
    struct LoadMapHookInvocation
    {
        ReadOnlyUObjectHandle engine;
        ReadOnlyUObjectHandle world;
        UnrealFURLSnapshot url;
        ReadOnlyUObjectHandle pending_game;
        std::string error;
    };

    // Clang lowers UE 5.1's non-trivial FURL value parameter to an indirect
    // pointer under the Itanium ABI. This manager deliberately hooks that raw
    // five-pointer SysV signature so it never copies or destroys engine-owned
    // FString/TArray storage.
    class LoadMapHookManager
    {
      public:
        using Callback = std::function<std::optional<bool>(const LoadMapHookInvocation&)>;

        LoadMapHookManager() = default;
        ~LoadMapHookManager();

        LoadMapHookManager(const LoadMapHookManager&) = delete;
        LoadMapHookManager& operator=(const LoadMapHookManager&) = delete;

        [[nodiscard]] bool start(ReadOnlyUnrealRuntime& runtime, std::string& error) noexcept;
        void stop() noexcept;
        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] std::uint64_t target_address() const noexcept;

        [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> register_hook(
                Callback pre,
                Callback post,
                std::string& error) noexcept;
        [[nodiscard]] bool unregister_hook(std::uint64_t pre_id,
                                           std::uint64_t post_id,
                                           std::string& error) noexcept;

      private:
        using LoadMapFunction = bool (*)(void*, void*, void*, void*, void*);

        struct CallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            Callback pre;
            Callback post;
        };

        static bool detour(void* engine,
                           void* world_context,
                           void* url,
                           void* pending_game,
                           void* error_string) noexcept;
        bool dispatch(void* engine,
                      void* world_context,
                      void* url,
                      void* pending_game,
                      void* error_string) noexcept;
        [[nodiscard]] bool snapshot_invocation(void* engine,
                                               void* world_context,
                                               void* url,
                                               void* pending_game,
                                               void* error_string,
                                               LoadMapHookInvocation& output,
                                               std::string& error) const noexcept;

        static std::atomic<LoadMapHookManager*> s_active;
        static std::atomic<LoadMapFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ReadOnlyUnrealRuntime* m_runtime{};
        ue4ss::linux::InlineHook m_hook;
        std::atomic<LoadMapFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        std::uint64_t m_target{};
        std::mutex m_callbacks_mutex;
        std::vector<std::shared_ptr<CallbackRecord>> m_callbacks;
        std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };
} // namespace ue4ss::linux::core
