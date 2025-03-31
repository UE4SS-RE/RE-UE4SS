#include <DynamicOutput/Output.hpp>
#include <LuaType/LuaUFunction.hpp>
#include <LuaType/LuaUInterface.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UInterface.hpp>

namespace RC::LuaType
{
    UInterface::UInterface(Unreal::UInterface* object)
        : UObjectBase<Unreal::UInterface, UInterfaceName>(object) /*LuaMadeSimple::Type::RemoteObject<Unreal::UInterface>("UInterface", object)*/
    {
    }

    auto UInterface::construct(const LuaMadeSimple::Lua& lua, Unreal::UInterface* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        add_to_global_unreal_objects_map(unreal_object);

        LuaType::UInterface lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaType::UObject::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::UInterface>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto UInterface::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto UInterface::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::ToString, []([[maybe_unused]] const LuaMadeSimple::Lua& lua) -> int {
            if (!lua.is_userdata())
            {
                lua.throw_error(std::format("{} __tostring metamethod called but there was no userdata", ClassName::ToString()));
            }

            std::string name;

            auto* uinterface = lua.get_userdata<UInterface>().get_remote_cpp_object();
            name.append(ClassName::ToString());
            name.append(std::format("<{}>: {:016X}", to_string(uinterface->GetName()), reinterpret_cast<uintptr_t>(uinterface)));

            lua.set_string(name);

            return 1;
        });
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto UInterface::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

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
