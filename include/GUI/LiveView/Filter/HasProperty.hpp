#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>

namespace RC::GUI::Filter
{
    class HasProperty
    {
    public:
        static inline StringType s_debug_name{STR("HasProperty")};
        static inline StringType s_value{};
        static inline std::string s_internal_value{};

        static auto post_eval(UObject* object) -> bool
        {
            return !s_value.empty() && !object->GetPropertyByNameInChain(s_value.c_str());
        }
    };
}


