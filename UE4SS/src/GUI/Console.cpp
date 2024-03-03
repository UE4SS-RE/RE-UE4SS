#include <ctype.h>
#include <memory>

#include <DynamicOutput/DynamicOutput.hpp>
#include <GUI/Console.hpp>
#include <GUI/GUI.hpp>
#include <GUI/ImGuiUtility.hpp>
#include <UE4SSProgram.hpp>

#include <imgui_internal.h>

namespace RC::GUI
{
    // Wrapper for CalcTextSizeA.
    auto CalcTextSize(const char* text, float max_width, const char** remaining = nullptr) -> ImVec2
    {
        ImGuiContext& g = *GImGui;
        ImFont* font = g.Font;
        const float font_size = g.FontSize;
        ImVec2 text_size = font->CalcTextSizeA(font_size, max_width, -1.0f, text, nullptr, remaining);
        text_size.x = IM_FLOOR(text_size.x + 0.99999f);
        return text_size;
    }

    auto Console::GetLanguageDefinitionNone() -> const TextEditor::LanguageDefinition&
    {
        static bool inited = false;
        static TextEditor::LanguageDefinition langDef;
        if (!inited)
        {
            langDef.mName = "None";
            inited = true;
        }
        return langDef;
    }

    auto Console::GetPalette() const -> const TextEditor::Palette&
    {
        const static TextEditor::Palette p = {{
                ImGui::ColorConvertFloat4ToU32(ImVec4{0.800f, 0.800f, 0.800f, 1.0f}), // Default
                0xffd69c56,                                                           // Keyword
                0xff00ff00,                                                           // Number
                0xff7070e0,                                                           // String
                0xff70a0e0,                                                           // Char literal
                0xffffffff,                                                           // Punctuation
                0xff408080,                                                           // Preprocessor
                0xffaaaaaa,                                                           // Identifier
                0xff9bc64d,                                                           // Known identifier
                0xffc040a0,                                                           // Preproc identifier
                0xff206020,                                                           // Comment (single line)
                0xff406020,                                                           // Comment (multi line)
                ImGui::ColorConvertFloat4ToU32(ImVec4{0.156f, 0.156f, 0.156f, 1.0f}), // Background
                0xffe0e0e0,                                                           // Cursor
                ImGui::ColorConvertFloat4ToU32(ImVec4{0.65f, 0.24f, 0.57f, 0.38f}),   // Selection
                0x800020ff,                                                           // ErrorMarker
                0x40f08000,                                                           // Breakpoint
                0xff707000,                                                           // Line number
                0x40000000,                                                           // Current line fill
                0x40808080,                                                           // Current line fill (inactive)
                0x40a0a0a0,                                                           // Current line edge
        }};
        return p;
    }

    auto Console::render() -> void
    {
        /*
        auto max_line_width = m_current_console_output_width - 10.0f;
        std::string console_buffer{};
        {
            std::lock_guard<std::mutex> guard(m_lines_mutex);
            for (const auto& line : m_lines)
            {
                if (!m_filter.PassFilter(line.c_str()))
                {
                    continue;
                }
                const char* remaining{};
                auto line_width = CalcTextSize(line.c_str(), m_current_console_output_width, &remaining).x;
                auto max_string_length = remaining - line.c_str();
                if (max_string_length > 0 && line_width > max_line_width)
                {
                    auto num_lines = static_cast<int>(std::ceil(line.length() / max_string_length)) + 1;
                    int last_line{};
                    for (int i = 0; num_lines > 0 && i < num_lines; ++i)
                    {
                        console_buffer.append(line.substr(last_line, max_string_length));
                        console_buffer.append("\n");
                        last_line = last_line + max_string_length;
                    }
                }
                else
                {
                    console_buffer.append(line);
                    console_buffer.append("\n");
                }
            }
        }

        const float footer_height_to_reserve = (ImGui::GetStyle().ItemSpacing.y * 10.0f) + ImGui::GetFrameHeightWithSpacing();
        ImGui_InputTextMultiline_WithAutoScroll("##consolebuffer", console_buffer.data(), console_buffer.size() + 1, {-10, -footer_height_to_reserve},
        ImGuiInputTextFlags_ReadOnly, nullptr, nullptr, &m_previous_max_scroll_y); m_current_console_output_width = ImGui::CalcItemWidth();

        ImGui::Separator();

        ImGui::BeginDisabled(true);
        bool reclaim_focus{};
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory; auto text_edit_callback_wrapper = [](ImGuiInputTextCallbackData* data) -> int { Console* console =
        static_cast<Console*>(data->UserData); Output::send(SYSSTR("text_edit_callback_wrapper\n"));
            //return console->text_edit_callback(data);
            return 0;
        };
        ImGui::PushItemWidth(-12.0f);
        if (ImGui::InputText("##console_input_buffer", m_input_buffer, IM_ARRAYSIZE(m_input_buffer), input_text_flags, text_edit_callback_wrapper, this))
        {
            Output::send(SYSSTR("ConsoleInput\n"));
            reclaim_focus = true;
        }
        ImGui::PopItemWidth();

        ImGui::SetItemDefaultFocus();
        if (reclaim_focus)
        {
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::EndDisabled();
        //*/

        /**/
        const float footer_height_to_reserve = ((ImGui::GetStyle().ItemSpacing.y * 10.0f) + ImGui::GetFrameHeightWithSpacing()) / YDIV;
        {
            std::lock_guard<std::mutex> guard(m_lines_mutex);
            m_text_editor.Render("TextEditor", {-10 / XDIV, -footer_height_to_reserve});
        }
        ImGui_AutoScroll("TextEditor", &m_previous_max_scroll_y);
        //*/
    }

