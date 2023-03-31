#ifndef RC_DYNAMICOUTPUT_COMMON_HPP
#define RC_DYNAMICOUTPUT_COMMON_HPP

#ifndef RC_DYNAMIC_OUTPUT_EXPORTS
#ifndef RC_DYNAMIC_OUTPUT_BUILD_STATIC
#ifndef RC_DYNOUT_API
#define RC_DYNOUT_API __declspec(dllimport)
#endif
#else
#ifndef RC_DYNOUT_API
#define RC_DYNOUT_API
#endif
#endif
#else
#ifndef RC_DYNOUT_API
#define RC_DYNOUT_API __declspec(dllexport)
#endif
#endif

#endif //RC_DYNAMICOUTPUT_COMMON_HPP
