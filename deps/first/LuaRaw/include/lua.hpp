// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

// When Lua is compiled as C, we need extern "C" wrappers
// When Lua is compiled as C++, we don't want them
#ifndef LUA_COMPILED_AS_CPP
extern "C" {
#endif
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
#ifndef LUA_COMPILED_AS_CPP
}
#endif