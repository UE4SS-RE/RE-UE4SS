#pragma once

#include <LuaType/LuaUStruct.hpp>

namespace RC::Unreal
{
    class UStruct;
    class UScriptStruct;
    class FStructProperty;
} // namespace RC::Unreal

namespace RC::LuaType
{
    // Simple wrapper that contains the UScriptStruct* and the FStructProperty*
    struct ScriptStructWrapper
    {
        Unreal::UScriptStruct* script_struct{};
        void* start_of_struct{};
        Unreal::FStructProperty* property{};

        // Accessor the LocalParam API
        void* get_data_ptr()
        {
            return start_of_struct;
        }
    };

    struct UScriptStructName
    {
        constexpr static const char* ToString()
        {
            return "UScriptStruct";
        }
    };
    class UScriptStruct : public LocalObjectBase<ScriptStructWrapper, UScriptStructName>
    {
      public:
        using Super = UStruct;

      private:
        explicit UScriptStruct(ScriptStructWrapper object);

      public:
        UScriptStruct() = delete;
        auto static construct(const LuaMadeSimple::Lua&, ScriptStructWrapper&) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, BaseObject&) -> const LuaMadeSimple::Lua::Table;

      private:
        auto static setup_metamethods(BaseObject&) -> void;

      private:
        template <LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table&) -> void;
        auto static handle_unreal_property_value(const LuaMadeSimple::Type::Operation operation,
                                                 const LuaMadeSimple::Lua&,
                                                 ScriptStructWrapper&,
                                                 Unreal::FName property_name) -> void;
        auto static prepare_to_handle(const LuaMadeSimple::Type::Operation, const LuaMadeSimple::Lua&) -> void;
    };
} // namespace RC::LuaType
