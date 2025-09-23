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
