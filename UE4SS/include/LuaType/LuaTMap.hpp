#pragma once

#include <LuaType/LuaUObject.hpp>

#include <Unreal/Core/UObject/UObjectHierarchyFwd.hpp>

namespace RC::Unreal
{

}

namespace RC::LuaType
{
    struct TMapName
    {
        constexpr static const char* ToString()
        {
            return "TMap";
        }
    };

    class TMap : public RemoteObjectBase<Unreal::FScriptMap, TMapName>
    {
    private:
        Unreal::UObject* m_base;

        Unreal::FMapProperty* m_property;
        Unreal::FProperty* m_key_property;
        Unreal::FProperty* m_value_property;

    private:
        explicit TMap(const PusherParams&);

    public:
        TMap() = delete;

        auto static construct(const PusherParams&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;

        enum class MapOperation
        {
            Find,
            Add,
            Contains,
            Remove,
            Empty,
        };

        auto static prepare_to_handle(MapOperation, const LuaMadeSimple::Lua&) -> void;
    };

    struct FScriptMapInfo
    {
        Unreal::FProperty* key{};
        Unreal::FProperty* value{};

        Unreal::FName key_fname{};
        Unreal::FName value_fname{};

        Unreal::FScriptMapLayout layout{};

        FScriptMapInfo(Unreal::FProperty* key, Unreal::FProperty* value);

        /**
        * Validates existence of lua pushers for this key/values in this structure.
        * Throws if a pusher for a key/value was not found
        *
        * @param lua Lua state to throw against.
        */
        void validate_pushers(const LuaMadeSimple::Lua& lua);
    };
}