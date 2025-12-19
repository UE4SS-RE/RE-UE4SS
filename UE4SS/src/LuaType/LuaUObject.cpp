#include <Helpers/Casting.hpp>
#include <LuaType/LuaAActor.hpp>
#include <LuaType/LuaCustomProperty.hpp>
#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaUnrealString.hpp>
#include <LuaType/LuaFText.hpp>
#include <LuaType/LuaFWeakObjectPtr.hpp>
#include <LuaType/LuaTArray.hpp>
#include <LuaType/LuaTSet.hpp>
#include <LuaType/LuaTMap.hpp>
#include <LuaType/LuaTSoftObjectPtr.hpp>
#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUEnum.hpp>
#include <LuaType/LuaUFunction.hpp>
#include <LuaType/LuaUInterface.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaUScriptStruct.hpp>
#include <LuaType/LuaUStruct.hpp>
#include <LuaType/LuaUWorld.hpp>
#include <LuaType/LuaUDataTable.hpp>
#include <LuaType/LuaXDelegateProperty.hpp>
#include <LuaType/LuaXObjectProperty.hpp>
#include <LuaType/LuaXProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/AActor.hpp>
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/World.hpp>
#include <Unreal/Engine/UDataTable.hpp>

#pragma warning(default : 4005)
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/Integer.hpp>

namespace RC::LuaType
{
    std::unordered_set<size_t> s_lua_unreal_objects{};
    std::mutex s_lua_unreal_objects_map_mutex{};

    auto add_to_global_unreal_objects_map(Unreal::UObject* object) -> void
    {
        if (object)
        {
            std::lock_guard lock{s_lua_unreal_objects_map_mutex};
            s_lua_unreal_objects.emplace(object->HashObject());
        }
    }

    auto remove_from_global_unreal_objects_map(const Unreal::UObject* object) -> void
    {
        if (object)
        {
            std::lock_guard lock{s_lua_unreal_objects_map_mutex};
            if (const auto it = s_lua_unreal_objects.find(object->HashObject()); it != s_lua_unreal_objects.end())
            {
                s_lua_unreal_objects.erase(it);
            }
        }
    }

    auto is_object_in_global_unreal_object_map(Unreal::UObject* object) -> bool
    {
        std::lock_guard lock{s_lua_unreal_objects_map_mutex};
        return object && s_lua_unreal_objects.contains(object->HashObject());
    }

    FLuaObjectDeleteListener FLuaObjectDeleteListener::s_lua_object_delete_listener{};
    void FLuaObjectDeleteListener::NotifyUObjectDeleted(const Unreal::UObjectBase* object, [[maybe_unused]] int32_t index)
    {
        remove_from_global_unreal_objects_map(static_cast<const Unreal::UObject*>(object));
    }

    auto call_ufunction_from_lua(const LuaMadeSimple::Lua& lua) -> int
    {
        if (!lua.is_userdata())
        {
            lua.throw_error("[UFunction::setup_metamethods -> __call] Attempted to call a UFunction without UFunction userdata attached");
        }

        auto& lua_object = lua.get_userdata<LuaType::UFunction>();

        Unreal::UFunction* func{};
        Unreal::UObject* calling_context{};
        bool is_first_userdata_function = lua_object.get_remote_cpp_object()->IsA<Unreal::UFunction>();

        if (is_first_userdata_function)
        {
            func = lua_object.get_remote_cpp_object();
            calling_context = lua_object.get_base();
        }
        else
        {
            calling_context = lua_object.get_remote_cpp_object();
        }

        if (lua.is_userdata())
        {
            auto& lua_object2 = lua.get_userdata<LuaType::UE4SSBaseObject>(1, true);
            if (!is_first_userdata_function && lua_object2.derives_from_function())
            {
                func = lua.get_userdata<LuaType::UFunction>(1).get_remote_cpp_object();
            }
            else if (func && !calling_context)
            {
                calling_context = lua.get_userdata<LuaType::UObject>(1).get_remote_cpp_object();
            }
            else
            {
                lua.discard_value();
            }
        }

        if (!func || !calling_context)
        {
            lua.throw_error("[UFunction::call_ufunction_from_lua] Tried calling function without both UFunction and calling context");
        }

        uint8_t num_ufunc_params = func->GetNumParms();
        uint16_t return_value_offset = func->GetReturnValueOffset();

        // Unreal::FProperty* param_next = func->get_child_properties<Unreal::FProperty*>();
        Unreal::FProperty* return_value_property{};
        Unreal::FName return_value_property_type{0u, 0u};
        int32_t return_value_property_offset_internal{};

        DynamicUnrealFunctionData dynamic_unreal_function_data{};
        DynamicUnrealFunctionOutParameters dynamic_unreal_function_out_parameters{};
        bool has_out_params{false};

        // if (param_next)
        if (func->HasChildren())
        {
            uint8_t num_supplied_params = Helper::Integer::to<uint8_t>(lua.get_stack_size());

            // When return_value_offset is 0xFFFF it means that there is no return value so num_ufunc_params is accurate
            // Otherwise you must subtract 1 to account for the return value (stored in the same struct and counts as a param)
            // The ternary makes sure that we never have a negative number of params
            bool has_return_value = return_value_offset != 0xFFFF;
            uint8_t num_expected_params = num_ufunc_params;
            uint8_t num_expected_params_with_return_value = has_return_value ? (num_expected_params - 1 < 0 ? 0 : num_expected_params - 1) : num_expected_params;

            if (num_supplied_params != num_expected_params_with_return_value)
            {
                lua.throw_error(fmt::format("[UFunction::setup_metamethods -> __call] UFunction expected {} parameters, received {}",
                                            num_expected_params,
                                            num_supplied_params));
            }

            // for (uint8_t i = 0; i < num_expected_params; ++i)
            // uint8_t i = 0;
            for (Unreal::FProperty* param_next : Unreal::TFieldRange<Unreal::FProperty>(func, Unreal::EFieldIterationFlags::IncludeDeprecated))
            {
                // if (i > 0)
                //{
                //     // If not the first iteration then get the next param
                //     // If it's the first iteration then paramNext was already set earlier
                //     param_next = param_next->get_next<Unreal::XProperty*>();
                // }

                // if (!param_next)
                //{
                //     break;
                // }

                if (!param_next->HasAnyPropertyFlags(Unreal::CPF_Parm))
                {
                    continue;
                }

                int32_t offset_internal = param_next->GetOffset_Internal();
                Unreal::FName property_type_fname = param_next->GetClass().GetFName();

                // Is the offset of this parameter is the same as the ReturnValueOffset in the UFunction ?
                // If yes, then this parameter should be treated as the return value
                // If no, then treat this as just another parameter
                if (offset_internal == return_value_offset)
                {
                    return_value_property = param_next;
                    return_value_property_type = property_type_fname;
                    return_value_property_offset_internal = offset_internal;
                }
                else
                {
                    if (param_next->HasAnyPropertyFlags(Unreal::CPF_OutParm) && !param_next->HasAnyPropertyFlags(Unreal::CPF_ReturnParm) &&
                        !param_next->HasAnyPropertyFlags(Unreal::CPF_ConstParm))
                    {
                        has_out_params = true;

                        // Store pointer to params property, and a Lua ref to the corresponding table

                        if (!lua.is_table())
                        {
                            lua.throw_error(
                                    "Tried storing reference to a Lua table for an 'Out' parameter when calling a UFunction but no table was on the stack");
                        }

                        // Duplicate the Lua function to the top of the stack for luaL_ref
                        lua_pushvalue(lua.get_lua_state(), 1);

                        // Take a reference to the Lua function (it also pops it of the stack)
                        dynamic_unreal_function_out_parameters.add({.property = param_next, .lua_ref = lua.registry().make_ref()});

                        if (!param_next->IsA<Unreal::FStructProperty>() && !param_next->IsA<Unreal::FArrayProperty>())
                        {
                            continue;
                        }
                    }

                    int32_t name_comparison_index = property_type_fname.GetComparisonIndex();

                    if (StaticState::m_property_value_pushers.contains(name_comparison_index))
                    {
                        void* data = &dynamic_unreal_function_data.data[offset_internal];

                        const PusherParams pusher_params{.operation = Operation::Set,
                                                         .lua = lua,
                                                         .base = static_cast<Unreal::UObject*>(static_cast<void*>(dynamic_unreal_function_data.data)),
                                                         .data = data,
                                                         .property = param_next};
                        StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                    }
                    else
                    {
                        std::string parameter_type_name = to_string(property_type_fname.ToString());
                        std::string parameter_name = to_string(param_next->GetName());
                        lua.throw_error(
                                fmt::format("Tried calling UFunction without a registered handler for parameter. Parameter '{}' of type '{}' not supported.",
                                            parameter_name,
                                            parameter_type_name));
                    }
                }
            };
        }

        // It's assumed that everything is safe here
        // Every part of the system is designed to throw when stuff goes wrong
        // This code should never be executed in those cases
        calling_context->ProcessEvent(func, dynamic_unreal_function_data.data);

        // If there are any 'Out' params, update the table that they correspond to
        if (has_out_params)
        {
            for (auto const [param, lua_table_ref] : dynamic_unreal_function_out_parameters.get())
            {
                if (!param)
                {
                    break;
                }

                // Use the stored registry index to put the original Lua table for the 'Out' param back on the stack
                lua.registry().get_table_ref(lua_table_ref);
                auto lua_table = lua.get_table();

                auto reuse_same_table = param->IsA<Unreal::FArrayProperty>() || param->IsA<Unreal::FStructProperty>() || param->IsA<Unreal::FSetProperty>();
                if (!reuse_same_table)
                {
                    lua_table.add_key(to_string(param->GetName()).c_str());
                }

                Unreal::FName param_type_fname = param->GetClass().GetFName();
                int32_t name_comparison_index = param_type_fname.GetComparisonIndex();

                if (StaticState::m_property_value_pushers.contains(name_comparison_index))
                {
                    uint8_t* data = &dynamic_unreal_function_data.data[param->GetOffset_Internal()];

                    const PusherParams pusher_params{
                            .operation = Operation::GetNonTrivialLocal,
                            .lua = lua,
                            .base = static_cast<Unreal::UObject*>(static_cast<void*>(dynamic_unreal_function_data.data)),
                            .data = data,
                            .property = param,
                            .create_new_if_get_non_trivial_local = !reuse_same_table,
                    };
                    StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                }
                else
                {
                    std::string param_type_name = to_string(param_type_fname.ToString());
                    lua.throw_error(fmt::format("Tried calling UFunction without a registered handler 'Out' param. Type '{}' not supported.", param_type_name));
                }

                if (!reuse_same_table)
                {
                    lua_table.fuse_pair();
                }
            }
        }

        // If there's a return value, then forward it to the Lua script
        if (return_value_property)
        {
            int32_t name_comparison_index = return_value_property_type.GetComparisonIndex();

            if (StaticState::m_property_value_pushers.contains(name_comparison_index))
            {
                uint8_t* data = &dynamic_unreal_function_data.data[return_value_property_offset_internal];

                const PusherParams pusher_params{.operation = Operation::GetNonTrivialLocal,
                                                 .lua = lua,
                                                 .base = static_cast<Unreal::UObject*>(static_cast<void*>(data)), // Base is the start of the params struct
                                                 .data = data,
                                                 .property = return_value_property};
                StaticState::m_property_value_pushers[name_comparison_index](pusher_params);

                return 1;
            }
            else
            {
                std::string return_value_type_name = to_string(return_value_property_type.ToString());
                lua.throw_error(fmt::format("Tried calling UFunction without a registered handler for return value. Return value of type '{}' not supported.",
                                            return_value_type_name));
            }

            return 0;
        }
        else
        {
            return 0;
        }
    }

