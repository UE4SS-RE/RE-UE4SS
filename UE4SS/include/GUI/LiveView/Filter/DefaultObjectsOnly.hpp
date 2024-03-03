#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>

namespace RC::GUI::Filter
{
    class DefaultObjectsOnly
    {
      public:
        static inline UEStringType s_debug_name{STR("DefaultObjectsOnly")};
        static inline bool s_enabled{};

        static auto pre_eval(UObject* object) -> bool
        {
            return s_enabled && !object->HasAnyFlags(static_cast<EObjectFlags>(RF_ClassDefaultObject | RF_ArchetypeObject));
        }
    };
} // namespace RC::GUI::Filter
