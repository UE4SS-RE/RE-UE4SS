#ifndef LUAWRAPPERGENERATOR_LUASETUP_HPP
#define LUAWRAPPERGENERATOR_LUASETUP_HPP

#include <atomic>
#include <format>
#include <functional>

#include <LuaBindings/States/MainState/Main.hpp>

namespace RC::LuaBindings
{
static std::unordered_map<std::string, void (*)(lua_State*)> s_state_setup_functions{
    {"MainState", &lua_setup_state_MainState},
};

auto lua_setup_state(lua_State* lua_state, const std::string& state_name) -> void
{
    if (auto it = s_state_setup_functions.find(state_name); it != s_state_setup_functions.end())
    {
        it->second(lua_state);
    }
    else
    {
        luaL_error(lua_state, std::format("Was unable to find lua state type '{}'", state_name).c_str());
    }
}
} // RC::LuaBindings

#endif //LUAWRAPPERGENERATOR_LUASETUP_HPP
