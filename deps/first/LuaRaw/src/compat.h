
#include <lua.h>
#include <lauxlib.h>


// https://github.com/lunarmodules/lua-compat-5.3/blob/master/c-api/compat-5.3.h

#define lua_getglobal(L, n) \
  (lua_getfield((L), LUA_GLOBALSINDEX, (n)), lua_type((L), -1))

#define lua_getuservalue(L, i) \
  (lua_getfenv((L), (i)), lua_type((L), -1))
#define lua_setuservalue(L, i) \
  (luaL_checktype((L), -1, LUA_TTABLE), lua_setfenv((L), (i)))

#define lua_rawget(L, i) \
  (lua_rawget((L), (i)), lua_type((L), -1))
#define lua_rawgeti(L, i, n) \
  (lua_rawgeti((L), (i), (n)), lua_type((L), -1))

// regression: rawlen does not trigger metamethods but objlen does
#define lua_rawlen(L, i) lua_objlen((L), (i))

// maybe would work as no-op???
#define lua_resetthread(L) 


void* lua_newuserdatauv(lua_State* L, size_t sz, int nuvalue);
int lua_getiuservalue(lua_State* L, int idx, int n);
int lua_setiuservalue(lua_State* L, int idx, int n);

int lua_isinteger (lua_State* L, int index);

int lua_absindex(lua_State* L, int i);
void lua_rotate(lua_State* L, int idx, int n);

const char *luaL_tolstring (lua_State *L, int idx, size_t *len);
