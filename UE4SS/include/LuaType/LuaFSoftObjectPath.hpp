#pragma once

#include <LuaType/LuaUObject.hpp>
#include <Unreal/Property/FSoftObjectProperty.hpp>

namespace RC::LuaType
{
    struct FSoftObjectPathName
    {
        constexpr static const char* ToString()
        {
            return "FSoftObjectPath";
        }
    };
    class FSoftObjectPath : public LocalObjectBase<Unreal::FSoftObjectPath, FSoftObjectPathName>
    {
      private:
        explicit FSoftObjectPath(Unreal::FSoftObjectPath& object);

      public:
        FSoftObjectPath() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FSoftObjectPath&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(LuaMadeSimple::Lua::Table&, std::string_view) -> void;
    };
} // namespace RC::LuaType
