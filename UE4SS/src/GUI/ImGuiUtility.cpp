#include <stdexcept>

#include <GUI/ImGuiUtility.hpp>
#undef TEXT
#include <UE4SSProgram.hpp>
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace RC::GUI
{
    auto ImGui_AutoScroll(const char* label, float* previous_max_scroll_y) -> void
    {
        // NOTE: This function makes use of 'imgui_internal.h'.
        //       As a result, this function is prone to break if you update Imgui.
        ImGuiContext& g = *GImGui;
#ifdef WIN32
        const char* child_window_name = NULL;
        ImFormatStringToTempBuffer(&child_window_name, NULL, "%s/%s_%08X", g.CurrentWindow->Name, label, ImGui::GetID(label));
#else
        static char child_window_name[1024];
        sprintf(child_window_name, "%s/%s_%08X", g.CurrentWindow->Name, label, ImGui::GetID(label));
#endif
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
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
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

    static auto dump_json_object_pair(const JSON::TypedKeyValuePair<JSON::Object> element) -> void;
    static auto dump_json_array_pair(const JSON::TypedKeyValuePair<JSON::Array> element) -> void;
    static auto dump_json_string_pair(const JSON::TypedKeyValuePair<JSON::String> element) -> void;
    static auto dump_json_integer_pair(const JSON::TypedKeyValuePair<JSON::Number> element) -> void;
    static auto dump_json_object_value(const JSON::Object& element) -> void;
    static auto dump_json_array_value(const JSON::Array& element) -> void;
    static auto dump_json_string_value(const JSON::String& element) -> void;
    static auto dump_json_integer_value(const JSON::Number& element) -> void;

    static int indent_level{};
    static auto indent() -> SystemStringType
    {
        SystemStringType indents{};
        for (int i = 0; i < indent_level; ++i)
        {
            indents.append(SYSSTR("    "));
        }
        return indents;
    }

    static auto dump_element(const JSON::Value& element) -> void
    {
        if (element.is<JSON::Object>())
        {
            dump_json_object_value(*element.as<JSON::Object>());
        }
        else if (element.is<JSON::Array>())
        {
            dump_json_array_value(*element.as<JSON::Array>());
        }
        else if (element.is<JSON::String>())
        {
            dump_json_string_value(*element.as<JSON::String>());
        }
        else if (element.is<JSON::Number>())
        {
            dump_json_integer_value(*element.as<JSON::Number>());
        }
    }

    static auto dump_element(const JSON::KeyValuePair& element) -> void
    {
        if (element.is<JSON::Object>())
        {
            dump_json_object_pair(element.as<JSON::Object>());
        }
        else if (element.is<JSON::Array>())
        {
            dump_json_array_pair(element.as<JSON::Array>());
        }
        else if (element.is<JSON::String>())
        {
            dump_json_string_pair(element.as<JSON::String>());
        }
        else if (element.is<JSON::Number>())
        {
            dump_json_integer_pair(element.as<JSON::Number>());
        }
    }

    static auto dump_json_object_pair(const JSON::TypedKeyValuePair<JSON::Object> element) -> void
    {
        Output::send<LogLevel::Verbose>(SYSSTR("{}Object: {}\n"), indent(), element.key);

        const auto& object = element.value->get();
        for (const auto& [_, inner_element] : object)
        {
            ++indent_level;
            dump_element(*inner_element.get());
            --indent_level;
        }
    }

    static auto dump_json_array_pair(const JSON::TypedKeyValuePair<JSON::Array> element) -> void
    {
        Output::send<LogLevel::Verbose>(SYSSTR("{}Array: {}\n"), indent(), element.key);

        const auto& array = element.value->get();
        for (const auto& inner_element : array)
        {
            ++indent_level;
            dump_element(*inner_element.get());
            --indent_level;
        }
    }

    static auto dump_json_string_pair(const JSON::TypedKeyValuePair<JSON::String> element) -> void
    {
        Output::send<LogLevel::Verbose>(SYSSTR("{}String: {} == {}\n"), indent(), element.key, element.value->get_view());
    }

    static auto dump_json_integer_pair(const JSON::TypedKeyValuePair<JSON::Number> element) -> void
    {
        Output::send<LogLevel::Verbose>(SYSSTR("{}Integer: {} == {}\n"), indent(), element.key, element.value->get<int64_t>());
    }

    static auto dump_json_object_value(const JSON::Object& element) -> void
    {
        ++indent_level;
        Output::send<LogLevel::Verbose>(SYSSTR("{}Object <anon>\n"), indent());

        const auto& object = element.get();
        for (const auto& [_, inner_element] : object)
        {
            ++indent_level;
            dump_element(*inner_element.get());
            --indent_level;
        }

        --indent_level;
    }

    static auto dump_json_array_value(const JSON::Array& element) -> void
    {
        ++indent_level;
        Output::send<LogLevel::Verbose>(SYSSTR("{}Array <anon>\n"), indent());

        const auto& array = element.get();
        for (const auto& inner_element : array)
        {
            ++indent_level;
            dump_element(*inner_element.get());
            --indent_level;
        }

        --indent_level;
    }

    static auto dump_json_string_value(const JSON::String& element) -> void
    {
        Output::send<LogLevel::Verbose>(SYSSTR("{}String: {}\n"), indent(), element.get_view());
    }

    static auto dump_json_integer_value(const JSON::Number& element) -> void
    {
        Output::send<LogLevel::Verbose>(SYSSTR("{}Integer: {}\n"), indent(), element.get<int64_t>());
    }
} // namespace RC::GUI
