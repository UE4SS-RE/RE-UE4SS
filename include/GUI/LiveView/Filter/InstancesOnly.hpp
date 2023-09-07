#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>

namespace RC::GUI::Filter
{
    class InstancesOnly
    {
    public:
        static inline StringType s_debug_name{STR("InstancesOnly")};
        static inline bool s_enabled{};
        static auto pre_eval(UObject* object) -> bool
        {
            return s_enabled && !is_instance(object);
        }
    };
}


