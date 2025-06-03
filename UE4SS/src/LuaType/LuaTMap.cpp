#include <LuaType/LuaTMap.hpp>

#include <DynamicOutput/DynamicOutput.hpp>

#include <Unreal/Property/FMapProperty.hpp>


namespace RC::LuaType
{
    TMap::TMap(const PusherParams& params)
        : RemoteObjectBase<Unreal::FScriptMap, TMapName>(static_cast<Unreal::FScriptMap*>(params.data)), m_base(params.base),
          m_property(static_cast<Unreal::FMapProperty*>(params.property)),
          m_key_property(m_property->GetKeyProp()), m_value_property(m_property->GetValueProp())
    {
    }

    auto TMap::construct(const PusherParams& params) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::TMap lua_object{params};

        if (!lua_object.m_key_property)
        {
            Output::send<LogLevel::Error>(STR("TMap::construct: m_key_property is nullptr for {}"), lua_object.m_property->GetFullName());
        }

        if (!lua_object.m_value_property)
        {
            Output::send<LogLevel::Error>(STR("TMap::construct: m_value_property is nullptr for {}"), lua_object.m_property->GetFullName());
        }

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = params.lua.get_metatable(metatable_name);
        if (params.lua.is_nil(-1))
        {
            params.lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FScriptMap>::construct(params.lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            params.lua.new_metatable<LuaType::TMap>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to lua
        params.lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto TMap::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FScriptMap>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto TMap::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Length,
                                             [](const LuaMadeSimple::Lua& lua) {
                                                 auto lua_object = lua.get_userdata<TMap>();
                                                 lua.set_integer(lua_object.get_remote_cpp_object()->Num());
                                                 return 1;
                                             });
    }