    auto auto_construct_object(const LuaMadeSimple::Lua& lua, Unreal::UObject* object) -> void
    {
        // If the UObject is nullptr (which is valid), then construct an empty Lua object to enable chaining
        if (!object)
        {
            UObject::construct(lua, nullptr);
        }
        else if (object->IsA<Unreal::UFunction>())
        {
            UFunction::construct(lua, nullptr, static_cast<Unreal::UFunction*>(object));
        }
        else if (object->IsA<Unreal::UClass>())
        {
            UClass::construct(lua, static_cast<Unreal::UClass*>(object));
        }
        else if (object->IsA<Unreal::UScriptStruct>())
        {
            ScriptStructWrapper script_struct_wrapper{static_cast<Unreal::UScriptStruct*>(object), nullptr, nullptr};
            UScriptStruct::construct(lua, script_struct_wrapper);
        }
        else if (object->IsA<Unreal::UDataTable>())
        {
            UDataTable::construct(lua, static_cast<Unreal::UDataTable*>(object));
        }
        else if (object->IsA<Unreal::UStruct>())
        {
            UStruct::construct(lua, static_cast<Unreal::UStruct*>(object));
        }
        else if (object->IsA<Unreal::UEnum>())
        {
            UEnum::construct(lua, static_cast<Unreal::UEnum*>(object));
        }
        else if (object->IsA<Unreal::UWorld>())
        {
            UWorld::construct(lua, static_cast<Unreal::UWorld*>(object));
        }
        else if (object->IsA<Unreal::AActor>())
        {
            AActor::construct(lua, static_cast<Unreal::AActor*>(object));
        }
        else
        {
            UObject::construct(lua, object);
        }
    }

    auto construct_fname(const LuaMadeSimple::Lua& lua) -> void
    {
        const auto& lua_object = lua.get_userdata<UObject>();
        LuaType::FName::construct(lua, lua_object.get_remote_cpp_object()->GetNamePrivate());
    }

    auto construct_uclass(const LuaMadeSimple::Lua& lua) -> void
    {
        const auto& lua_object = lua.get_userdata<UObject>();
        LuaType::UClass::construct(lua, lua_object.get_remote_cpp_object()->GetClassPrivate());
    }

    auto construct_xproperty(const LuaMadeSimple::Lua& lua, Unreal::FProperty* property) -> void
    {
        auto_construct_property(lua, property);
    }

    auto push_unhandledproperty([[maybe_unused]] const PusherParams& params) -> void
    {
        // TODO: Implement or remove this function
    }

    auto push_objectproperty(const PusherParams& params) -> void
    {
        // We're already setup for the alternate branch inside the get_property_vc() function
        // We don't need to supply a property name because the alternate branch has already been told about the property from the push_unreal_object() function()
        Unreal::UObject** property_value = static_cast<Unreal::UObject**>(params.data);

        // Finally construct the Lua object
        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            auto_construct_object(params.lua, *property_value);
            break;
        case Operation::Set: {
            if (params.lua.is_userdata(params.stored_at_index))
            {
                const auto& lua_object = params.lua.get_userdata<LuaType::UObject>(params.stored_at_index);
                *property_value = lua_object.get_remote_cpp_object();
            }
            else if (params.lua.is_nil(params.stored_at_index))
            {
                params.lua.discard_value(params.stored_at_index);
                *property_value = nullptr;
            }
            else
            {
                params.throw_error("push_objectproperty", "Value must be UObject or nil");
            }
            break;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.throw_error("push_objectproperty", "Unhandled Operation");
            break;
        }
    }

    auto push_classproperty(const PusherParams& params) -> void
    {
        Unreal::UClass** property_value = static_cast<Unreal::UClass**>(params.data);

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::UClass::construct(params.lua, *property_value);
            break;
        case Operation::Set: {
            if (params.lua.is_userdata(params.stored_at_index))
            {
                const auto& lua_object = params.lua.get_userdata<LuaType::UClass>(params.stored_at_index);
                *property_value = lua_object.get_remote_cpp_object();
            }
            else if (params.lua.is_nil(params.stored_at_index))
            {
                params.lua.discard_value(params.stored_at_index);
            }
            else
            {
                params.throw_error("push_classproperty", "Value must be UClass or nil");
            }
            break;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.throw_error("push_classproperty", "Unhandled Operation");
            break;
        }
    }

    auto push_int8property(const PusherParams& params) -> void
    {
        // Some code that might be useful later
        // Keeping it here for now
        /*
        LuaType::Object<int8_t>::construct(lua, property_value, [&](const LuaMadeSimple::Lua& lua, int8_t* value) {
            printf_s("returning Int8Property\n");
            lua.set_integer(*value);
        });
        //*/

        push_integer<int8_t>(params);
    }

    auto push_int16property(const PusherParams& params) -> void
    {
        push_integer<int16_t>(params);
    }

    auto push_intproperty(const PusherParams& params) -> void
    {
        push_integer<int32_t>(params);
    }

    auto push_int64property(const PusherParams& params) -> void
    {
        push_integer<int64_t>(params);
    }

    auto push_byteproperty(const PusherParams& params) -> void
    {
        push_integer<uint8_t>(params);
    }

    auto push_uint16property(const PusherParams& params) -> void
    {
        push_integer<uint16_t>(params);
    }

