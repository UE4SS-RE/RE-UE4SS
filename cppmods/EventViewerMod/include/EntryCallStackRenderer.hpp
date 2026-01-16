#pragma once
#include <Structs.h>
#include <vector>

namespace RC::EventViewerMod
{
    class EntryCallStackRenderer
    {
    public:
        EntryCallStackRenderer() = delete;
        EntryCallStackRenderer(const EntryCallStackRenderer& Other) = delete;
        EntryCallStackRenderer(EntryCallStackRenderer&& Other) noexcept = delete;
        EntryCallStackRenderer& operator=(const EntryCallStackRenderer& Other) = delete;
        EntryCallStackRenderer& operator=(EntryCallStackRenderer&& Other) noexcept = delete;

        EntryCallStackRenderer(size_t target_idx, std::vector<CallStackEntry> context);

        auto render() -> bool;
    private:
        size_t m_target_idx;
        const CallStackEntry* m_target_ptr;
        std::vector<CallStackEntry> m_context;
        bool m_disable_indent_colors = false;
        bool m_show_full_context = false;
        // ImGui popups need an explicit OpenPopup() call. We request it once so the modal can appear.
        bool m_requested_open = false;

        auto save() const -> void;
    };
}