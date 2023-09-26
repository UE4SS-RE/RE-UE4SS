#pragma once

#ifndef RC_FUNCTION_TIMER_EXPORTS
#ifndef RC_FUNCTION_TIMER_BUILD_STATIC
#ifndef RC_FNCTMR_API
#define RC_FNCTMR_API __declspec(dllimport)
#endif
#else
#ifndef RC_FNCTMR_API
#define RC_FNCTMR_API
#endif
#endif
#else
#ifndef RC_FNCTMR_API
#define RC_FNCTMR_API __declspec(dllexport)
#endif
#endif
