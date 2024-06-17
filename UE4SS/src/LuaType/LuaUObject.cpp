#include <Helpers/Casting.hpp>
#include <LuaType/LuaAActor.hpp>
#include <LuaType/LuaCustomProperty.hpp>
#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaFString.hpp>
#include <LuaType/LuaFText.hpp>
#include <LuaType/LuaFWeakObjectPtr.hpp>
#include <LuaType/LuaTArray.hpp>
#include <LuaType/LuaTSoftClassPtr.hpp>
#include <LuaType/LuaUClass.hpp>
#include <LuaType/LuaUEnum.hpp>
#include <LuaType/LuaUFunction.hpp>
#include <LuaType/LuaUInterface.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaUScriptStruct.hpp>
#include <LuaType/LuaUStruct.hpp>
#include <LuaType/LuaUWorld.hpp>
#include <LuaType/LuaXObjectProperty.hpp>
#include <LuaType/LuaXProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/AActor.hpp>
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FWeakObjectProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/World.hpp>
#pragma warning(default : 4005)
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/Integer.hpp>

namespace RC::LuaType
{
    std::unordered_set<size_t> s_lua_unreal_objects{};

    auto add_to_global_unreal_objects_map(Unreal::UObject* object) -> void
    {
        if (object)
        {
            s_lua_unreal_objects.emplace(object->HashObject());
        }
    }

    auto is_object_in_global_unreal_object_map(Unreal::UObject* object) -> bool
    {
        return object && s_lua_unreal_objects.contains(object->HashObject());
    }

