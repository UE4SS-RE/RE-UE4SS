#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/UClass.hpp>

namespace RC::GUI::Filter
{
    class ClassNamesFilter
    {
      public:
        static inline StringType s_debug_name{STR("ClassNamesFilter")};
        static inline std::string s_internal_class_names{};
        static inline std::vector<StringType> list_class_names;
        static inline bool b_is_exclude{true};

        static auto post_eval(UObject* object) -> bool
        {
            if (!list_class_names.empty() && object)
            {
                auto class_name = object->GetClassPrivate()->GetName();
                if (b_is_exclude)
                {
                    return std::ranges::find(list_class_names, class_name) != list_class_names.end();
                }
                else
                {
                    auto it = std::ranges::find(list_class_names, class_name);
                    return it == list_class_names.end();
                }
            }
            return false;
        }
    };
} // namespace RC::GUI::Filter