    template <LuaMadeSimple::Type::IsFinal is_final>
    auto TMap::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("IsValid",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<TMap>();

                           lua.set_bool(lua_object.get_remote_cpp_object());

                           return 1;
                       });

        table.add_pair("Find",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(MapOperation::Find, lua);
                           return 1;
                       });

        table.add_pair("Add",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(MapOperation::Add, lua);
                           return 1;
                       });

        table.add_pair("Contains",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(MapOperation::Contains, lua);
                           return 1;
                       });

        table.add_pair("Remove",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(MapOperation::Remove, lua);
                           return 1;
                       });

        table.add_pair("Empty",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(MapOperation::Empty, lua);
                           return 1;
                       });

        table.add_pair("ForEach",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           TMap& lua_object = lua.get_userdata<TMap>();

                           FScriptMapInfo info(lua_object.m_key_property, lua_object.m_value_property);
                           info.validate_pushers(lua);

                           Unreal::FScriptMap* map = lua_object.get_remote_cpp_object();

                           Unreal::int32 max_index = map->GetMaxIndex();
                           for (Unreal::int32 i = 0; i < max_index; i++)
                           {
                               if (!map->IsValidIndex(i))
                               {
                                   continue;
                               }

                               // Duplicate the Lua function so that we can use it in subsequent iterations of this loop (call_function pops the function from the stack)
                               lua_pushvalue(lua.get_lua_state(), 1);

                               void* entry_data = map->GetData(i, info.layout);

                               // pass key (P1), value (P2)
                               PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::GetParam,
                                                          .lua = lua,
                                                          .base = lua_object.m_base,
                                                          .data = entry_data,
                                                          .property = nullptr};

                               pusher_params.property = info.key;
                               StaticState::m_property_value_pushers[static_cast<int32_t>(info.key_fname.GetComparisonIndex())](pusher_params);

                               pusher_params.data = static_cast<uint8_t*>(pusher_params.data) + info.layout.ValueOffset;
                               pusher_params.property = info.value;
                               StaticState::m_property_value_pushers[static_cast<int32_t>(info.value_fname.GetComparisonIndex())](pusher_params);

                               // Call function passing key & value
                               // Mutating the key is undefined behavior
                               lua.call_function(2, 0);
                           }

                           return 1;
                       });

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type",
                           [](const LuaMadeSimple::Lua& lua) -> int {
                               lua.set_string(ClassName::ToString());
                               return 1;
                           });

            // If this is the final object then we also want to finalize creating the table
            // If not then it's the responsibility of the overriding object to call 'make_global()'
            // table.make_global(ClassName::ToString());
        }
    }

    auto TMap::prepare_to_handle(const MapOperation operation, const LuaMadeSimple::Lua& lua) -> void
    {
        TMap& lua_object = lua.get_userdata<TMap>();

        FScriptMapInfo info(lua_object.m_key_property, lua_object.m_value_property);
        info.validate_pushers(lua);

        Unreal::FScriptMap* map = lua_object.get_remote_cpp_object();

        switch (operation)
        {
        case MapOperation::Find: {
            Unreal::TArray<Unreal::uint8> key_data{};
            key_data.Init(0, info.layout.ValueOffset);

            {
                const PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::Set,
                                                 .lua = lua,
                                                 .base = lua_object.m_base,
                                                 .data = key_data.GetData(),
                                                 .property = info.key};
                StaticState::m_property_value_pushers[static_cast<int32_t>(info.key_fname.GetComparisonIndex())](pusher_params);
            }

            Unreal::uint8* value_ptr = map->FindValue(key_data.GetData(),
                                                      info.layout,
                                                      [&](const void* src) -> Unreal::uint32 {
                                                          return info.key->GetValueTypeHash(src);
                                                      },
                                                      [&](const void* a, const void* b) -> bool {
                                                          return info.key->Identical(a, b);
                                                      });
            if (!value_ptr)
            {
                lua.throw_error("Map key not found.");
            }

            const PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::GetParam,
                                             .lua = lua,
                                             .base = lua_object.m_base,
                                             .data = value_ptr,
                                             .property = info.value};
            StaticState::m_property_value_pushers[static_cast<int32_t>(info.value_fname.GetComparisonIndex())](pusher_params);
            break;
        }
        case MapOperation::Add: {
            Unreal::TArray<Unreal::uint8> pair_data{};
            pair_data.Init(0, info.layout.SetLayout.Size);

            PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::Set,
                                       .lua = lua,
                                       .base = lua_object.m_base,
                                       .data = pair_data.GetData(),
                                       .property = info.key};

            StaticState::m_property_value_pushers[static_cast<int32_t>(info.key_fname.GetComparisonIndex())](pusher_params);

            pusher_params.property = info.value;
            pusher_params.data = static_cast<uint8_t*>(pusher_params.data) + info.layout.ValueOffset;
            StaticState::m_property_value_pushers[static_cast<int32_t>(info.value_fname.GetComparisonIndex())](pusher_params);

            void* key_ptr = pair_data.GetData();
            void* value_ptr = pair_data.GetData() + info.layout.ValueOffset;

            auto construct_fn = [&](Unreal::FProperty* property, const void* ptr, void* new_element) {
                if (property->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_ZeroConstructor))
                {
                    Unreal::FMemory::Memzero(new_element, property->GetSize());
                }
                else
                {
                    property->InitializeValue(new_element);
                }

                property->CopySingleValueToScriptVM(new_element, ptr);
            };

            auto destruct_fn = [&](Unreal::FProperty* property, void* element) {
                if (!property->HasAnyPropertyFlags(
                        Unreal::EPropertyFlags::CPF_IsPlainOldData |
                        Unreal::EPropertyFlags::CPF_NoDestructor))
                {
                    property->DestroyValue(element);
                }
            };

            map->Add(key_ptr,
                     value_ptr,
                     info.layout,
                     [&](const void* src) -> Unreal::uint32 {
                         return info.key->GetValueTypeHash(src);
                     },
                     [&](const void* a, const void* b) -> bool {
                         return info.key->Identical(a, b);
                     },
                     [&](void* new_element_key) {
                         construct_fn(info.key, key_ptr, new_element_key);
                     },
                     [&](void* new_element_value) {
                         construct_fn(info.value, value_ptr, new_element_value);
                     },
                     [&](void* existing_element_value) {
                         info.value->CopySingleValueToScriptVM(existing_element_value, value_ptr);
                     },
                     [&](void* element_key) {
                         destruct_fn(info.key, element_key);
                     },
                     [&](void* element_value) {
                         destruct_fn(info.value, element_value);
                     });
            break;
        }
        case MapOperation::Contains: {
            Unreal::TArray<Unreal::uint8> key_data{};
            key_data.Init(0, info.layout.ValueOffset);

            PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::Set,
                                       .lua = lua,
                                       .base = lua_object.m_base,
                                       .data = key_data.GetData(),
                                       .property = info.key};
            StaticState::m_property_value_pushers[static_cast<int32_t>(info.key_fname.GetComparisonIndex())](pusher_params);

            Unreal::int32 index = map->FindPairIndex(key_data.GetData(),
                                                     info.layout,
                                                     [&](const void* src) {
                                                         return info.key->GetValueTypeHash(src);
                                                     },
                                                     [&](const void* a, const void* b) {
                                                         return info.key->Identical(a, b);
                                                     });

            lua.set_bool(index != Unreal::INDEX_NONE);

            break;
        }
        case MapOperation::Remove: {
            Unreal::TArray<Unreal::uint8> key_data{};
            key_data.Init(0, info.layout.ValueOffset);

            PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::Set,
                                       .lua = lua,
                                       .base = lua_object.m_base,
                                       .data = key_data.GetData(),
                                       .property = info.key};
            StaticState::m_property_value_pushers[static_cast<int32_t>(info.key_fname.GetComparisonIndex())](pusher_params);

            Unreal::int32 index = map->FindPairIndex(key_data.GetData(),
                                                     info.layout,
                                                     [&](const void* src) {
                                                         return info.key->GetValueTypeHash(src);
                                                     },
                                                     [&](const void* a, const void* b) {
                                                         return info.key->Identical(a, b);
                                                     });

            if (index != Unreal::INDEX_NONE)
            {
                map->RemoveAt(index, info.layout);
            }

            break;
        }
        case MapOperation::Empty: {
            map->Empty(0, info.layout);
            break;
        }
        }
    }

    FScriptMapInfo::FScriptMapInfo(Unreal::FProperty* key, Unreal::FProperty* value) :
        key(key), value(value),
        key_fname(key->GetClass().GetFName()), value_fname(value->GetClass().GetFName())
    {
        layout = Unreal::FScriptMap::GetScriptLayout(key->GetSize(),
                                                     key->GetMinAlignment(),
                                                     value->GetSize(),
                                                     value->GetMinAlignment());

    }

    void FScriptMapInfo::validate_pushers(const LuaMadeSimple::Lua& lua)
    {
        int32_t key_comparison_index = static_cast<int32_t>(key_fname.GetComparisonIndex());
        if (!StaticState::m_property_value_pushers.contains(key_comparison_index))
        {
            std::string inner_type_name = to_string(key_fname.ToString());
            lua.throw_error(fmt::format("Tried interacting with a map with an unsupported key type {}", inner_type_name));
        }

        int32_t value_comparison_index = static_cast<int32_t>(value_fname.GetComparisonIndex());
        if (!StaticState::m_property_value_pushers.contains(value_comparison_index))
        {
            std::string inner_type_name = to_string(key_fname.ToString());
            lua.throw_error(fmt::format("Tried interacting with a map with an unsupported value type {}", inner_type_name));
        }
    }


}