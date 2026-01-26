#pragma once

// EventViewerMod: Call stack modal renderer.
//
// The main Stack view is a scrolling, time-ordered history. This helper renders a *focused slice*:
// given a selected CallStackEntry, it builds a context vector and renders it in an ImGui modal.
//
// Concepts:
// - "Root" caller: the depth==0 entry that started the call chain. An entry can be its own root.
// - Full context: show all entries produced by the root call chain.
// - Focused context: show the path leading to the selected entry, the entry itself, and then
//   everything that happens underneath that entry.
//
// Note: ImGui modals require calling OpenPopup() on the frame you want the modal to begin opening.

#include <Structs.hpp>
#include <vector>

namespace RC::EventViewerMod
{
    // Renders the call stack/context modal for a single selected entry.
    // The context vector is prepared by the caller (Client) and passed in by value.
    class EntryCallStackRenderer
    {
      public:
        EntryCallStackRenderer() = delete;
        EntryCallStackRenderer(const EntryCallStackRenderer& Other) = delete;
        EntryCallStackRenderer(EntryCallStackRenderer&& Other) noexcept = delete;
        EntryCallStackRenderer& operator=(const EntryCallStackRenderer& Other) = delete;
        EntryCallStackRenderer& operator=(EntryCallStackRenderer&& Other) noexcept = delete;

        EntryCallStackRenderer(size_t target_idx, std::vector<CallStackEntry> context);

        // Returns false when the modal is finished and can be destroyed.
        auto render() -> bool;

      private:
        size_t m_target_idx;
        const CallStackEntry* m_target_ptr;
        std::vector<CallStackEntry> m_context;
        std::string m_last_save_path;
        bool m_disable_indent_colors = false;
        bool m_show_full_context = false;
        // ImGui popups need an explicit OpenPopup() call. We request it once so the modal can appear.
        bool m_requested_open = false;

        auto save() -> void;
    };
} // namespace RC::EventViewerMod
