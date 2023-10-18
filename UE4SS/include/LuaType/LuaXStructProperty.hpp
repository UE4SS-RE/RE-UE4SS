#pragma once

#include <LuaType/LuaXProperty.hpp>

namespace RC::Unreal
{
    class FStructProperty;
}

namespace RC::LuaType
{
    struct FStructPropertyName
    {
        constexpr static const char* ToString()
        {
            return "StructProperty";
        }
    };
    class XStructProperty : public RemoteObjectBase<Unreal::FStructProperty, FStructPropertyName>
    {
      public:
        using Super = XProperty;

      private:
        explicit XStructProperty(Unreal::FStructProperty* object);

      public:
        XStructProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FStructProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
