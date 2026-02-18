#include <stdexcept>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <GUI/ImGuiUtility.hpp>
#undef TEXT
#include <UE4SSProgram.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace RC::GUI
{
    auto ImGui_AutoScroll(const char* label, float* previous_max_scroll_y) -> void
    {
        // NOTE: This function makes use of 'imgui_internal.h'.
        //       As a result, this function is prone to break if you update Imgui.
        ImGuiContext& g = *GImGui;
        const char* child_window_name = NULL;
        ImFormatStringToTempBuffer(&child_window_name, NULL, "%s/%s_%08X", g.CurrentWindow->Name, label, ImGui::GetID(label));
        ImGuiWindow* child_window = ImGui::FindWindowByName(child_window_name);
        if (child_window)
        {
            if (child_window->ScrollMax.y > *previous_max_scroll_y)
            {
                ImGui::SetScrollY(child_window, child_window->ScrollMax.y);
            }
            *previous_max_scroll_y = child_window->ScrollMax.y;
        }
    }

    auto ImGui_InputTextMultiline_WithAutoScroll(const char* label,
                                                 char* buf,
                                                 size_t buf_size,
                                                 const ImVec2& size,
                                                 ImGuiInputTextFlags flags,
                                                 ImGuiInputTextCallback callback,
                                                 void* user_data,
                                                 float* previous_max_scroll_y)
    {
        if (!previous_max_scroll_y)
        {
            throw std::runtime_error{"Cannot call 'ImGui_InputTextMultiline_WithAutoScroll' with parameter 'previous_max_scroll_y' == nullptr"};
        }

        ImGui::InputTextMultiline(label, buf, buf_size, size, flags, callback, user_data);
        ImGui_AutoScroll(label, previous_max_scroll_y);
    }

    // https://github.com/ocornut/imgui/issues/319#issuecomment-345795629
    auto ImGui_Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size) -> bool
    {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y) + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = ImVec2(bb.Min.x, bb.Min.y) + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

    auto ImGui_GetID(int int_id) -> ImGuiID
    {
        ImGuiWindow* window = GImGui->CurrentWindow;
        return window->GetID(int_id);
    }

    auto ImGui_TreeNodeEx(const char* label, int int_id, ImGuiTreeNodeFlags flags) -> bool
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        return ImGui::TreeNodeBehavior(window->GetID(int_id), flags, label, NULL);
    }

    auto ImGui_TreeNodeEx(const char* label, void* ptr_id, ImGuiTreeNodeFlags flags) -> bool
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        return ImGui::TreeNodeBehavior(window->GetID(ptr_id), flags, label, NULL);
    }

    auto ImGui_TreeNodeEx(const char* label, const char* str_id, ImGuiTreeNodeFlags flags) -> bool
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        return ImGui::TreeNodeBehavior(window->GetID(str_id), flags, label, NULL);
    }
} // namespace RC::GUI
