#pragma once

#include <string>

// We will use this once we solve the circular dependency issue
//#include <Unreal/Core/HAL/Platform.hpp>

// This is a debug flag to force the use of u16string for testing purposes
// char16_t and wchar_t are two different types, so we need to force the use of one of them
// to ensure we have covered all the cases. 
// #define FORCE_U16

namespace RC {

#ifdef FORCE_U16
    using CharType = char16_t;
    #define STR(str) u ## str
#else
    using CharType = wchar_t;
    #define STR(str) L ## str
#endif

    using StringType = std::basic_string<CharType>;
    using StringViewType = std::basic_string_view<CharType>;
    using StreamIType = std::basic_ifstream<CharType>;
    using StreamOType = std::basic_ofstream<CharType>;
} // namespace RC
