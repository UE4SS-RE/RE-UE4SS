#ifndef UE4SS_REWRITTEN_SOL_GLOBALS_HPP
#define UE4SS_REWRITTEN_SOL_GLOBALS_HPP

#include <Mod/SolHelpers.hpp>

namespace RC
{
    auto setup_lua_global_functions(sol::state_view sol) -> void;
}

#endif // UE4SS_REWRITTEN_SOL_GLOBALS_HPP
