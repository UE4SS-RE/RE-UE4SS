#include <format>

#include <UVTD/Helpers.hpp>
#include <Helpers/String.hpp>

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
}