#pragma once

#include <LuaType/LuaXProperty.hpp>

namespace RC::Unreal
{
    class FNumericProperty;
}

namespace RC::LuaType
{
    struct FNumericPropertyName
    {
        constexpr static const char* ToString()
        {
            return "NumericProperty";
        }
    };
    class XNumericProperty : public RemoteObjectBase<Unreal::FNumericProperty, FNumericPropertyName>
    {
    public:
        using Super = XProperty;

    private:
        explicit XNumericProperty(Unreal::FNumericProperty* object);

    public:
        XNumericProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FNumericProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
