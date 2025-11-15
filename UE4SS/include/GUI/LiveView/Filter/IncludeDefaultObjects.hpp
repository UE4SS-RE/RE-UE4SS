#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <GUI/LiveView/Filter/DefaultObjectsOnly.hpp>

namespace RC::GUI::Filter
{
    class IncludeDefaultObjects
    {
      public:
        static inline StringType s_debug_name{STR("IncludeDefaultObjects")};
        static inline bool s_enabled{true};

        static auto pre_eval(UObject* object) -> bool
        {
            return !s_enabled && !DefaultObjectsOnly::s_enabled && object->HasAnyFlags(static_cast<EObjectFlags>(RF_ClassDefaultObject | RF_ArchetypeObject));
        }
    };
} // namespace RC::GUI::Filter
