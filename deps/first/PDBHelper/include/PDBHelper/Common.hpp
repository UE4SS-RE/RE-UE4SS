#pragma once

#ifndef RC_PDB_HELPER_EXPORTS
#ifndef RC_PDB_HELPER_BUILD_STATIC
#ifndef RC_PDB_API
#define RC_PDB_API __declspec(dllimport)
#endif
#else
#ifndef RC_PDB_API
#define RC_PDB_API
#endif
#endif
#else
#ifndef RC_PDB_API
#define RC_PDB_API __declspec(dllexport)
#endif
#endif
