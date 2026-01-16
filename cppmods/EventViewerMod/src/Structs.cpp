#include <Structs.h>
#include <Client.h>
#include <Unreal/UnrealInitializer.hpp>

#include <array>
#include <sstream>

#include <imgui.h>
#include <imgui_internal.h>

inline constexpr static unsigned ALPHA = 200;
inline constexpr static std::array COLORS = {
    IM_COL32(255, 0, 0, ALPHA),     // red
    IM_COL32(0, 0, 255, ALPHA),     // blue
    IM_COL32(0, 255, 0, ALPHA),     // green
    IM_COL32(255, 176, 0, ALPHA),   // orange
    IM_COL32(176, 255, 0, ALPHA),   // lime
    IM_COL32(0, 255, 255, ALPHA),   // cyan
    IM_COL32(255, 255, 0, ALPHA),   // yellow
};
inline constexpr static auto SELECTED_COLOR = ImVec4{1.0f, 1.0f, 0.0f, 1.0f};

namespace RC::EventViewerMod
{
    auto copy_to_clipboard(const std::string_view& string) -> void
    {
        ImGui::LogToClipboard();
        ImVec2 dummy {0,0};
        ImGui::LogRenderedText(&dummy, string.data(), string.data() + string.size());
        ImGui::LogFinish();
    };

    EntryBase::EntryBase(const bool is_tick)
        : is_tick(is_tick)
    {
    }

    CallStackEntry::CallStackEntry(const EMiddlewareHookTarget hook_target,
                                   const AllNameStringViews strings,
                                   const uint32_t depth,
                                   const std::thread::id thread_id,
                                   const bool is_tick)
        : EntryBase(is_tick),
          hook_target(hook_target),
          depth(depth),
          thread_id(thread_id)
    {
        // Inheritance is used to avoid extra indirection/caches misses.
        // (StringPool owns the backing storage for these views.)
        function_hash = strings.function_hash;
        function_name = strings.function_name;
        lower_cased_function_name = strings.lower_cased_function_name;
        full_name = strings.full_name;
        lower_cased_full_name = strings.lower_cased_full_name;
    }

    auto CallStackEntry::render(const int indent_delta, const ECallStackEntryRenderFlags_ flags) const -> void
    {
        render_indents(indent_delta);
        if (flags & ECallStackEntryRenderFlags_IndentColors)
        {
            const auto indent_width = ImGui::GetStyle().IndentSpacing * static_cast<float>(depth);
            auto min = ImGui::GetCursorScreenPos();
            auto max = min;
            min.x -= indent_width;
            max.y += ImGui::GetTextLineHeight();
            ImGui::GetWindowDrawList()->AddRectFilled(min, max, COLORS[depth % COLORS.size()]);
        }
        if (flags & ECallStackEntryRenderFlags_Highlight) [[unlikely]]
        {
            ImGui::TextColored(SELECTED_COLOR, to_prefix_string(hook_target));
            ImGui::SameLine();
            ImGui::TextColored(SELECTED_COLOR, full_name.data());
        }
        else [[likely]]
        {
            ImGui::TextUnformatted(to_prefix_string(hook_target));
            ImGui::SameLine();
            ImGui::TextUnformatted(full_name.data());
        }

        if (flags & ECallStackEntryRenderFlags_WithSupportMenus)
        {
            render_support_menus(flags);
        }
    }

    auto CallStackEntry::to_string_with_prefix() const -> std::wstring
    {
        std::wstring out;
        for (uint32_t i = 0; i < depth; ++i) out += L"\t";
        out += ensure_str(to_prefix_string(hook_target)) + ensure_str(full_name) + L"\n";
        return out;
    }

    auto CallStackEntry::render_indents(const int indent_delta) const -> void
    {
        if (indent_delta > 0)
        {
            for (int i = 0; i < indent_delta; ++i)
            {
                ImGui::Indent();
            }
        }
        else if (indent_delta < 0)
        {
            for (int i = indent_delta; i != 0; ++i)
            {
                ImGui::Unindent();
            }
        }
    }

