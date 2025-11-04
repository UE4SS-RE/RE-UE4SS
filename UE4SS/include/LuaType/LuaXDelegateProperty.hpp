#pragma once

#include <LuaType/LuaXProperty.hpp>

namespace RC::Unreal
{
    class FDelegateProperty;
    class FMulticastDelegateProperty;
    class FMulticastSparseDelegateProperty;
}

namespace RC::LuaType
{
    struct PusherParams;

    // FDelegateProperty (single-cast delegate)
    struct FDelegatePropertyName
    {
        constexpr static const char* ToString()
        {
            return "DelegateProperty";
        }
    };

    class XDelegateProperty : public RemoteObjectBase<Unreal::FDelegateProperty, FDelegatePropertyName>
    {
    public:
        using Super = XProperty;

    private:
        explicit XDelegateProperty(Unreal::FDelegateProperty* object);

    public:
        XDelegateProperty() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FDelegateProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };

    // FMulticastDelegateProperty (multicast inline delegate)
    struct FMulticastDelegatePropertyName
    {
        constexpr static const char* ToString()
        {
            return "MulticastDelegateProperty";
        }
    };

    class XMulticastDelegateProperty : public RemoteObjectBase<Unreal::FMulticastDelegateProperty, FMulticastDelegatePropertyName>
    {
    public:
        using Super = XProperty;

    private:
        Unreal::UObject* m_base{};
        Unreal::FMulticastDelegateProperty* m_property{};

    private:
        explicit XMulticastDelegateProperty(Unreal::FMulticastDelegateProperty* object);
        explicit XMulticastDelegateProperty(const PusherParams&);

    public:
        XMulticastDelegateProperty() = delete;
        auto static construct(const PusherParams&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FMulticastDelegateProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };

    // FMulticastSparseDelegateProperty (multicast sparse delegate)
    struct FMulticastSparseDelegatePropertyName
    {
        constexpr static const char* ToString()
        {
            return "MulticastSparseDelegateProperty";
        }
    };

    class XMulticastSparseDelegateProperty : public RemoteObjectBase<Unreal::FMulticastSparseDelegateProperty, FMulticastSparseDelegatePropertyName>
    {
    public:
        using Super = XProperty;

    private:
        Unreal::UObject* m_base{};
        Unreal::FMulticastSparseDelegateProperty* m_property{};

    private:
        explicit XMulticastSparseDelegateProperty(Unreal::FMulticastSparseDelegateProperty* object);
        explicit XMulticastSparseDelegateProperty(const PusherParams&);

    public:
        XMulticastSparseDelegateProperty() = delete;
        auto static construct(const PusherParams&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::FMulticastSparseDelegateProperty*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;

    private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
    };
} // namespace RC::LuaType