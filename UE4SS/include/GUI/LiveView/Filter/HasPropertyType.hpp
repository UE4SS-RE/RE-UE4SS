#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/NameTypes.hpp>
#include <vector>

namespace RC::GUI::Filter
{
    class HasPropertyType
    {
      public:
        static inline StringType s_debug_name{STR("HasPropertyType")};
        static inline std::string s_internal_property_types{};
        static inline std::vector<FName> list_property_types{};

        static auto post_eval(UObject* object) -> bool
        {
            if (list_property_types.empty() || !object)
            {
                return false;
            }

            auto as_struct = Cast<UStruct>(object);
            if (!as_struct)
            {
                as_struct = object->GetClassPrivate();
            }

            bool should_filter_object_out = true;
            for (const auto& property : as_struct->ForEachPropertyInChain())
            {
                for (const auto& property_type : list_property_types)
                {
                    if (property->GetClass().GetFName() == property_type)
                    {
                        should_filter_object_out = false;
                        break;
                    }
                }
            }
            return should_filter_object_out;
        }
    };
} // namespace RC::GUI::Filter
