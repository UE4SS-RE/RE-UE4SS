#pragma once

#include <LuaMadeSimple/LuaObject.hpp>
#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class UStruct;
}

namespace RC::LuaType
{
    struct UStructName
    {
        constexpr static const char* ToString()
        {
            return "UClass";
        }
    };
    class UStruct : public UObjectBase<Unreal::UStruct, UStructName>
    {
      public:
        using Super = UObject;

      private:
        explicit UStruct(Unreal::UStruct* object);

      public:
        auto derives_from_class() -> bool override
        {
            return true;
        }

      public:
        UStruct() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::UStruct*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      public:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
