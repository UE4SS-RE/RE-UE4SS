#pragma once

// EventViewerMod: ImGui-facing front-end.
//
// Owns persistent UI state (filters, selected view, per-thread buffers) and pulls capture entries
// from the Middleware each frame. The hot path (hooking + enqueue) lives in Middleware; Client
// is deliberately written as a consumer that can be throttled (max ms / max count per frame).
//
// Threading notes:
// - render() is expected to be called only on the ImGui thread.
// - request_save_state() may be called from any thread (it uses an atomic flag).

#include <filesystem>
#include <memory>

#include <Middleware.hpp>
#include <Structs.hpp>
#include <FilterCountRenderer.hpp>

#include <UE4SSProgram.hpp>

namespace RC::EventViewerMod
{
    class EntryCallStackRenderer;

    class Client
    {
    public:
        // [Thread-ImGui]
        auto render() -> void;

        // [Thread-Any] Saves state on the next frame.
        auto request_save_state() -> void;

        // [Thread-ImGui]
        auto add_to_white_list(std::string_view item) -> void;

        // [Thread-ImGui]
        auto add_to_black_list(std::string_view item) -> void;

        // [Thread-ImGui]
        auto render_entry_stack_modal(const CallStackEntry* entry) -> void;

        // [Thread-Any]
        static auto GetInstance() -> Client&;

    private:
        Client();

        auto render_cfg() -> void;
        auto render_perf_opts() -> void;
        auto render_view() -> void;

        static auto combo_with_flags(const char* label, int* current_item, const char* const items[], int items_count, ImGuiComboFlags_ flags = ImGuiComboFlags_None) -> bool;

        auto save_state() -> void;
        auto load_state() -> void;
        auto check_save_request() -> bool;

        auto apply_filters_to_history(bool whitelist_changed, bool blacklist_changed, bool tick_changed) -> void;
        auto dequeue() -> void;

        auto passes_filters(std::string_view test_str) const -> bool;

        enum class ESaveMode { none, current, all };
        auto save(ESaveMode mode) -> void;
        auto serialize_view(ThreadInfo& info, EMode mode, EMiddlewareHookTarget hook_target, std::ofstream& out) const -> void;
        auto serialize_all_views(std::ofstream& out) -> void;

        auto clear_threads() -> void;

        UIState m_state{};

        Middleware& m_middleware;

        std::filesystem::path m_cfg_path{};
        std::filesystem::path m_dump_dir{};

        std::unique_ptr<EntryCallStackRenderer> m_entry_call_stack_renderer{};
        FilterCountRenderer m_filter_count_renderer{};

        bool m_imgui_thread_id_set = false;
    };
} // namespace RC::EventViewerMod
