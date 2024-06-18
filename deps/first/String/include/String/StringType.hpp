#pragma once

#include <string>
#include <type_traits>

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

    // convert a T* pointer to a CharType* pointer and keep the alignment and constness if the size of the types is the same
    template<class T>
    auto static ToCharTypePtr(T* ptr) {
        static_assert(sizeof(T) == sizeof(CharType), "Sizes of T and CharType must be the same");

        using CharPtrType = std::conditional_t<
            std::is_const_v<T>,
            const CharType,
            CharType
        >*;

        return reinterpret_cast<CharPtrType>(ptr);
    }
    
    template<class T, class CharT>
    auto static FromCharTypePtr(CharT* ptr) {
        static_assert(sizeof(T) == sizeof(CharType), "Sizes of T and CharType must be the same");
        static_assert(std::is_same_v<std::decay_t<CharT>, CharType>, "Must be a CharType");
        using TargetPtrType = std::conditional_t<
            std::is_const_v<CharT>,
            const T,
            T
        >*;
        return reinterpret_cast<TargetPtrType>(ptr);
    }
} // namespace RC
