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

#include <Unreal/UObject.hpp>

namespace RC::EventViewerMod
{
    static_assert(std::is_same_v<StringType, std::wstring>,
                  "EventViewerMod expects StringType to be std::wstring for ImGui encoding; needs refactor if that changes.");

    struct FunctionNameStringViews
    {
        uint32_t function_hash;
        std::string_view function_name;
        std::string_view lower_cased_function_name;
    };

    struct AllNameStringViews : FunctionNameStringViews
    {
        std::string_view full_name;
        std::string_view lower_cased_full_name;
    };

    // Note: these types are intentionally cheap-to-move so they can be passed through
    // moodycamel::ConcurrentQueue by value (high throughput, minimal allocator churn).
    struct EntryBase
    {
        EntryBase() = default;
        explicit EntryBase(bool is_tick);

        bool is_tick = false;
        bool is_disabled = false;
    };

    // Stores both original-case and lower-cased string views (for case-insensitive filtering).
    struct CallStackEntry : EntryBase, AllNameStringViews
    {
        CallStackEntry() = default;
        CallStackEntry(EMiddlewareHookTarget hook_target,
                       AllNameStringViews strings,
                       uint32_t depth,
                       std::thread::id thread_id,
                       bool is_tick);

        auto render_with_colored_indent_space(int indent_delta, bool with_support_menus) const -> void;
        auto render(int indent_delta, bool with_support_menus) const -> void;

        EMiddlewareHookTarget hook_target = EMiddlewareHookTarget::All;
        uint32_t depth = 0;
        std::thread::id thread_id{};

    private:
        auto render_indents(int indent_delta) const -> void;
        auto render_support_menus() const -> void;
    };

    // Stores both original-case and lower-cased function name views (for case-insensitive filtering).
    struct CallFrequencyEntry : EntryBase, FunctionNameStringViews
    {
        CallFrequencyEntry() = default;
        CallFrequencyEntry(const FunctionNameStringViews& strings, bool is_tick);
        auto render(bool with_support_menus) const -> void;
        uint64_t frequency = 1;

        // OR'd EMiddlewareHookTarget values that have invoked this function so far.
        uint32_t source_flags = 0;
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

    struct UIState
    {
        bool enabled = false;                                                    // [Savable] [Thread-ImGui]
        bool started = false;                                                    // [Thread-ImGui]
        bool show_tick = false;                                                  // [Savable] [Thread-ImGui]
        bool disable_indent_colors = false;                                      // [Savable] [Thread-ImGui]
        EMiddlewareHookTarget hook_target = EMiddlewareHookTarget::ProcessEvent; // [Savable] [Thread-ImGui]
        EMode mode = EMode::Stack;                                               // [Savable] [Thread-ImGui]
        uint16_t dequeue_max_ms = 10;                                            // [Savable] [Thread-ImGui]
        uint16_t dequeue_max_count = 5000;                                       // [Savable] [Thread-ImGui] TODO raise this to uint64

        std::string blacklist;                                                   // [Savable] [Thread-ImGui]
        std::vector<std::string> blacklist_tokens;                               // [Thread-ImGui] (lower-cased tokens)

        std::string whitelist;                                                   // [Savable] [Thread-ImGui]
        std::vector<std::string> whitelist_tokens;                               // [Thread-ImGui] (lower-cased tokens)

        std::vector<ThreadInfo> threads{};                                      // [Thread-ImGui]
        int current_thread = 0;                                                // [Thread-ImGui]

        std::atomic_flag needs_save = ATOMIC_FLAG_INIT;                          // [Thread-Any]
    };
} // namespace RC::EventViewerMod
