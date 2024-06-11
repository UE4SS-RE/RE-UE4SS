#pragma once

#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/UFunction.hpp>

namespace RC::GUI::Filter
{
    class FunctionParamFlags
    {
      public:
        static inline UEStringType s_debug_name{STR("FunctionParamFlags")};
        static inline bool s_enabled{};
        static inline std::array<bool, 255> s_checkboxes{};
        static inline EPropertyFlags s_value{};
        static inline bool s_include_return_property{};

        static auto post_eval(UObject* object) -> bool
        {
            if (s_value != CPF_None)
            {
                if (!object->IsA<UFunction>())
                {
                    return true;
                }
                auto as_function = static_cast<UFunction*>(object);
                auto first_property = as_function->GetFirstProperty();
                if (!first_property || (first_property->HasAnyPropertyFlags(CPF_ReturnParm) && !first_property->HasNext()))
                {
                    return true;
                }
                bool has_all_required_flags{true};
                for (const auto& param : as_function->ForEachProperty())
                {
                    if (param->HasAnyPropertyFlags(CPF_ReturnParm) && !s_include_return_property)
                    {
                        continue;
                    }
                    has_all_required_flags = param->HasAllPropertyFlags(s_value);
                }
                return !has_all_required_flags;
            }
            else
            {
                return false;
            }
        }
    };
} // namespace RC::GUI::Filter
