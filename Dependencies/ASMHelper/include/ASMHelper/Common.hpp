#ifndef RC_ASM_HELPER_COMMON_HPP
#define RC_ASM_HELPER_COMMON_HPP

#ifndef RC_ASM_HELPER_EXPORTS
#ifndef RC_ASM_HELPER_BUILD_STATIC
#ifndef RC_ASM_API
#define RC_ASM_API __declspec(dllimport)
#endif
#else
#ifndef RC_ASM_API
#define RC_ASM_API
#endif
#endif
#else
#ifndef RC_ASM_API
#define RC_ASM_API __declspec(dllexport)
#endif
#endif

#endif //RC_ASM_HELPER_COMMON_HPP
