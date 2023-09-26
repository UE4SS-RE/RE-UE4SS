#pragma once

#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class FProperty;
}

namespace RC::LuaType
{
    auto auto_construct_property(const LuaMadeSimple::Lua&, Unreal::FProperty*) -> void;

    struct FPropertyName
    {
        constexpr static const char* ToString()
        {
            return "Property";
        }
    };
    class XProperty : public RemoteObjectBase<Unreal::FProperty, FPropertyName>
    {
      public:
        using Super = RemoteObjectBase<Unreal::FProperty, FPropertyName>;

      private:
        explicit XProperty(Unreal::FProperty* object);

      public:
        XProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      public:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
