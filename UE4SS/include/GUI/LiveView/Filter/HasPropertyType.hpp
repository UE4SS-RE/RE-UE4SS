#pragma once

#include <vector>

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/NameTypes.hpp>

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

            auto should_filter = [&](FProperty* property) {
                for (const auto& property_type : list_property_types)
                {
                    if (property->GetClass().GetFName() == property_type)
                    {
                        highlight(property);
                        should_filter_object_out = false;
                        break;
                    }
                }
            };

            for (FProperty* property : as_struct->ForEachProperty())
            {
                should_filter(property);
            }
            for (UStruct* super_struct : as_struct->ForEachSuperStruct())
            {
                for (FProperty* property : super_struct->ForEachProperty())
                {
                    should_filter(property);
                }
            }
            return should_filter_object_out;
        }
    };
} // namespace RC::GUI::Filter
