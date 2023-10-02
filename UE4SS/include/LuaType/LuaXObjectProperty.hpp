#pragma once

#include <LuaType/LuaXProperty.hpp>

namespace RC::Unreal
{
    class FObjectProperty;
}

namespace RC::LuaType
{
    struct FObjectPropertyName
    {
        constexpr static const char* ToString()
        {
            return "ObjectProperty";
        }
    };
    class XObjectProperty : public RemoteObjectBase<Unreal::FObjectProperty, FObjectPropertyName>
    {
      public:
        using Super = XProperty;

      private:
        explicit XObjectProperty(Unreal::FObjectProperty* object);

      public:
        XObjectProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FObjectProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
