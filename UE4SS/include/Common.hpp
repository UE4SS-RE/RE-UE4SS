#pragma once

#ifndef RC_UE4SS_EXPORTS
#ifndef RC_UE4SS_API
#define RC_UE4SS_API __declspec(dllimport)
#endif
#else
#ifndef RC_UE4SS_API
#define RC_UE4SS_API __declspec(dllexport)
#endif
#endif
