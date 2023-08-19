#ifndef UE4SS_GUI_INSTANCES_ONLY_HPP
#define UE4SS_GUI_INSTANCES_ONLY_HPP

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

#endif //UE4SS_GUI_INSTANCES_ONLY_HPP
