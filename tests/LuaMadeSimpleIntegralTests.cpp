#include <cstdint>
#include <exception>

#include <LuaMadeSimple/LuaMadeSimple.hpp>

int main()
{
    auto lua = RC::LuaMadeSimple::new_state();
    auto values = lua.prepare_new_table();

    constexpr std::int64_t expected_value = -4'294'967'296LL;
    try
    {
        values.add_pair("signed_64", expected_value);
    }
    catch (const std::exception&)
    {
        return 1;
    }

    values.make_global("Values");
    lua_getglobal(lua.get_lua_state(), "Values");
    lua_getfield(lua.get_lua_state(), -1, "signed_64");

    if (!lua_isinteger(lua.get_lua_state(), -1))
    {
        return 2;
    }
    if (lua_tointeger(lua.get_lua_state(), -1) != expected_value)
    {
        return 3;
    }

    return 0;
}
