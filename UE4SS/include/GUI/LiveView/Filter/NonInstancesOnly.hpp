#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/UFunction.hpp>

namespace RC::GUI::Filter
{
    class NonInstancesOnly
    {
      public:
        static inline UEStringType s_debug_name{STR("NonInstancesOnly")};
        static inline bool s_enabled{};

        static auto pre_eval(UObject* object) -> bool
        {
            return s_enabled && (object->IsA<UFunction>() || is_instance(object));
        }
    };
} // namespace RC::GUI::Filter
