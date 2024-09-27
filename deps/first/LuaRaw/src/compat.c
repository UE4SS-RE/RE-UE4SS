#include "compat.h"

#include "lauxlib.h"
#include "lua.h"


/*
This function creates and pushes on the stack a new full userdata,
with nuvalue associated Lua values, called user values, plus an
associated block of raw memory with size bytes. (The user values
can be set and read with the functions lua_setiuservalue and
lua_getiuservalue.)

The function returns the address of the block of memory. Lua
ensures that this address is valid as long as the corresponding
userdata is alive (see §2.5). Moreover, if the userdata is marked
for finalization (see §2.5.3), its address is valid at least until
the call to its finalizer.
*/
// I
// O +userdata
void* lua_newuserdatauv(lua_State* L, size_t sz, int nuvalue) {
    void* ret = lua_newuserdata(L, sz); // +userdata0

    lua_createtable(L, nuvalue, 0); // +table0
    lua_setuservalue(L, -2); // -table0 => userdata0.fenv

    return ret;
}


/*
Pushes onto the stack the n-th user value associated with the full
userdata at the given index and returns the type of the pushed
value.

If the userdata does not have that value, pushes nil and returns
LUA_TNONE.
*/
// I userdata@idx
// O +any/nil
int lua_getiuservalue(lua_State* L, int idx, int n) {
    // userdata? userdata@idx
    luaL_checktype(L, idx, LUA_TUSERDATA);

    lua_getuservalue(L, idx); // userdata@idx.fenv -> +table0

    if (!lua_istable(L, -1)) { // table? table0
        lua_pop(L, 1); // -table0
        lua_pushnil(L); // +nil0
        return LUA_TNONE;
    }

    // TODO check if environment is wrong

    lua_rawgeti(L, -1, n); // table0[n] -> +any0
    lua_remove(L, -2); // -table0

    return lua_type( L, -1); // ? any0
}


/*
Pops a value from the stack and sets it as the new n-th user value
associated to the full userdata at the given index. Returns 0 if
the userdata does not have that value.
*/
// I userdata@idx, any0
// O -any0
int lua_setiuservalue(lua_State* L, int idx, int n) {
    if (!lua_isuserdata(L, idx)) { // userdata? userdata@idx
        lua_pop(L, 1); // -any0

        // i forgor how to throw proper error
        luaL_checktype(L, idx, LUA_TUSERDATA); // error
        return 0;
    }

    lua_getuservalue(L, idx); // userdata@idx.fenv -> +table0
    if (!lua_istable(L, -1)) { // table? table0
        lua_pop(L, 2); // -table0 -any0

        // TODO proper error "userdata does not have uservalues"
        return 0;
    }

    lua_insert(L, -2); // any0 table0 -> table0 any0
    lua_rawseti(L, -2, n); // -any0 -> table0[n]
    lua_pop(L, 1); // -table0

    // TODO check if originally has the value

    return 1;
}



int lua_isinteger(lua_State *L, int index) {
    if (lua_type(L, index) == LUA_TNUMBER) {
        lua_Number n = lua_tonumber(L, index);
        lua_Integer i = lua_tointeger(L, index);
        if (i == n)
            return 1;
    }
    return 0;
}



// https://github.com/lunarmodules/lua-compat-5.3/blob/master/c-api/compat-5.3.c
static void compat53_reverse (lua_State *L, int a, int b) {
    for (; a < b; ++a, --b) {
        lua_pushvalue(L, a);
        lua_pushvalue(L, b);
        lua_replace(L, a);
        lua_replace(L, b);
    }
}

int lua_absindex(lua_State* L, int i) {
    if (i < 0 && i > LUA_REGISTRYINDEX)
        i += lua_gettop(L) + 1;
    return i;
}

void lua_rotate(lua_State* L, int idx, int n) {
    int n_elems = 0;
    idx = lua_absindex(L, idx);
    n_elems = lua_gettop(L)-idx+1;
    if (n < 0)
        n += n_elems;
    if ( n > 0 && n < n_elems) {
        luaL_checkstack(L, 2, "not enough stack slots available");
        n = n_elems - n;
        compat53_reverse(L, idx, idx+n-1);
        compat53_reverse(L, idx+n, idx+n_elems-1);
        compat53_reverse(L, idx, idx+n_elems-1);
    }
}

const char *luaL_tolstring (lua_State *L, int idx, size_t *len) {
    if (!luaL_callmeta(L, idx, "__tostring")) {  /* no metafield? */
        switch (lua_type(L, idx)) {
            case LUA_TNUMBER:
            case LUA_TSTRING:
                lua_pushvalue(L, idx);
                break;
            case LUA_TBOOLEAN:
                lua_pushstring(L, (lua_toboolean(L, idx) ? "true" : "false"));
                break;
            case LUA_TNIL:
                lua_pushliteral(L, "nil");
                break;
            default:
                lua_pushfstring(L, "%s: %p", luaL_typename(L, idx), lua_topointer(L, idx));
                break;
        }
    }
    return lua_tolstring(L, -1, len);
}
