#include <LuaType/LuaTSet.hpp>

#include <DynamicOutput/DynamicOutput.hpp>

#include <Unreal/Property/FSetProperty.hpp>

namespace RC::LuaType
{
    TSet::TSet(const PusherParams& params)
        : RemoteObjectBase<Unreal::FScriptSet, TSetName>(static_cast<Unreal::FScriptSet*>(params.data)), m_base(params.base),
          m_property(static_cast<Unreal::FSetProperty*>(params.property)), m_element_property(m_property->GetElementProp())
    {
    }

    auto TSet::construct(const PusherParams& params) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::TSet lua_object{params};

        if (!lua_object.m_element_property)
        {
            Output::send<LogLevel::Error>(STR("TSet::construct: m_element_property is nullptr for {}"), lua_object.m_property->GetFullName());
        }

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = params.lua.get_metatable(metatable_name);
        if (params.lua.is_nil(-1))
        {
            params.lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FScriptSet>::construct(params.lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            params.lua.new_metatable<LuaType::TSet>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to lua
        params.lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto TSet::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FScriptSet>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto TSet::setup_metamethods(BaseObject& base_object) -> void
    {
        // Add Length metamethod (# operator in Lua)
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Length,
                                             [](const LuaMadeSimple::Lua& lua) {
                                                 auto lua_object = lua.get_userdata<TSet>();
                                                 lua.set_integer(lua_object.get_remote_cpp_object()->Num());
                                                 return 1;
                                             });
        
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto TSet::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("IsValid",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           auto& lua_object = lua.get_userdata<TSet>();

                           lua.set_bool(lua_object.get_remote_cpp_object());

                           return 1;
                       });

        table.add_pair("Add",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(SetOperation::Add, lua);
                           return 1;
                       });

        table.add_pair("Contains",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(SetOperation::Contains, lua);
                           return 1;
                       });

        table.add_pair("Remove",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(SetOperation::Remove, lua);
                           return 1;
                       });

        table.add_pair("Empty",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(SetOperation::Empty, lua);
                           return 1;
                       });

        table.add_pair("ForEach",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           prepare_to_handle(SetOperation::ForEach, lua);
                           return 1;
                       });

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type",
                           [](const LuaMadeSimple::Lua& lua) -> int {
                               lua.set_string(ClassName::ToString());
                               return 1;
                           });
        }
    }

    auto TSet::prepare_to_handle(const SetOperation operation, const LuaMadeSimple::Lua& lua) -> void
    {
        TSet& lua_object = lua.get_userdata<TSet>();

        FScriptSetInfo info(lua_object.m_element_property);
        info.validate_pushers(lua);

        Unreal::FScriptSet* set = lua_object.get_remote_cpp_object();

        switch (operation)
        {
        case SetOperation::Add: {
            Unreal::TArray<Unreal::uint8> element_data{};
            element_data.Init(0, info.layout.Size);

            PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::Set,
                                        .lua = lua,
                                        .base = lua_object.m_base,
                                        .data = element_data.GetData(),
                                        .property = info.element};
            StaticState::m_property_value_pushers[static_cast<int32_t>(info.element_fname.GetComparisonIndex())](pusher_params);

            void* element_ptr = element_data.GetData();

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

            set->Add(element_ptr,
                     info.layout,
                     [&](const void* src) -> Unreal::uint32 {
                         return info.element->GetValueTypeHash(src);
                     },
                     [&](const void* a, const void* b) -> bool {
                         return info.element->Identical(a, b);
                     },
                     [&](void* new_element) {
                         construct_fn(info.element, element_ptr, new_element);
                     },
                     [&](void* element) {
                         destruct_fn(info.element, element);
                     });
            break;
        }
        case SetOperation::Contains: {
            Unreal::TArray<Unreal::uint8> element_data{};
            element_data.Init(0, info.layout.Size);

            PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::Set,
                                       .lua = lua,
                                       .base = lua_object.m_base,
                                       .data = element_data.GetData(),
                                       .property = info.element};
            StaticState::m_property_value_pushers[static_cast<int32_t>(info.element_fname.GetComparisonIndex())](pusher_params);

            // Create a SetHelper with the set data
            Unreal::FScriptSetHelper SetHelper(lua_object.m_property, set);
    
            // Use FindElementIndex with just the element data
            Unreal::int32 index = SetHelper.FindElementIndex(element_data.GetData());

            lua.set_bool(index != Unreal::INDEX_NONE);

            break;
        }
        case SetOperation::Remove: {
            Unreal::TArray<Unreal::uint8> element_data{};
            element_data.Init(0, info.layout.Size);

            PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::Set,
                                       .lua = lua,
                                       .base = lua_object.m_base,
                                       .data = element_data.GetData(),
                                       .property = info.element};
            StaticState::m_property_value_pushers[static_cast<int32_t>(info.element_fname.GetComparisonIndex())](pusher_params);

            // Create a SetHelper with the set data
            Unreal::FScriptSetHelper SetHelper(lua_object.m_property, set);
    
            // Use FindElementIndex with just the element data
            Unreal::int32 index = SetHelper.FindElementIndex(element_data.GetData());

            if (index != Unreal::INDEX_NONE)
            {
                SetHelper.RemoveAt(index);
            }

            break;
        }
        case SetOperation::Empty: {
            set->Empty(0, info.layout);
            break;
        }
        case SetOperation::ForEach: {
            Unreal::int32 max_index = set->GetMaxIndex();
            for (Unreal::int32 i = 0; i < max_index; i++)
            {
                if (!set->IsValidIndex(i))
                {
                    continue;
                }

                // Duplicate the Lua function so we can use it in subsequent iterations
                lua_pushvalue(lua.get_lua_state(), 1);

                void* element_data = set->GetData(i, info.layout);

                // pass element (P1)
                PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::GetParam,
                                           .lua = lua,
                                           .base = lua_object.m_base,
                                           .data = element_data,
                                           .property = info.element};

                StaticState::m_property_value_pushers[static_cast<int32_t>(info.element_fname.GetComparisonIndex())](pusher_params);

                // Call function passing element, expecting 1 return value
                lua.call_function(1, 1);

                // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                if (lua.is_bool(2) && lua.get_bool(2))
                {
                    break;
                }
                else
                {
                    // There's a 'nil' on the stack because we told Lua that we expect a return value.
                    // Lua will put 'nil' on the stack if the Lua function doesn't explicitly return anything.
                    // We discard the 'nil' here, otherwise the Lua stack is corrupted on the next iteration of the 'ForEach' loop.
                    // We explicitly specify index 2 because we duplicated the function earlier and that's located at index 1.
                    lua.discard_value(2);
                }
            }
            break;
        }
        }
    }

    FScriptSetInfo::FScriptSetInfo(Unreal::FProperty* element) :
        element(element),
        element_fname(element->GetClass().GetFName())
    {
        layout = Unreal::FScriptSet::GetScriptLayout(element->GetSize(),
                                                    element->GetMinAlignment());
    }

    void FScriptSetInfo::validate_pushers(const LuaMadeSimple::Lua& lua)
    {
        int32_t element_comparison_index = static_cast<int32_t>(element_fname.GetComparisonIndex());
        if (!StaticState::m_property_value_pushers.contains(element_comparison_index))
        {
            std::string element_type_name = to_string(element_fname.ToString());
            lua.throw_error(fmt::format("Tried interacting with a set with an unsupported element type {}", element_type_name));
        }
    }
}