    auto push_uint32property(const PusherParams& params) -> void
    {
        push_integer<uint32_t>(params);
    }

    auto push_uint64property(const PusherParams& params) -> void
    {
        push_integer<uint64_t>(params);
    }

    /**
     * Helper function to convert a Lua table to a UScriptStruct in memory
     * @param lua The Lua state
     * @param script_struct The struct type to convert to
     * @param data Pointer to the memory where the struct should be written
     * @param table_index Stack index where the Lua table is located
     * @param base Optional UObject base for context
     */
    auto convert_lua_table_to_struct(
            const LuaMadeSimple::Lua& lua,
            Unreal::UScriptStruct* script_struct,
            void* data,
            int table_index,
            Unreal::UObject* base
            ) -> void
    {
        if (!script_struct || !data)
        {
            lua.throw_error("convert_lua_table_to_struct: script_struct or data is null");
            return;
        }

        for (Unreal::FProperty* field : Unreal::TFieldRange<Unreal::FProperty>(script_struct, Unreal::EFieldIterationFlags::IncludeSuper | Unreal::EFieldIterationFlags::IncludeDeprecated))
        {
            Unreal::FName field_type_fname = field->GetClass().GetFName();
            const std::string field_name = to_utf8_string(field->GetName());

            // Push the field name (key for the table)
            lua_pushstring(lua.get_lua_state(), field_name.c_str());

            // Get the value from the table
            // For lua_rawget, we need the actual stack position of the table
            // If table_index is positive, it stays the same even after pushing the key
            // If it's negative, we need to adjust for the key we just pushed
            
            // Adjust index for negative values after pushing key
            int adjusted_index = table_index;
            if (table_index < 0)
            {
                adjusted_index = table_index - 1;
            }

            auto table_value_type = lua_rawget(lua.get_lua_state(), adjusted_index);

            // If there was nothing in the table for this field, leave default value
            if (table_value_type == LUA_TNIL || table_value_type == LUA_TNONE)
            {
                lua.discard_value(-1);
                continue;
            }

            int32_t name_comparison_index = field_type_fname.GetComparisonIndex();

            if (StaticState::m_property_value_pushers.contains(name_comparison_index))
            {
                void* field_data = &static_cast<uint8_t*>(data)[field->GetOffset_Internal()];

                const PusherParams pusher_params{
                        .operation = Operation::Set,
                        .lua = lua,
                        .base = base ? base : static_cast<Unreal::UObject*>(data),
                        .data = field_data,
                        .property = field,
                        .stored_at_index = -1 // Value is at top of stack
                };

                StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
            }
            else
            {
                // Just skip fields without handlers and pop the value
                lua.discard_value(-1);
                Output::send<LogLevel::Verbose>(
                        STR("convert_lua_table_to_struct: Skipping field '{}' of type '{}' (no handler)\n"),
                        to_wstring(field_name),
                        field_type_fname.ToString()
                        );

            }
        }
    }

    /**
     * Helper function to convert a UScriptStruct to a Lua table
     * @param lua The Lua state
     * @param script_struct The struct type to convert from
     * @param data Pointer to the struct data to read
     * @param create_new_table Whether to create a new table or use existing one on stack
     * @param base Optional UObject base for context
     */
    auto convert_struct_to_lua_table(
            const LuaMadeSimple::Lua& lua,
            Unreal::UScriptStruct* script_struct,
            void* data,
            bool create_new_table,
            Unreal::UObject* base
            ) -> void
    {
        if (!script_struct || !data)
        {
            lua.set_nil();
            return;
        }

        LuaMadeSimple::Lua::Table lua_table = create_new_table
                                                  ? lua.prepare_new_table()
                                                  : lua.get_table();

        for (Unreal::FProperty* field : Unreal::TFieldRange<Unreal::FProperty>(script_struct, Unreal::EFieldIterationFlags::IncludeSuper | Unreal::EFieldIterationFlags::IncludeDeprecated))
        {
            std::string field_name = to_utf8_string(field->GetName());
            Unreal::FName field_type_fname = field->GetClass().GetFName();
            int32_t name_comparison_index = field_type_fname.GetComparisonIndex();

            // Check if we can handle this field type
            bool can_handle = StaticState::m_property_value_pushers.contains(name_comparison_index);

            if (can_handle)
            {
                // If it's a container, also check if we can handle the inner type
                if (field->IsA<Unreal::FArrayProperty>())
                {
                    auto* array_prop = static_cast<Unreal::FArrayProperty*>(field);
                    auto* inner = array_prop->GetInner();
                    int32_t inner_comparison_index = inner->GetClass().GetFName().GetComparisonIndex();
                    can_handle = StaticState::m_property_value_pushers.contains(inner_comparison_index);
                }
                else if (field->IsA<Unreal::FSetProperty>())
                {
                    auto* set_prop = static_cast<Unreal::FSetProperty*>(field);
                    auto* element = set_prop->GetElementProp();
                    int32_t element_comparison_index = element->GetClass().GetFName().GetComparisonIndex();
                    can_handle = StaticState::m_property_value_pushers.contains(element_comparison_index);
                }
                else if (field->IsA<Unreal::FMapProperty>())
                {
                    auto* map_prop = static_cast<Unreal::FMapProperty*>(field);
                    int32_t key_index = map_prop->GetKeyProp()->GetClass().GetFName().GetComparisonIndex();
                    int32_t value_index = map_prop->GetValueProp()->GetClass().GetFName().GetComparisonIndex();
                    can_handle = StaticState::m_property_value_pushers.contains(key_index) &&
                                 StaticState::m_property_value_pushers.contains(value_index);
                }
            }

            if (can_handle)
            {
                lua_table.add_key(field_name.c_str());

                const PusherParams pusher_params{
                        .operation = Operation::GetNonTrivialLocal,
                        .lua = lua,
                        .base = base ? base : Helper::Casting::ptr_cast<Unreal::UObject*>(data),
                        .data = &static_cast<uint8_t*>(data)[field->GetOffset_Internal()],
                        .property = field
                };

                StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                lua_table.fuse_pair();
            }
            else
            {
                // Skip fields without handlers
                Output::send<LogLevel::Verbose>(
                        STR("convert_struct_to_lua_table: Skipping field '{}' of type '{}' (no handler)\n"),
                        to_wstring(field_name),
                        field_type_fname.ToString()
                        );
            }
        }

        lua_table.make_local();
    }

