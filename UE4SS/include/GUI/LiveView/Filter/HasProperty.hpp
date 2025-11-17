#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>

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
            for (const auto& property_string : list_properties)
            {
                if (property_string.empty())
                {
                    continue;
                }
                const auto property_name = FName(FromCharTypePtr<TCHAR>(property_string.c_str()));
                const auto as_struct = object->IsA<UStruct>() ? static_cast<UStruct*>(object) : object->GetClassPrivate();
                FProperty* found_property{};
                for (FProperty* property : as_struct->ForEachProperty())
                {
                    if (property->GetFName().Equals(property_name))
                    {
                        found_property = property;
                        break;
                    }
                }
                for (UStruct* super_struct : as_struct->ForEachSuperStruct())
                {
                    for (FProperty* property : super_struct->ForEachProperty())
                    {
                        if (property->GetFName().Equals(property_name))
                        {
                            found_property = property;
                            break;
                        }
                    }
                }
                if (!found_property)
                {
                    return true;
                }
                else
                {
                    highlight(found_property);
                }
            }

            return false;
        }
    };
} // namespace RC::GUI::Filter
