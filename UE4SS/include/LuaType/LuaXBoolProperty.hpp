#pragma once

#include <LuaType/LuaXProperty.hpp>

namespace RC::Unreal
{
    class FBoolProperty;
}

namespace RC::LuaType
{
    struct FBoolPropertyName
    {
        constexpr static const char* ToString()
        {
            return "ObjectProperty";
        }
    };
    class XBoolProperty : public RemoteObjectBase<Unreal::FBoolProperty, FBoolPropertyName>
    {
      public:
        using Super = XProperty;

      private:
        explicit XBoolProperty(Unreal::FBoolProperty* object);

      public:
        XBoolProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FBoolProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
