#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaXArrayProperty.hpp>
#include <LuaType/LuaXBoolProperty.hpp>
#include <LuaType/LuaXEnumProperty.hpp>
#include <LuaType/LuaXFieldClass.hpp>
#include <LuaType/LuaXObjectProperty.hpp>
#include <LuaType/LuaXProperty.hpp>
#include <LuaType/LuaXStructProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    auto auto_construct_property(const LuaMadeSimple::Lua& lua, Unreal::FProperty* property) -> void
    {
        // If the FProperty is nullptr (which is valid), then construct an empty Lua object to enable chaining
        if (!property)
        {
            XProperty::construct(lua, nullptr);
        }
        else if (auto* as_object_property = Unreal::CastField<Unreal::FObjectProperty>(property); as_object_property)
        {
            XObjectProperty::construct(lua, as_object_property);
        }
        else if (auto* as_bool_property = Unreal::CastField<Unreal::FBoolProperty>(property); as_bool_property)
        {
            XBoolProperty::construct(lua, as_bool_property);
        }
        else if (auto* as_struct_property = Unreal::CastField<Unreal::FStructProperty>(property); as_struct_property)
        {
            XStructProperty::construct(lua, as_struct_property);
        }
        else if (auto* as_enum_property = Unreal::CastField<Unreal::FEnumProperty>(property); as_enum_property)
        {
            XEnumProperty::construct(lua, as_enum_property);
        }
        else if (auto* as_array_property = Unreal::CastField<Unreal::FArrayProperty>(property); as_array_property)
        {
            XArrayProperty::construct(lua, as_array_property);
        }
        else
        {
            XProperty::construct(lua, property);
        }
    }

    XProperty::XProperty(Unreal::FProperty* object) : RemoteObjectBase<Unreal::FProperty, FPropertyName>(object)
    {
    }

    auto XProperty::construct(const LuaMadeSimple::Lua& lua, Unreal::FProperty* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // XProperty has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        table.add_pair("GetClass", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XProperty>();

            LuaType::XFieldClass::construct(lua, lua_object.get_remote_cpp_object()->GetClass());
            return 1;
        });

        table.add_pair("GetFullName", [](const LuaMadeSimple::Lua& lua) -> int {
            // Get the userdata from the Lua stack
            // We're making an assumption here about the type
            // I know of no way to get around this assumption
            const auto& lua_object = lua.get_userdata<XProperty>();

            if (lua_object.get_remote_cpp_object())
            {
                // Set the return value to the ansi version of the full name
                lua.set_string(to_system(lua_object.get_remote_cpp_object()->GetFullName()).c_str());
            }
            else
            {
                // We have a nullptr, lets return 'nil' for easy object verification in Lua
                lua.set_nil();
            }

            return 1;
        });

        table.add_pair("GetFName", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XProperty>();
            LuaType::FName::construct(lua, lua_object.get_remote_cpp_object()->GetFName());
            return 1;
        });

        table.add_pair("GetOffset_Internal", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<XProperty>();
            lua.set_integer(lua_object.get_remote_cpp_object()->GetOffset_Internal());
            return 1;
        });

        table.add_pair("IsA", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'IsA'.
Overloads:
#1: IsA(PropertyTypes PropertyType)"};

            const auto& lua_object = lua.get_userdata<XProperty>();
            if (lua.is_table())
            {
                int64_t ffield_class_pointer{};
                lua.for_each_in_table([&](const LuaMadeSimple::LuaTableReference<LuaMadeSimple::Lua>& table) {
                    if (table.key.is_string() && table.key.get_string() == "FFieldClassPointer")
                    {
                        if (!table.value.is_integer())
                        {
                            lua.throw_error("Table value for key 'FFieldClassPointer' must be integer");
                        }
                        ffield_class_pointer = table.value.get_integer();
                        return true;
                    }

                    return false;
                });

                if (!ffield_class_pointer)
                {
                    lua.throw_error("Could not find FFieldClassPointer");
                }

                if (Unreal::Version::IsAtLeast(4, 25))
                {
                    auto* ffield_class = std::bit_cast<Unreal::FFieldClass*>(ffield_class_pointer);
                    lua.set_bool(lua_object.get_remote_cpp_object()->IsA(ffield_class));
                }
                else
                {
                    auto* ffield_class = std::bit_cast<Unreal::UClass*>(ffield_class_pointer);
                    lua.set_bool(lua_object.get_remote_cpp_object()->IsA(ffield_class));
                }
                return 1;
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            lua.set_bool(false);
            return 1;
        });

        table.add_pair("ContainerPtrToValuePtr", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ContainerPtrToValuePtr'.
Overloads:
#1: ContainerPtrToValuePtr(UObjectDerivative Container, integer ArrayIndex = 0))"};

            const auto& lua_object = lua.get_userdata<XProperty>();

            if (!lua.is_userdata())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            const auto& container = lua.get_userdata<UObject>();

            int32_t array_index{};
            if (lua.is_integer())
            {
                array_index = static_cast<int32_t>(lua.get_integer());
            }
            else if (lua.is_nil())
            {
                lua.discard_value();
            }

            void* data = lua_object.get_remote_cpp_object()->ContainerPtrToValuePtr<uint8_t>(container.get_remote_cpp_object(), array_index);
            lua_pushlightuserdata(lua.get_lua_state(), data);
            return 1;
        });

        table.add_pair("ImportText", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'ImportText'.
Overloads:
#1: ImportText(string Buffer, lightuserdata Data, integer PortFlags, UObject OwnerObject))"};

            const auto& lua_object = lua.get_userdata<XProperty>();

            SystemStringType buffer;
            if (lua.is_string())
            {
                buffer = to_system(lua.get_string());
            }
            else
            {
                throw std::runtime_error{error_overload_not_found};
            }

            void* data{};
            if (lua_islightuserdata(lua.get_lua_state(), 1))
            {
                data = lua_touserdata(lua.get_lua_state(), 1);
                lua_remove(lua.get_lua_state(), 1);
            }
            else
            {
                throw std::runtime_error{error_overload_not_found};
            }

            int32_t port_flags{};
            if (lua.is_integer())
            {
                port_flags = static_cast<int32_t>(lua.get_integer());
            }
            else
            {
                throw std::runtime_error{error_overload_not_found};
            }

            if (!lua.is_userdata())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            auto* owner_object = lua.get_userdata<UObject>().get_remote_cpp_object();

            lua_object.get_remote_cpp_object()->ImportText(to_ue(buffer).c_str(), data, port_flags, owner_object, nullptr);
            return 0;
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
} // namespace RC::LuaType
