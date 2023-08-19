#ifndef UE4SS_GUI_INCLUDE_DEFAULT_OBJECTS_HPP
#define UE4SS_GUI_INCLUDE_DEFAULT_OBJECTS_HPP

#include <GUI/LiveView/Filter/SearchFilter.hpp>

namespace RC::GUI::Filter
{
    class IncludeDefaultObjects
    {
    public:
        static inline StringType s_debug_name{STR("IncludeDefaultObjects")};
        static inline bool s_enabled{true};

        static auto pre_eval(UObject* object) -> bool
        {
            return !s_enabled && object->HasAnyFlags(static_cast<EObjectFlags>(RF_ClassDefaultObject | RF_ArchetypeObject));
        }
    };
}

#endif //UE4SS_GUI_INCLUDE_DEFAULT_OBJECTS_HPP
