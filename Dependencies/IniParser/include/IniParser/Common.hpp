#ifndef INI_PARSER_COMMON_HPP
#define INI_PARSER_COMMON_HPP

#ifndef RC_INI_PARSER_EXPORTS
#ifndef RC_INI_PARSER_BUILD_STATIC
#ifndef RC_INI_PARSER_API
#define RC_INI_PARSER_API __declspec(dllimport)
#endif
#else
#ifndef RC_INI_PARSER_API
#define RC_INI_PARSER_API
#endif
#endif
#else
#ifndef RC_INI_PARSER_API
#define RC_INI_PARSER_API __declspec(dllexport)
#endif
#endif

#endif //INI_PARSER_COMMON_HPP
