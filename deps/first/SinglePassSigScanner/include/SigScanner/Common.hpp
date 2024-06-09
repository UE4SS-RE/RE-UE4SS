#pragma once

#ifdef WIN32

#ifndef RC_SINGLE_PASS_SIG_SCANNER_EXPORTS
#ifndef RC_SINGLE_PASS_SIG_SCANNER_BUILD_STATIC
#ifndef RC_SPSS_API
#define RC_SPSS_API __declspec(dllimport)
#endif
#else
#ifndef RC_SPSS_API
#define RC_SPSS_API
#endif
#endif
#else
#ifndef RC_SPSS_API
#define RC_SPSS_API __declspec(dllexport)
#endif
#endif

#else

#ifndef RC_SPSS_API
#define RC_SPSS_API
#endif

#endif
