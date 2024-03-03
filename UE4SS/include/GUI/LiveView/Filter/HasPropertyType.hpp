#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/NameTypes.hpp>

namespace RC::GUI::Filter
{
    class HasPropertyType
    {
      public:
        static inline UEStringType s_debug_name{STR("HasPropertyType")};
        static inline FName s_value{};
        static inline std::string s_internal_value{};

        static auto post_eval(UObject* object) -> bool
        {
            if (!s_value || s_internal_value.empty())
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
                if (property->GetClass().GetFName() == s_value)
                {
                    should_filter_object_out = false;
                    break;
                }
            }
            return should_filter_object_out;
        }
    };
} // namespace RC::GUI::Filter
