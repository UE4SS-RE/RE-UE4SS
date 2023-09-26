#pragma once

#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class AActor;
}

namespace RC::LuaType
{
    struct AActorName
    {
        constexpr static const char* ToString()
        {
            return "AActor";
        }
    };
    class AActor : public UObjectBase<Unreal::AActor, AActorName>
    {
      private:
        explicit AActor(Unreal::AActor* object);

      public:
        AActor() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::AActor*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
