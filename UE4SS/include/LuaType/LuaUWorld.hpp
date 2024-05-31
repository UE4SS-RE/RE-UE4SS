#pragma once

#include <LuaMadeSimple/LuaObject.hpp>
#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class UWorld;
}

namespace RC::LuaType
{
    struct UWorldName
    {
        constexpr static const char* ToString()
        {
            return "UWorld";
        }
    };
    class UWorld : public RemoteObjectBase<Unreal::UWorld, UWorldName>
    {
      private:
        explicit UWorld(Unreal::UWorld* object);

      public:
        UWorld() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::UWorld*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
