#ifndef UE4SS_REWRITTEN_LUAMOD_HPP
#define UE4SS_REWRITTEN_LUAMOD_HPP

#include <LuaType/LuaUObject.hpp>

namespace RC
{
    class Mod;
}

namespace RC::LuaType
{
    struct ModName { constexpr static const char* ToString() { return "ModRef"; }};
    class Mod : public RemoteObjectBase<RC::Mod, ModName>
    {
    private:
        explicit Mod(RC::Mod* object);

    public:
        Mod() = delete;
        auto static construct(const LuaMadeSimple::Lua&, RC::Mod*) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template<LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
}


#endif //UE4SS_REWRITTEN_LUAMOD_HPP
