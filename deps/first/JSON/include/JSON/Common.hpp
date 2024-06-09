#pragma once

#ifdef WIN32

#ifndef RC_JSON_EXPORTS
#ifndef RC_JSON_BUILD_STATIC
#ifndef RC_JSON_API
#define RC_JSON_API __declspec(dllimport)
#endif
#else
#ifndef RC_JSON_API
#define RC_JSON_API
#endif
#endif
#else
#ifndef RC_JSON_API
#define RC_JSON_API __declspec(dllexport)
#endif
#endif

#else

#ifndef RC_JSON_API
#define RC_JSON_API
#endif

#endif

#include <cstdint>

#include <File/File.hpp>

namespace RC::JSON
{
    class Value;

    namespace Parser
    {
        class ScopeStack;
    }

    auto inline indent(int32_t* indent_level, SystemStringType& string) -> void
    {
        if (!indent_level)
        {
            throw std::runtime_error{"Must supply an indent_level pointer"};
        };

        for (int32_t i = 0; i < *indent_level; ++i)
        {
            string.append(SYSSTR("  "));
        }
    }
} // namespace RC::JSON
