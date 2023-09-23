// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

/*
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
*/

// Lua was compiled as C++ so 'extern "C"' should not be used here
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
