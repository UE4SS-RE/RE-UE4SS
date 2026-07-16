#pragma once

#ifndef RC_HELPERS_EXPORTS
#ifndef RC_HELPERS_BUILD_STATIC
#ifndef RC_HELPERS_API
#define RC_HELPERS_API __declspec(dllimport)
#endif
#else
#ifndef RC_HELPERS_API
#define RC_HELPERS_API
#endif
#endif
#else
#ifndef RC_HELPERS_API
#define RC_HELPERS_API __declspec(dllexport)
#endif
#endif
