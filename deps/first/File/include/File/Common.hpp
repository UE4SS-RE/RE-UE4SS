#pragma once

#ifdef WIN32

#ifndef RC_FILE_EXPORTS
#ifndef RC_FILE_BUILD_STATIC
#ifndef RC_FILE_API
#define RC_FILE_API __declspec(dllimport)
#endif
#else
#ifndef RC_FILE_API
#define RC_FILE_API
#endif
#endif
#else
#ifndef RC_FILE_API
#define RC_FILE_API __declspec(dllexport)
#endif
#endif

#else

#ifndef RC_FILE_API
#define RC_FILE_API
#endif

#endif