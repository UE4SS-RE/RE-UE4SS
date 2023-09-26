#pragma once

#include <LuaType/LuaXProperty.hpp>

namespace RC::Unreal
{
    class FInterfaceProperty;
}

namespace RC::LuaType
{
    struct FInterfacePropertyName
    {
        constexpr static const char* ToString()
        {
            return "InterfaceProperty";
        }
    };
    class XInterfaceProperty : public RemoteObjectBase<Unreal::FInterfaceProperty, FInterfacePropertyName>
    {
      public:
        using Super = XProperty;

      private:
        explicit XInterfaceProperty(Unreal::FInterfaceProperty* object);

      public:
        XInterfaceProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FInterfaceProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
