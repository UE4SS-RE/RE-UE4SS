#include <format>

#include <Helpers/String.hpp>
#include <UVTD/ConfigUtil.hpp>
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
        // Use ConfigUtil instead of hardcoded list
        for (const auto& prefixed : ConfigUtil::GetUPrefixToFPrefix())
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

    auto normalize_type_for_comparison(const File::StringType& type) -> File::StringType
    {
        File::StringType normalized = type;

        // TSizedDefaultAllocator<32> replaced FDefaultAllocator but is the same thing
        size_t pos = 0;
        while ((pos = normalized.find(STR("TSizedDefaultAllocator<32>"), pos)) != File::StringType::npos)
        {
            normalized.replace(pos, 26, STR("FDefaultAllocator"));
        }

        // Remove spaces before '>' (e.g. "TArray<T >" vs "TArray<T>")
        pos = 0;
        while ((pos = normalized.find(STR(" >"), pos)) != File::StringType::npos)
        {
            normalized.erase(pos, 1);
        }

        while (!normalized.empty() && normalized.back() == ' ')
        {
            normalized.pop_back();
        }

        // Inside containers, TObjectPtr<T> is layout-equivalent to T*
        if (normalized.starts_with(STR("TArray<")) ||
            normalized.starts_with(STR("TSet<")) ||
            normalized.starts_with(STR("TMap<")))
        {
            size_t tobj_pos = 0;
            while ((tobj_pos = normalized.find(STR("TObjectPtr<"), tobj_pos)) != File::StringType::npos)
            {
                size_t start = tobj_pos + 11;
                int depth = 1;
                size_t end = start;
                while (end < normalized.size() && depth > 0)
                {
                    if (normalized[end] == '<') depth++;
                    else if (normalized[end] == '>') depth--;
                    if (depth > 0) end++;
                }

                if (depth == 0)
                {
                    File::StringType inner_type = normalized.substr(start, end - start);
                    normalized.replace(tobj_pos, end - tobj_pos + 1, inner_type + STR("*"));
                }
                else
                {
                    tobj_pos++;
                }
            }
        }

        // Normalize pointer spacing: "T *" -> "T*"
        pos = 0;
        while ((pos = normalized.find(STR(" *"), pos)) != File::StringType::npos)
        {
            normalized.erase(pos, 1);
        }

        return normalized;
    }

    auto unify_uobject_array_if_needed(StringType& out_variable_type) -> bool
    {
        static constexpr StringViewType fixed_uobject_array_string = STR("FFixedUObjectArray");
        static constexpr StringViewType chunked_fixed_uobject_array_string = STR("FChunkedFixedUObjectArray");
        static constexpr StringViewType non_chunked_410_and_earlier = STR("TStaticIndirectArrayThreadSafeRead<UObjectBase,8388608,16384>");
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
        else if (auto non_chunked_410_and_earlier_array_pos = out_variable_type.find(non_chunked_410_and_earlier); non_chunked_410_and_earlier_array_pos != out_variable_type.npos)
        {
            out_variable_type.replace(non_chunked_410_and_earlier_array_pos, non_chunked_410_and_earlier.length(), STR("TUObjectArray"));
            return true;
        }
        else
        {
            return false;
        }
    }
} // namespace RC::UVTD