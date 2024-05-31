#pragma once

#include <LuaType/LuaXProperty.hpp>

namespace RC::Unreal
{
    class FEnumProperty;
}

namespace RC::LuaType
{
    struct FEnumPropertyName
    {
        constexpr static const char* ToString()
        {
            return "EnumProperty";
        }
    };
    class XEnumProperty : public RemoteObjectBase<Unreal::FEnumProperty, FEnumPropertyName>
    {
      public:
        using Super = XProperty;

      private:
        explicit XEnumProperty(Unreal::FEnumProperty* object);

      public:
        XEnumProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FEnumProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
