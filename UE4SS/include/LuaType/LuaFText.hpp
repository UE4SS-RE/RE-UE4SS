#pragma once

#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class FText;
}

namespace RC::LuaType
{
    struct FTextName
    {
        constexpr static const char* ToString()
        {
            return "FText";
        }
    };
    class FText : public LocalObjectBase<Unreal::FText, FTextName>
    {
      private:
        explicit FText(Unreal::FText object);

      public:
        FText() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FText) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
