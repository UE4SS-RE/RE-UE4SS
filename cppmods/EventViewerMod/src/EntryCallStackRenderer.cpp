#include "../include/EntryCallStackRenderer.hpp"

#include <imgui.h>

#include "UE4SSProgram.hpp"

namespace RC::EventViewerMod
{
    using namespace std::literals::string_literals;

    EntryCallStackRenderer::EntryCallStackRenderer(const size_t target_idx, std::vector<CallStackEntry> context)
            : m_target_idx(target_idx),
            m_context(std::move(context))
    {
    }

    auto EntryCallStackRenderer::render() -> bool
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        // BeginPopupModal() will *never* return true unless the popup has been opened.
        // Because the renderer is created from a context-menu click (possibly in a different
        // frame / popup), we request opening once here in the stable render path.
        if (!m_requested_open)
        {
            ImGui::OpenPopup("Entry Call Stack##entrycallstackmodal");
            m_requested_open = true;
        }

        if (ImGui::BeginPopupModal("Entry Call Stack##entrycallstackmodal", nullptr, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::Checkbox("Show Full Context", &m_show_full_context);
            ImGui::Checkbox("Disable Indent Colors", &m_disable_indent_colors);

            // auto area = ImGui::GetContentRegionAvail();
            // auto& padding = ImGui::GetStyle().WindowPadding;
            // auto& scroll_size = ImGui::GetStyle().ScrollbarSize;
            // area.y -= ((padding.y + scroll_size) * 2);
            // area.x -= (padding.x + scroll_size);
            ImGui::BeginChild("##entrycallstackview", {0,0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar);

            int prev_depth = 0;
            bool have_prev = false;
            int current_indent = 0;
            int id = 0;
            for (auto entry: m_context)
            {
                if (entry.is_disabled && !m_show_full_context) continue;
                const int depth = static_cast<int>(entry.depth);
                const int delta = have_prev ? (depth - prev_depth) : depth;
                ImGui::PushID(id++);
                m_disable_indent_colors ? entry.render(delta, false) : entry.render_with_colored_indent_space(delta, false); //TODO add render() bool arg to skip call stack renderer
                ImGui::PopID();
                current_indent += delta;
                prev_depth = depth;
                have_prev = true;
            }

            // Reset indent state so the rest of the popup doesn't inherit the last depth.
            while (current_indent > 0)
            {
                ImGui::Unindent();
                --current_indent;
            }

            ImGui::EndChild();

            if (ImGui::Button("Save##entrycallstacksave"))
            {
                save();
            }
            ImGui::SameLine();
            // NOTE:
            // Do *not* early-return before EndPopup().
            // ImGui maintains multiple internal stacks (window stack, ID stack, etc.).
            // Skipping EndPopup() will eventually trip assertions like "Calling PopId() too many times".
            bool keep_open = true;
            if (ImGui::Button("Close##entrycallstackclose"))
            {
                ImGui::CloseCurrentPopup();
                keep_open = false;
            }

            ImGui::EndPopup();

            if (!keep_open)
            {
                return false;
            }
        }

        return true;
    }

    auto EntryCallStackRenderer::save() const -> void
    {
        static const auto wd = std::filesystem::path{StringType{UE4SSProgram::get_program().get_working_directory()}};
        static const auto captures_root = wd / "Mods" / "EventViewerMod" / "captures";
        std::error_code ec;
        std::filesystem::create_directories(captures_root, ec);

        const auto now = std::chrono::system_clock::now();
        const std::time_t now_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm{};
        localtime_s(&local_tm, &now_t);

        std::ostringstream oss;
        // Windows filenames cannot contain ':'.
        oss << std::put_time(&local_tm, "%Y-%m-%d %H-%M-%S");

        const auto filename =
                "EventViewerMod Capture-Entry "s + std::string(m_context[m_target_idx].full_name) + " " +
                oss.str() + ".txt";
        std::ofstream out{captures_root / filename};

        for (const auto& entry : m_context)
        {
            if (entry.is_disabled && !m_show_full_context) continue;
            for (auto i = 0u; i < entry.depth; ++i) out << "\t";
            out << entry.full_name << '\n';
        }

        out.close();

        Output::send<LogLevel::Verbose>(STR("[EventViewerMod] Saved capture to {}"), (captures_root / filename).c_str());
    }
}