    auto CallStackEntry::render_support_menus(const ECallStackEntryRenderFlags_ flags) const -> void
    {
        ImGui::SetItemTooltip("Right click for options");
        if (ImGui::BeginPopupContextItem("EntryPopup##ep", ImGuiPopupFlags_MouseButtonRight))
        {
            if (flags & ECallStackEntryRenderFlags_WithSupportMenusCallStackModal & ~ECallStackEntryRenderFlags_WithSupportMenus)
            {
                if (ImGui::MenuItem("Show Call Stack")) Client::GetInstance().render_entry_stack_modal(this); // need to do this outside
                ImGui::Separator();
            }
            if (ImGui::MenuItem("Copy Caller Name##ccn")) copy_to_clipboard({full_name.begin(), function_name.begin() - 1});
            if (ImGui::MenuItem("Copy Function Name##cfn")) copy_to_clipboard(function_name);
            if (ImGui::MenuItem("Copy Full Name##cfln")) copy_to_clipboard(full_name);
            ImGui::Separator();
            if (ImGui::MenuItem("Add Caller to Whitelist##cwl")) Client::GetInstance().add_to_white_list({full_name.begin(), function_name.begin() - 1});
            if (ImGui::MenuItem("Add Function to Whitelist##fwl")) Client::GetInstance().add_to_white_list(function_name);
            if (ImGui::MenuItem("Add Full Name to Whitelist##fnwl")) Client::GetInstance().add_to_white_list(full_name);
            ImGui::Separator();
            if (ImGui::MenuItem("Add Caller to Blacklist##cbl")) Client::GetInstance().add_to_black_list({full_name.begin(), function_name.begin() - 1});
            if (ImGui::MenuItem("Add Function to Blacklist##fbl")) Client::GetInstance().add_to_black_list(function_name);
            if (ImGui::MenuItem("Add Full Name to Blacklist##fnb")) Client::GetInstance().add_to_black_list(full_name);
            ImGui::EndPopup();
        }
    }

    CallFrequencyEntry::CallFrequencyEntry(const FunctionNameStringViews& strings, const bool is_tick)
        : EntryBase(is_tick)
    {
        function_hash = strings.function_hash;
        function_name = strings.function_name;
        lower_cased_function_name = strings.lower_cased_function_name;
    }

    auto CallFrequencyEntry::render(const ECallFrequencyEntryRenderFlags_ flags) const -> void
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(function_name.data());
        if (flags & ECallFrequencyEntryRenderFlags_WithSupportMenus) render_support_menus();
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%llu", static_cast<unsigned long long>(frequency));
    }

    auto CallFrequencyEntry::render_support_menus() const -> void
    {
        ImGui::SetItemTooltip("Right click for options");
        if (ImGui::BeginPopupContextItem("EntryPopup##fep", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem("Copy Function Name##cfn")) copy_to_clipboard(function_name);
            if (ImGui::MenuItem("Add Function to Whitelist##fwl")) Client::GetInstance().add_to_white_list(function_name);
            if (ImGui::MenuItem("Add Function to Blacklist##fbl"))Client::GetInstance().add_to_black_list(function_name);
            ImGui::EndPopup();
        }
    }

    ThreadInfo::ThreadInfo(const std::thread::id thread_id)
        : thread_id(thread_id),
          is_game_thread(RC::Unreal::GetGameThreadId() == thread_id)
    {
    }

    auto ThreadInfo::id_string() -> const char*
    {
        if (m_id_string.empty())
        {
            std::stringstream ss;
            ss << thread_id;
            m_id_string = ss.str();
            if (is_game_thread)
            {
                m_id_string += " (Game)";
            }
        }
        return m_id_string.c_str();
    }

    auto ThreadInfo::clear() -> void
    {
        call_frequencies.clear();
        call_stack.clear();
    }
} // namespace RC::EventViewerMod
