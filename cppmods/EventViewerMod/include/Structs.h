#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

#include <Enums.h>
#include <String/StringType.hpp>

namespace RC::EventViewerMod
{
    static_assert(std::is_same_v<StringType, std::wstring>,
                  "EventViewerMod expects StringType to be std::wstring for ImGui encoding; needs refactor if that changes.");

    // Note: these types are intentionally cheap-to-move so they can be passed through
    // moodycamel::ConcurrentQueue by value (high throughput, minimal allocator churn).
    struct EntryBase
    {
        EntryBase() = default;
        EntryBase(std::string text, bool is_tick);

        std::string text{};
        bool is_tick = false;
        bool is_disabled = false;
    };

    struct CallStackEntry : EntryBase
    {
        CallStackEntry() = default;
        CallStackEntry(EMiddlewareHookTarget hook_target,
                       const StringType& context_name,
                       const StringType& function_name,
                       uint32_t depth,
                       std::thread::id thread_id,
                       bool is_tick);

        auto render_with_colored_indent_space(int indent_delta) const -> void;
        auto render(int indent_delta) const -> void;

        EMiddlewareHookTarget hook_target = EMiddlewareHookTarget::ProcessEvent;
        uint32_t depth = 0;
        std::thread::id thread_id{};

    private:
        auto render_indents(int indent_delta) const -> void;
    };

    struct CallFrequencyEntry : EntryBase
    {
        CallFrequencyEntry() = default;
        CallFrequencyEntry(std::string text, bool is_tick);

        uint64_t frequency = 1;
    };

    struct ThreadInfo
    {
        explicit ThreadInfo(std::thread::id thread_id);

        const std::thread::id thread_id;
        const bool is_game_thread;

        // High-throughput capture history (fast filtering/search due to contiguous storage).
        std::vector<CallStackEntry> call_stack;

        // Aggregated frequency view; list allows O(1) reordering via splice without shifting elements.
        std::list<CallFrequencyEntry> call_frequencies;

        auto id_string() -> const char*;
        auto clear() -> void;

    private:
        std::string m_id_string;
    };

    struct TargetInfo
    {
        std::vector<ThreadInfo> threads;
        int current_thread = 0;

        auto clear() -> void;
    };

    struct UIState
    {
        bool enabled = false;                                                    // [Savable] [Thread-ImGui]
        bool started = false;                                                    // [Thread-ImGui]
        bool show_tick = false;                                                  // [Savable] [Thread-ImGui]
        EMiddlewareHookTarget hook_target = EMiddlewareHookTarget::ProcessEvent; // [Savable] [Thread-ImGui]
        EMode mode = EMode::Stack;                                               // [Savable] [Thread-ImGui]
        EMiddlewareThreadScheme thread_scheme = EMiddlewareThreadScheme::ConcurrentQueue; // [Savable] [Thread-ImGui]
        uint16_t dequeue_max_ms = 10;                                            // [Savable] [Thread-ImGui]
        uint16_t dequeue_max_count = 50;                                         // [Savable] [Thread-ImGui]

        std::string blacklist;                                                   // [Savable] [Thread-ImGui]
        std::vector<std::string_view> blacklist_tokens;                          // [Thread-ImGui] (views into blacklist)

        std::string whitelist;                                                   // [Savable] [Thread-ImGui]
        std::vector<std::string_view> whitelist_tokens;                          // [Thread-ImGui] (views into whitelist)

        std::array<TargetInfo, EMiddlewareHookTarget_Size> targets{};            // [Thread-ImGui]

        std::atomic_flag needs_save = ATOMIC_FLAG_INIT;                          // [Thread-Any]
    };
} // namespace RC::EventViewerMod
