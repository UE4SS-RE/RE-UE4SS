#ifndef UE4SS_REWRITTEN_LUAUENUM_HPP
#define UE4SS_REWRITTEN_LUAUENUM_HPP

#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class UEnum;
}

namespace RC::LuaType
{
    struct UEnumName { constexpr static const char* ToString() { return "UEnum"; }};
    class UEnum : public RemoteObjectBase<Unreal::UEnum, UEnumName>
    {
    public:
        using Super = UObject;

    private:
        explicit UEnum(Unreal::UEnum* object);
        static inline std::optional<int32_t> metamethods_ref { std::nullopt };

    public:
        UEnum() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::UEnum*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template<LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&, std::string_view) -> void;
    };
}


#endif //UE4SS_REWRITTEN_LUAUENUM_HPP
