#include <FilesystemWatcher.hpp>

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>

using namespace std::chrono_literals;

namespace
{
    struct CallbackState
    {
        std::mutex mutex{};
        std::condition_variable cv{};
        std::filesystem::path path{};
        bool match_all{};
        bool called{};
    };

    auto wait_for_callback(CallbackState& state) -> bool
    {
        std::unique_lock lock{state.mutex};
        return state.cv.wait_for(lock, 2s, [&] { return state.called; });
    }

    auto make_temp_dir(const char* suffix) -> std::filesystem::path
    {
        auto dir = std::filesystem::temp_directory_path() /
                   (std::string{"ue4ss_watcher_"} + suffix + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(dir);
        return dir;
    }

    auto record_callback(CallbackState& state, const std::filesystem::path& path, bool match_all) -> void
    {
        {
            std::lock_guard lock{state.mutex};
            state.path = path;
            state.match_all = match_all;
            state.called = true;
        }
        state.cv.notify_one();
    }
} // namespace

int main()
{
    {
        RC::FilesystemWatcher never_started{};
    }

    const auto first_dir = make_temp_dir("first");
    const auto second_dir = make_temp_dir("second");

    CallbackState named_state{};
    {
        RC::FilesystemWatcher watcher{};
        watcher.m_min_duration_between_notifications = 0ms;
        watcher.add_dir(first_dir);
        watcher.add_dir(second_dir);
        watcher.m_watches.emplace_back(
                [&](const std::filesystem::path& path, bool match_all) { record_callback(named_state, path, match_all); }, std::string{"Example"});
        watcher.start_async_polling([](const std::filesystem::path&, bool) {});

        const auto created = second_dir / "libExample.so";
        std::ofstream{created} << "fixture";

        if (!wait_for_callback(named_state) || named_state.path != created || named_state.match_all)
        {
            return 1;
        }
    }

    CallbackState wildcard_state{};
    {
        RC::FilesystemWatcher watcher{};
        watcher.m_min_duration_between_notifications = 0ms;
        watcher.add_dir(first_dir);
        watcher.start_async_polling(
                [&](const std::filesystem::path& path, bool match_all) { record_callback(wildcard_state, path, match_all); });

        const auto created = first_dir / "AnyName.so";
        std::ofstream{created} << "fixture";

        if (!wait_for_callback(wildcard_state) || wildcard_state.path != created || !wildcard_state.match_all)
        {
            return 2;
        }
    }

    std::filesystem::remove_all(first_dir);
    std::filesystem::remove_all(second_dir);
    return 0;
}
