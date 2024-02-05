#pragma once

#include <unordered_map>
#include <vector>

#include "Value.hpp"

namespace RC::Ini
{
    struct Section
    {
        std::unordered_map<SystemStringType, Value> key_value_pairs{};
        std::vector<SystemStringType> ordered_list{};
        bool is_ordered_list{};
    };

    template <typename F>
    concept CallableWithKeyValuePair = std::invocable<F&, SystemStringType, Ini::Value&>;

    class List
    {
      private:
        Section* m_section{};

      public:
        List(Section* section) : m_section(section)
        {
        }

      public:
        auto size() -> size_t
        {
            if (!m_section)
            {
                return 0;
            }
            return m_section->ordered_list.size();
        }

        template <typename Callable>
        auto for_each(Callable callable)
        {
            if (!m_section)
            {
                return;
            }
            if constexpr (CallableWithKeyValuePair<Callable>)
            {
                for (const auto& [key, value] : m_section->key_value_pairs)
                {
                    callable(key, value);
                }
            }
            else
            {
                for (size_t i = 0; i < m_section->ordered_list.size(); ++i)
                {
                    callable(i, m_section->ordered_list[i]);
                }
            }
        }
    };

    // Backwards compatibility just in case some external code made direct references to 'OrderedList'.
    using OrderedList = List;
} // namespace RC::Ini
