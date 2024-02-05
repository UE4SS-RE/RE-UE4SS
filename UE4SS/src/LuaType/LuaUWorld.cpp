#include <DynamicOutput/Output.hpp>
#include <LuaType/LuaAActor.hpp>
#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUWorld.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/GameplayStatics.hpp>
#include <Unreal/Rotator.hpp>
#include <Unreal/Transform.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UnrealCoreStructs.hpp>
#include <Unreal/World.hpp>

namespace RC::LuaType
{
    UWorld::UWorld(Unreal::UWorld* object) : RemoteObjectBase<Unreal::UWorld, UWorldName>(object)
    {
    }

    auto UWorld::construct(const LuaMadeSimple::Lua& lua, Unreal::UWorld* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        add_to_global_unreal_objects_map(unreal_object);

        LuaType::UWorld lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaType::UObject::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::UWorld>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto UWorld::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = UObject::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto UWorld::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
        // UWorld has no metamethods
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto UWorld::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("SpawnActor", [](const LuaMadeSimple::Lua& lua) -> int {
            const auto& lua_object = lua.get_userdata<UWorld>();

            std::string error_overload_not_found{R"(
No overload found for function 'SpawnActor'.
Overloads:
#1: SpawnActor(UClass Class, table Location, table Rotation))"};

            if (!lua.is_userdata())
            {
                lua.throw_error(error_overload_not_found);
            }

            const auto& actor_class = lua.get_userdata<UClass>();

            Unreal::FVector location{};
            if (lua.is_userdata())
            {
                // location = lua.get_userdata<FVector>().get_remote_cpp_object();
                lua.throw_error(error_overload_not_found);
            }
            else if (lua.is_table())
            {
                lua.for_each_in_table([&](const LuaMadeSimple::LuaTableReference<LuaMadeSimple::Lua>& table) {
                    if (table.key.is_string() && table.key.get_string() == "X" && table.value.is_number())
                    {
                        location.SetX(table.value.get_number());
                    }

                    if (table.key.is_string() && table.key.get_string() == "Y" && table.value.is_number())
                    {
                        location.SetY(table.value.get_number());
                    }

                    if (table.key.is_string() && table.key.get_string() == "Z" && table.value.is_number())
                    {
                        location.SetZ(table.value.get_number());
                    }
                    return false;
                });
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            Unreal::FRotator rotation{};
            if (lua.is_userdata())
            {
                // location = lua.get_userdata<FRotator>().get_remote_cpp_object();
                lua.throw_error(error_overload_not_found);
            }
            else if (lua.is_table())
            {
                lua.for_each_in_table([&](const LuaMadeSimple::LuaTableReference<LuaMadeSimple::Lua>& table) {
                    if (table.key.is_string() && table.key.get_string() == "Yaw" && table.value.is_number())
                    {
                        rotation.SetYaw(table.value.get_number());
                    }

                    if (table.key.is_string() && table.key.get_string() == "Pitch" && table.value.is_number())
                    {
                        rotation.SetPitch(table.value.get_number());
                    }

                    if (table.key.is_string() && table.key.get_string() == "Roll" && table.value.is_number())
                    {
                        rotation.SetRoll(table.value.get_number());
                    }
                    return false;
                });
            }
            else
            {
                lua.throw_error(error_overload_not_found);
            }

            auto* actor = lua_object.get_remote_cpp_object()->SpawnActor(actor_class.get_remote_cpp_object(), &location, &rotation);
            LuaType::AActor::construct(lua, actor);

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
} // namespace RC::LuaType
