#pragma once

namespace RC::EventViewerMod
{
    class FilterCountRenderer
    {
    public:
        FilterCountRenderer() = default;
        auto add() -> void;
        auto render_and_reset(bool show_tooltip) -> void;
        static auto render(size_t count, bool show_tooltip) -> void;
    private:
        size_t m_count = 0;
    };
}