#pragma once

#include <JSON/JSON.hpp>
// #include <JSONParser/JSON.hpp>
#include <JSON/Parser/Parser.hpp>
#include <imgui.h>

namespace RC::GUI
{
    auto ImGui_AutoScroll(const char* label, float* previous_max_scroll_y) -> void;
    auto ImGui_InputTextMultiline_WithAutoScroll(const char* label,
                                                 char* buf,
                                                 size_t buf_size,
                                                 const ImVec2& size = ImVec2(0, 0),
                                                 ImGuiInputTextFlags flags = 0,
                                                 ImGuiInputTextCallback callback = 0,
                                                 void* user_data = 0,
                                                 float* previous_max_scroll_y = nullptr);
    auto ImGui_Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
            -> bool;
    auto ImGui_GetID(int int_id) -> ImGuiID;
    auto ImGui_TreeNodeEx(const char* label, int int_id, ImGuiTreeNodeFlags flags = 0) -> bool;
    auto ImGui_TreeNodeEx(const char* label, void* ptr_id, ImGuiTreeNodeFlags flags = 0) -> bool;
    auto ImGui_TreeNodeEx(const char* label, const char* str_id, ImGuiTreeNodeFlags flags) -> bool;
} // namespace RC::GUI
