#pragma once

#include <LuaType/LuaUObject.hpp>
#include <Unreal/FURL.hpp>

namespace RC::LuaType
{
    struct FURLName
    {
        constexpr static const char* ToString()
        {
            return "FURL";
        }
    };

    class FURL : public LocalObjectBase<Unreal::FURL, FURLName>
    {
      private:
        explicit FURL(Unreal::FURL object);

      public:
        FURL() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FURL) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
