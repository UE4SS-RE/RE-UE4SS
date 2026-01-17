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

        auto render(int indent_delta, ECallStackEntryRenderFlags_ flags = ECallStackEntryRenderFlags_None) const -> void;

        // Provides a copy of the entry's key string fields. Don't use with ImGui, only use for logging/saving.
        // Use the string_views for ImGui since they utilize the string pool.
        auto to_string_with_prefix() const -> std::wstring;

        EMiddlewareHookTarget hook_target = EMiddlewareHookTarget::All;
        uint32_t depth = 0;
        std::thread::id thread_id{};

    private:
        auto render_indents(int indent_delta) const -> void;
        auto render_support_menus(ECallStackEntryRenderFlags_ flags) const -> void;
    };

    // Stores both original-case and lower-cased function name views (for case-insensitive filtering).
    struct CallFrequencyEntry : EntryBase, FunctionNameStringViews
    {
        CallFrequencyEntry() = default;
        CallFrequencyEntry(const FunctionNameStringViews& strings, bool is_tick);
        auto render(ECallFrequencyEntryRenderFlags_ flags) const -> void;
        uint64_t frequency = 1;

        // OR'd EMiddlewareHookTarget values that have invoked this function so far.
        uint32_t source_flags = 0;

    private:
        auto render_support_menus() const -> void;
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
        bool show_tick = true;                                                   // [Savable] [Thread-ImGui]
        bool disable_indent_colors = false;                                      // [Savable] [Thread-ImGui]
        bool thread_explicitly_chosen = false;                                   // [Thread-ImGui] User selected a thread
        bool thread_implicitly_set = false;                                      // [Thread-ImGui] System set thread to game thread if not explicit
        EMiddlewareHookTarget hook_target = EMiddlewareHookTarget::All;          // [Savable] [Thread-ImGui]
        EMode mode = EMode::Stack;                                               // [Savable] [Thread-ImGui]
        uint16_t dequeue_max_ms = 10;                                            // [Savable] [Thread-ImGui]
        uint16_t text_virtualization_count = 100;                                // [Savable] [Thread-ImGui]
        uint64_t text_temp_virtualization_count = text_virtualization_count;
        uint32_t dequeue_max_count = 100000;                                     // [Savable] [Thread-ImGui]

        std::string blacklist;                                                   // [Savable] [Thread-ImGui]
        std::vector<std::string> blacklist_tokens;                               // [Thread-ImGui] (lower-cased tokens)

        std::string whitelist;                                                   // [Savable] [Thread-ImGui]
        std::vector<std::string> whitelist_tokens;                               // [Thread-ImGui] (lower-cased tokens)

        std::vector<ThreadInfo> threads{};                                      // [Thread-ImGui]
        int current_thread = 0;                                                // [Thread-ImGui]

        std::atomic_flag needs_save = ATOMIC_FLAG_INIT;                          // [Thread-Any]
        std::string last_save_path; //[Thread-ImGui]
    };
} // namespace RC::EventViewerMod
