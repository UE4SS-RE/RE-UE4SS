#pragma once

#include <LuaType/LuaUObject.hpp>
#pragma warning(disable : 4005)
#include <Unreal/Core/Containers/ScriptArray.hpp>
#pragma warning(default : 4005)

namespace RC::Unreal
{
    class UObject;
    class FProperty;
    class FByteProperty;
    class FArrayProperty;
} // namespace RC::Unreal

namespace RC::LuaType
{
    struct PusherParams;

    struct TArrayName
    {
        constexpr static const char* ToString()
        {
            return "TArray";
        }
    };
    class TArray : public RemoteObjectBase<Unreal::FScriptArray, TArrayName>
    {
      private:
        Unreal::UObject* m_base;

        Unreal::FArrayProperty* m_property;
        Unreal::FProperty* m_inner_property;

      private:
        explicit TArray(const PusherParams&);

      public:
        TArray() = delete;
        auto static construct(const PusherParams&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
        auto static handle_unreal_property_value(
                const LuaMadeSimple::Type::Operation, const LuaMadeSimple::Lua&, Unreal::FScriptArray*, int64_t array_index, const TArray& lua_object) -> void;
        auto static prepare_to_handle(const LuaMadeSimple::Type::Operation, const LuaMadeSimple::Lua&) -> void;
    };
} // namespace RC::LuaType