    auto push_structproperty(const PusherParams& params) -> void
    {
        // params.base = Base of the object that this struct belongs in
        // params.data = Start of the struct
        // params.property = The corresponding FStructProperty
        auto* struct_property = Unreal::CastField<Unreal::FStructProperty>(params.property);
        Unreal::UScriptStruct* script_struct = struct_property->GetStruct();

        auto iterate_struct_and_turn_into_lua_table = [&](const LuaMadeSimple::Lua& lua, void* data_ptr) {
            convert_struct_to_lua_table(
                    lua,
                    script_struct,
                    data_ptr,
                    params.create_new_if_get_non_trivial_local,
                    params.base
                    );
        };

        auto lua_table_to_memory = [&]() {
            convert_lua_table_to_struct(
                    params.lua,
                    script_struct,
                    params.data,
                    params.stored_at_index,
                    params.base
                    );
            params.lua.discard_value(params.stored_at_index);
        };

        auto lua_to_memory = [&]() {
            if (params.lua.is_userdata(params.stored_at_index))
            {
                // StructData as userdata
                params.throw_error("push_structproperty::lua_to_memory", "StructData as userdata is not yet implemented but there's userdata on the stack");
            }
            else if (params.lua.is_table(params.stored_at_index))
            {
                // StructData as table
                lua_table_to_memory();
            }
            else if (params.lua.is_nil(params.stored_at_index))
            {
                params.lua.discard_value(params.stored_at_index);
            }
            else
            {
                params.throw_error("push_structproperty::lua_to_memory", "Parameter must be of type 'StructProperty' or table");
            }
        };

        auto property_value = ScriptStructWrapper{struct_property->GetStruct(), params.data, struct_property};

        switch (params.operation)
        {
        case Operation::Get:
            UScriptStruct::construct(params.lua, property_value);
            return;
        case Operation::GetNonTrivialLocal:
            iterate_struct_and_turn_into_lua_table(params.lua, params.data);
            return;
        case Operation::Set:
            lua_to_memory();
            return;
        case Operation::GetParam:
            LocalUnrealParam<ScriptStructWrapper>::construct(params.lua, property_value, params.base, params.property);
            return;
        default:
            params.throw_error("push_structproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_structproperty", "Operation not supported");
    }

    auto push_arrayproperty(const PusherParams& params) -> void
    {
        auto iterate_array_and_turn_into_lua_table = [&](const LuaMadeSimple::Lua& lua, Unreal::FProperty* array_property, void* data_ptr) {
            auto array_inner = static_cast<Unreal::FArrayProperty*>(array_property)->GetInner();
            Unreal::FName property_type_fname = array_inner->GetClass().GetFName();
            int32_t name_comparison_index = property_type_fname.GetComparisonIndex();

            if (StaticState::m_property_value_pushers.contains(name_comparison_index))
            {
                Unreal::FScriptArray* array_container = static_cast<Unreal::FScriptArray*>(data_ptr);

                uint8_t* array_data = static_cast<uint8_t*>(array_container->GetData());
                int32_t array_size = array_container->Num();

                // Get the table from the stack or create a new one
                LuaMadeSimple::Lua::Table lua_table = [&]() {
                    if (params.create_new_if_get_non_trivial_local)
                    {
                        return lua.prepare_new_table();
                    }
                    else
                    {
                        return lua.get_table();
                    }
                }();

                for (int32_t i = 0; i < array_size; ++i)
                {
                    // Array index
                    lua_table.add_key(i + 1);

                    const PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::GetParam,
                                                     .lua = lua,
                                                     .base = params.base,
                                                     .data = array_data + (i * array_inner->GetElementSize()),
                                                     .property = array_inner};
                    StaticState::m_property_value_pushers[name_comparison_index](pusher_params);

                    lua_table.fuse_pair();
                }

                lua_table.make_local();
            }
            else
            {
                std::string property_type_name = to_utf8_string(property_type_fname.ToString());
                params.throw_error("push_arrayproperty",
                                   "Tried interacting with an array but the inner property has no registered handler.",
                                   "Inner property type",
                                   property_type_name);
            }
        };

        auto lua_table_to_memory = [&]() {
            Unreal::FArrayProperty* array_property = static_cast<Unreal::FArrayProperty*>(params.property);
            Unreal::FProperty* inner = array_property->GetInner();
            Unreal::FName inner_type_fname = inner->GetClass().GetFName();

            int32_t name_comparison_index = inner_type_fname.GetComparisonIndex();
            if (!StaticState::m_property_value_pushers.contains(name_comparison_index))
            {
                std::string inner_type_name = to_utf8_string(inner_type_fname.ToString());
                params.throw_error("push_arrayproperty", "Tried pushing ArrayProperty with unsupported inner type", "Inner property type", inner_type_name);
            }

            size_t array_element_size = inner->GetElementSize();

            auto array = new (params.data) Unreal::FScriptArray{};
            size_t table_length = lua_rawlen(params.lua.get_lua_state(), 1);
            bool has_elements = table_length > 0;

            size_t element_index{0};

            if (has_elements)
            {

                params.lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
                    // Skip this table entry if the key wasn't numerical, who knows what the user put in their script
                    if (!table.key.is_integer())
                    {
                        return false;
                    }

                    params.lua.insert_value(-2);
                    array->AddZeroed(1, inner->GetSize(), inner->GetMinAlignment());
                    const PusherParams pusher_params{.operation = Operation::Set,
                                                     .lua = params.lua,
                                                     .base = static_cast<Unreal::UObject*>(array->GetData()), // Base is the start of the params struct
                                                     .data = &static_cast<uint8_t*>(array->GetData())[array_element_size * element_index],
                                                     .property = inner};
                    StaticState::m_property_value_pushers[name_comparison_index](pusher_params);

                    ++element_index;

                    return false;
                });
            }

            // auto* to_array = static_cast<Unreal::FScriptArray*>(params.data);
            if (has_elements)
            {
                // to_array->SetAllocatorInstance(array);
                // size_t new_array_num = element_index - 1 <= 0 ? 0 : element_index - 1;
                // to_array->SetArrayNum(static_cast<int32_t>(new_array_num));
                // to_array->SetArrayMax(to_array->GetArrayNum());
            }
            else
            {
                // to_array->SetAllocatorInstance(nullptr);
                // to_array->SetArrayNum(0);
                // to_array->SetArrayMax(0);
            }

