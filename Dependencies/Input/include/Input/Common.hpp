#ifndef RC_INPUT_COMMON_HPP
#define RC_INPUT_COMMON_HPP

#ifndef RC_INPUT_EXPORTS
#ifndef RC_INPUT_BUILD_STATIC
#ifndef RC_INPUT_API
#define RC_INPUT_API __declspec(dllimport)
#endif
#else
#ifndef RC_INPUT_API
#define RC_INPUT_API
#endif
#endif
#else
#ifndef RC_INPUT_API
#define RC_INPUT_API __declspec(dllexport)
#endif
#endif

#endif //RC_INPUT_COMMON_HPP
