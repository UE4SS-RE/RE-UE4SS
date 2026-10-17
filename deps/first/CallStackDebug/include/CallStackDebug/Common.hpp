#pragma once

#ifndef RC_CALL_STACK_DEBUG_EXPORTS
#ifndef RC_CALL_STACK_DEBUG_BUILD_STATIC
#ifndef RC_CALL_STACK_DEBUG_API
#define RC_CALL_STACK_DEBUG_API __declspec(dllimport)
#endif
#else
#ifndef RC_CALL_STACK_DEBUG_API
#define RC_CALL_STACK_DEBUG_API
#endif
#endif
#else
#ifndef RC_CALL_STACK_DEBUG_API
#define RC_CALL_STACK_DEBUG_API __declspec(dllexport)
#endif
#endif
