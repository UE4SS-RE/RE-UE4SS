#pragma once

#include <LuaType/LuaUObject.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>

namespace RC::LuaType
{
    struct TSoftClassPtrName
    {
        constexpr static const char* ToString()
        {
            return "TSoftClassPtr";
        }
    };
    class TSoftClassPtr : public LocalObjectBase<Unreal::FSoftObjectPtr, TSoftClassPtrName>
    {
      private:
        explicit TSoftClassPtr(Unreal::FSoftObjectPtr object);

      public:
        TSoftClassPtr() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FSoftObjectPtr&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(LuaMadeSimple::Lua::Table&, std::string_view) -> void;
    };
} // namespace RC::LuaType
