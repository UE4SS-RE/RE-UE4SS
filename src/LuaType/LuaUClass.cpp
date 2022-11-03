#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUFunction.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <DynamicOutput/Output.hpp>

namespace RC::LuaType
{
    UClass::UClass(Unreal::UClass* object) : UObjectBase<Unreal::UClass, UClassName>(object) /*LuaMadeSimple::Type::RemoteObject<Unreal::UClass>("UClass", object)*/ {}

    auto UClass::construct(const LuaMadeSimple::Lua& lua, Unreal::UClass* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::UClass lua_object{unreal_object};

        LuaMadeSimple::Lua::Table table = LuaType::UObject::construct(lua, lua_object);

        // Setup functions that can be called on this object
        setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);

        setup_metamethods(lua_object);

        // Transfer the object & its ownership fully to Lua
        lua.transfer_stack_object(std::move(lua_object), ClassName::ToString(), lua_object.get_metamethods());

        return table;
    }

    auto UClass::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto UClass::setup_metamethods([[maybe_unused]]BaseObject& base_object) -> void
    {
        // UClass has no metamethods
    }

    template<LuaMadeSimple::Type::IsFinal is_final>
    auto UClass::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        // CDO = ClassDefaultObject
        table.add_pair("GetCDO", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<UClass>();

            if (!lua_object.get_remote_cpp_object())
            {
                LuaType::UObject::construct(lua, nullptr);
                Output::send(STR("[Lua][Error] Tried getting the CDO but the UClass instance is nullptr\n"));
            }
            else
            {
                LuaType::UObject::construct(lua, lua_object.get_remote_cpp_object()->GetClassDefaultObject());
            }

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
            table.make_global(ClassName::ToString());
        }
    }
}
