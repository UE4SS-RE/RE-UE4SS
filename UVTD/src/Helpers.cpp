#include <format>

#include <Helpers/String.hpp>
#include <UVTD/Helpers.hpp>

namespace RC::UVTD
{
    auto to_string_type(const char* c_str) -> UEStringType
    {
        return to_ue(c_str);
    }

    auto change_prefix(UEStringType input, bool is_425_plus) -> std::optional<UEStringType>
    {
        for (const auto& prefixed : s_uprefix_to_fprefix)
        {
            for (size_t index = input.find(prefixed); index != input.npos; index = input.find(prefixed))
            {
                if (is_425_plus) return {};
                input.replace(index, 1, STR("F"));
                index++;
            }
        }

        return input;
    }

    auto unify_uobject_array_if_needed(UEStringType& out_variable_type) -> bool
    {
        static constexpr UEStringViewType fixed_uobject_array_string = STR("FFixedUObjectArray");
        static constexpr UEStringViewType chunked_fixed_uobject_array_string = STR("FChunkedFixedUObjectArray");
        if (auto fixed_uobject_array_pos = out_variable_type.find(fixed_uobject_array_string); fixed_uobject_array_pos != out_variable_type.npos)
        {
            out_variable_type.replace(fixed_uobject_array_pos, fixed_uobject_array_string.length(), STR("TUObjectArray"));
            return true;
        }
        else if (auto chunked_fixed_uobject_array_pos = out_variable_type.find(chunked_fixed_uobject_array_string); chunked_fixed_uobject_array_pos != out_variable_type.npos)
        {
            out_variable_type.replace(chunked_fixed_uobject_array_pos, chunked_fixed_uobject_array_string.length(), STR("TUObjectArray"));
            return true;
        }
        else
        {
            return false;
        }
    }
} // namespace RC::UVTD