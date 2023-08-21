#ifndef UE4SS_GUI_DEFAULT_OBJECTS_ONLY_HPP
#define UE4SS_GUI_DEFAULT_OBJECTS_ONLY_HPP

#include <GUI/LiveView/Filter/SearchFilter.hpp>

namespace RC::GUI::Filter
{
    class DefaultObjectsOnly
    {
    public:
        static inline StringType s_debug_name{STR("DefaultObjectsOnly")};
        static inline bool s_enabled{};

        static auto pre_eval(UObject* object) -> bool
        {
            return s_enabled && !object->HasAnyFlags(static_cast<EObjectFlags>(RF_ClassDefaultObject | RF_ArchetypeObject));
        }
    };
}

#endif //UE4SS_GUI_DEFAULT_OBJECTS_ONLY_HPP
