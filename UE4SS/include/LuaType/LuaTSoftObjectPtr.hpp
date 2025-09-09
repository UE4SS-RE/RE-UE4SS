#pragma once

#include <LuaType/LuaUObject.hpp>
#include <Unreal/Property/FSoftObjectProperty.hpp>

namespace RC::LuaType
{
    struct TSoftObjectPtrName
    {
        constexpr static const char* ToString()
        {
            return "TSoftObjectPtr";
        }
    };
    class TSoftObjectPtr : public LocalObjectBase<Unreal::FSoftObjectPtr, TSoftObjectPtrName>
    {
      private:
        explicit TSoftObjectPtr(Unreal::FSoftObjectPtr object);

      public:
        TSoftObjectPtr() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FSoftObjectPtr&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(LuaMadeSimple::Lua::Table&, std::string_view) -> void;
    };
} // namespace RC::LuaType
