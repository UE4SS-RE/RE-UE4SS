#include <format>

#include <Helpers/Casting.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaUScriptStruct.hpp>
#include <LuaType/LuaUStruct.hpp>
#include <LuaType/LuaXStructProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/UScriptStruct.hpp>
#pragma warning(default : 4005)
#include <DynamicOutput/Output.hpp>

namespace RC::LuaType
{
    UScriptStruct::UScriptStruct(ScriptStructWrapper object) : LocalObjectBase<ScriptStructWrapper, UScriptStructName>(std::move(object))
    {
    }

    auto UScriptStruct::construct(const LuaMadeSimple::Lua& lua, ScriptStructWrapper& unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        add_to_global_unreal_objects_map(unreal_object.script_struct);

        LuaType::UScriptStruct lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            // LuaType::Super::construct(lua, lua_object);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::UScriptStruct>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto UScriptStruct::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = lua.prepare_new_table();

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto UScriptStruct::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Index, [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(LuaMadeSimple::Type::Operation::Get, lua);
            return 1;
        });

        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::NewIndex, [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(LuaMadeSimple::Type::Operation::Set, lua);
            return 0;
        });
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto UScriptStruct::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        table.add_pair("GetBaseAddress", [](const LuaMadeSimple::Lua& lua) -> int {
            // Update: We are no longer storing the base so this function has no use anymore
            lua.throw_error("WARNING! Use of deprecated & removed function 'UScriptStruct::GetBaseAddress'!");
            return 0;
        });

        table.add_pair("GetStructAddress", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<UScriptStruct>();

            auto* data = Helper::Casting::ptr_cast<void*>(lua_object.get_local_cpp_object().start_of_struct);
            lua.set_integer(std::bit_cast<uintptr_t>(data));

            return 1;
        });

        table.add_pair("GetPropertyAddress", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<UScriptStruct>();

            lua.set_integer(std::bit_cast<uintptr_t>(lua_object.get_local_cpp_object().property));

            return 1;
        });

        table.add_pair("IsValid", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<UScriptStruct>();

            if (lua_object.get_local_cpp_object().script_struct)
            {
                lua.set_bool(true);
            }
            else
            {
                lua.set_bool(false);
            }

            return 1;
        });

        table.add_pair("IsMappedToObject", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<UScriptStruct>();

            if (lua_object.get_local_cpp_object().start_of_struct)
            {
                lua.set_bool(true);
            }
            else
            {
                lua.set_bool(false);
            }

            return 1;
        });

        table.add_pair("IsMappedToProperty", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<UScriptStruct>();

            if (lua_object.get_local_cpp_object().property)
            {
                lua.set_bool(true);
            }
            else
            {
                lua.set_bool(false);
            }

            return 1;
        });

        table.add_pair("GetProperty", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<UScriptStruct>();
            XStructProperty::construct(lua, lua_object.get_local_cpp_object().property);
            return 1;
        });

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.set_string(ClassName::ToString());
                return 1;
            });

            // If this is the final object then we also want to finalize creating the table
            // If not then it's the responsibility of the overriding object to call 'make_global()'
            // table.make_global(ClassName::ToString());
        }
    }

    auto UScriptStruct::handle_unreal_property_value(const LuaMadeSimple::Type::Operation operation,
                                                     const LuaMadeSimple::Lua& lua,
                                                     ScriptStructWrapper& struct_data,
                                                     RC::Unreal::FName property_name) -> void
    {
        // Access the given property in the given UScriptStruct

        auto property = static_cast<Unreal::FStructProperty*>(struct_data.script_struct->FindProperty(property_name));
        if (!property)
        {
            lua.throw_error(fmt::format("[handle_unreal_property_value]: Was unable to retrieve property '{}' mapped to '{}'",
                                        to_string(property_name.ToString()),
                                        to_string(struct_data.script_struct->GetFullName())));
        }

        auto property_type_fname = property->GetClass().GetFName();
        int32_t name_comparison_index = property_type_fname.GetComparisonIndex();

        if (StaticState::m_property_value_pushers.contains(name_comparison_index))
        {
            void* data = Helper::Casting::ptr_cast<void*>(struct_data.start_of_struct, property->GetOffset_Internal());

            const PusherParams pusher_params{.operation = operation, .lua = lua, .base = nullptr, .data = data, .property = property};
            StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
        }
        else
        {
            std::string property_type_name = to_string(property_type_fname.ToString());
            lua.throw_error(fmt::format("Tried accessing unreal property without a registered handler (via StructProperty). Property type '{}' not supported.",
                                        property_type_name));
        }
    }

    auto UScriptStruct::prepare_to_handle(const LuaMadeSimple::Type::Operation operation, const LuaMadeSimple::Lua& lua) -> void
    {
        auto& lua_object = lua.get_userdata<UScriptStruct>();

        Unreal::FName property_name = Unreal::FName(to_wstring(lua.get_string()));

        // Check if property_name is 'NONE'
        if (property_name.GetComparisonIndex() == 0)
        {
            // No property was found so lets return nil and let the Lua script handle this failure
            lua.set_nil();
            return;
        }

        handle_unreal_property_value(operation, lua, lua_object.get_local_cpp_object(), property_name);
    }
} // namespace RC::LuaType
