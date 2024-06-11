#include <GUI/SearcherWidget.hpp>

#include <imgui.h>

namespace RC::GUI
{
    SearcherWidget::SearcherWidget(SearchModeChangedCallback ufunction_caller_search_mode_changed_callback,
                                   IteratorCallback all_iterator,
                                   IteratorCallback search_iterator,
                                   void* userdata)
    {
        m_search_by_name_buffer = new char[m_search_buffer_capacity];
        strncpy_s(m_search_by_name_buffer,
                  m_default_search_buffer.size() + sizeof(char),
                  m_default_search_buffer.data(),
                  m_default_search_buffer.size() + sizeof(char));
        m_all_iterator = all_iterator;
        m_search_iterator = search_iterator;
        m_ufunction_caller_search_mode_changed_callback = ufunction_caller_search_mode_changed_callback;
        m_userdata = userdata;
    }

    auto SearcherWidget::was_search_requested() -> bool
    {
        return m_is_searching_by_name;
    }

    auto SearcherWidget::get_search_value() -> const std::string&
    {
        return m_name_to_search_by;
    }

    auto search_field_always_callback(ImGuiInputTextCallbackData* data) -> int
    {
        auto typed_this = static_cast<SearcherWidget*>(data->UserData);
        if (typed_this->m_search_field_clear_requested && !typed_this->m_search_field_cleared)
        {
            strncpy_s(data->Buf, 1, "", 1);
            data->BufTextLen = 0;
            data->BufDirty = true;
            typed_this->m_search_field_cleared = true;
        }
        return 1;
    }

    auto SearcherWidget::render() -> void
    {
        if (ImGui::InputText("##Search by name",
                             m_search_by_name_buffer,
                             m_search_buffer_capacity,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways,
                             &search_field_always_callback,
                             this))
        {
            std::string search_buffer{m_search_by_name_buffer};
            if (search_buffer.empty())
            {
                // Output::send(SYSSTR("Search all functions\n"));
                m_name_to_search_by.clear();
                m_current_iterator = m_all_iterator;
                m_is_searching_by_name = false;
                m_ufunction_caller_search_mode_changed_callback(m_userdata, SearchMode::All);
            }
            else
            {
                // Output::send(SYSSTR("Search for: {}\n"), search_buffer.empty() ? STR("") : to_wstring(search_buffer));
                m_name_to_search_by = search_buffer;
                m_current_iterator = m_search_iterator;
                m_is_searching_by_name = true;
                m_ufunction_caller_search_mode_changed_callback(m_userdata, SearchMode::ByName);
            }
        }
        if (!m_is_searching_by_name && m_search_field_clear_requested && !ImGui::IsItemActive())
        {
            strncpy_s(m_search_by_name_buffer,
                      m_default_search_buffer.size() + sizeof(char),
                      m_default_search_buffer.data(),
                      m_default_search_buffer.size() + sizeof(char));
            m_search_field_clear_requested = false;
            m_search_field_cleared = false;
        }
        if (ImGui::IsItemClicked())
        {
            if (!m_is_searching_by_name)
            {
                m_search_field_clear_requested = true;
            }
        }
    }
} // namespace RC::GUI
