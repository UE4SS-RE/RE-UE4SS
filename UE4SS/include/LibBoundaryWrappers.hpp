#pragma once

#include <cstdint>

#include <String/StringType.hpp>

// Everything in this file is subject to ABI breakage, be extremely careful!
// Everything in this file must be exported.

namespace RC
{
    struct RC_UE4SS_API CString
    {
        const CharType* data{};
        size_t size{};
    };
}
