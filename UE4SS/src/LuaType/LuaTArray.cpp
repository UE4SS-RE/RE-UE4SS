#include <format>

#include <LuaType/LuaTArray.hpp>
#include <LuaType/LuaUObject.hpp>
#pragma warning(disable : 4005)
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    TArray::TArray(const PusherParams& params)
        : RemoteObjectBase<Unreal::FScriptArray, TArrayName>(static_cast<Unreal::FScriptArray*>(params.data)), m_base(params.base),

          m_property(static_cast<Unreal::FArrayProperty*>(params.property)), m_inner_property(m_property->GetInner())
    {
    }

    auto TArray::construct(const PusherParams& params) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::TArray lua_object{params};

        if (!lua_object.m_inner_property)
        {
            Output::send<LogLevel::Error>(STR("TArray::construct: m_inner_property is nullptr for {}"), lua_object.m_property->GetFullName());
        }

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = params.lua.get_metatable(metatable_name);
        if (params.lua.is_nil(-1))
        {
            params.lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FScriptArray>::construct(params.lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            params.lua.new_metatable<LuaType::TArray>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        params.lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto TArray::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FScriptArray>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto TArray::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Index, [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(LuaMadeSimple::Type::Operation::Get, lua);
            return 1;
        });

        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::NewIndex, [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(LuaMadeSimple::Type::Operation::Set, lua);
            return 1;
        });

        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Length, [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TArray>();
            lua.set_integer(lua_object.get_remote_cpp_object()->Num());
            return 1;
        });
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto TArray::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("GetArrayAddress", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TArray>();

            lua.set_integer(reinterpret_cast<uintptr_t>(lua_object.get_remote_cpp_object()));

            return 1;
        });

        table.add_pair("GetArrayNum", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TArray>();

            lua.set_integer(lua_object.get_remote_cpp_object()->Num());

            return 1;
        });

        table.add_pair("GetArrayMax", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TArray>();

            lua.set_integer(lua_object.get_remote_cpp_object()->Max());

            return 1;
        });

        table.add_pair("GetArrayDataAddress", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TArray>();

            lua.set_integer(reinterpret_cast<uintptr_t>(lua_object.get_remote_cpp_object()->GetData()));

            return 1;
        });

        table.add_pair("Empty", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TArray>();

            lua_object.get_remote_cpp_object()->Empty(0, lua_object.m_inner_property->GetElementSize(), lua_object.m_inner_property->GetMinAlignment());

            return 0;
        });

        // This is a read-only ForEach implementation (if inner type is trivial)
        // TODO: Make it writable
        table.add_pair("ForEach", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TArray>();

            Unreal::FName property_type_name = lua_object.m_inner_property->GetClass().GetFName();
            int32_t name_comparison_index = property_type_name.GetComparisonIndex();

            if (StaticState::m_property_value_pushers.contains(name_comparison_index))
            {
                uint8_t* array_data = static_cast<uint8_t*>(lua_object.get_remote_cpp_object()->GetData());

                int32_t array_size = lua_object.get_remote_cpp_object()->Num();
                for (int32_t i = 0; i < array_size; ++i)
                {
                    // Duplicate the Lua function so that we can use it in subsequent iterations of this loop (call_function pops the function from the stack)
                    lua_pushvalue(lua.get_lua_state(), 1);

                    // Set the 'index' parameter for the Lua function (P1)
                    lua.set_integer(i + 1); // Adding 1 here to account for that fact that Lua tables are 1-indexed

                    // Set the 'elem' parameter for the Lua function (P2)
                    // TODO: Fix crash that occurs here.
                    //       It appears that the Lua stack is getting corrupted somehow, or lua_object is getting GC'd by Lua.
                    //       It seems to only affect large arrays, and I don't know how to fix it.
                    void* property_value = array_data + (i * lua_object.m_inner_property->GetElementSize());
                    const PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::GetParam,
                                                     .lua = lua,
                                                     .base = lua_object.m_base,
                                                     .data = property_value,
                                                     .property = lua_object.m_inner_property};
                    StaticState::m_property_value_pushers[name_comparison_index](pusher_params);

                    // Call function passing index & the element
                    // The element is read-only for all trivial types
                    // The element is writable if it's a UObject
                    lua.call_function(2, 0);
                }
            }
            else
            {
                lua.throw_error(fmt::format("[TArray:ForEach] Tried iterating an array but the unreal property has no registered handler (via ArrayProperty). "
                                            "Property type '{}' not supported.",
                                            to_string(property_type_name.ToString())));
            }

            return 0;
        });

        table.add_pair("IsValid", [](const LuaMadeSimple::Lua& lua) -> int {
            auto& lua_object = lua.get_userdata<TArray>();

            lua.set_bool(lua_object.get_remote_cpp_object());

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

    auto TArray::handle_unreal_property_value(const LuaMadeSimple::Type::Operation operation,
                                              const LuaMadeSimple::Lua& lua,
                                              Unreal::FScriptArray* array,
                                              int64_t array_index,
                                              const TArray& lua_object) -> void
    {
        Unreal::FName property_type_fname = lua_object.m_inner_property->GetClass().GetFName();
        int32_t name_comparison_index = property_type_fname.GetComparisonIndex();

        if (StaticState::m_property_value_pushers.contains(name_comparison_index))
        {
            uint8_t* array_data = static_cast<uint8_t*>(array->GetData());
            void* property_value = array_data + (array_index * lua_object.m_inner_property->GetElementSize());

            const PusherParams pusher_params{.operation = operation,
                                             .lua = lua,
                                             .base = lua_object.m_base,
                                             .data = property_value,
                                             .property = lua_object.m_inner_property};
            StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
        }
        else
        {
            std::string property_type_name = to_string(property_type_fname.ToString());
            lua.throw_error(fmt::format("Tried accessing unreal property without a registered handler (via ArrayProperty). Property type '{}' not supported.",
                                        property_type_name));
        }
    }

    auto TArray::prepare_to_handle(const LuaMadeSimple::Type::Operation operation, const LuaMadeSimple::Lua& lua) -> void
    {
        auto& lua_object = lua.get_userdata<TArray>();
        int64_t array_index64 = lua.get_integer() - 1; // Subtracting 1 here to account for that fact that Lua tables are 1-indexed
        if (array_index64 < 0 || array_index64 > std::numeric_limits<int32_t>::max())
        {
            lua.throw_error("TArray index out of range.");
        }
        int32_t array_index = static_cast<int32_t>(array_index64);

        auto array = lua_object.get_remote_cpp_object();
        auto num = array->Num();
        if (array_index >= num || num == 0)
        {
            // We've verified with the above if-statement that we're not out-of-bounds for an int32.
            int32_t count = array_index == 0 || num == 0 ? 1 : static_cast<int32_t>(array_index - num + 1);
            lua_object.get_remote_cpp_object()->AddZeroed(count, lua_object.m_inner_property->GetElementSize(), Unreal::DEFAULT_ALIGNMENT);
        }

        handle_unreal_property_value(operation, lua, lua_object.get_remote_cpp_object(), array_index, lua_object);
    }
} // namespace RC::LuaType
