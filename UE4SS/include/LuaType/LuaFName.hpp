#pragma once

#include <LuaType/LuaUObject.hpp>
#include <Unreal/NameTypes.hpp>

namespace RC::LuaType
{
    struct FNameName
    {
        constexpr static const char* ToString()
        {
            return "FName";
        }
    };
    class FName : public LocalObjectBase<Unreal::FName, FNameName>
    {
      private:
        explicit FName(Unreal::FName object);

      public:
        FName() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FName) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
