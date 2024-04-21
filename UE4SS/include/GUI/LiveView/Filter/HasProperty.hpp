#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/UClass.hpp>

namespace RC::GUI::Filter
{
    class HasProperty
    {
      public:
        static inline StringType s_debug_name{STR("HasProperty")};
        static inline std::string s_internal_properties{};
        static inline std::vector<StringType> list_properties{};

        static auto post_eval(UObject* object) -> bool
        {
            for (const auto& property : list_properties)
            {
                if (!property.empty() && !object->GetPropertyByNameInChain(property.c_str())) return true;
            }
            return false;
        }
    };
} // namespace RC::GUI::Filter
