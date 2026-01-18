#include <EntryCallStackRenderer.hpp>

#include <imgui.h>
#include <Enums.hpp>
#include <HelpStrings.hpp>
#include <UE4SSProgram.hpp>

// EventViewerMod: rendering for the call stack/context modal.
//
// The context vector is prepared by Client (based on the selected history entry). This renderer
// is intentionally simple: it only worries about ImGui state management and printing rows.
namespace RC::EventViewerMod
{
    using namespace std::literals::string_literals;

    EntryCallStackRenderer::EntryCallStackRenderer(const size_t target_idx, std::vector<CallStackEntry> context)
            : m_target_idx(target_idx),
            m_context(std::move(context))
    {
        m_target_ptr = &m_context[m_target_idx];
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
            HelpMarker(HelpStrings::HELP_CEMODAL_SHOW_FULL_CONTEXT);
            ImGui::Checkbox("Disable Indent Colors", &m_disable_indent_colors);

            const auto btn_height = ImGui::GetFrameHeightWithSpacing();
            auto view_area = ImGui::GetContentRegionAvail();
            view_area.y -= btn_height;
            ImGui::BeginChild("##entrycallstackview", view_area, ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle, ImGuiWindowFlags_HorizontalScrollbar);

            int prev_depth = 0;
            bool have_prev = false;
            int current_indent = 0;
            int id = 0;
            uint8_t flags = m_disable_indent_colors ? ECallStackEntryRenderFlags_None : ECallStackEntryRenderFlags_IndentColors;
            flags |= ECallStackEntryRenderFlags_WithSupportMenus;
            for (const auto& entry: m_context)
            {
                if (entry.is_disabled && !m_show_full_context) continue;
                const int depth = static_cast<int>(entry.depth);
                const int delta = have_prev ? (depth - prev_depth) : depth;
                ImGui::PushID(id++);
                &entry != m_target_ptr ? entry.render(delta, static_cast<ECallStackEntryRenderFlags_>(flags))
                                        : entry.render(delta, static_cast<ECallStackEntryRenderFlags_>(flags | ECallStackEntryRenderFlags_Highlight));
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
            HelpMarker(HelpStrings::HELP_CEMODAL_SAVE);
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

            if (ImGui::BeginPopupModal("Saved Entry File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::Text("Saved file to %s", m_last_save_path.c_str());
                ImGui::PopTextWrapPos();
                if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::EndPopup();

            if (!keep_open)
            {
                return false;
            }
        }

        return true;
    }

    auto EntryCallStackRenderer::save() -> void
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
                "EventViewerMod Capture-Entry "s + std::string(m_context[m_target_idx].function_name) + " " +
                oss.str() + ".txt";
        const auto path = captures_root / filename;
        std::wofstream out{path};

        for (const auto& entry : m_context)
        {
            if (entry.is_disabled && !m_show_full_context) continue;
            auto str = entry.to_string_with_prefix();
            out << str;
        }

        out.close();
        m_last_save_path = path.string();
        ImGui::OpenPopup("Saved Entry File");
    }
}
