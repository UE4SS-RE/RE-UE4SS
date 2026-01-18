#include <FilterCountRenderer.hpp>
#include <imgui.h>

namespace RC::EventViewerMod
{
    auto FilterCountRenderer::add() -> void
    {
        ++m_count;
    }

    auto FilterCountRenderer::render_and_reset(const bool show_tooltip) -> void
    {
        if (!m_count) return;
        static ImVec4 text_color { 1.0f, 1.0f, 0.0f, 1.0f };
        ImGui::TextColored(text_color, "<%llu Calls Filtered>", m_count);
        if (show_tooltip) ImGui::SetItemTooltip("The call above and the call below may not be related. Right click an entry -> Show Call Stack to see related calls.");
        m_count = 0;
    }
}