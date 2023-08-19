#ifndef UE4SS_GUI_NON_INSTANCES_ONLY_HPP
#define UE4SS_GUI_NON_INSTANCES_ONLY_HPP

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/UFunction.hpp>

namespace RC::GUI::Filter
{
    class NonInstancesOnly
    {
    public:
        static inline StringType s_debug_name{STR("NonInstancesOnly")};
        static inline bool s_enabled{};

        static auto pre_eval(UObject* object) -> bool
        {
            return s_enabled && (object->IsA<UFunction>() || is_instance(object));
        }
    };
}

#endif //UE4SS_GUI_NON_INSTANCES_ONLY_HPP