    FLuaObjectDeleteListener FLuaObjectDeleteListener::s_lua_object_delete_listener{};
    void FLuaObjectDeleteListener::NotifyUObjectDeleted(const Unreal::UObjectBase* object, [[maybe_unused]] int32_t index)
    {
        if (auto it = s_lua_unreal_objects.find(static_cast<const Unreal::UObject*>(object)->HashObject()); it != s_lua_unreal_objects.end())
        {
            s_lua_unreal_objects.erase(it);
        }
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
            for (Unreal::FProperty* param_next : func->ForEachProperty())
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

                        if (!param_next->IsA<Unreal::FStructProperty>())
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

                auto reuse_same_table = param->IsA<Unreal::FArrayProperty>() || param->IsA<Unreal::FStructProperty>();
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
            if (params.lua.is_userdata())
            {
                const auto& lua_object = params.lua.get_userdata<LuaType::UObject>(params.stored_at_index);
                *property_value = lua_object.get_remote_cpp_object();
            }
            else if (params.lua.is_nil())
            {
                *property_value = nullptr;
            }
            else
            {
                params.lua.throw_error("[push_objectproperty] Value must be UObject or nil");
            }
            break;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.lua.throw_error("[push_objectproperty] Unhandled Operation");
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
            if (params.lua.is_userdata())
            {
                const auto& lua_object = params.lua.get_userdata<LuaType::UClass>(params.stored_at_index);
                *property_value = lua_object.get_remote_cpp_object();
            }
            else if (params.lua.is_nil())
            {
                params.lua.discard_value();
            }
            else
            {
                params.lua.throw_error("[push_classproperty] Value must be UClass or nil");
            }
            break;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.lua.throw_error("[push_classproperty] Unhandled Operation");
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

    auto push_structproperty(const PusherParams& params) -> void
    {
        // params.base = Base of the object that this struct belongs in
        // params.data = Start of the struct
        // params.property = The corresponding FStructProperty
        auto* struct_property = Unreal::CastField<Unreal::FStructProperty>(params.property);
        auto* script_struct = struct_property->GetStruct();

        auto iterate_struct_and_turn_into_lua_table = [&](const LuaMadeSimple::Lua& lua, void* data_ptr) {
            auto* data = static_cast<unsigned char*>(data_ptr);

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

            // Put all the script struct properties into the table
            for (Unreal::FProperty* field : script_struct->ForEachPropertyInChain())
            {
                // There can be non-property items in the linked list, like 'ExecuteUbergraph'
                // We only care about actual properties, so let's ignore anything else
                // TODO: This may have only been the case before the ForEachProperty abstraction
                //       The abstraction filters out any non-property types
                //       Commenting out this code for now, should investigate this later to confirm
                // if (!Unreal::TypeChecker::is_property(field))
                //{
                //    return LoopAction::Continue;
                //}

                std::string field_name = to_string(field->GetName());

                Unreal::FName field_type = field->GetClass().GetFName();
                int32_t name_comparison_index = field_type.GetComparisonIndex();

                if (StaticState::m_property_value_pushers.contains(name_comparison_index))
                {
                    // This example uses the Vector struct and its "IntProperty Vector:X" property
                    // Push key (i.e: X)
                    lua_table.add_key(field_name.c_str());

                    // Push value (i.e: 4.243)
                    const PusherParams pusher_params{.operation = Operation::GetNonTrivialLocal,
                                                     .lua = lua,
                                                     .base = Helper::Casting::ptr_cast<Unreal::UObject*>(data), // Base is the start of the params struct
                                                     .data = &data[field->GetOffset_Internal()],
                                                     .property = field,
                                                     .stored_at_index = params.stored_at_index};
                    StaticState::m_property_value_pushers[name_comparison_index](pusher_params);

                    // On the top of the stack now: Value for this field in the struct
                    lua_table.fuse_pair();
                }
                else
                {
                    std::string field_type_name = to_string(field_type.ToString());
                    lua.throw_error(fmt::format("Tried getting without a registered handler. 'StructProperty'.'{}' not supported. Field: '{}'",
                                                field_type_name,
                                                field_name));
                }
            };

            lua_table.make_local();
        };

        auto lua_table_to_memory = [&]() {
            // At the bottom of the stack now: table that has struct data

            // Duplicating the table and putting the duplicate at the top of the stack
            lua_pushvalue(params.lua.get_lua_state(), 1);

            for (Unreal::FProperty* field : script_struct->ForEachPropertyInChain())
            {
                Unreal::FName field_type_fname = field->GetClass().GetFName();
                const std::string field_name = to_string(field->GetName());

                // At the top of the stack now: table that has struct data

                // Pushing the field name (key for the table)
                lua_pushstring(params.lua.get_lua_state(), field_name.c_str());

                // At the top of the stack now: key to find in table (string)

                // Pushing on to the stack, the value corresponding to table[key] if it exists
                auto table_value_type = lua_rawget(params.lua.get_lua_state(), -2);

                // At the top of the stack now: the value corresponding to table[key] or nil

                // If there was nothing in the table for this field, leave default value and move on to the next field
                if (table_value_type == LUA_TNIL || table_value_type == LUA_TNONE)
                {
                    // Remove the 'nil' from the stack
                    params.lua.discard_value(-1);
                    continue;
                }

                int32_t name_comparison_index = field_type_fname.GetComparisonIndex();

                if (StaticState::m_property_value_pushers.contains(name_comparison_index))
                {
                    unsigned char* data = static_cast<unsigned char*>(params.data);
                    data = &data[field->GetOffset_Internal()];

                    const PusherParams pusher_params{
                            .operation = Operation::Set,
                            .lua = params.lua,
                            .base = static_cast<Unreal::UObject*>(params.data), // Base is the start of the params struct
                            .data = data,
                            .property = field,
                            .stored_at_index = -1 // Using -1 here because we know the value is at the top of the stack instead of at the bottom of the stack
                    };
                    StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                }
                else
                {
                    std::string field_type_name = to_string(field_type_fname.ToString());
                    params.lua.throw_error(fmt::format("Tried pushing (Operation::Set) StructProperty without a registered handler for field '{} {}'.",
                                                       field_type_name,
                                                       field_name));
                }
            };

            // Discard the original & the duplicated tables
            params.lua.discard_value(1);  // Original
            params.lua.discard_value(-1); // Duplicated
        };

        auto lua_to_memory = [&]() {
            if (params.lua.is_userdata())
            {
                // StructData as userdata
                params.lua.throw_error("[push_structproperty::lua_to_memory] StructData as userdata is not yet implemented but there's userdata on the stack");
            }
            else if (params.lua.is_table())
            {
                // StructData as table
                lua_table_to_memory();
            }
            else if (params.lua.is_nil())
            {
                params.lua.discard_value();
            }
            else
            {
                params.lua.throw_error("[push_structproperty::lua_to_memory] Parameter must be of type 'StructProperty' or table");
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
            params.lua.throw_error("[push_structproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_structproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
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
                std::string property_type_name = to_string(property_type_fname.ToString());
                lua.throw_error(
                        fmt::format("Tried interacting with an array but the inner property has no registered handler. Property type '{}' not supported.",
                                    property_type_name));
            }
        };

        auto lua_table_to_memory = [&]() {
            Unreal::FArrayProperty* array_property = static_cast<Unreal::FArrayProperty*>(params.property);
            Unreal::FProperty* inner = array_property->GetInner();
            Unreal::FName inner_type_fname = inner->GetClass().GetFName();

            int32_t name_comparison_index = inner_type_fname.GetComparisonIndex();
            if (!StaticState::m_property_value_pushers.contains(name_comparison_index))
            {
                std::string inner_type_name = to_string(inner_type_fname.ToString());
                params.lua.throw_error(fmt::format("Tried pushing (Operation::Set) ArrayProperty with unsupported inner type of '{}'", inner_type_name));
            }

            size_t array_element_size = inner->GetElementSize();

            unsigned char* array{};
            size_t table_length = lua_rawlen(params.lua.get_lua_state(), 1);
            bool has_elements = table_length > 0;

            size_t element_index{0};

            if (has_elements)
            {
                array = new unsigned char[array_property->GetElementSize() * table_length];

                params.lua.for_each_in_table([&](LuaMadeSimple::LuaTableReference table) -> bool {
                    // Skip this table entry if the key wasn't numerical, who knows what the user put in their script
                    if (!table.key.is_integer())
                    {
                        return false;
                    }

                    params.lua.insert_value(-2);
                    const PusherParams pusher_params{.operation = Operation::Set,
                                                     .lua = params.lua,
                                                     .base = static_cast<Unreal::UObject*>(static_cast<void*>(array)), // Base is the start of the params struct
                                                     .data = &array[array_element_size * element_index],
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
            if (params.lua.is_userdata())
            {
                // TArray as userdata
                params.lua.throw_error("[push_arrayproperty::lua_to_memory] StructData as userdata is not yet implemented but there's userdata on the stack");
            }
            else if (params.lua.is_table())
            {
                // TArray as table
                lua_table_to_memory();
            }
            else if (params.lua.is_nil())
            {
                params.lua.discard_value();
            }
            else
            {
                params.lua.throw_error("[push_arrayproperty::lua_to_memory] Parameter must be of type 'StructProperty' or table");
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
            params.lua.throw_error("[push_arrayproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_arrayproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
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
            params.lua.throw_error("[push_floatproperty] data pointer is nullptr");
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
            params.lua.throw_error("[push_floatproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_floatproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_doubleproperty(const PusherParams& params) -> void
    {
        double* double_ptr = static_cast<double*>(params.data);
        if (!double_ptr)
        {
            params.lua.throw_error("[push_doubleproperty] data pointer is nullptr");
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
            params.lua.throw_error("[push_doubleproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_doubleproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_boolproperty(const PusherParams& params) -> void
    {
        uint8_t* bitfield_ptr = static_cast<uint8_t*>(params.data);
        if (!bitfield_ptr)
        {
            params.lua.throw_error("[push_boolproperty] data pointer is nullptr");
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
            params.lua.throw_error("[push_boolproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_boolproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_enumproperty(const PusherParams& params) -> void
    {
        Unreal::UEnum* enum_ptr = static_cast<Unreal::FEnumProperty*>(params.property)->GetEnum();
        if (!enum_ptr)
        {
            params.lua.throw_error("[push_enumproperty] data pointer is nullptr");
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
            params.lua.throw_error("[push_enumproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_enumproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_weakobjectproperty(const PusherParams& params) -> void
    {
        if (!params.data)
        {
            params.lua.throw_error("[push_weakobjectproperty] data pointer is nullptr");
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
            params.lua.throw_error("[push_weakobjectproperty] Operation::GetParam is not supported");
            return;
        default:
            params.lua.throw_error("[push_weakobjectproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_weakobjectproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_nameproperty(const PusherParams& params) -> void
    {
        Unreal::FName* name = static_cast<Unreal::FName*>(params.data);
        if (!name)
        {
            params.lua.throw_error("[push_nameproperty] data pointer is nullptr");
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
            params.lua.throw_error("[push_nameproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_nameproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_textproperty(const PusherParams& params) -> void
    {
        Unreal::FText* text = static_cast<Unreal::FText*>(params.data);
        if (!text)
        {
            params.lua.throw_error("[push_textproperty] data pointer is nullptr");
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
            params.lua.throw_error("[push_textproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_textproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_strproperty(const PusherParams& params) -> void
    {
        Unreal::FString* string = static_cast<Unreal::FString*>(params.data);
        if (!string)
        {
            params.lua.throw_error("[push_strproperty] data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::FString::construct(params.lua, string);
            return;
        case Operation::Set: {
            if (params.lua.is_string())
            {
                auto lua_string = params.lua.get_string();
                auto fstring = Unreal::FString{to_wstring(lua_string).c_str()};
                *string = fstring;
            }
            else if (params.lua.is_userdata())
            {
                auto& rhs = params.lua.get_userdata<LuaType::FString>();
                string->SetCharArray(rhs.get_local_cpp_object().GetCharTArray());
            }
            else
            {
                params.lua.throw_error("[push_strproperty] StrProperty can only be set to a string or FString");
            }
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.lua.throw_error("[push_strproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_strproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_softclassproperty(const PusherParams& params) -> void
    {
        auto soft_ptr = static_cast<Unreal::FSoftObjectPtr*>(params.data);
        if (!soft_ptr)
        {
            params.lua.throw_error("[push_softclassproperty] data pointer is nullptr");
        }

        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            LuaType::TSoftClassPtr::construct(params.lua, *soft_ptr);
            return;
        case Operation::Set: {
            auto& lua_object = params.lua.get_userdata<LuaType::TSoftClassPtr>(params.stored_at_index);
            *soft_ptr = lua_object.get_local_cpp_object();
            return;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            return;
        default:
            params.lua.throw_error("[push_softclassproperty] Unhandled Operation");
            break;
        }

        params.lua.throw_error(fmt::format("[push_softclassproperty] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }

    auto push_interfaceproperty(const PusherParams& params) -> void
    {
        auto property_value = static_cast<Unreal::UInterface**>(params.data);
        if (!property_value)
        {
            params.lua.throw_error("[push_interfaceproperty] data pointer is nullptr");
        }

        // Finally construct the Lua object
        switch (params.operation)
        {
        case Operation::GetNonTrivialLocal:
        case Operation::Get:
            auto_construct_object(params.lua, *property_value);
            break;
        case Operation::Set: {
            if (params.lua.is_userdata())
            {
                const auto& lua_object = params.lua.get_userdata<LuaType::UInterface>(params.stored_at_index);
                *property_value = lua_object.get_remote_cpp_object();
            }
            else if (params.lua.is_nil())
            {
                *property_value = nullptr;
            }
            else
            {
                params.lua.throw_error("[push_interfaceproperty] Value must be UInterface or nil");
            }
            break;
        }
        case Operation::GetParam:
            RemoteUnrealParam::construct(params.lua, params.data, params.base, params.property);
            break;
        default:
            params.lua.throw_error("[push_interfaceproperty] Unhandled Operation");
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
            auto* object_class = Unreal::UObjectGlobals::StaticFindObject<Unreal::UClass*>(nullptr, nullptr, to_wstring(lua.get_string()));
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

    auto RemoteUnrealParam::construct(const LuaMadeSimple::Lua& lua, void* param_ptr, Unreal::UObject* base, Unreal::FProperty* property)
            -> const LuaMadeSimple::Lua::Table
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
        table.add_pair("get", [](const LuaMadeSimple::Lua& lua) -> int {
            prepare_to_handle(Operation::Get, lua);
            return 1;
        });

        table.add_pair("set", [](const LuaMadeSimple::Lua& lua) -> int {
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
