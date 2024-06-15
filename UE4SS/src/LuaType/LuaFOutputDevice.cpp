#include <Helpers/String.hpp>
#include <LuaType/LuaFOutputDevice.hpp>

#pragma warning(disable : 4005)
#include <Unreal/FOutputDevice.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    FOutputDevice::FOutputDevice(Unreal::FOutputDevice* object) : RemoteObjectBase<Unreal::FOutputDevice, FOutputDeviceName>(object)
    {
    }

    auto FOutputDevice::construct(const LuaMadeSimple::Lua& lua, Unreal::FOutputDevice* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::FOutputDevice lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            SelfType::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::FOutputDevice>(metatable_name, lua_object.get_metamethods());
        }
        // Transfer the object & its ownership fully to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto FOutputDevice::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FOutputDevice>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto FOutputDevice::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // FOutputDevice has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto FOutputDevice::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("Log", [](const LuaMadeSimple::Lua& lua) -> int {
            std::string error_overload_not_found{R"(
No overload found for function 'Log'.
Overloads:
#1: Log(string Message))"};

            const auto& lua_object = lua.get_userdata<FOutputDevice>();

            if (!lua.is_string())
            {
                throw std::runtime_error{error_overload_not_found};
            }
            auto message = lua.get_string();

            lua_object.get_remote_cpp_object()->Log(to_ue_string(message).c_str());

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
