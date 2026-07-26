#include <FilesystemWatcher.hpp>

#ifdef _MSC_VER
#include "FilesystemWatcher_Windows.cpp_impl"
#else
#include "FilesystemWatcher_Linux.cpp_impl"
#endif

namespace RC
{
    FilesystemWatcher::FilesystemWatcher(FilesystemWatcher&& other) noexcept
        : m_paths(std::move(other.m_paths)), m_handle(other.m_handle), m_handles(std::move(other.m_handles)), m_watches(std::move(other.m_watches)),
          m_stop_source(std::move(other.m_stop_source)), m_polling_thread(std::move(other.m_polling_thread)), m_state(std::move(other.m_state)),
          m_last_notification(other.m_last_notification), m_min_duration_between_notifications(other.m_min_duration_between_notifications)
    {
        other.m_handle = nullptr;
        other.m_handles.clear();
    }

    FilesystemWatcher& FilesystemWatcher::operator=(FilesystemWatcher&& other) noexcept
    {
        if (this == &other) return *this;
        FilesystemWatcher incoming{std::move(other)};
        std::swap(m_paths, incoming.m_paths);
        std::swap(m_handle, incoming.m_handle);
        std::swap(m_handles, incoming.m_handles);
        std::swap(m_watches, incoming.m_watches);
        std::swap(m_stop_source, incoming.m_stop_source);
        std::swap(m_polling_thread, incoming.m_polling_thread);
        std::swap(m_state, incoming.m_state);
        std::swap(m_last_notification, incoming.m_last_notification);
        std::swap(m_min_duration_between_notifications, incoming.m_min_duration_between_notifications);
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
