#pragma once

// A thread-safe type (if used correctly) used to watch for filesystem changes.
// Create a FilesystemWatcher, and call 'add' on it once for each file or directory that you want to watch.
// You can use Sync.hpp to synchronize between the main thread and the loader thread, useful for automatic hot-reloading.

#include <filesystem>
#include <utility>
#include <vector>
#include <thread>
#include <stop_token>
#include <chrono>
#include <functional>
#include <cstdint>

#include <Sync.hpp>

namespace RC
{
    struct FilesystemWatch
    {
      public:
        using NotificationFunctionType = std::function<void(const std::filesystem::path& file, bool match_all)>;
        NotificationFunctionType notify{};
        std::string name{};

      public:
        FilesystemWatch() = default;
        FilesystemWatch(NotificationFunctionType notify, std::string name) : notify{std::move(notify)}, name{std::move(name)}
        {
        }
        FilesystemWatch(NotificationFunctionType notify, const std::filesystem::path& name) : notify{std::move(notify)}, name{name.filename().string()}
        {
        }
    };

    // clang-format off
    enum class LibReloaderState
    {
        Idle = -1,                          // Main thread is executing, loader thread is waiting for notification from the OS.
        ReloadRequested_WaitForMain = 0,    // Loader got notification from OS, and is now paused until main thread replies it's safe to reload the .dll/.so file.
                                            // This allows the main thread to finish executing its current frame before the .dll/.so file is unloaded.
                                            // Without this, the .dll/.so file could be unloaded at the same time that it's being used which would cause a SIGSEGV or SIGBUS error.
        ReloadCanStart_WaitForLoader = 1,   // Main thread received notification from loader, and is now in a safe state to reload, and is now paused until loader replies it's safe to continue executing.
        ReloadCompleted = 2,                // Loader thread has finished reloading the .dll/.so file, and now notifies main of such and then immediately goes into idle state.
                                            // When the main thread receives the notification from the loader, it also go into idle state.
    };
    // clang-format on

    class FilesystemWatcher
    {
      public:
        std::vector<std::filesystem::path> m_paths{};
        void* m_handle{};
        std::vector<void*> m_handles{};
        std::vector<FilesystemWatch> m_watches{};
        std::stop_source m_stop_source{};
        std::thread m_polling_thread{};
        ThreadState m_state{};
        std::chrono::time_point<std::chrono::high_resolution_clock> m_last_notification{std::chrono::high_resolution_clock::now()};
        std::chrono::milliseconds m_min_duration_between_notifications{4000};

      private:
        static constexpr int32_t s_polling_timeout_ms = 100;

      public:
        FilesystemWatcher() = default;
        FilesystemWatcher(FilesystemWatcher&& other) noexcept;
        FilesystemWatcher& operator=(FilesystemWatcher&& other) noexcept;
        ~FilesystemWatcher();

      public:
        auto add_dir(const std::filesystem::path& dir) -> void;
        auto start_async_polling(const FilesystemWatch::NotificationFunctionType& notify) -> void;
        auto get_thread_state() -> ThreadState&
        {
            return m_state;
        }

      private:
        auto poll(std::stop_token) -> void;

      private:
        friend auto init_filesystem_watcher(FilesystemWatcher&, const std::filesystem::path&) -> void;
    };
} // namespace RC