    auto Console::render_search_box() -> void
    {
        m_filter.Draw("Search log", 200 / XDIV);
    }

    static auto LogLevel_to_ImColor(Color::Color color) -> std::pair<ImColor, ImColor>
    {
        switch (color)
        {
        case Color::Default:
            return {g_imgui_text_editor_default_bg_color, g_imgui_text_editor_default_text_color};
        case Color::NoColor:
            return {g_imgui_text_editor_normal_bg_color, g_imgui_text_editor_normal_text_color};
        case Color::Cyan:
            return {g_imgui_text_editor_verbose_bg_color, g_imgui_text_editor_verbose_text_color};
        case Color::Yellow:
            return {g_imgui_text_editor_warning_bg_color, g_imgui_text_editor_warning_text_color};
        case Color::Red:
            return {g_imgui_text_editor_error_bg_color, g_imgui_text_editor_error_text_color};
        case Color::Green:
            return {g_imgui_text_editor_default_bg_color, g_imgui_text_green_color};
        case Color::Blue:
            return {g_imgui_text_editor_default_bg_color, g_imgui_text_blue_color};
        case Color::Purple:
            return {g_imgui_text_editor_default_bg_color, g_imgui_text_purple_color};
        }

        throw std::runtime_error{"[LogLevel_to_ImColor] Unhandled log_level"};
    }

    auto Console::add_line(const std::string& line, Color::Color color) -> void
    {
        std::lock_guard<std::mutex> guard(m_lines_mutex);
        if (m_text_editor.GetTotalLines() < 0)
        {
            throw std::runtime_error{"Somehow we negative amount of lines in the console"};
        }
        if (static_cast<size_t>(m_text_editor.GetTotalLines()) >= m_maximum_num_lines)
        {
            m_text_editor.ClearLines();
        }
        if (m_lines.size() >= m_maximum_num_lines)
        {
            m_lines.clear();
        }
        if (color != Color::Default && color != Color::NoColor)
        {
            m_text_editor.GetLineColorMarkers().emplace(m_text_editor.GetTotalLines() + 1, LogLevel_to_ImColor(color));
        }
        m_lines.emplace_back(line);
        m_text_editor.AddTextLine(line);
    }

    auto Console::add_line(const std::wstring& line, Color::Color color) -> void
    {
        auto ansi_string = to_string(line);
        std::lock_guard<std::mutex> guard(m_lines_mutex);
        if (m_text_editor.GetTotalLines() < 0)
        {
            throw std::runtime_error{"Somehow we negative amount of lines in the console"};
        }
        if (static_cast<size_t>(m_text_editor.GetTotalLines()) >= m_maximum_num_lines)
        {
            m_text_editor.ClearLines();
        }
        if (m_lines.size() >= m_maximum_num_lines)
        {
            m_lines.clear();
        }
        if (color != Color::Default && color != Color::NoColor)
        {
            m_text_editor.GetLineColorMarkers().emplace(m_text_editor.GetTotalLines() + 1, LogLevel_to_ImColor(color));
        }
        m_lines.emplace_back(ansi_string);
        m_text_editor.AddTextLine(ansi_string);
    }
} // namespace RC::GUI
