#ifndef RC_PARSER_BASE_COMMON_HPP
#define RC_PARSER_BASE_COMMON_HPP

#ifndef RC_EXPORT
#ifndef RC_PARSER_BASE_BUILD_STATIC
#ifndef RC_PB_API
#define RC_PB_API __declspec(dllimport)
#endif
#else
#ifndef RC_PB_API
#define RC_PB_API
#endif
#endif
#else
#ifndef RC_PB_API
#define RC_PB_API __declspec(dllexport)
#endif
#endif

#endif //RC_PARSER_BASE_COMMON_HPP
