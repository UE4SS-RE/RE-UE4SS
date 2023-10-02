#pragma once

#include <LuaMadeSimple/LuaObject.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaUStruct.hpp>

namespace RC::Unreal
{
    class UInterface;
}

namespace RC::LuaType
{
    struct UInterfaceName
    {
        constexpr static const char* ToString()
        {
            return "UInterface";
        }
    };
    class UInterface : public UObjectBase<Unreal::UInterface, UInterfaceName>
    {
      public:
        using Super = LuaType::UStruct;

      private:
        explicit UInterface(Unreal::UInterface* object);

      public:
        auto derives_from_class() -> bool override
        {
            return true;
        }

      public:
        UInterface() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::UInterface*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      public:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
