#pragma once

#include <array>

#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <LuaType/LuaUObject.hpp>

namespace RC::Unreal
{
    class FProperty;
}

namespace RC::LuaType
{
    struct DynamicUnrealFunctionData
    {
        uint8_t data[0x200]{};
    };

    struct PropertyAndLuaRefPair
    {
        Unreal::FProperty* property;
        int32_t lua_ref;
    };

    class DynamicUnrealFunctionOutParameters
    {
      private:
        size_t m_last{0};
        using ParamsArray = std::array<PropertyAndLuaRefPair, 10>;
        ParamsArray m_params;

      public:
        auto add(const PropertyAndLuaRefPair) -> void;
        auto get(size_t index) -> const PropertyAndLuaRefPair&;
        auto get() -> const ParamsArray&;
    };

    struct UFunctionName
    {
        constexpr static const char* ToString()
        {
            return "UFunction";
        }
    };
    class UFunction : public UObjectBase<Unreal::UFunction, UFunctionName>
    {
      private:
        // This is the 'this' pointer for this UFunction
        Unreal::UObject* m_base{};

      private:
        explicit UFunction(Unreal::UObject* base, Unreal::UFunction* function);

      public:
        UFunction() = delete;
        auto static construct(const LuaMadeSimple::Lua&, Unreal::UObject*, Unreal::UFunction*) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;

      public:
        auto derives_from_ufunction() -> bool override
        {
            return true;
        }

      public:
        auto get_base() -> Unreal::UObject*
        {
            return m_base;
        }
    };
} // namespace RC::LuaType
