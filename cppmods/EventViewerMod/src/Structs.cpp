#include <Structs.h>

#include <Helpers/String.hpp>

#include <Unreal/UnrealInitializer.hpp>

#include <array>
#include <sstream>
#include <utility>

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

namespace RC::EventViewerMod
{
    EntryBase::EntryBase(std::string text, const bool is_tick)
        : text(std::move(text)), is_tick(is_tick)
    {
    }

    CallStackEntry::CallStackEntry(const EMiddlewareHookTarget hook_target,
                                   const StringType& context_name,
                                   const StringType& function_name,
                                   const uint32_t depth,
                                   const std::thread::id thread_id,
                                   const bool is_tick)
        : EntryBase(std::move(RC::to_string(context_name + STR(".") + function_name)), is_tick),
          hook_target(hook_target),
          depth(depth),
          thread_id(thread_id)
    {
    }

    auto CallStackEntry::render_with_colored_indent_space(const int indent_delta) const -> void
    {
        render_indents(indent_delta);
        const auto indent_width = ImGui::GetStyle().IndentSpacing * static_cast<float>(depth);
        auto min = ImGui::GetCursorScreenPos();
        auto max = min;
        min.x -= indent_width;
        max.y += ImGui::GetTextLineHeight();
        ImGui::GetWindowDrawList()->AddRectFilled(min, max, COLORS[depth % COLORS.size()]);
        ImGui::TextUnformatted(text.c_str());
    }

    auto CallStackEntry::render(const int indent_delta) const -> void
    {
        render_indents(indent_delta);
        ImGui::TextUnformatted(text.c_str());
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

    CallFrequencyEntry::CallFrequencyEntry(std::string text, const bool is_tick)
        : EntryBase(std::move(text), is_tick)
    {
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

    auto TargetInfo::clear() -> void
    {
        threads.clear();
        current_thread = 0;
    }
} // namespace RC::EventViewerMod
