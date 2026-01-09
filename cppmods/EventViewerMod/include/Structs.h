#pragma once
#include <thread>
#include <vector>
#include <list>
#include <functional>
#include <unordered_map>
#include <string>
#include <memory>
#include <string_view>
#include <atomic>
#include <type_traits>
#include <thread>
#include <array>

#include <String/StringType.hpp>
#include <Enums.h>

namespace RC::EventViewerMod
{
    static_assert(std::is_same_v<StringType, std::wstring>, "EventViewerMod expects StringType to be std::wstring for ImGui encoding, needs refactor!");

    struct CallStackEntry
    {
        CallStackEntry(StringType context_name, StringType function_name, uint32_t depth, std::thread::id thread_id, bool is_tick);

        const StringType context_name;
        const StringType function_name;
        const uint32_t depth;
        const std::thread::id thread_id;
        const bool is_tick;
        bool is_disabled = false;

        uint32_t visual_depth;

        auto entry_as_string() -> const char*;
        auto entry_as_std_string() -> const std::string&;
    private:
        std::string m_print_name;
    };

    struct CallFrequencyEntry
    {
        const std::string function_name;
        uint64_t frequency = 1;
        bool is_disabled = false;
    };

    struct ThreadInfo
    {
        explicit ThreadInfo(std::thread::id thread_id);

        const std::thread::id thread_id;
        const bool is_game_thread;

        std::list<std::unique_ptr<CallStackEntry>> call_stack;
        std::list<std::unique_ptr<CallFrequencyEntry>> call_frequencies;

        auto operator<=>(const ThreadInfo& other) const noexcept
        {
            return other.thread_id <=> thread_id;
        }

        auto operator<=>(const std::thread::id& other) const noexcept
        {
            return other <=> thread_id;
        }

        auto id_string() -> const char*;
    private:
        std::string m_id_string;
    };

    struct TargetInfo
    {
        std::vector<ThreadInfo> threads;
        int current_thread = 0;
    };

    struct UIState
    {
        bool enabled = false; // [Savable] [Thread-ImGui]
        bool started = false; // [Thread-ImGui]
        bool show_tick = false; // [Savable] [Thread-ImGui]
        EMiddlewareHookTarget hook_target = EMiddlewareHookTarget::ProcessEvent; // [Savable] [Thread-ImGui]
        EMode mode = EMode::Stack; // [Savable] [Thread-ImGui]
        EMiddlewareThreadScheme thread_scheme = EMiddlewareThreadScheme::ConcurrentQueue; // [Savable] [Thread-ImGui]
        uint16_t dequeue_max_ms = 10; // [Savable] [Thread-ImGui]
        uint16_t dequeue_max_count = 50; // [Savable] [Thread-ImGui]
        std::string blacklist; // [Savable] [Thread-ImGui]
        std::vector<std::string_view> blacklist_tokens; // [Thread-ImGui]
        std::string whitelist; // [Savable] [Thread-ImGui]
        std::vector<std::string_view> whitelist_tokens; // [Thread-ImGui
        std::thread::id current_thread; // [Thread-ImGui]
        std::array<TargetInfo, EMiddlewareHookTarget_Size> targets {}; // [Thread-ImGui]
        std::atomic_flag needs_save; // [Thread-Any]
    };
}