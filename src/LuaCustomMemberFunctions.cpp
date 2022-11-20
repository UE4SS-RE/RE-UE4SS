#include <LuaCustomMemberFunctions.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <ExceptionHandling.hpp>
#include <LuaBindings/States/MainState/Main.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>

#define EMPLACE_INTO_UNREAL_PROPERTY_MAP(PropertyTypeName) \
auto PropertyTypeName##_name = FName(STR(#PropertyTypeName)); \
if (PropertyTypeName##_name == NAME_None) \
{ \
    throw std::runtime_error{"FName for '"#PropertyTypeName"' was NAME_None"}; \
} \
g_unreal_property_to_lua.emplace(PropertyTypeName##_name, &PropertyTypeName##_to_lua); \
g_unreal_property_from_lua.emplace(PropertyTypeName##_name, &PropertyTypeName##_from_lua);

namespace RC::UnrealRuntimeTypes
{
    using namespace ::RC::Unreal;

    std::unordered_map<FName, void (*)(lua_State*, ToLuaParams)> g_unreal_property_to_lua{};
    std::unordered_map<FName, void (*)(lua_State*, FromLuaParams)> g_unreal_property_from_lua{};

    auto populate_unreal_property_to_lua_map() -> void
    {
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(ObjectProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(ClassProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(Int8Property)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(Int16Property)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(IntProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(Int64Property)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(ByteProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(UInt16Property)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(UInt32Property)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(UInt64Property)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(StructProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(ArrayProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(FloatProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(DoubleProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(BoolProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(EnumProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(WeakObjectProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(NameProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(TextProperty)
        EMPLACE_INTO_UNREAL_PROPERTY_MAP(StrProperty)
    }

    auto ObjectProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_UObjectMetatable", ::RC::Unreal::UObject>(lua_state, params.base_no_processing_required ? params.base : *params.property->ContainerPtrToValuePtr<UObject*>(params.base), params.pointer_depth);
    }
    auto ClassProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_UClassMetatable", ::RC::Unreal::UClass>(lua_state, *params.property->ContainerPtrToValuePtr<UClass*>(params.base));
    }
    auto Int8Property_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushinteger(lua_state, *params.property->ContainerPtrToValuePtr<int8_t>(params.base));
    }
    auto Int16Property_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushinteger(lua_state, *params.property->ContainerPtrToValuePtr<int16_t>(params.base));
    }
    auto IntProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushinteger(lua_state, *params.property->ContainerPtrToValuePtr<int32_t>(params.base));
    }
    auto Int64Property_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushinteger(lua_state, *params.property->ContainerPtrToValuePtr<int64_t>(params.base));
    }
    auto ByteProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        if (params.should_containerize)
        {
            LuaBindings::lua_uint8_t_to_lua_from_heap(lua_state, params.property->ContainerPtrToValuePtr<uint8_t>(params.base), params.pointer_depth);
        }
        else
        {
            lua_pushinteger(lua_state, *params.property->ContainerPtrToValuePtr<uint8_t>(params.base));
        }
    }
    auto UInt16Property_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushinteger(lua_state, *params.property->ContainerPtrToValuePtr<uint16_t>(params.base));
    }
    auto UInt32Property_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushinteger(lua_state, *params.property->ContainerPtrToValuePtr<uint32_t>(params.base));
    }
    auto UInt64Property_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushinteger(lua_state, *params.property->ContainerPtrToValuePtr<uint64_t>(params.base));
    }
    auto StructProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        auto struct_property = static_cast<FStructProperty*>(params.property);
        void* script_struct_ptr{};
        if (params.should_copy_script_struct_contents)
        {
            auto script_struct = struct_property->GetStruct();
            auto struct_ops = script_struct->GetCppStructOps();
            script_struct_ptr = FMemory::Malloc(struct_ops->GetSize(), struct_ops->GetAlignment());
            script_struct->CopyScriptStruct(script_struct_ptr, struct_property->ContainerPtrToValuePtr<void*>(params.base));
        }
        else
        {
            script_struct_ptr = params.base;
        }
        LuaUScriptStruct lua_script_struct{
            /*.base = */static_cast<UObject*>(script_struct_ptr),
            /*.struct_property = */struct_property,
            /*.script_struct = */struct_property->GetStruct(),
            /*.is_user_constructed = */false,
        };
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__UnrealRuntimeTypes_LuaUScriptStructMetatable", LuaUScriptStruct, false, true>(lua_state, lua_script_struct, 0);
    }
    auto ArrayProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        auto property = static_cast<FArrayProperty*>(params.property);
        ArrayTest array_test{
                params.base_no_processing_required ? static_cast<FScriptArray*>(params.base) : property->ContainerPtrToValuePtr<FScriptArray>(params.base),
                static_cast<size_t>(property->GetElementSize()),
                static_cast<size_t>(property->GetMinAlignment()),
                "",
                property->GetInner()->IsA<FObjectProperty>(),
                property,
        };
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__UnrealRuntimeTypes_ArrayTestMetatable", ::RC::UnrealRuntimeTypes::ArrayTest, false, true>(lua_state, array_test, 0);
    }
    auto FloatProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushnumber(lua_state, *params.property->ContainerPtrToValuePtr<float>(params.base));
    }
    auto DoubleProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        lua_pushnumber(lua_state, *params.property->ContainerPtrToValuePtr<double>(params.base));
    }
    auto BoolProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        auto property = static_cast<FBoolProperty*>(params.property);
        lua_pushboolean(lua_state, property->GetPropertyValueInContainer(params.base));
    }
    auto EnumProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        auto property = static_cast<FEnumProperty*>(params.property);
        auto the_enum = property->GetEnum();

        lua_newtable(lua_state);

        the_enum->ForEachName([&](FName key, uint8 value) {
            lua_pushstring(lua_state, to_string(key.ToString()).c_str());
            lua_pushinteger(lua_state, value);
            lua_rawset(lua_state, -3);
            return LoopAction::Continue;
        });

        auto enum_lua_name_back_compat = std::format("Enum_{}", to_string(property->GetName()));
        auto enum_lua_name = std::format("Enum_{}", to_string(the_enum->GetName()));

        lua_pushvalue(lua_state, -1);
        lua_setglobal(lua_state, enum_lua_name_back_compat.c_str());
        lua_setglobal(lua_state, enum_lua_name.c_str());

        lua_pushnumber(lua_state, *property->ContainerPtrToValuePtr<uint8_t>(params.base));
    }
    auto WeakObjectProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_FWeakObjectPtrMetatable", ::RC::Unreal::FWeakObjectPtr>(lua_state, params.property->ContainerPtrToValuePtr<FWeakObjectPtr>(params.base));
    }
    auto NameProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_FNameMetatable", ::RC::Unreal::FName>(lua_state, params.property->ContainerPtrToValuePtr<FName>(params.base));
    }
    auto TextProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_FTextMetatable", ::RC::Unreal::FText>(lua_state, params.property->ContainerPtrToValuePtr<FText>(params.base));
    }
    auto StrProperty_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_FStringMetatable", ::RC::Unreal::FString>(lua_state, params.property->ContainerPtrToValuePtr<FString>(params.base));
    }
    auto function_to_lua(lua_State* lua_state, ToLuaParams params) -> void
    {
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_UFunctionMetatable", ::RC::Unreal::UFunction>(lua_state, params.function);
    }

    auto ObjectProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<UObject**>(params.base) : params.property->ContainerPtrToValuePtr<UObject*>(params.base);
        auto new_value = LuaBindings::lua_util_userdata_Get<"__RC__Unreal_UObjectMetatable", ::RC::Unreal::UObject*, LuaBindings::convertible_to___RC__Unreal_UObject>(lua_state, params.lua_stack_index);
        *property_value_ptr = new_value;
    }
    auto ClassProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<UClass**>(params.base) : params.property->ContainerPtrToValuePtr<UClass*>(params.base);
        auto new_value = LuaBindings::lua_util_userdata_Get<"__RC__Unreal_UClassMetatable", ::RC::Unreal::UClass*, LuaBindings::convertible_to___RC__Unreal_UClass>(lua_state, params.lua_stack_index);
        *property_value_ptr = new_value;
    }
    auto Int8Property_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<int8_t*>(params.base) : params.property->ContainerPtrToValuePtr<int8_t>(params.base);
        auto new_value = static_cast<int8_t>(lua_tointeger(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto Int16Property_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<int16_t*>(params.base) : params.property->ContainerPtrToValuePtr<int16_t>(params.base);
        auto new_value = static_cast<int16_t>(lua_tointeger(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto IntProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<int32_t*>(params.base) : params.property->ContainerPtrToValuePtr<int32_t>(params.base);
        if (lua_isuserdata(lua_state, 1))
        {
            *params.out_data = lua_touserdata(lua_state, 1);
            *property_value_ptr = *static_cast<int32_t*>(*params.out_data);
        }
        else
        {
            *property_value_ptr = static_cast<int32_t>(lua_tointeger(lua_state, 1));
        }
    }
    auto Int64Property_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<int64_t*>(params.base) : params.property->ContainerPtrToValuePtr<int64_t>(params.base);
        auto new_value = static_cast<int64_t>(lua_tointeger(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto ByteProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<uint8_t*>(params.base) : params.property->ContainerPtrToValuePtr<uint8_t>(params.base);
        auto new_value = static_cast<uint8_t>(lua_tointeger(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto UInt16Property_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<uint16_t*>(params.base) : params.property->ContainerPtrToValuePtr<uint16_t>(params.base);
        auto new_value = static_cast<uint16_t>(lua_tointeger(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto UInt32Property_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<uint32_t*>(params.base) : params.property->ContainerPtrToValuePtr<uint32_t>(params.base);
        auto new_value = static_cast<uint32_t>(lua_tointeger(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto UInt64Property_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<uint64_t*>(params.base) : params.property->ContainerPtrToValuePtr<uint64_t>(params.base);
        auto new_value = static_cast<uint64_t>(lua_tointeger(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto StructProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        if (!params.should_copy_script_struct_contents && !params.out_data)
        {
            luaL_error(lua_state, "A ScriptStruct cannot be modified, please specify a property of this ScriptStruct to modify.");
        }

        auto& script_struct_ref = LuaBindings::lua_util_userdata_Get<"__RC__UnrealRuntimeTypes_LuaUScriptStructMetatable", ::RC::UnrealRuntimeTypes::LuaUScriptStruct&, LuaBindings::convertible_to___RC__UnrealRuntimeTypes_LuaUScriptStruct>(lua_state, params.lua_stack_index);
        script_struct_ref.script_struct->CopyScriptStruct(params.property->ContainerPtrToValuePtr<void*>(params.base), script_struct_ref.base);
        if (params.out_data)
        {
            *params.out_data = script_struct_ref.base;
        }
    }
    auto ArrayProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void/* { luaL_error(lua_state, "newindex handler not yet implemented (ArrayProperty)"); }*/
    {
        auto array_property = static_cast<FArrayProperty*>(params.property);
        auto* script_array_ref = LuaBindings::lua_util_userdata_Get<"__RC__UnrealRuntimeTypes_ArrayTestMetatable", ::RC::UnrealRuntimeTypes::ArrayTest*, LuaBindings::convertible_to___RC__UnrealRuntimeTypes_ArrayTest>(lua_state, params.lua_stack_index);
        if (params.out_data)
        {
            // Destructively overrides whatever values these members used to hold.
            // They are supposed to be default constructed by Lua.
            // I'm not sure if this code could ever execute in some other circumstance.
            script_array_ref->Property = array_property;
            script_array_ref->ElementSize = array_property->GetElementSize();
            script_array_ref->ElementMinAlignment = array_property->GetMinAlignment();
            *params.out_data = script_array_ref->GetScriptArray();
        }
        else
        {
            Output::send(STR("ArrayProperty_from_lua not yet implemented for __newindex."));
        }

        params.property->ContainerPtrToValuePtr<int16_t>(params.base);
        script_array_ref->Property->CopyCompleteValue(params.property->ContainerPtrToValuePtr<void*>(params.base), script_array_ref->GetScriptArray());
    }
    auto FloatProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<float*>(params.base) : params.property->ContainerPtrToValuePtr<float>(params.base);
        auto new_value = static_cast<float>(lua_tonumber(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto DoubleProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<double*>(params.base) : params.property->ContainerPtrToValuePtr<double>(params.base);
        auto new_value = static_cast<double>(lua_tonumber(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto BoolProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        static_cast<FBoolProperty*>(params.property)->SetPropertyValueInContainer(params.base, lua_toboolean(lua_state, 1));
    }
    auto EnumProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<uint8_t*>(params.base) : params.property->ContainerPtrToValuePtr<uint8_t>(params.base);
        auto new_value = static_cast<uint8_t>(lua_tointeger(lua_state, 1));
        *property_value_ptr = new_value;
    }
    auto WeakObjectProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void { luaL_error(lua_state, "newindex handler not yet implemented (WeakObjectProperty)"); }
    auto NameProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<FName*>(params.base) : params.property->ContainerPtrToValuePtr<FName>(params.base);
        *property_value_ptr = LuaBindings::lua_util_userdata_Get<"__RC__Unreal_FNameMetatable", ::RC::Unreal::FName, LuaBindings::convertible_to___RC__Unreal_FName>(lua_state, params.lua_stack_index);
    }
    auto TextProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<FText*>(params.base) : params.property->ContainerPtrToValuePtr<FText>(params.base);
        *property_value_ptr = LuaBindings::lua_util_userdata_Get<"__RC__Unreal_FTextMetatable", ::RC::Unreal::FText, LuaBindings::convertible_to___RC__Unreal_FText>(lua_state, params.lua_stack_index);
    }
    auto StrProperty_from_lua(lua_State* lua_state, FromLuaParams params) -> void
    {
        auto property_value_ptr = params.base_no_processing_required ? static_cast<FString*>(params.base) : params.property->ContainerPtrToValuePtr<FString>(params.base);
        *property_value_ptr = LuaBindings::lua_util_userdata_Get<"__RC__Unreal_FStringMetatable", ::RC::Unreal::FString, LuaBindings::convertible_to___RC__Unreal_FString>(lua_state, params.lua_stack_index);
    }
}

namespace RC
{
    using namespace ::RC::Unreal;

    std::unordered_map<FName, UScriptStruct*> g_script_struct_cache_map{};

    auto setup_script_struct_cache_map() -> void
    {
        UObjectGlobals::ForEachUObject([](UObject* object, ...) {
            if (auto as_script_struct = Cast<UScriptStruct>(object); as_script_struct)
            {
                g_script_struct_cache_map.emplace(as_script_struct->GetNamePrivate(), as_script_struct);
            }
            return LoopAction::Continue;
        });
    }

    auto setup_global_metatable(lua_State* lua_state) -> void
    {
        lua_getglobal(lua_state, "_G");

        lua_newtable(lua_state);
        lua_pushliteral(lua_state, "__index");
        lua_pushcclosure(lua_state, [](lua_State* lua_state) -> int {
            // Stack
            // -2 (1): _G
            // -1 (2): string (key to access in _G, or lookup in the reflection system)

            if (!lua_isstring(lua_state, -1))
            {
                lua_pushnil(lua_state);
                return 1;
            }

            auto key = to_wstring(lua_tostring(lua_state, -1));
            if (lua_rawget(lua_state, -2) != LUA_TNIL)
            {
                return 1;
            }
            else
            {
                // Stack
                // -2 (1): _G
                // -1 (2): nil (from lua_rawget)
                lua_pop(lua_state, 1);

                if (auto it = g_script_struct_cache_map.find(FName(key)); it != g_script_struct_cache_map.end())
                {
                    lua_pushlightuserdata(lua_state, it->second);
                    lua_pushcclosure(lua_state, [](lua_State* lua_state) -> int {
                        // TODO: Use the virtual ICppStructOps::Construct when the wrapper has been implemented.
                        //       For now, we'll construct it and zero-fill the entire struct.
                        auto script_struct = static_cast<UScriptStruct*>(lua_touserdata(lua_state, lua_upvalueindex(1)));
                        auto struct_ops = script_struct->GetCppStructOps();
                        auto default_constructed_script_struct = FMemory::Malloc(struct_ops->GetSize(), struct_ops->GetAlignment());
                        script_struct->InitializeStruct(default_constructed_script_struct);
                        UnrealRuntimeTypes::LuaUScriptStruct lua_script_struct{
                                /*.base = */static_cast<UObject*>(default_constructed_script_struct),
                                /*.struct_property = */nullptr,
                                /*.script_struct = */script_struct,
                                /*.is_user_constructed = */true,
                        };
                        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__UnrealRuntimeTypes_LuaUScriptStructMetatable", UnrealRuntimeTypes::LuaUScriptStruct, false, true>(lua_state, lua_script_struct, 0);
                        return 1;
                    }, 1);
                    return 1;
                }
                else
                {
                    lua_pushnil(lua_state);
                    return 1;
                }
            }
        }, 0);
        lua_rawset(lua_state, -3);

        // Stack
        // -2 (1): _G
        // -1 (2): metatable (created above)
        lua_setmetatable(lua_state, -2);
        lua_pop(lua_state, 1);
    }

    auto UObjectBase_memberr_function_wrapper_MyTestFunc(lua_State* lua_state) -> int
    {
        auto [_, self] = LuaBindings::internal___RC__Unreal__UObjectBase_get_self(lua_state);

        printf_s("MyTestFunc says hello, 'this' == %S\n", static_cast<Unreal::UObject*>(self)->GetFullName().c_str());

        return 0;
    }

    auto UObjectBase_member_function_wrapper_GetNamePrivate(lua_State* lua_state) -> int
    {
        auto [_, self] = LuaBindings::internal___RC__Unreal__UObjectBase_get_self(lua_state);

        printf_s("GetNamePrivate redirected!\n");

        auto return_value = self->GetNamePrivate();
        auto* userdata = static_cast<::RC::Unreal::FName*>(lua_newuserdatauv(lua_state, sizeof(::RC::Unreal::FName), 1));
        lua_pushinteger(lua_state, 0);
        lua_setiuservalue(lua_state, -2, 1);
        new(userdata) ::RC::Unreal::FName{return_value};
        luaL_getmetatable(lua_state, "__RC__Unreal_FNameMetatable");
        lua_setmetatable(lua_state, -2);
        return 1;
    }

    auto UObjectBase_member_function_wrapper_Cast(lua_State* lua_state) -> int
    {
        try
        {
            auto [_, self] = LuaBindings::internal___RC__Unreal__UObject_get_self(lua_state);
            auto param_1 = LuaBindings::lua_util_userdata_Get<"__RC__Unreal_UClassMetatable", ::RC::Unreal::UClass*, LuaBindings::convertible_to___RC__Unreal_UClass>(lua_state, -1);

            auto construct_empty_object = [&]() {
                LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_UObjectMetatable", ::RC::Unreal::UObject>(lua_state, nullptr);
            };

            if (!param_1)
            {
                construct_empty_object();
                return 1;
            }

            lua_getmetatable(lua_state, 1);
            lua_pushliteral(lua_state, "__cxx_name");
            lua_rawget(lua_state, -2);
            auto cxx_name = std::string{lua_tostring(lua_state, -1)};
            lua_pop(lua_state, 2);

            if (auto type_handler = LuaBindings::lua_ue_type_name_to_lua_object_from_heap.find(to_string(param_1->GetName())); type_handler != LuaBindings::lua_ue_type_name_to_lua_object_from_heap.end())
            {
                type_handler->second(lua_state, self->IsA(param_1) ? self : nullptr, 1);
            }
            else if (auto type_handler = LuaBindings::lua_type_name_to_lua_object_from_heap.find(cxx_name); type_handler != LuaBindings::lua_type_name_to_lua_object_from_heap.end())
            {
                type_handler->second(lua_state, self->IsA(param_1) ? self : nullptr, 1);
            }
            else
            {
                construct_empty_object();
            }

            return 1;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }

//#define MAKE_LAMBDA(LambdaName, LambdaIdentifier, LambdaCallOperator) \
//class RC_LAMBDA_##LambdaIdentifier \
//{ \
//public: \
//    inline /*constexpr */ void operator()() const \
//    { \
//        LambdaCallOperator \
//    } \
//     \
//private: \
//    float * d; \
//    int c; \
//public: \
//    RC_LAMBDA_##LambdaIdentifier(float * _d, int & _c) : d{_d} , c{_c} {} \
//}; \
//auto LambdaName = RC_LAMBDA_##LambdaIdentifier{}; \





    auto FField_member_function_wrapper_CastField(lua_State* lua_state) -> int
    {
        //MAKE_LAMBDA(lambda, 0, {
        //    printf_s("fhfdhfdgh\n");
        //})

        try
        {
            auto [_, self] = LuaBindings::internal___RC__Unreal__FField_get_self(lua_state);
            auto& param_1 = LuaBindings::lua_util_userdata_Get<"__RC__Unreal_FFieldClassVariantMetatable", ::RC::Unreal::FFieldClassVariant&, LuaBindings::convertible_to___RC__Unreal_FFieldClassVariant>(lua_state, -1);

            lua_getmetatable(lua_state, 1);
            lua_pushliteral(lua_state, "__cxx_name");
            lua_rawget(lua_state, -2);
            auto cxx_name = std::string{lua_tostring(lua_state, -1)};
            lua_pop(lua_state, 2);

            if (auto type_handler = LuaBindings::lua_ue_type_name_to_lua_object_from_heap.find(to_string(param_1.GetName())); type_handler != LuaBindings::lua_ue_type_name_to_lua_object_from_heap.end())
            {
                type_handler->second(lua_state, self->IsA(param_1) ? self : nullptr, 1);
            }
            else if (auto type_handler = LuaBindings::lua_type_name_to_lua_object_from_heap.find(cxx_name); type_handler != LuaBindings::lua_type_name_to_lua_object_from_heap.end())
            {
                type_handler->second(lua_state, self->IsA(param_1) ? self : nullptr, 1);
            }
            else
            {
                LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_FFieldMetatable", ::RC::Unreal::FField>(lua_state, nullptr);
                printf_s("dgdsgfsdg");
            }

            return 1;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }

    auto UEnum_member_function_wrapper_ForEachName(lua_State* lua_state) -> int
    {
        try
        {
            // Prologue
            auto [_, self] = LuaBindings::internal___RC__Unreal__UEnum_get_self(lua_state);

            // Native call
            luaL_argcheck(lua_state, lua_isfunction(lua_state, 1), 2, "");
            auto param_inter_1 = luaL_ref(lua_state, LUA_REGISTRYINDEX);
            auto param_function_ref_1 = [=](FName lambda_param_1, int64_t lambda_param_2) -> RC::LoopAction {
                if (lua_rawgeti(lua_state, LUA_REGISTRYINDEX, param_inter_1) != LUA_TFUNCTION)
                {
                    luaL_error(lua_state, std::format("Expected 'function' got '{}'", lua_typename(lua_state, -1)).c_str());
                }

                {
                    LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_FNameMetatable", ::RC::Unreal::FName, false>(lua_state, lambda_param_1, 0);
                }

                {
                    lua_pushinteger(lua_state, lambda_param_2);
                }

                if (int status = lua_pcall(lua_state, 2, 1, 0); status != LUA_OK)
                {
                    throw std::runtime_error{std::format("lua_pcall returned {}", LuaBindings::resolve_status_message(lua_state, status))};
                }

                if (lua_isinteger(lua_state, -1))
                {
                    return static_cast<RC::LoopAction>(lua_tointeger(lua_state, -1));
                }
                else
                {
                    return RC::LoopAction{};
                }
            };
            auto param_1 = param_function_ref_1;

            self->ForEachName(param_1);

            return 0;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }

    //static std::function<void()> callable_container{};
    //struct A
    //{
//
    //};
//
    //auto Test(void(*TestCallable)()) -> void
    //{
    //    TestCallable();
    //}
//
    //auto Test2(lua_State* lua_state) -> int
    //{
    //    callable_container = [&]() {
//
    //    };
//
    //    Test(callable_container.operator bool());
    //}

    auto UClass_member_function_wrapper_StaticClass(lua_State* lua_state) -> int
    {
        printf_s("UClass::StaticClass redirected!\n");

        auto return_value = ::RC::Unreal::UClass::StaticClass();
        auto* userdata = static_cast<::RC::Unreal::UClass*>(lua_newuserdatauv(lua_state, sizeof(::RC::Unreal::UClass*), 1));
        lua_pushinteger(lua_state, 1);
        lua_setiuservalue(lua_state, -2, 1);
        new(userdata) ::RC::Unreal::UClass*{return_value};
        luaL_getmetatable(lua_state, "__RC__Unreal_UClassMetatable");
        lua_setmetatable(lua_state, -2);
        return 1;
    }

    auto UObjectBase_metamethod_wrapper_Index(lua_State* lua_state, void* untyped_self) -> int
    {
        // Type has already been confirmed but we can't pass it properly because of how we store pointers to these wrapper functions.
        auto typed_self = static_cast<Unreal::UObject*>(untyped_self);

        printf_s("UObjectBase __index metamethod overridden\n");

        // Remove the 'this' userdata from the stack.
        // This is done because we already have what it holds in 'typed_self' and we don't need to do further processing on the userdata.
        lua_remove(lua_state, 1);

        File::StringType property_name_final{};
        auto property_name_ansi = lua_tostring(lua_state, 1);
        if (lua_isstring(lua_state, 1))
        {
            property_name_final = to_wstring(property_name_ansi);
        }
        else
        {
            throw std::runtime_error{"Indexing into UObjectBase and derivatives by non-string is not allowed."};
        }

        auto* function = typed_self->GetFunctionByNameInChain(property_name_final.c_str());
        FProperty* property{};
        if (!function)
        {
            property = typed_self->GetPropertyByNameInChain(property_name_final.c_str());
        }

        if (!property && !function)
        {
            luaL_error(lua_state, std::format("No function or property by name '{}' found in UObject '{}'", property_name_ansi, to_string(typed_self->GetFullName())).c_str());
        }

        if (function)
        {
            UnrealRuntimeTypes::function_to_lua(lua_state, UnrealRuntimeTypes::ToLuaParams{
                    .base = typed_self,
                    .out_data = nullptr,
                    .property = nullptr,
                    .function = function,
                    .pointer_depth = 0,
                    .base_no_processing_required = false,
                    .should_copy_script_struct_contents = false,
                    .should_containerize = false,
            });
            return 1;
        }
        else if (property)
        {
            if (auto it = UnrealRuntimeTypes::g_unreal_property_to_lua.find(property->GetClass().GetFName()); it != UnrealRuntimeTypes::g_unreal_property_to_lua.end())
            {
                it->second(lua_state, UnrealRuntimeTypes::ToLuaParams{
                        .base = typed_self,
                        .out_data = nullptr,
                        .property = property,
                        .function = nullptr,
                        .pointer_depth = property->IsA<FArrayProperty>() && static_cast<FArrayProperty*>(property)->GetInner()->IsA<FObjectProperty>() ? 2u : 1u,
                        .base_no_processing_required = false,
                        .should_copy_script_struct_contents = false,
                        .should_containerize = false,
                });
                return 1;
            }
            else
            {
                luaL_error(lua_state, std::format("Property type '{}' not supported (for property: '{}' in '{}')", to_string(property->GetClass().GetName()), property_name_ansi, to_string(typed_self->GetFullName())).c_str());
            }
        }
        else
        {
            luaL_error(lua_state, std::format("No property or function found with name '{}' in '{}')", property_name_ansi, to_string(typed_self->GetFullName())).c_str());
        }

        return 0;
    }

    auto UObjectBase_metamethod_wrapper_NewIndex(lua_State* lua_state, void* untyped_self) -> int
    {
        // Type has already been confirmed but we can't pass it properly because of how we store pointers to these wrapper functions.
        auto typed_self = static_cast<Unreal::UObject*>(untyped_self);
        printf_s("UObjectBase __newindex metamethod overridden\n");
        lua_remove(lua_state, 1);

        File::StringType property_name_final{};
        auto property_name_ansi = lua_tostring(lua_state, 1);
        if (lua_isstring(lua_state, 1))
        {
            property_name_final = to_wstring(property_name_ansi);
        }
        else
        {
            throw std::runtime_error{"Indexing into UObjectBase and derivatives by non-string is not allowed."};
        }
        // Remove the property name string from the stack
        lua_remove(lua_state, 1);

        auto property = typed_self->GetPropertyByNameInChain(property_name_final.c_str());
        if (!property)
        {
            luaL_error(lua_state, std::format("No property by name '{}' found in UObject '{}'", property_name_ansi, to_string(typed_self->GetFullName())).c_str());
        }

        if (auto it = UnrealRuntimeTypes::g_unreal_property_from_lua.find(property->GetClass().GetFName()); it != UnrealRuntimeTypes::g_unreal_property_from_lua.end())
        {
            it->second(lua_state, UnrealRuntimeTypes::FromLuaParams{
                    .base = typed_self,
                    .out_data = nullptr,
                    .property = property,
                    .function = nullptr,
                    .pointer_depth = property->IsA<FArrayProperty>() && static_cast<FArrayProperty*>(property)->GetInner()->IsA<FObjectProperty>() ? 2u : 1u,
                    .base_no_processing_required = false,
                    .should_copy_script_struct_contents = false,
                    .should_containerize = false,
            });
            return 1;
        }
        else
        {
            luaL_error(lua_state, std::format("Property type '{}' not supported (for property: '{}' in '{}')", to_string(property->GetClass().GetName()), property_name_ansi, to_string(typed_self->GetFullName())).c_str());
        }

        return 0;
    }

    auto UFunction_metamethod_wrapper_Call(lua_State* lua_state, void* untyped_self) -> int
    {
        // Stack (variation #1):
        // Code example #1: Character:GetMovementComponent(3, 4, 5)
        // Code example #2: GetMovementComponent(Character, 3, 4, 5)
        // -5   1       userdata        00000232A2EBC618        UFunction                   Immediately popped from the stack
        // -4   2       userdata        00000232A2EBB498        UObject, this/context       Immediately popped from the stack
        // -3   3       number  3                               Param #1                    Only popped from the stack if the UFunction has parameters
        // -2   4       number  4                               Param #2                    Only popped from the stack if the UFunction has parameters
        // -1   5       number  5                               Param #3                    Only popped from the stack if the UFunction has parameters

        auto [_, function] = LuaBindings::internal___RC__Unreal__UFunction_get_self(lua_state);
        auto [__, object] = LuaBindings::internal___RC__Unreal__UObject_get_self(lua_state);

        auto num_expected_params = function->GetNumParms();
        auto return_value_offset = function->GetReturnValueOffset();
        bool has_return_value = return_value_offset != 0xFFFF;
        if (has_return_value && num_expected_params > 0) { --num_expected_params; }

        auto params_size = function->GetParmsSize();
        uint8_t* params_raw{};
        if (params_size > 0)
        {
            if (params_size > 1024) { luaL_error(lua_state, "Tried allocating too much data on the stack"); }
            params_raw = static_cast<uint8_t*>(_alloca(params_size));
            FMemory::Memzero(params_raw, params_size);
        }

        auto num_supplied_params = static_cast<uint8_t>(lua_gettop(lua_state));
        if (num_expected_params > num_supplied_params)
        {
            luaL_error(lua_state, to_string(std::format(STR("Not enough parameters supplied for call to '{}', expected '{}', got '{}'"), function->GetFullName(), num_expected_params, num_supplied_params)).c_str());
        }
        else if (num_supplied_params > num_expected_params)
        {
            Output::send<LogLevel::Verbose>(STR("Function '{}' called with superfluous parameters.\n"), function->GetFullName());
        }

        std::vector<std::pair<FProperty*, void*>> out_params{};

        FProperty* return_value_property{};
        bool has_out_params{};
        function->ForEachProperty([&](FProperty* child) {
            if (child->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                return_value_property = child;
                return LoopAction::Continue;
            }
            /*
            else if (child->HasAnyPropertyFlags(CPF_OutParm))
            {
                // TODO: Handle out params.
                //       Decide whether to break compatibility and only accept user-constructed data (i.e. UScriptStruct),
                //       or whether to still allow tables to be passed.
                //       If a table is passed, it can't simply be converted to UScriptData or anything else because I don't think we can change the value of the parameter and have the reflect in Lua.
                //       Not 100% sure though so some testing should be done.
                //       For now, table are not allowed.
                //       Currently, only UScriptStructs work.
                //       Anything else doesn't work.
                //       Need to come up with a way of handling other types.
                //       Do we need to support integers and other basic types as out params ?
                //       Based on what I've seen in Unreal Header Tool, any type can be an out param.
                //       This complicates things because now we need a handler for every type.
                //       A Lua script also needs to be able to construct something representing that type that can be passed back & fourth between Lua and C++ by reference.
                //       Maybe add a 'containerize' bool to the params of the '<Type>Property_from_lua' handlers.
                //       For example, if 'containerize' is true for '<Int8Property_from_lua', then instead of using 'lua_tointeger', it would use 'lua_int8_t_to_lua_from_heap' which is userdata 'int8_tMetaTable'.
                //       It's not a complicated type, it's just an int8_t* wrapped in userdata.
                //       It's possible that not all handlers need to do anything special.
            }
            //*/
            else
            {
                if (auto it = UnrealRuntimeTypes::g_unreal_property_from_lua.find(child->GetClass().GetFName()); it != UnrealRuntimeTypes::g_unreal_property_from_lua.end())
                {
                    auto is_out_param = child->HasAnyPropertyFlags(CPF_OutParm);
                    void* data{};
                    it->second(lua_state, UnrealRuntimeTypes::FromLuaParams{
                            .base = params_raw,
                            .out_data = is_out_param ? &data : nullptr,
                            .property = child,
                            .function = nullptr,
                            .pointer_depth = child->IsA<FArrayProperty>() && static_cast<FArrayProperty*>(child)->GetInner()->IsA<FObjectProperty>() ? 2u : 1u,
                            .base_no_processing_required = false,
                            .should_copy_script_struct_contents = true,
                            .should_containerize = is_out_param,
                            .lua_stack_index = 1,
                    });
                    if (is_out_param) { out_params.emplace_back(std::pair{child, data}); }
                    lua_remove(lua_state, 1);
                }
                else
                {
                    luaL_error(lua_state, std::format("Property type '{}' not supported (for parameter: '{}' for '{}')", to_string(child->GetClass().GetName()), to_string(child->GetName()), to_string(function->GetFullName())).c_str());
                }
            }

            return LoopAction::Continue;
        });

        object->ProcessEvent(function, params_raw);

        if (!out_params.empty())
        {
            for (const auto& out_param : out_params)
            {
                auto param = out_param.first;
                auto data_ptr = out_param.second;
                if (!data_ptr) { continue; }
                param->CopyCompleteValue(data_ptr, param->ContainerPtrToValuePtr<void*>(params_raw));
            }
        }

        if (return_value_property)
        {
            if (auto it = UnrealRuntimeTypes::g_unreal_property_to_lua.find(return_value_property->GetClass().GetFName()); it != UnrealRuntimeTypes::g_unreal_property_to_lua.end())
            {
                it->second(lua_state, UnrealRuntimeTypes::ToLuaParams{
                        .base = params_raw,
                        .out_data = nullptr,
                        .property = return_value_property,
                        .function = nullptr,
                        .pointer_depth = return_value_property->IsA<FArrayProperty>() && static_cast<FArrayProperty*>(return_value_property)->GetInner()->IsA<FObjectProperty>() ? 2u : 1u,
                        .base_no_processing_required = false,
                        .should_copy_script_struct_contents = true,
                        .should_containerize = false,
                });
                return 1;
            }
            else
            {
                luaL_error(lua_state, std::format("Property type '{}' not supported (for return property: '{}' for '{}')", to_string(return_value_property->GetClass().GetName()), to_string(return_value_property->GetName()), to_string(function->GetFullName())).c_str());
            }
        }

        Output::send<LogLevel::Error>(STR("UFunction handler not fully yet implemented\n"));
        return 0;
    }

    auto ArrayTest_metamethod_wrapper_GC(lua_State* lua_state, void* untyped_self) -> int
    {
        printf_s("ArrayTest GC\n");
        auto self = static_cast<UnrealRuntimeTypes::ArrayTest*>(untyped_self);
        self->~ArrayTest();
        return 0;
    }

    auto LuaUScriptStruct_metamethod_wrapper_Index(lua_State* lua_state, void* untyped_self) -> int
    {
        auto typed_self = static_cast<UnrealRuntimeTypes::LuaUScriptStruct*>(untyped_self);
        lua_remove(lua_state, 1);

        File::StringType property_name_final{};
        auto property_name_ansi = lua_tostring(lua_state, 1);
        if (lua_isstring(lua_state, 1))
        {
            property_name_final = to_wstring(property_name_ansi);
        }
        else
        {
            throw std::runtime_error{"Indexing into UScriptStruct and derivatives by non-string is not allowed."};
        }

        if (property_name_final == STR("type"))
        {
            lua_pushcfunction(lua_state, [](lua_State* lua_state) {
                lua_pushliteral(lua_state, "UScriptStruct");
                return 1;
            });
            return 1;
        }

        auto property = typed_self->script_struct->GetPropertyByNameInChain(property_name_final.c_str());
        if (!property)
        {
            luaL_error(lua_state, std::format("No property by name '{}' found in UScriptStruct '{}'", property_name_ansi, to_string(typed_self->script_struct->GetFullName())).c_str());
        }

        if (auto it = UnrealRuntimeTypes::g_unreal_property_to_lua.find(property->GetClass().GetFName()); it != UnrealRuntimeTypes::g_unreal_property_to_lua.end())
        {
            it->second(lua_state, UnrealRuntimeTypes::ToLuaParams{
                    .base = typed_self->struct_property ? typed_self->struct_property->ContainerPtrToValuePtr<UObject*>(typed_self->base) : std::bit_cast<UObject**>(typed_self->base),
                    .out_data = nullptr,
                    .property = property,
                    .function = nullptr,
                    .pointer_depth = property->IsA<FArrayProperty>() && static_cast<FArrayProperty*>(property)->GetInner()->IsA<FObjectProperty>() ? 2u : 1u,
                    .base_no_processing_required = false,
                    .should_copy_script_struct_contents = false,
                    .should_containerize = false,
            });
            return 1;
        }
        else
        {
            luaL_error(lua_state, std::format("Property type '{}' not supported (for property: '{}' in '{}')", to_string(property->GetClass().GetName()), property_name_ansi, to_string(typed_self->script_struct->GetFullName())).c_str());
        }

        return 0;
    }

    auto LuaUScriptStruct_metamethod_wrapper_NewIndex(lua_State* lua_state, void* untyped_self) -> int
    {
        auto typed_self = static_cast<UnrealRuntimeTypes::LuaUScriptStruct*>(untyped_self);
        lua_remove(lua_state, 1);

        File::StringType property_name_final{};
        auto property_name_ansi = lua_tostring(lua_state, 1);
        if (lua_isstring(lua_state, 1))
        {
            property_name_final = to_wstring(property_name_ansi);
        }
        else
        {
            throw std::runtime_error{"Indexing into UScriptStruct and derivatives by non-string is not allowed."};
        }
        lua_remove(lua_state, 1);

        auto property = typed_self->script_struct->GetPropertyByNameInChain(property_name_final.c_str());
        if (!property)
        {
            luaL_error(lua_state, std::format("No property by name '{}' found in UScriptStruct '{}'", property_name_ansi, to_string(typed_self->script_struct->GetFullName())).c_str());
        }

        if (auto it = UnrealRuntimeTypes::g_unreal_property_from_lua.find(property->GetClass().GetFName()); it != UnrealRuntimeTypes::g_unreal_property_from_lua.end())
        {
            it->second(lua_state, UnrealRuntimeTypes::FromLuaParams{
                    .base = typed_self->base,
                    .out_data = nullptr,
                    .property = property,
                    .function = nullptr,
                    .pointer_depth = property->IsA<FArrayProperty>() && static_cast<FArrayProperty*>(property)->GetInner()->IsA<FObjectProperty>() ? 2u : 1u,
                    .base_no_processing_required = false,
                    .should_copy_script_struct_contents = false,
                    .should_containerize = false,
            });
            return 1;
        }
        else
        {
            luaL_error(lua_state, std::format("Property type '{}' not supported (for property: '{}' in '{}')", to_string(property->GetClass().GetName()), property_name_ansi, to_string(typed_self->script_struct->GetFullName())).c_str());
        }

        return 0;
    }

    auto LuaUScriptStruct_metamethod_wrapper_GC(lua_State* lua_state, void* untyped_self) -> int
    {
        printf_s("LuaUScriptStruct GC\n");
        auto self = static_cast<UnrealRuntimeTypes::LuaUScriptStruct*>(untyped_self);
        self->~LuaUScriptStruct();
        return 0;
    }

    auto ArrayTest_member_function_wrapper_GetElementAtIndex(lua_State* lua_state) -> int
    {
        try
        {
            // Prologue
            auto [_, self] = LuaBindings::internal___RC__UnrealRuntimeTypes__ArrayTest_get_self(lua_state);

            // Native call
            luaL_argcheck(lua_state, lua_isnumber(lua_state, 1), 2, "");
            auto param_1 = static_cast<int32_t>(lua_tonumber(lua_state, 1));

            auto element = self->GetElementAtIndex(param_1);

            bool unhandled_type{};
            if (self->Property)
            {
                if (auto type_handler = UnrealRuntimeTypes::g_unreal_property_to_lua.find(self->Property->GetClass().GetFName()); type_handler != UnrealRuntimeTypes::g_unreal_property_to_lua.end())
                {
                    type_handler->second(lua_state, UnrealRuntimeTypes::ToLuaParams{
                            .base = self->GetScriptArray(),
                            .out_data = nullptr,
                            .property = self->Property,
                            .function = nullptr,
                            .pointer_depth = self->Property->GetInner()->IsA<FObjectProperty>() ? 2u : 1u,
                            .base_no_processing_required = false,
                            .should_copy_script_struct_contents = false,
                            .should_containerize = false,
                    });
                }
                else
                {
                    unhandled_type = true;
                }
            }
            else if (auto type_handler = LuaBindings::lua_type_name_to_lua_object_from_heap.find(self->TypeName); type_handler != LuaBindings::lua_type_name_to_lua_object_from_heap.end())
            {
                type_handler->second(lua_state, element, self->TypeIsAlwaysPointer ? 2 : 1);
            }
            else
            {
                unhandled_type = true;
            }

            if (unhandled_type)
            {
                throw std::runtime_error{std::format("Unhandled array type: '{}'", self->TypeName)};
            }

            return 1;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }

    auto ArrayTest_member_function_wrapper_ForEach(lua_State* lua_state) -> int
    {
        try
        {
            // Prologue
            auto [_, aself] = LuaBindings::internal___RC__UnrealRuntimeTypes__ArrayTest_get_self(lua_state);
            auto self = aself;

            // Native call
            luaL_argcheck(lua_state, lua_isfunction(lua_state, 1), 2, "");
            auto param_inter_1 = luaL_ref(lua_state, LUA_REGISTRYINDEX);
            auto param_function_ref_1 = [&](int32_t lambda_param_1, void* lambda_param_2) -> LoopAction {
                if (lua_rawgeti(lua_state, LUA_REGISTRYINDEX, param_inter_1) != LUA_TFUNCTION)
                {
                    luaL_error(lua_state, std::format("Expected 'function' got '{}'", lua_typename(lua_state, -1)).c_str());
                }

                lua_pushnumber(lua_state, static_cast<int32_t>(lambda_param_1));

                bool unhandled_type{};
                if (self->Property)
                {
                    if (auto type_handler = UnrealRuntimeTypes::g_unreal_property_to_lua.find(self->Property->GetInner()->GetClass().GetFName()); type_handler != UnrealRuntimeTypes::g_unreal_property_to_lua.end())
                    {
                        type_handler->second(lua_state, UnrealRuntimeTypes::ToLuaParams{
                                .base = lambda_param_2,
                                .out_data = nullptr,
                                .property = self->Property->GetInner(),
                                .function = nullptr,
                                .pointer_depth = self->Property->GetInner()->IsA<FObjectProperty>() ? 2u : 1u,
                                .base_no_processing_required = true,
                                .should_copy_script_struct_contents = false,
                                .should_containerize = true,
                        });
                    }
                    else
                    {
                        unhandled_type = true;
                    }
                }
                else if (auto type_handler = LuaBindings::lua_type_name_to_lua_object_from_heap.find(self->TypeName); type_handler != LuaBindings::lua_type_name_to_lua_object_from_heap.end())
                {
                    // lambda_param_2 == Element.
                    // Element == T*, meaning for it's UObject** for UObjects.
                    if (self->TypeIsAlwaysPointer)
                    {
                        //type_handler->second(lua_state, *static_cast<void**>(lambda_param_2), 1);
                        type_handler->second(lua_state, lambda_param_2, 2);
                    }
                    else
                    {
                        type_handler->second(lua_state, lambda_param_2, 1);
                    }
                }
                else
                {
                    throw std::runtime_error{std::format("Unhandled array type: '{}'", self->TypeName)};
                }

                if (int status = lua_pcall(lua_state, 2, 1, 0); status != LUA_OK)
                {
                    throw std::runtime_error{std::format("lua_pcall returned {}", LuaBindings::resolve_status_message(lua_state, status))};
                }

                if (lua_isboolean(lua_state, -1))
                {
                    return static_cast<LoopAction>(lua_tointeger(lua_state, -1));
                }
                else
                {
                    return LoopAction{};
                }
            };
            auto param_1 = LuaBindings::fnptr<LoopAction(int32_t, void*)>(param_function_ref_1);

            self->ForEach(param_1);
            return 0;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }

    auto lua_warn_wrapper(lua_State* lua_state) -> int
    {
        try
        {
            // Native call
            luaL_argcheck(lua_state, lua_isstring(lua_state, 1), 2, "");
            auto param_1 = std::string{lua_tostring(lua_state, 1)};

            luaL_traceback(lua_state, lua_state, param_1.c_str(), 1);
            Output::send<LogLevel::Warning>(STR("Warning: {}"), to_wstring(lua_tostring(lua_state, -1)));

            return 0;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }

    auto lua_print_wrapper(lua_State* lua_state) -> int
    {
        if (!lua_isstring(lua_state, 1))
        {
            luaL_error(lua_state, "The 'print' function requires the first parameter to be 'string'");
        }

        auto format = to_wstring(lua_tostring(lua_state, 1));
        lua_remove(lua_state, 1);

        // TODO: Pass 'format' to UEs OutputDevice

        // Emulate the original 'print' Lua function by adding a newline.
        format.append(STR("\n"));

        Output::send(format);

        return 0;
    }

    auto lua_FindAllOf_wrapper(lua_State* lua_state) -> int
    {
        //auto FindAllOf(FName SuperStruct, std::vector<UObject*>& ChunkIndex) -> void;
        //auto FindAllOf(const wchar_t* ClassName, std::vector<UObject*>& OutStorage) -> void;
        //auto FindAllOf(std::wstring_view ClassName, std::vector<UObject*>& OutStorage) -> void;
        //auto FindAllOf(const std::wstring& ClassName, std::vector<UObject*>& OutStorage) -> void;
        //auto FindAllOf(std::string_view ClassName, std::vector<UObject*>& OutStorage) -> void;
        //auto FindAllOf(const std::string& ClassName, std::vector<UObject*>& OutStorage) -> void;

        FName super_struct;
        //auto out_storage = static_cast<TArray<UObject*>*>(FMemory::Malloc(sizeof(TArray<UObject*>)));
        //new (out_storage) TArray<UObject*>{};
        auto out_storage = TArray<UObject*>{};
        printf_s("num: %i\n", out_storage.Num());
        printf_s("max: %i\n", out_storage.Max());

        bool param_1_is_fname{};
        int fname_pointer_depth{};
        bool fname_is_pointer{};
        if (lua_isuserdata(lua_state, 1))
        {
            lua_getiuservalue(lua_state, 1, 1);
            fname_pointer_depth = lua_tointeger(lua_state, -1);
            fname_is_pointer = fname_pointer_depth > 0;
            lua_getmetatable(lua_state, 1);
            lua_pushliteral(lua_state, "__name");
            lua_rawget(lua_state, -2);
            auto metatable_name = std::string{lua_tostring(lua_state, -1)};
            if (LuaBindings::convertible_to___RC__Unreal_FName.contains(metatable_name))
            {
                param_1_is_fname = true;
            }
            lua_pop(lua_state, 3);
        }

        if (param_1_is_fname)
        {
            if (fname_is_pointer)
            {
                auto param_1_outer_most = lua_touserdata(lua_state, 1);
                if (!param_1_outer_most)
                {
                    auto param_1_pointer = static_cast<FName*>(LuaBindings::deref(param_1_outer_most, fname_pointer_depth));
                    if (param_1_pointer)
                    {
                        super_struct = *param_1_pointer;
                    }
                }
            }
            else
            {
                super_struct = *static_cast<FName*>(lua_touserdata(lua_state, 1));
            }
        }
        else if (lua_isinteger(lua_state, 1))
        {
            if (lua_isinteger(lua_state, 2))
            {
                super_struct = FName(static_cast<uint32_t>(lua_tointeger(lua_state, 1)), static_cast<uint32_t>(lua_tointeger(lua_state, 2)));
            }
            else
            {
                super_struct = FName(lua_tointeger(lua_state, 1));
            }
        }
        else if (lua_isstring(lua_state, 1))
        {
            super_struct = FName(to_wstring(lua_tostring(lua_state, 1)));
        }
        else
        {
            luaL_error(lua_state, "Param 1 for FindAllOf must be either an FName or an integer.");
        }

        std::vector<UObject*> vector_of_objects{};
        UObjectGlobals::FindAllOf(super_struct, vector_of_objects);
        for (const auto& object : vector_of_objects)
        {
            out_storage.Add(object);
        }

        //auto array_wrapper = UnrealRuntimeTypes::ArrayTest{std::bit_cast<FScriptArray*>(out_storage), sizeof(UObject*), alignof(UObject*), "::RC::Unreal::UObject", true};
        auto array_wrapper = UnrealRuntimeTypes::ArrayTest{std::move(*std::bit_cast<FScriptArray*>(&out_storage)), sizeof(UObject*), alignof(UObject*), "::RC::Unreal::UObject", true};
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__UnrealRuntimeTypes_ArrayTestMetatable", ::RC::UnrealRuntimeTypes::ArrayTest, false, true>(lua_state, array_wrapper, 0);

        return 1;
    }

    auto lua_RegisterKeyBind_wrapper(lua_State* lua_state) -> int
    {
        try
        {
            if (!lua_isinteger(lua_state, 1))
            {
                luaL_error(lua_state, "RegisterKeyBind: First param must be integer (enum: Key).");
            }

            auto key = static_cast<Input::Key>(lua_tointeger(lua_state, 1));
            Input::Handler::ModifierKeyArray modifier_keys{};
            bool has_modifier_keys{};
            int function_ref{};

            if (lua_istable(lua_state, 2))
            {
                lua_pushnil(lua_state);
                for (lua_Integer i = 0; lua_next(lua_state, 2); ++i)
                {
                    if (!lua_isinteger(lua_state, -1))
                    {
                        luaL_error(lua_state, "RegisterKeyBind: Value in Keys table must be integer.");
                    }

                    modifier_keys[i] = static_cast<Input::ModifierKey>(lua_tointeger(lua_state, -1));
                    has_modifier_keys = true;
                    lua_pop(lua_state, 1);
                }

                if (!lua_isfunction(lua_state, 3))
                {
                    luaL_error(lua_state, "RegisterKeyBind: Third param must be function.");
                }

                function_ref = luaL_ref(lua_state, LUA_REGISTRYINDEX);
                Output::send(STR("setting function_ref to {}\n"), function_ref);
            }
            else if (lua_isfunction(lua_state, 2))
            {
                function_ref = luaL_ref(lua_state, LUA_REGISTRYINDEX);
                Output::send(STR("setting function_ref to {}\n"), function_ref);
            }

            auto lua_callable_wrapper = [=]() {
                TRY([&] {
                    Output::send(STR("calling function_ref {}\n"), function_ref);
                    if (lua_rawgeti(lua_state, LUA_REGISTRYINDEX, function_ref) != LUA_TFUNCTION)
                    {
                        throw std::runtime_error{"RegisterKeyBind: Tried calling keybind callback but the callback reference wasn't a function."};
                    }

                    if (int status = lua_pcall(lua_state, 0, 0, 0); status != LUA_OK)
                    {
                        throw std::runtime_error{std::format("RegisterKeyBind: lua_pcall returned {}", LuaBindings::resolve_status_message(lua_state, status)).c_str()};
                    }
                });
            };

            if (has_modifier_keys)
            {
                UE4SSProgram::get_program().register_keydown_event(key, modifier_keys, lua_callable_wrapper);
            }
            else
            {
                UE4SSProgram::get_program().register_keydown_event(key, lua_callable_wrapper);
            }

            return 0;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }
}

namespace RC::LuaBackCompat
{
    auto StaticFindObject(UClass* ObjectClass, UObject* InObjectPackage, const wchar_t* OrigInName, bool bExactClass) -> UObject*
    {
        return UObjectGlobals::StaticFindObject<UObject*>(ObjectClass, InObjectPackage, OrigInName, bExactClass);
    }

    auto StaticFindObject(const wchar_t* OrigInName) -> UObject*
    {
        return UObjectGlobals::StaticFindObject<UObject*>(nullptr, nullptr, OrigInName);
    }

    struct NotifyOnNewObjectCallback
    {
        std::function<void(UObject*)> callable{};
        UClass* class_filter{};

        NotifyOnNewObjectCallback(std::function<void(UObject*)> callable, UClass* class_filter) : callable(std::move(callable)), class_filter(class_filter) {}
    };
    static std::vector<NotifyOnNewObjectCallback> s_notify_on_new_object_callbacks{};

    static auto static_construct_object_hook_wrapper(const FStaticConstructObjectParameters& params, UObject* constructed_object) -> UObject*
    {
        try
        {
            auto object_class = constructed_object->GetClassPrivate();
            while (object_class)
            {
                for (const auto& callback_data : s_notify_on_new_object_callbacks)
                {
                    if (object_class != callback_data.class_filter) { continue; }
                    callback_data.callable(constructed_object);
                }

                object_class = object_class->GetSuperClass();
            }
        }
        catch (std::exception& e)
        {
            Output::send<LogLevel::Error>(STR("Error during StaticConstructHook wrapper: {}\n"), to_wstring(e.what()));
        }
        return constructed_object;
    }

    auto NotifyOnNewObject(const wchar_t* class_name, std::function<void(UObject*)>& callable) -> void
    {
        auto the_class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, class_name);
        if (!the_class) { throw std::runtime_error{"NotifyOnNewObject: First parameter must be string."}; }

        s_notify_on_new_object_callbacks.emplace_back(callable, the_class);

        static bool static_construct_object_already_hooked{};
        if (!static_construct_object_already_hooked) { Hook::RegisterStaticConstructObjectPostCallback(&static_construct_object_hook_wrapper); }
    }

    static auto set_is_in_game_thread(lua_State* lua_state, bool new_value)
    {
        lua_pushboolean(lua_state, new_value);
        lua_setfield(lua_state, LUA_REGISTRYINDEX, "IsInGameThread");
    }

    static auto is_in_game_thread(lua_State* lua_state) -> bool
    {
        lua_getfield(lua_state, LUA_REGISTRYINDEX, "IsInGameThread");
        return lua_toboolean(lua_state, -1);
    }

    struct RegisterHookCustomData
    {
        lua_State* lua_state{};
        const int lua_function_ref{};
        CallbackId pre_callback_id{};
        CallbackId post_callback_id{};
        bool has_return_value{};
        // Will be non-nullptr if the UFunction has a return value
        FProperty* return_property{};
    };
    static std::vector<std::unique_ptr<RegisterHookCustomData>> g_hooked_script_function_data{};

    static auto lua_RegisterHook_pre_hook(UnrealScriptFunctionCallableContext context, void *custom_data)
    {
        if (!custom_data)
        {
            Output::send<LogLevel::Error>(STR("Wrapper 'lua_RegisterHook_pre_hook' was called with nullptr for custom_data.\nCannot execute hook.\n"));
        }

        // Fetch the data corresponding to this hook
        auto& lua_data = *static_cast<RegisterHookCustomData*>(custom_data);

        // This is a promise that we're in the game thread, used by other functions to ensure that they don't execute when unsafe
        set_is_in_game_thread(lua_data.lua_state, true);

        // Use the stored registry index to put a Lua function on the Lua stack
        // This is the function that was provided by the Lua call to "RegisterHook"
        if (lua_rawgeti(lua_data.lua_state, LUA_REGISTRYINDEX, lua_data.lua_function_ref) != LUA_TFUNCTION)
        {
            lua_remove(lua_data.lua_state, -1);
            throw std::runtime_error("[lua_RegisterHook_pre_hook] Ref was not function");
        }

        // Set up the first param (context / this-ptr)
        // TODO: Check what happens if a static UFunction is hooked since they don't have any context
        LuaBindings::lua_Userdata_to_lua_from_heap<"__RC__Unreal_UObjectMetatable", ::RC::UObject>(lua_data.lua_state, context.Context);

        // Attempt at dynamically fetching the params
        uint16_t return_value_offset = context.TheStack.CurrentNativeFunction->GetReturnValueOffset();

        // 'ReturnValueOffset' is 0xFFFF if the UFunction return type is void
        lua_data.has_return_value = return_value_offset != 0xFFFF;

        uint8_t num_unreal_params = context.TheStack.CurrentNativeFunction->GetNumParms();
        if (lua_data.has_return_value)
        {
            // Subtract one from the number of params if there's a return value
            // This is because Unreal treats the return value as a param, and it's included in the 'NumParms' member variable
            --num_unreal_params;
        }

        bool has_properties_to_process = lua_data.has_return_value || num_unreal_params > 0;
        if (has_properties_to_process && context.TheStack.Locals)
        {
            int32_t current_param_offset{};

            context.TheStack.CurrentNativeFunction->ForEachProperty([&](FProperty* func_prop) {
                // Skip this property if it's not a parameter
                if (!func_prop->HasAnyPropertyFlags(EPropertyFlags::CPF_Parm))
                {
                    return LoopAction::Continue;
                }


                // Skip this property if it's the return value
                if (lua_data.has_return_value && func_prop->GetOffset_Internal() == return_value_offset)
                {
                    lua_data.return_property = func_prop;
                    return LoopAction::Continue;
                }

                if (auto it = UnrealRuntimeTypes::g_unreal_property_to_lua.find(func_prop->GetClass().GetFName()); it != UnrealRuntimeTypes::g_unreal_property_to_lua.end())
                {
                    it->second(lua_data.lua_state,
                               UnrealRuntimeTypes::ToLuaParams{
                                   .base = &context.TheStack.Locals[current_param_offset],
                                   .out_data = nullptr,
                                   .property = func_prop,
                                   .function = nullptr,
                                   .pointer_depth = 1,
                                   .base_no_processing_required = false,
                                   .should_copy_script_struct_contents = false,
                                   .should_containerize = true,
                               });
                }
                else
                {
                    throw std::runtime_error(to_string(std::format(STR("Property type '{}' not supported (for parameter: '{}' in '{}')"), func_prop->GetClass().GetName(), func_prop->GetName(), context.Context->GetFullName())).c_str());
                }

                return LoopAction::Continue;
            });

            // Call the Lua function with the correct number of parameters & return values
            // Increasing the 'num_params' by one to account for the 'this / context' param
            if (int status = lua_pcall(lua_data.lua_state, num_unreal_params + 1, 1, 0); status != LUA_OK)
            {
                throw std::runtime_error(std::format("[lua_RegisterHook_pre_hook] lua_pcall returned (error in callback) {}", LuaBindings::resolve_status_message(lua_data.lua_state, status)));
            }

            // This function continues in 'lua_unreal_script_function_hook_post' which executes immediately after the original function gets called

            // No longer promising to be in the game thread
            set_is_in_game_thread(lua_data.lua_state, false);
        }
    }

    static auto lua_RegisterHook_post_hook(UnrealScriptFunctionCallableContext context, void *custom_data)
    {
        if (!custom_data)
        {
            Output::send<LogLevel::Error>(STR("Wrapper 'lua_RegisterHook_post_hook' was called with nullptr for custom_data.\nCannot execute hook.\n"));
        }

        // Fetch the data corresponding to this hook
        auto& lua_data = *static_cast<RegisterHookCustomData*>(custom_data);

        // This is a promise that we're in the game thread, used by other functions to ensure that they don't execute when unsafe
        set_is_in_game_thread(lua_data.lua_state, true);

        // If 'nil' exists on the Lua stack, that means that the UFunction expected a return value but the Lua script didn't return anything
        // So we can simply clean the stack and let the UFunction decide the return value on its own
        if (lua_isnil(lua_data.lua_state, -1))
        {
            lua_remove(lua_data.lua_state, -1);
        }
        else if (lua_data.has_return_value && lua_data.return_property && context.RESULT_DECL)
        {
            if (auto it = UnrealRuntimeTypes::g_unreal_property_from_lua.find(lua_data.return_property->GetClass().GetFName()); it != UnrealRuntimeTypes::g_unreal_property_from_lua.end())
            {
                it->second(lua_data.lua_state,
                           UnrealRuntimeTypes::FromLuaParams{
                               .base = &context.RESULT_DECL,
                               .out_data = &context.RESULT_DECL,
                               .property = lua_data.return_property,
                               .function = nullptr,
                               .pointer_depth = 1,
                               .base_no_processing_required = false,
                               .should_copy_script_struct_contents = false,
                               .should_containerize = false,
                           });
            }
            else
            {
                throw std::runtime_error{to_string(std::format(STR("Property type '{}' not supported (for return value: '{}' in '{}')"),lua_data.return_property->GetClass().GetName(), lua_data.return_property->GetName(), context.TheStack.CurrentNativeFunction->GetFullName())).c_str()};
            }
        }

        // No longer promising to be in the game thread
        set_is_in_game_thread(lua_data.lua_state, false);
    }

    auto lua_RegisterHook_wrapper(lua_State* lua_state) -> int
    {
        try
        {
            if (!lua_isstring(lua_state, 1))
            {
                luaL_error(lua_state, "RegisterHook: First param must be string (full function name without type prefix).");
            }
            const auto function_name = to_wstring(lua_tostring(lua_state, 1));

            if (!lua_isfunction(lua_state, 2))
            {
                luaL_error(lua_state, "RegisterHook: Second param must be function.");
            }
            const auto lua_function_ref = luaL_ref(lua_state, LUA_REGISTRYINDEX);

            const auto unreal_function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, function_name.c_str());
            if (!unreal_function)
            {
                luaL_error(lua_state, to_string(std::format(STR("RegisterHook: Function '{}' not found."), function_name)).c_str());
            }

            auto& custom_data = g_hooked_script_function_data.emplace_back(std::make_unique<RegisterHookCustomData>(RegisterHookCustomData{
                .lua_state = lua_state,
                .lua_function_ref = lua_function_ref,
            }));

            const auto pre_callback_id = unreal_function->RegisterPreHook(&lua_RegisterHook_pre_hook, custom_data.get());
            const auto post_callback_id = unreal_function->RegisterPostHook(&lua_RegisterHook_post_hook, custom_data.get());

            custom_data->pre_callback_id = pre_callback_id;
            custom_data->post_callback_id = post_callback_id;

            return 0;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }

    auto lua_UObjectBase_IsA_wrapper(lua_State* lua_state) -> int
    {
        try
        {
            auto [_, self] = LuaBindings::internal___RC__Unreal__UObjectBase_get_self(lua_state);

            UClass* param_1{};
            if (lua_isstring(lua_state, 1))
            {
                param_1 = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, to_wstring(lua_tostring(lua_state, 1)));
            }
            else if (lua_isuserdata(lua_state, 1))
            {
                param_1 = LuaBindings::lua_util_userdata_Get<"__RC__Unreal_UClassMetatable", ::RC::Unreal::UClass*, LuaBindings::convertible_to___RC__Unreal_UClass>(lua_state, 1);
            }
            else
            {
                throw std::runtime_error{"UObjectBase::IsA: The first parameter must be either UClass or string."};
            }

            auto return_value = self->IsA(param_1);

            lua_pushboolean(lua_state, return_value);
            return 1;
        }
        catch (std::exception& e)
        {
            luaL_error(lua_state, e.what());
            return 0;
        }
    }
}

namespace RC::UnrealRuntimeTypes
{
    auto Array_Type_Handler_Ptr() -> void
    {

    }

    auto Array_Type_Handler_WChar_T() -> void
    {

    }

    Array::Array(size_t TypeSize, std::vector<std::string>& TypeNames) : TypeSize(TypeSize)
    {

    }
}

