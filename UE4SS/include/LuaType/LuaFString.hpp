#pragma once

#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class FString;
}

namespace RC::LuaType
{
    struct FStringName
    {
        constexpr static const char* ToString()
        {
            return "FString";
        }
    };
    class FString : public LocalObjectBase<Unreal::FString, FStringName>
    {
      private:
        explicit FString(Unreal::FString* object);

      public:
        FString() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FString*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
