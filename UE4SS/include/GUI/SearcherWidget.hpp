#pragma once

#include <string>

struct ImGuiInputTextCallbackData;

namespace RC::GUI
{
    class SearcherWidget
    {
      public:
        enum class SearchMode
        {
            All,
            ByName,
        };

      public:
        using IteratorCallback = void (*)(void* userdata);
        using SearchModeChangedCallback = void (*)(void* userdata, SearchMode);

      public:
        SearcherWidget() = delete;
        SearcherWidget(SearchModeChangedCallback, IteratorCallback all_iterator, IteratorCallback search_iterator, void* userdata = nullptr);

      private:
        friend auto search_field_always_callback(ImGuiInputTextCallbackData* data) -> int;
        constexpr static size_t m_search_buffer_capacity = 2000;
        std::string_view m_default_search_buffer{"Search name..."};
        std::string m_name_to_search_by{};
        char* m_search_by_name_buffer{};
        IteratorCallback m_current_iterator{};
        IteratorCallback m_all_iterator{};
        IteratorCallback m_search_iterator{};
        SearchModeChangedCallback m_ufunction_caller_search_mode_changed_callback{};
        void* m_userdata{};
        bool m_search_field_clear_requested{};
        bool m_search_field_cleared{};
        bool m_is_searching_by_name{};

      public:
        auto render() -> void;

      public:
        auto was_search_requested() -> bool;
        auto get_search_value() -> const std::string&;
    };
} // namespace RC::GUI
