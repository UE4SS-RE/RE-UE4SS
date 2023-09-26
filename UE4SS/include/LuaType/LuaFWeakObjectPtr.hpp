#pragma once

#include <LuaType/LuaUObject.hpp>
#pragma warning(disable : 4005)
#include <Unreal/FWeakObjectPtr.hpp>
#pragma warning(default : 4005)

namespace RC::Unreal
{
    class UClass;
}

namespace RC::LuaType
{
    struct FWeakObjectPtrName
    {
        constexpr static const char* ToString()
        {
            return "FWeakObjectPtr";
        }
    };
    class FWeakObjectPtr : public LocalObjectBase<Unreal::FWeakObjectPtr, FWeakObjectPtrName>
    {
      private:
        explicit FWeakObjectPtr(Unreal::FWeakObjectPtr object);

      public:
        FWeakObjectPtr() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FWeakObjectPtr) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
