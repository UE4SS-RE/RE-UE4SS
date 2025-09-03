#pragma once

#include <LuaType/LuaUObject.hpp>
#pragma warning(disable : 4005)
#include <Unreal/Core/Containers/Set.hpp>
#pragma warning(default : 4005)

namespace RC::Unreal
{
    class UObject;
    class FProperty;
    class FSetProperty;
}

namespace RC::LuaType
{
    struct PusherParams;

    struct TSetName
    {
        constexpr static const char* ToString()
        {
            return "TSet";
        }
    };

    class TSet : public RemoteObjectBase<Unreal::FScriptSet, TSetName>
    {
    private:
        Unreal::UObject* m_base;
        Unreal::FSetProperty* m_property;
        Unreal::FProperty* m_element_property;

    private:
        explicit TSet(const PusherParams&);

    public:
        TSet() = delete;
        auto static construct(const PusherParams&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;

        enum class SetOperation
        {
            Add,
            Contains,
            Remove,
            Empty,
            ForEach
        };

        auto static prepare_to_handle(SetOperation, const LuaMadeSimple::Lua&) -> void;
    };

    struct FScriptSetInfo
    {
        Unreal::FProperty* element{};
        Unreal::FName element_fname{};
        Unreal::FScriptSetLayout layout{};

        FScriptSetInfo(Unreal::FProperty* element);

        /**
         * Validates existence of lua pushers for this element in this structure.
         * Throws if a pusher for the element was not found
         *
         * @param lua Lua state to throw against.
         */
        void validate_pushers(const LuaMadeSimple::Lua& lua);
    };
}