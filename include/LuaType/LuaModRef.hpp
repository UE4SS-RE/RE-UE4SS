#ifndef UE4SS_REWRITTEN_LUAMODREF_HPP
#define UE4SS_REWRITTEN_LUAMODREF_HPP

#include <LuaType/LuaUObject.hpp>

namespace RC
{
    class LuaMod;
}

namespace RC::LuaType
{
    struct ModName
    {
        constexpr static const char* ToString()
        {
            return "ModRef";
        }
    };
    class LuaModRef : public RemoteObjectBase<RC::LuaMod, ModName>
    {
    private:
        explicit LuaModRef(RC::LuaMod* object);

    public:
        LuaModRef() = delete;
        auto static construct(const LuaMadeSimple::Lua&, RC::LuaMod*) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template<LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
}


#endif //UE4SS_REWRITTEN_LUAMODREF_HPP