            // Remove the table from the stack to remain consistent to the pusher system, if there were no elements
            // Otherwise it get removed in the end of for_each_in_table loop
            // Other systems might rely on this behavior
            if (!has_elements)
            {
                params.lua.discard_value();
            }
        };

        auto lua_to_memory = [&]() {
            if (params.lua.is_userdata(params.stored_at_index))
            {
                // Handle TArray userdata
                auto& lua_tarray = params.lua.get_userdata<TArray>(params.stored_at_index);
                Unreal::FScriptArray* source_array = lua_tarray.get_remote_cpp_object();
                
                Unreal::FArrayProperty* array_property = static_cast<Unreal::FArrayProperty*>(params.property);
                Unreal::FProperty* inner = array_property->GetInner();
                
                auto dest_array = static_cast<Unreal::FScriptArray*>(params.data);
                
                // Clear destination array
                dest_array->Empty(0, inner->GetSize(), inner->GetMinAlignment());
                
                // Copy elements from source to destination
                int32_t num_elements = source_array->Num();
                if (num_elements > 0)
                {
                    dest_array->AddZeroed(num_elements, inner->GetSize(), inner->GetMinAlignment());
                    
                    for (int32_t i = 0; i < num_elements; i++)
                    {
                        void* src_element = static_cast<uint8_t*>(source_array->GetData()) + (i * inner->GetSize());
                        void* dest_element = static_cast<uint8_t*>(dest_array->GetData()) + (i * inner->GetSize());
                        
                        inner->CopySingleValueToScriptVM(dest_element, src_element);
                    }
                }
            }
            else if (params.lua.is_table(params.stored_at_index))
            {
                // TArray as table
                lua_table_to_memory();
            }
            else if (params.lua.is_nil(params.stored_at_index))
            {
                // Empty array
                auto array = static_cast<Unreal::FScriptArray*>(params.data);
                Unreal::FArrayProperty* array_property = static_cast<Unreal::FArrayProperty*>(params.property);
                Unreal::FProperty* inner = array_property->GetInner();
                array->Empty(0, inner->GetSize(), inner->GetMinAlignment());
            }
            else
            {
                params.throw_error("push_arrayproperty::lua_to_memory", "Parameter must be of type 'TArray' or table");
            }
        };

        switch (params.operation)
        {
        case Operation::Get:
            TArray::construct(params);
            return;
        case Operation::GetNonTrivialLocal:
            iterate_array_and_turn_into_lua_table(params.lua, params.property, params.data);
            return;
        case Operation::Set:
            lua_to_memory();
            return;
        case Operation::GetParam:
            // TODO: Test this
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_arrayproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_arrayproperty", "Operation not supported");
    }

    auto push_setproperty(const PusherParams& params) -> void
    {
        auto set_to_lua_table = [&](const LuaMadeSimple::Lua& lua, Unreal::FProperty* property, void* data_ptr) {
            Unreal::FSetProperty* set_property = static_cast<Unreal::FSetProperty*>(property);
            
            FScriptSetInfo info(set_property->GetElementProp());
            info.validate_pushers(lua);
            
            Unreal::FScriptSet* set = static_cast<Unreal::FScriptSet*>(data_ptr);
            
            LuaMadeSimple::Lua::Table lua_table = [&]() {
                if (params.create_new_if_get_non_trivial_local)
                {
                    return lua.prepare_new_table();
                }
                else
                {
                    return lua.get_table();
                }
            }();
            
            // Create a standard Lua table with integer indices that can be iterated with ipairs
            int index = 1; // Start at 1 for Lua-style indexing
            Unreal::int32 max_index = set->GetMaxIndex();
            for (Unreal::int32 i = 0; i < max_index; i++)
            {
                if (!set->IsValidIndex(i))
                {
                    continue;
                }
                
                // Set the numeric key for the table entry (using sequential indices for easy iteration)
                lua_table.add_key(index++);
                
                // Get the element data at this index
                void* element_data = set->GetData(i, info.layout);
                
                // Push the element value to Lua
                PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::GetParam,
                                          .lua = lua,
                                          .base = params.base,
                                          .data = element_data,
                                          .property = info.element};
                                          
                StaticState::m_property_value_pushers[static_cast<int32_t>(info.element_fname.GetComparisonIndex())](pusher_params);
                
                // Combine key and value in the table
                lua_table.fuse_pair();
            }
            
            lua_table.make_local();
        };
        
        auto lua_table_to_set = [&]() {
            Unreal::FSetProperty* set_property = static_cast<Unreal::FSetProperty*>(params.property);
            
            FScriptSetInfo info(set_property->GetElementProp());
            info.validate_pushers(params.lua);
            
            auto set = new(params.data) Unreal::FScriptSet{};
            
            // Define construct and destruct functions first
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
            
            // The table is at params.stored_at_index
            // We need to ensure it's accessible for iteration
            
            params.lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
                // Prepare temporary storage for the element
                Unreal::TArray<Unreal::uint8> element_data{};
                element_data.Init(0, info.layout.Size);
                
                // For sets, we want to use the value, not the key
                // The for_each_in_table callback provides key and value
                // We need to push the value onto the stack for the pusher
                params.lua.insert_value(-2);  // Push the value onto the stack
                
                PusherParams pusher_params{.operation = Operation::Set,
                                          .lua = params.lua,
                                          .base = params.base,
                                          .data = element_data.GetData(),
                                          .property = info.element};
                                          
                StaticState::m_property_value_pushers[static_cast<int32_t>(info.element_fname.GetComparisonIndex())](pusher_params);
                
                // Add element to the set
                void* element_ptr = element_data.GetData();
                
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
                        
                return false;
            });
            
            // Rehash the set for better performance
            set->Rehash(info.layout,
                       [&](const void* src) -> Unreal::uint32 {
                           return info.element->GetValueTypeHash(src);
                       });
        };
        
        auto lua_to_memory = [&]() {
            if (params.lua.is_userdata(params.stored_at_index))
            {
                // Handle TSet userdata
                auto& lua_tset = params.lua.get_userdata<TSet>(params.stored_at_index);
                Unreal::FScriptSet* source_set = lua_tset.get_remote_cpp_object();
                
                Unreal::FSetProperty* set_property = static_cast<Unreal::FSetProperty*>(params.property);
                FScriptSetInfo info(set_property->GetElementProp());
                
                auto set = new(params.data) Unreal::FScriptSet{};
                
                // Define construct and destruct functions for copying
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
                
                // Copy elements from source set to destination set
                Unreal::int32 max_index = source_set->GetMaxIndex();
                for (Unreal::int32 i = 0; i < max_index; i++)
                {
                    if (!source_set->IsValidIndex(i))
                    {
                        continue;
                    }
                    
                    void* element_ptr = source_set->GetData(i, info.layout);
                    
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
                }
                
                set->Rehash(info.layout,
                           [&](const void* src) -> Unreal::uint32 {
                               return info.element->GetValueTypeHash(src);
                           });
            }
            else if (params.lua.is_table(params.stored_at_index))
            {
                // TSet as table
                lua_table_to_set();
            }
            else if (params.lua.is_nil(params.stored_at_index))
            {
                // Empty set
                new(params.data) Unreal::FScriptSet{};
            }
            else
            {
                params.throw_error("push_setproperty::lua_to_memory", "Parameter must be of type 'TSet' or table");
            }
        };
        
        switch (params.operation)
        {
        case Operation::Get:
            TSet::construct(params);
            return;
        case Operation::GetNonTrivialLocal:
            set_to_lua_table(params.lua, params.property, params.data);
            return;
        case Operation::Set:
            lua_to_memory();
            return;
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_setproperty", "Unhandled Operation");
            break;
        }
        
        params.throw_error("push_setproperty", fmt::format("Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_mapproperty(const PusherParams& params) -> void
    {
        auto map_to_lua_table = [&](const LuaMadeSimple::Lua& lua, Unreal::FProperty* property, void* data_ptr) {
            Unreal::FMapProperty* map_property = static_cast<Unreal::FMapProperty*>(property);

            FScriptMapInfo info(map_property->GetKeyProp(), map_property->GetValueProp());
            info.validate_pushers(lua);

            Unreal::FScriptMap* map = static_cast<Unreal::FScriptMap*>(data_ptr);

            LuaMadeSimple::Lua::Table lua_table = [&]() {
                if (params.create_new_if_get_non_trivial_local)
                {
                    return lua.prepare_new_table();
                }
                else
                {
                    return lua.get_table();
                }
            }();

            Unreal::int32 max_index = map->GetMaxIndex();
            for (Unreal::int32 i = 0; i < max_index; i++)
            {
                if (!map->IsValidIndex(i))
                {
                    continue;
                }

                PusherParams pusher_params{.operation = LuaMadeSimple::Type::Operation::GetParam,
                                           .lua = lua,
                                           .base = params.base,
                                           .data = static_cast<uint8_t*>(map->GetData(i, info.layout)),
                                           .property = nullptr};

                pusher_params.property = info.key;
                StaticState::m_property_value_pushers[static_cast<int32_t>(info.key_fname.GetComparisonIndex())](pusher_params);

                pusher_params.data = static_cast<uint8_t*>(pusher_params.data) + info.layout.ValueOffset;
                pusher_params.property = info.value;
                StaticState::m_property_value_pushers[static_cast<int32_t>(info.value_fname.GetComparisonIndex())](pusher_params);

                lua_table.fuse_pair();
            }

            lua_table.make_local();
        };

        auto lua_table_to_map = [&]() {
            Unreal::FMapProperty* map_property = static_cast<Unreal::FMapProperty*>(params.property);

            FScriptMapInfo info(map_property->GetKeyProp(), map_property->GetValueProp());
            info.validate_pushers(params.lua);

            auto map = new(params.data) Unreal::FScriptMap{};

            params.lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
                params.lua.insert_value(-2);
                params.lua.insert_value(-1);

                // be careful with this function, if you add a duplicate entry,
                // rehashing the map will NOT remove it
                // if you want identical behavior to TMap::Add use the Add function.
                Unreal::int32 added_index = map->AddUninitialized(info.layout);

                PusherParams pusher_params{.operation = Operation::Set,
                                           .lua = params.lua,
                                           .base = static_cast<Unreal::UObject*>(map->GetData(0, info.layout)),
                                           .data = map->GetData(added_index, info.layout),
                                           .property = nullptr};

                Unreal::FMemory::Memzero(pusher_params.data, info.layout.SetLayout.Size);

                pusher_params.property = info.key;
                StaticState::m_property_value_pushers[static_cast<int32_t>(info.key_fname.GetComparisonIndex())](pusher_params);

                pusher_params.data = static_cast<uint8_t*>(pusher_params.data) + info.layout.ValueOffset;
                pusher_params.property = info.value;
                StaticState::m_property_value_pushers[static_cast<int32_t>(info.value_fname.GetComparisonIndex())](pusher_params);

                return false;
            });

            map->Rehash(info.layout,
                        [&](const void* src) -> Unreal::uint32 {
                            return info.key->GetValueTypeHash(src);
                        });
        };

        switch (params.operation)
        {
        case Operation::Get:
            TMap::construct(params);
            return;
        case Operation::GetNonTrivialLocal:
            map_to_lua_table(params.lua, params.property, params.data);
            return;
        case Operation::Set: {
            if (params.lua.is_userdata(params.stored_at_index))
            {
                // Handle TMap userdata
                auto& lua_tmap = params.lua.get_userdata<TMap>(params.stored_at_index);
                Unreal::FScriptMap* source_map = lua_tmap.get_remote_cpp_object();
                
                Unreal::FMapProperty* map_property = static_cast<Unreal::FMapProperty*>(params.property);
                FScriptMapInfo info(map_property->GetKeyProp(), map_property->GetValueProp());
                if (!info.key || !info.value)
                {
                    params.throw_error("push_mapproperty", "Invalid key or value property");
                }
                
                auto dest_map = new(params.data) Unreal::FScriptMap{};
                
                // Copy elements from source to destination
                Unreal::int32 max_index = source_map->GetMaxIndex();
                for (Unreal::int32 i = 0; i < max_index; i++)
                {
                    if (!source_map->IsValidIndex(i))
                    {
                        continue;
                    }
                    
                    // Get source key/value pair
                    void* src_pair = source_map->GetData(i, info.layout);
                    
                    // Add new entry to destination map
                    Unreal::int32 dest_index = dest_map->AddUninitialized(info.layout);
                    void* dest_pair = dest_map->GetData(dest_index, info.layout);
                    
                    // Initialize the destination memory
                    Unreal::FMemory::Memzero(dest_pair, info.layout.SetLayout.Size);
                    
                    // Copy key
                    info.key->CopySingleValueToScriptVM(dest_pair, src_pair);
                    
                    // Copy value
                    void* src_value = static_cast<uint8_t*>(src_pair) + info.layout.ValueOffset;
                    void* dest_value = static_cast<uint8_t*>(dest_pair) + info.layout.ValueOffset;
                    info.value->CopySingleValueToScriptVM(dest_value, src_value);
                }
                
                // Rehash the destination map
                dest_map->Rehash(info.layout,
                                [&](const void* src) -> Unreal::uint32 {
                                    return info.key->GetValueTypeHash(src);
                                });
            }
            else if (params.lua.is_table(params.stored_at_index))
            {
                // TMap as table
                lua_table_to_map();
            }
            else if (params.lua.is_nil(params.stored_at_index))
            {
                // Empty map
                new(params.data) Unreal::FScriptMap{};
            }
            else
            {
                params.throw_error("push_mapproperty", "Parameter must be of type 'TMap' or table");
            }
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_mapproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_mapproperty", fmt::format("Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }


    auto push_functionproperty(const FunctionPusherParams& params) -> void
    {
        UFunction::construct(params.lua, params.base, params.function);
    }

    auto push_floatproperty(const PusherParams& params) -> void
    {
        float* float_ptr = static_cast<float*>(params.data);
        if (!float_ptr)
        {
            params.throw_error("push_floatproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            params.lua.set_float(*float_ptr);
            return;
        case Operation::Set:
            *float_ptr = params.lua.get_float(params.stored_at_index);
            return;
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, float_ptr, params.base, params.property);
            return;
        default:
            params.throw_error("push_floatproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_floatproperty", "Operation not supported");
    }

    auto push_doubleproperty(const PusherParams& params) -> void
    {
        double* double_ptr = static_cast<double*>(params.data);
        if (!double_ptr)
        {
            params.throw_error("push_doubleproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            params.lua.set_number(*double_ptr);
            return;
        case Operation::Set:
            *double_ptr = params.lua.get_number(params.stored_at_index);
            return;
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, double_ptr, params.base, params.property);
            return;
        default:
            params.throw_error("push_doubleproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_doubleproperty", "Operation not supported");
    }

    auto push_boolproperty(const PusherParams& params) -> void
    {
        uint8_t* bitfield_ptr = static_cast<uint8_t*>(params.data);
        if (!bitfield_ptr)
        {
            params.throw_error("push_boolproperty", "data pointer is nullptr");
        }

        Unreal::FBoolProperty* bp = static_cast<Unreal::FBoolProperty*>(params.property);
        uint8_t field_mask = bp->GetFieldMask();
        uint8_t byte_offset = bp->GetByteOffset();
        uint8_t* byte_value = bitfield_ptr + byte_offset;

        switch (params.operation)
        {
        case Operation::Get:
            params.lua.set_bool(!!(*byte_value & field_mask));
            return;
        case Operation::GetNonTrivialLocal:
            params.lua.set_bool(*static_cast<bool*>(params.data));
            return;
        case Operation::Set: {
            uint8_t byte_mask = bp->GetByteMask();

            *byte_value = ((*byte_value) & ~field_mask) | (params.lua.get_bool(params.stored_at_index) ? byte_mask : 0);
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, bitfield_ptr, params.base, params.property);
            return;
        default:
            params.throw_error("push_boolproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_boolproperty", "Operation not supported");
    }

    auto push_enumproperty(const PusherParams& params) -> void
    {
        Unreal::UEnum* enum_ptr = static_cast<Unreal::FEnumProperty*>(params.property)->GetEnum();
        if (!enum_ptr)
        {
            params.throw_error("push_enumproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get: {
            // Make global table to store enum name/value pairs -> START
            auto table = params.lua.prepare_new_table();

            std::string prop_name = to_string(params.property->GetName());

            for (const auto& elem : enum_ptr->ForEachName())
            {
                std::string elem_name = to_string(elem.Key.ToString());
                table.add_pair(elem_name.c_str(), elem.Value);
            }

            // TODO: Optimize this... it'll probably do a dynamic allocation here in order to fit the new beginning of the string
            prop_name.insert(0, "Enum_");
            table.make_global(prop_name);
            // Make global table to store enum name/value pairs -> END

            // Push the actual enum value to the Lua stack
            params.lua.set_integer(*static_cast<uint8_t*>(params.data));

            return;
        }
        case Operation::Set:
            // TODO: Verify that this works
            // TODO: Bounds checking to make sure we don't set an invalid uint8_t because Lua can send us up to 64 bits
            *static_cast<uint8_t*>(params.data) = static_cast<uint8_t>(params.lua.get_integer(params.stored_at_index));
            return;
        case Operation::GetParam:
            // TODO: Verify that this works
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_enumproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_enumproperty", "Operation not supported");
    }

    auto push_weakobjectproperty(const PusherParams& params) -> void
    {
        if (!params.data)
        {
            params.throw_error("push_weakobjectproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get: {
            if (!params.data)
            {
                LuaType::FWeakObjectPtr::construct(params.lua, Unreal::FWeakObjectPtr{});
            }
            else
            {
                LuaType::FWeakObjectPtr::construct(params.lua, *static_cast<Unreal::FWeakObjectPtr*>(params.data));
            }
            return;
        }
        case Operation::Set:
            // For now, doing nothing just to get past the error
            Output::send(STR("[push_weakobjectproperty] Operation::Set is not supported\n"));
            return;
        case Operation::GetParam:
            params.throw_error("push_weakobjectproperty", "Operation::GetParam is not supported");
            return;
        default:
            params.throw_error("push_weakobjectproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_weakobjectproperty", "Operation not supported");
    }

    auto push_nameproperty(const PusherParams& params) -> void
    {
        Unreal::FName* name = static_cast<Unreal::FName*>(params.data);
        if (!name)
        {
            params.throw_error("push_nameproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::FName::construct(params.lua, *name);
            return;
        case Operation::Set: {
            auto& lua_object = params.lua.get_userdata<LuaType::FName>(params.stored_at_index);
            *name = lua_object.get_local_cpp_object();
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_nameproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_nameproperty", "Operation not supported");
    }

    auto push_textproperty(const PusherParams& params) -> void
    {
        Unreal::FText* text = static_cast<Unreal::FText*>(params.data);
        if (!text)
        {
            params.throw_error("push_textproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::FText::construct(params.lua, *text);
            return;
        case Operation::Set: {
            auto& lua_other_object = params.lua.get_userdata<LuaType::FText>(params.stored_at_index);
            text->SetString(std::move(lua_other_object.get_local_cpp_object().ToFString()));
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_textproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_textproperty", "Operation not supported");
    }

    auto push_strproperty(const PusherParams& params) -> void
    {
        Unreal::FString* string = static_cast<Unreal::FString*>(params.data);
        if (!string)
        {
            params.throw_error("push_strproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::FString::construct(params.lua, string);
            return;
        case Operation::Set: {
            if (params.lua.is_string(params.stored_at_index))
            {
                auto lua_string = params.lua.get_string(params.stored_at_index);
                auto fstring = Unreal::FString{FromCharTypePtr<TCHAR>(ensure_str(lua_string).c_str())};
                *string = fstring;
            }
            else if (params.lua.is_userdata(params.stored_at_index))
            {
                auto& rhs = params.lua.get_userdata<LuaType::FString>(params.stored_at_index);
                string->SetCharArray(rhs.get_local_cpp_object().GetCharArray());
            }
            else
            {
                params.throw_error("push_strproperty", "StrProperty can only be set to a string or FString");
            }
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_strproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_strproperty", "Operation not supported");
    }

    auto push_utf8strproperty(const PusherParams& params) -> void
    {
        Unreal::FUtf8String* string = static_cast<Unreal::FUtf8String*>(params.data);
        if (!string)
        {
            params.throw_error("push_utf8strproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::FUtf8String::construct(params.lua, string);
            return;
        case Operation::Set: {
            if (params.lua.is_string(params.stored_at_index))
            {
                auto lua_string = params.lua.get_string(params.stored_at_index);
                *string = Unreal::FUtf8String(reinterpret_cast<const Unreal::UTF8CHAR*>(lua_string.data()));
            }
            else if (params.lua.is_userdata(params.stored_at_index))
            {
                auto& rhs = params.lua.get_userdata<LuaType::FUtf8String>(params.stored_at_index);
                *string = rhs.get_local_cpp_object();
            }
            else
            {
                params.throw_error("push_utf8strproperty", "Utf8StrProperty can only be set to a string or FUtf8String");
            }
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_utf8strproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_utf8strproperty", "Operation not supported");
    }

    auto push_ansistrproperty(const PusherParams& params) -> void
    {
        Unreal::FAnsiString* string = static_cast<Unreal::FAnsiString*>(params.data);
        if (!string)
        {
            params.throw_error("push_ansistrproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::FAnsiString::construct(params.lua, string);
            return;
        case Operation::Set: {
            if (params.lua.is_string(params.stored_at_index))
            {
                auto lua_string = params.lua.get_string(params.stored_at_index);
                *string = Unreal::FAnsiString(lua_string.data());
            }
            else if (params.lua.is_userdata(params.stored_at_index))
            {
                auto& rhs = params.lua.get_userdata<LuaType::FAnsiString>(params.stored_at_index);
                *string = rhs.get_local_cpp_object();
            }
            else
            {
                params.throw_error("push_ansistrproperty", "AnsiStrProperty can only be set to a string or FAnsiString");
            }
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_ansistrproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_ansistrproperty", "Operation not supported");
    }

    auto push_softobjectproperty(const PusherParams& params) -> void
    {
        auto soft_ptr = static_cast<Unreal::FSoftObjectPtr*>(params.data);
        if (!soft_ptr)
        {
            params.throw_error("push_softobjectproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::TSoftObjectPtr::construct(params.lua, *soft_ptr);
            return;
        case Operation::Set: {
            auto& lua_object = params.lua.get_userdata<LuaType::TSoftObjectPtr>(params.stored_at_index);
            *soft_ptr = lua_object.get_local_cpp_object();
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.throw_error("push_softobjectproperty", "Unhandled Operation");
            break;
        }

        params.throw_error("push_softobjectproperty", "Operation not supported");
    }

    auto push_interfaceproperty(const PusherParams& params) -> void
    {
        auto property_value = static_cast<Unreal::UInterface**>(params.data);
        if (!property_value)
        {
            params.throw_error("push_interfaceproperty", "data pointer is nullptr");
        }

        // Finally construct the Lua object
        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            auto_construct_object(params.lua, *property_value);
            break;
        case Operation::Set: {
            if (params.lua.is_userdata(params.stored_at_index))
            {
                const auto& lua_object = params.lua.get_userdata<LuaType::UInterface>(params.stored_at_index);
                *property_value = lua_object.get_remote_cpp_object();
            }
            else if (params.lua.is_nil(params.stored_at_index))
            {
                params.lua.discard_value(params.stored_at_index);
                *property_value = nullptr;
            }
            else
            {
                params.throw_error("push_interfaceproperty", "Value must be UInterface or nil");
            }
            break;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.throw_error("push_interfaceproperty", "Operation not supported");
            break;
        }
    }

    auto push_delegateproperty(const PusherParams& params) -> void
    {
        using namespace Unreal;
        auto* delegate_value = static_cast<FScriptDelegate*>(params.data);
        if (!delegate_value)
        {
            params.throw_error("push_delegateproperty", "data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get: {
            // Return a table with {Object = UObject, FunctionName = FName}
            LuaMadeSimple::Lua::Table lua_table = params.lua.prepare_new_table();

            lua_table.add_key("Object");
            auto_construct_object(params.lua, delegate_value->GetUObject());
            lua_table.fuse_pair();

            lua_table.add_key("FunctionName");
            FName::construct(params.lua, delegate_value->GetFunctionName());
            lua_table.fuse_pair();

            lua_table.make_local();
            break;
        }
        case Operation::Set: {
            if (params.lua.is_table(params.stored_at_index))
            {
                lua_pushstring(params.lua.get_lua_state(), "Object");
                int adjusted_index = params.stored_at_index;
                if (params.stored_at_index < 0)
                {
                    adjusted_index = params.stored_at_index - 1;
                }
                lua_rawget(params.lua.get_lua_state(), adjusted_index);

                Unreal::UObject* obj = nullptr;
                if (params.lua.is_userdata(-1))
                {
                    const auto& lua_object = params.lua.get_userdata<LuaType::UObject>(-1);
                    obj = lua_object.get_remote_cpp_object();
                }
                params.lua.discard_value();

                lua_pushstring(params.lua.get_lua_state(), "FunctionName");
                lua_rawget(params.lua.get_lua_state(), adjusted_index);

                Unreal::FName fname = NAME_None;
                if (params.lua.is_userdata(-1))
                {
                    auto& lua_fname = params.lua.get_userdata<LuaType::FName>(-1);
                    fname = lua_fname.get_local_cpp_object();
                }
                params.lua.discard_value();

                // Bind the delegate - this modifies the existing FWeakObjectPtr which calls
                // the engine's assignment operator that will allocate serial numbers
                delegate_value->BindUFunction(obj, fname);
                params.lua.discard_value(params.stored_at_index);
            }
            else if (params.lua.is_nil(params.stored_at_index))
            {
                delegate_value->Clear();
                params.lua.discard_value(params.stored_at_index);
            }
            else
            {
                params.throw_error("push_delegateproperty", "Value must be a table {Object = UObject, FunctionName = FName} or nil");
            }
            break;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.throw_error("push_delegateproperty", "Operation not supported");
            break;
        }
    }

    auto push_multicastdelegateproperty(const PusherParams& params) -> void
    {
        using namespace Unreal;

        auto* delegate_property = static_cast<FMulticastDelegateProperty*>(params.property);
        if (!delegate_property)
        {
            params.throw_error("push_multicastdelegateproperty", "property is not FMulticastDelegateProperty");
        }

        switch (params.operation)
        {
        case Operation::Get:
            XMulticastDelegateProperty::construct(params);
            break;
        case Operation::GetNonTrivialLocal: {
            const FMulticastScriptDelegate* delegate_value = delegate_property->GetMulticastDelegate(params.data);
            if (!delegate_value)
            {
                params.lua.set_nil();
                break;
            }

            LuaMadeSimple::Lua::Table lua_table = params.lua.prepare_new_table();

            int32 count = delegate_value->Num();
            for (int32 i = 0; i < count; ++i)
            {
                lua_table.add_key(i + 1); // Lua is 1-indexed

                LuaMadeSimple::Lua::Table delegate_entry = params.lua.prepare_new_table();

                delegate_entry.add_key("Object");
                auto_construct_object(params.lua, delegate_value->InvocationList[i].GetUObject());
                delegate_entry.fuse_pair();

                delegate_entry.add_key("FunctionName");
                FName::construct(params.lua, delegate_value->InvocationList[i].GetFunctionName());
                delegate_entry.fuse_pair();

                delegate_entry.make_local();
                lua_table.fuse_pair(); // Set array element
            }

            lua_table.make_local();
            break;
        }
        case Operation::Set:
            // Multicast delegates cannot be set directly. Use the property wrapper methods instead:
            // local prop = obj.OnSomething
            // prop:Add(targetObj, FName("FunctionName"))
            // prop:Remove(targetObj, FName("FunctionName"))
            // prop:Clear()
            params.throw_error("push_multicastdelegateproperty",
                               "Multicast delegate values are read-only. Use property methods: GetPropertyByName():Add/Remove/Clear");
            break;
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.throw_error("push_multicastdelegateproperty", "Operation not supported");
            break;
        }
    }

    auto push_multicastsparsedelegateproperty(const PusherParams& params) -> void
    {
        using namespace Unreal;

        // Sparse delegates work similarly to regular multicast delegates
        // The main difference is they use 1 byte + global storage instead of inline InvocationList
        auto* delegate_property = static_cast<FMulticastSparseDelegateProperty*>(params.property);
        if (!delegate_property)
        {
            params.throw_error("push_multicastsparsedelegateproperty", "property is not FMulticastSparseDelegateProperty");
        }

        switch (params.operation)
        {
        case Operation::Get:
            XMulticastSparseDelegateProperty::construct(params);
            break;
        case Operation::GetNonTrivialLocal: {
            const FMulticastScriptDelegate* delegate_value = delegate_property->GetMulticastDelegate(params.data);
            if (!delegate_value)
            {
                params.lua.set_nil();
                break;
            }

            LuaMadeSimple::Lua::Table lua_table = params.lua.prepare_new_table();

            int32 count = delegate_value->Num();
            for (int32 i = 0; i < count; ++i)
            {
                lua_table.add_key(i + 1); // Lua is 1-indexed

                LuaMadeSimple::Lua::Table delegate_entry = params.lua.prepare_new_table();

                delegate_entry.add_key("Object");
                auto_construct_object(params.lua, delegate_value->InvocationList[i].GetUObject());
                delegate_entry.fuse_pair();

                delegate_entry.add_key("FunctionName");
                FName::construct(params.lua, delegate_value->InvocationList[i].GetFunctionName());
                delegate_entry.fuse_pair();

                delegate_entry.make_local();
                lua_table.fuse_pair();
            }

            lua_table.make_local();
            break;
        }
        case Operation::Set:
            // Multicast delegates cannot be set directly. Use the property wrapper methods instead:
            // local prop = obj.OnSomething
            // prop:Add(targetObj, FName("FunctionName"))
            // prop:Remove(targetObj, FName("FunctionName"))
            // prop:Clear() 
            params.throw_error("push_multicastsparsedelegateproperty",
                               "Sparse multicast delegate values are read-only. Use property methods: GetPropertyByName():Add/Remove/Clear");
            break;
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.throw_error("push_multicastsparsedelegateproperty", "Operation not supported");
            break;
        }
    }

    auto static is_a_internal(const LuaMadeSimple::Lua& lua, Unreal::UObject* object, Unreal::UClass* object_class) -> bool
    {
        if (!object || !object_class)
        {
            return false;
        }
        else
        {
            return object->IsA(object_class);
        }
    }

    auto is_a_implementation(const LuaMadeSimple::Lua& lua) -> int
    {
        std::string error_overload_not_found{R"(
No overload found for function 'IsA'.
Overloads:
#1: IsA(UClass Class)
#2: IsA(string ObjectFullName)"};

        auto& lua_object = lua.get_userdata<UObject>();
        auto* object = lua_object.get_remote_cpp_object();

        if (lua.is_userdata())
        {
            auto& lua_object_class = lua.get_userdata<UClass>();
            auto* object_class = lua_object_class.get_remote_cpp_object();
            lua.set_bool(is_a_internal(lua, object, object_class));
        }
        else if (lua.is_string())
        {
            auto* object_class = Unreal::UObjectGlobals::StaticFindObject<Unreal::UClass*>(nullptr, nullptr, ensure_str(lua.get_string()));
            lua.set_bool(is_a_internal(lua, object, object_class));
        }
        else
        {
            lua.throw_error(error_overload_not_found);
        }

        return 1;
    }

    auto handle_unreal_property_value(
            const Operation operation, const LuaMadeSimple::Lua& lua, Unreal::UObject* base, Unreal::FName property_name, Unreal::FField* field) -> void
    {
        // In UE versions prior to 4.25, UFunctions can be found with 'find_property', and thus 'property' will not be nullptr
        // So you must take that into account when checking if the Lua script is trying to call a UFunction
        if (!field || field->GetClass().GetFName() == Unreal::GFunctionName)
        {
            Unreal::UFunction* func{};

            // We can take a shortcut if property is non-nullptr
            // It means that the UFunction was found and is stored in 'property', so we don't need to do anything to find it
            if (!field && property_name != Unreal::FName(0u, 0u))
            {
                func = base->GetFunctionByNameInChain(property_name);
            }
            else
            {
                // TODO: Figure out a better way to do this, ideally, there shouldn't be a need to bit_cast here
                func = std::bit_cast<Unreal::UFunction*>(field);
            }

            if (func)
            {
                push_functionproperty(FunctionPusherParams{.lua = lua, .base = base, .function = func});
            }
            else
            {
                // Construct an empty object to allow for safe chaining with a validity check at the end
                LuaType::UObject::construct(lua, nullptr);
            }

            return;
        }

        // Casting to XProperty here so that we can get access to property members
        // It needed to be FField above so that it could be converted to UFunction without force
        // This is because UFunction & XProperty both inherit from XField, but UFunction doesn't inherit from XProperty
        Unreal::FProperty* property = static_cast<Unreal::FProperty*>(field);

        Unreal::FName property_type = property->GetClass().GetFName();
        int32_t name_comparison_index = property_type.GetComparisonIndex();

        if (StaticState::m_property_value_pushers.contains(name_comparison_index))
        {
            void* data = static_cast<uint8_t*>(static_cast<void*>(base)) + property->GetOffset_Internal();

            const PusherParams pusher_params{.operation = operation, .lua = lua, .base = base, .data = data, .property = property};
            StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
        }
        else
        {
            // We can either throw an error and kill the execution
            /**/
            std::string property_type_name = to_string(property_type.ToString());
            lua.throw_error(fmt::format(
                    "[handle_unreal_property_value] Tried accessing unreal property without a registered handler. Property type '{}' not supported.",
                    property_type_name));
            //*/

            // Or we can treat unhandled property types as some sort of generic type
            // For example, we might want to pass the unhandled property value to some sort of low-level Lua API
            // Then the Lua script can deal with figuring out how to handle the type
            /*
            push_unhandledproperty(lua, base);
            //*/
        }
    }

    RemoteUnrealParam::RemoteUnrealParam(void* ptr_object, Unreal::UObject* base, Unreal::FProperty* property, const Unreal::FName type)
        : LuaMadeSimple::Type::RemoteObject<void>("Param", ptr_object), m_property(property), m_base(base), m_type(type)
    {
    }

    RemoteUnrealParam::RemoteUnrealParam(void* data_ptr, const Unreal::FName type) : LuaMadeSimple::Type::RemoteObject<void>("Param", data_ptr), m_type(type)
    {
    }

    auto RemoteUnrealParam::construct(const LuaMadeSimple::Lua& lua, void* param_ptr, Unreal::UObject* base, Unreal::FProperty* property) -> const LuaMadeSimple::Lua::Table
    {
        const Unreal::FName property_type = property->GetClass().GetFName();
        LuaType::RemoteUnrealParam lua_object{param_ptr, base, property, property_type};

        auto metatable_name = "RemoteUnrealParam";

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions(table);
            lua.new_metatable<LuaType::RemoteUnrealParam>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto RemoteUnrealParam::construct(const LuaMadeSimple::Lua& lua, void* data_ptr, const Unreal::FName type) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::RemoteUnrealParam lua_object{data_ptr, type};

        auto metatable_name = "RemoteUnrealParam";

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            lua.prepare_new_table();
            setup_metamethods(lua_object);
            setup_member_functions(table);
            lua.new_metatable<LuaType::RemoteUnrealParam>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto RemoteUnrealParam::setup_metamethods(BaseObject& base_object) -> void
    {
        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Index, [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(Operation::Get, lua);
            return 1;
        });

        base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::NewIndex, [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(Operation::Set, lua);
            return 0;
        });
    }

    auto RemoteUnrealParam::setup_member_functions(LuaMadeSimple::Lua::Table& table) -> void
    {
        table.add_pair("Get", [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(Operation::Get, lua);
            return 1;
        });

        table.add_pair("get", [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(Operation::Get, lua);
            return 1;
        });

        table.add_pair("set", [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(Operation::Set, lua);
            return 0;
        });

        table.add_pair("Set", [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(Operation::Set, lua);
            return 0;
        });

        // This class is not overridable so this is always the final class and because of that we always want to set the type & make it global
        table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
            lua.set_string("RemoteUnrealParam");
            return 1;
        });

        // If this is the final object then we also want to finalize creating the table
        // If not then it's the responsibility of the overriding object to call 'make_global()'
        // table.make_global("RemoteUnrealParam");
    }

    auto RemoteUnrealParam::prepare_to_handle(const Operation operation, const LuaMadeSimple::Lua& lua) -> void
    {
        auto& lua_object = lua.get_userdata<LuaType::RemoteUnrealParam>();

        int32_t type_name_comparison_index = lua_object.m_type.GetComparisonIndex();

        if (StaticState::m_property_value_pushers.contains(type_name_comparison_index))
        {
            const PusherParams pusher_params{.operation = operation,
                                             .lua = lua,
                                             .base = lua_object.m_base,
                                             .data = lua_object.get_remote_cpp_object(),
                                             .property = lua_object.m_property};
            StaticState::m_property_value_pushers[type_name_comparison_index](pusher_params);
        }
        else
        {
            // We can either throw an error and kill the execution
            /**/
            std::string property_type_name = to_string(lua_object.m_type.ToString());
            lua.throw_error(fmt::format(
                    "[RemoteUnrealParam::prepare_to_handle] Tried accessing unreal property without a registered handler. Property type '{}' not supported.",
                    property_type_name));
            //*/

            // Or we can treat unhandled property types as some sort of generic type
            // For example, we might want to pass the unhandled property value to some sort of low-level Lua API
            // Then the Lua script can deal with figuring out how to handle the type
            /*
            push_unhandledproperty(lua, base);
            //*/
        }
    }
} // namespace RC::LuaType
