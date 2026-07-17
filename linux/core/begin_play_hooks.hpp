#pragma once

#include "ue4ss/inline_hook.hpp"
#include "unreal_readonly.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace ue4ss::linux::core
{
    // UE4SS's RegisterBeginPlay* API hooks the native virtual AActor::BeginPlay
    // function, not the reflected ReceiveBeginPlay event. Palworld 1.0 uses
    // Unreal 5.1. Its Linux Itanium vtable has separate complete/deleting
    // destructor entries and therefore places BeginPlay one slot after the
    // generated MSVC layout.
    class BeginPlayHookManager
    {
      public:
        using Callback = std::function<void(const ReadOnlyUObjectHandle&)>;

        BeginPlayHookManager() = default;
        ~BeginPlayHookManager();

        BeginPlayHookManager(const BeginPlayHookManager&) = delete;
        BeginPlayHookManager& operator=(const BeginPlayHookManager&) = delete;

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
        using BeginPlayFunction = void (*)(void*);

        struct CallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            Callback pre;
            Callback post;
        };

        static void detour(void* actor) noexcept;
        void dispatch(void* actor) noexcept;

        static std::atomic<BeginPlayHookManager*> s_active;
        static std::atomic<BeginPlayFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ReadOnlyUnrealRuntime* m_runtime{};
        ue4ss::linux::InlineHook m_hook;
        std::atomic<BeginPlayFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        std::uint64_t m_target{};
        std::mutex m_callbacks_mutex;
        std::vector<std::shared_ptr<CallbackRecord>> m_callbacks;
        std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };

    // Native AActor::EndPlay companion to BeginPlayHookManager. The reason is
    // passed by reference to callbacks so the Lua RemoteUnrealParam exposed by
    // a pre-hook can retain UE4SS's ability to replace it before the original
    // virtual function is invoked.
    class EndPlayHookManager
    {
      public:
        using Callback = std::function<void(const ReadOnlyUObjectHandle&, std::int32_t&)>;

        EndPlayHookManager() = default;
        ~EndPlayHookManager();

        EndPlayHookManager(const EndPlayHookManager&) = delete;
        EndPlayHookManager& operator=(const EndPlayHookManager&) = delete;

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
        using EndPlayFunction = void (*)(void*, std::int32_t);

        struct CallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            Callback pre;
            Callback post;
        };

        static void detour(void* actor, std::int32_t reason) noexcept;
        void dispatch(void* actor, std::int32_t reason) noexcept;

        static std::atomic<EndPlayHookManager*> s_active;
        static std::atomic<EndPlayFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ReadOnlyUnrealRuntime* m_runtime{};
        ue4ss::linux::InlineHook m_hook;
        std::atomic<EndPlayFunction> m_original{};
        std::atomic<bool> m_accepting{};
        std::atomic<std::uint32_t> m_in_flight{};
        std::atomic<std::uint64_t> m_next_id{1u};
        std::uint64_t m_target{};
        std::mutex m_callbacks_mutex;
        std::vector<std::shared_ptr<CallbackRecord>> m_callbacks;
        std::mutex m_wait_mutex;
        std::condition_variable m_wait_wakeup;
    };

    // AGameModeBase::InitGameState has no declared arguments besides `this`.
    // The detour preserves all remaining SysV integer argument registers when
    // it calls the trampoline, mirroring UE4SS's existing defensive Windows
    // detour without exposing those unspecified values to mods.
    class InitGameStateHookManager
    {
      public:
        using Callback = std::function<void(const ReadOnlyUObjectHandle&)>;

        InitGameStateHookManager() = default;
        ~InitGameStateHookManager();

        InitGameStateHookManager(const InitGameStateHookManager&) = delete;
        InitGameStateHookManager& operator=(const InitGameStateHookManager&) = delete;

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
        using InitGameStateFunction = void (*)(void*, void*, void*, void*, void*, void*);

        struct CallbackRecord
        {
            std::atomic<bool> active{true};
            std::uint64_t pre_id{};
            std::uint64_t post_id{};
            Callback pre;
            Callback post;
        };

        static void detour(void* game_mode,
                           void* argument_1,
                           void* argument_2,
                           void* argument_3,
                           void* argument_4,
                           void* argument_5) noexcept;
        void dispatch(void* game_mode,
                      void* argument_1,
                      void* argument_2,
                      void* argument_3,
                      void* argument_4,
                      void* argument_5) noexcept;

        static std::atomic<InitGameStateHookManager*> s_active;
        static std::atomic<InitGameStateFunction> s_original_fallback;
        static std::mutex s_dispatch_mutex;

        ReadOnlyUnrealRuntime* m_runtime{};
        ue4ss::linux::InlineHook m_hook;
        std::atomic<InitGameStateFunction> m_original{};
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
