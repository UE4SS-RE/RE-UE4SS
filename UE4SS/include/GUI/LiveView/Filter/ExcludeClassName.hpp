#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/UClass.hpp>

namespace RC::GUI::Filter
{
    class ExcludeClassName
    {
      public:
        static inline UEStringType s_debug_name{STR("ExcludeClassName")};
        static inline UEStringType s_value{};
        static inline std::string s_internal_value{};

        static auto post_eval(UObject* object) -> bool
        {
            if (!s_value.empty())
            {
                auto class_name = object->GetClassPrivate()->GetName();
                auto pos = class_name.find(s_value);
                return pos != class_name.npos;
            }
            else
            {
                return false;
            }
        }
    };
} // namespace RC::GUI::Filter
