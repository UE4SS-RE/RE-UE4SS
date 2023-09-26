#pragma once

#include <LuaType/LuaUObject.hpp>

namespace RC::LuaType
{
    // Special variant of UClass that has to be used when dealing with the ClassPrivate of fields
    // This is because in <4.25, these classes are of type UClass, and in 4.25+ they are FFieldClass
    // This special class is only allowed to use functions that are safe for both (usually through VC)
    struct FFieldClassName
    {
        constexpr static const char* ToString()
        {
            return "FieldClass";
        }
    };
    // class XFieldClass : public RemoteObjectBase<Unreal::FFieldClass, FFieldClassName>
    class XFieldClass : public LocalObjectBase<Unreal::FFieldClassVariant, FFieldClassName>
    {
      private:
        explicit XFieldClass(Unreal::FFieldClassVariant object);

      public:
        XFieldClass() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FFieldClassVariant) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType
