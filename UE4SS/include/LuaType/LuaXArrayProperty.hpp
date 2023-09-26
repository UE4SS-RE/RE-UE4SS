#pragma once

#include <LuaType/LuaXProperty.hpp>

namespace RC::Unreal
{
    class FArrayProperty;
}

namespace RC::LuaType
{
    struct FArrayPropertyName
    {
        constexpr static const char* ToString()
        {
            return "ArrayProperty";
        }
    };
    class XArrayProperty : public RemoteObjectBase<Unreal::FArrayProperty, FArrayPropertyName>
    {
      public:
        using Super = XProperty;

      private:
        explicit XArrayProperty(Unreal::FArrayProperty* object);

      public:
        XArrayProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FArrayProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
