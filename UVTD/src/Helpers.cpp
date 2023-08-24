#include <format>

#include <Helpers/String.hpp>
#include <UVTD/Helpers.hpp>

namespace RC::UVTD
{
    auto to_string_type(const char* c_str) -> File::StringType
    {
#if RC_IS_ANSI == 1
        return File::StringType(c_str);
#else
        size_t count = strlen(c_str) + 1;
        wchar_t* converted_method_name = new wchar_t[count];

        size_t num_of_char_converted = 0;
        mbstowcs_s(&num_of_char_converted, converted_method_name, count, c_str, count);

        auto converted = File::StringType(converted_method_name);

        delete[] converted_method_name;

        return converted;
#endif
    }

    auto change_prefix(File::StringType input, bool is_425_plus) -> std::optional<File::StringType>
    {
        for (const auto& prefixed : uprefix_to_fprefix)
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

    auto unify_uobject_array_if_needed(StringType& out_variable_type) -> bool
    {
        static constexpr StringViewType fixed_uobject_array_string = STR("FFixedUObjectArray");
        static constexpr StringViewType chunked_fixed_uobject_array_string = STR("FChunkedFixedUObjectArray");
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