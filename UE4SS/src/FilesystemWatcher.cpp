#include <FilesystemWatcher.hpp>

#ifdef _MSC_VER
#include "FilesystemWatcher_Windows.cpp_impl"
#else
#include "FilesystemWatcher_Linux.cpp_impl"
#endif

namespace RC
{
    FilesystemWatcher::FilesystemWatcher(FilesystemWatcher&& other) noexcept
        : m_paths(std::move(other.m_paths)), m_handle(other.m_handle), m_watches(std::move(other.m_watches)), m_stop_source(std::move(other.m_stop_source)),
          m_polling_thread(std::move(other.m_polling_thread)), m_state(std::move(other.m_state)), m_last_notification(other.m_last_notification),
          m_min_duration_between_notifications(other.m_min_duration_between_notifications)
    {
    }

    FilesystemWatcher& FilesystemWatcher::operator=(FilesystemWatcher&& other) noexcept
    {
        if (this == &other) return *this;
        m_paths = std::move(other.m_paths);
        m_handle = other.m_handle;
        m_watches = std::move(other.m_watches);
        m_stop_source = std::move(other.m_stop_source);
        m_polling_thread = std::move(other.m_polling_thread);
        m_state = std::move(other.m_state);
        m_last_notification = other.m_last_notification;
        m_min_duration_between_notifications = other.m_min_duration_between_notifications;
        return *this;
    }

    auto FilesystemWatcher::add_dir(const std::filesystem::path& dir) -> void
    {
        m_paths.emplace_back(dir);
    }

    auto FilesystemWatcher::start_async_polling(const FilesystemWatch::NotificationFunctionType& notify) -> void
    {
        for (const auto& path : m_paths)
        {
            init_filesystem_watcher(*this, path);
        }
        m_watches.emplace_back(notify, std::string{"*"});
        m_polling_thread = std::thread{&FilesystemWatcher::poll, this, m_stop_source.get_token()};
    }
} // namespace RC
