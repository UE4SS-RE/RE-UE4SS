#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaXDelegateProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    // ========================================
    // XDelegateProperty (single-cast delegate)
    // ========================================

    XDelegateProperty::XDelegateProperty(Unreal::FDelegateProperty* object) : RemoteObjectBase<Unreal::FDelegateProperty, FDelegatePropertyName>(object)
    {
    }

    auto XDelegateProperty::construct(const LuaMadeSimple::Lua& lua, Unreal::FDelegateProperty* unreal_object) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XDelegateProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FDelegateProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XDelegateProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XDelegateProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FDelegateProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XDelegateProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XDelegateProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
        {
            table.add_pair("type",
                           [](const LuaMadeSimple::Lua& lua) -> int {
                               lua.set_string(ClassName::ToString());
                               return 1;
                           });
        }
    }

    // ========================================
    // XMulticastDelegateProperty (multicast inline delegate)
    // ========================================

    XMulticastDelegateProperty::XMulticastDelegateProperty(Unreal::FMulticastDelegateProperty* object)
        : RemoteObjectBase<Unreal::FMulticastDelegateProperty, FMulticastDelegatePropertyName>(object)
    {
    }

    XMulticastDelegateProperty::XMulticastDelegateProperty(const PusherParams& params)
        : RemoteObjectBase<Unreal::FMulticastDelegateProperty, FMulticastDelegatePropertyName>(
                  static_cast<Unreal::FMulticastDelegateProperty*>(params.property)),
          m_base(params.base),
          m_property(static_cast<Unreal::FMulticastDelegateProperty*>(params.property))
    {
    }

    auto XMulticastDelegateProperty::construct(const PusherParams& params) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XMulticastDelegateProperty lua_object{params};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = params.lua.get_metatable(metatable_name);
        if (params.lua.is_nil(-1))
        {
            params.lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FMulticastDelegateProperty>::construct(params.lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            params.lua.new_metatable<LuaType::XMulticastDelegateProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        params.lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XMulticastDelegateProperty::construct(const LuaMadeSimple::Lua& lua,
                                               Unreal::FMulticastDelegateProperty* unreal_object)
        -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XMulticastDelegateProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FMulticastDelegateProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XMulticastDelegateProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XMulticastDelegateProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FMulticastDelegateProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XMulticastDelegateProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XMulticastDelegateProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        // Add - Add a delegate to the invocation list
        table.add_pair("Add",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           // Expecting: property:Add(targetObject, functionName)
                           // Parameters: 1=targetObject, 2=functionName (string or FName)
                           auto* target_object = lua.get_userdata<UObject>(1).get_remote_cpp_object();

                           Unreal::FName fname;
                           if (lua.is_string(1))
                           {
                               fname = Unreal::FName(to_wstring(lua.get_string(1)), Unreal::FNAME_Add);
                           }
                           else
                           {
                               fname = lua.get_userdata<FName>(1).get_local_cpp_object();
                           }

                           Unreal::FScriptDelegate script_delegate;
                           script_delegate.BindUFunction(target_object, fname);

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);

                           property->AddDelegate(script_delegate, container, property_value);

                           return 0;
                       });

        // Remove - Remove a delegate from the invocation list
        table.add_pair("Remove",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           // Expecting: property:Remove(targetObject, functionName)
                           // Parameters: 1=targetObject, 2=functionName (string or FName)
                           auto* target_object = lua.get_userdata<UObject>(1).get_remote_cpp_object();

                           Unreal::FName fname;
                           if (lua.is_string(1))
                           {
                               fname = Unreal::FName(to_wstring(lua.get_string(1)), Unreal::FNAME_Add);
                           }
                           else
                           {
                               fname = lua.get_userdata<FName>(1).get_local_cpp_object();
                           }

                           Unreal::FScriptDelegate script_delegate;
                           script_delegate.BindUFunction(target_object, fname);

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);

                           property->RemoveDelegate(script_delegate, container, property_value);

                           return 0;
                       });

        // Broadcast - Fire the delegate, calling all bound functions
        table.add_pair("Broadcast",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);
                           const Unreal::FMulticastScriptDelegate* delegate_value = property->GetMulticastDelegate(property_value);

                           if (!delegate_value)
                           {
                               return 0; // No bindings, nothing to broadcast
                           }

                           Unreal::TObjectPtr<Unreal::UFunction>& signature_function = property->GetSignatureFunction();
                           if (!signature_function)
                           {
                               lua.throw_error("Delegate signature function not found");
                           }

                           size_t params_size = signature_function->GetStructureSize();
                           std::vector<uint8_t> params_buffer(params_size);

                           signature_function->InitializeStruct(params_buffer.data());

                           // Process parameters from Lua stack (parameters start at position 1 after self is consumed)
                           int lua_param_index = 1;
                           for (Unreal::FProperty* param : Unreal::TFieldRange<Unreal::FProperty>(signature_function, Unreal::EFieldIterationFlags::IncludeDeprecated))
                           {
                               if (!param->HasAnyPropertyFlags(Unreal::CPF_Parm) || param->HasAnyPropertyFlags(Unreal::CPF_ReturnParm))
                               {
                                   continue;
                               }

                               int32_t offset = param->GetOffset_Internal();
                               void* param_data = &params_buffer[offset];

                               int32_t name_comparison_index = param->GetClass().GetFName().GetComparisonIndex().ToUnstableInt();

                               if (StaticState::m_property_value_pushers.contains(name_comparison_index))
                               {
                                   const PusherParams pusher_params{
                                           .operation = Operation::Set,
                                           .lua = lua,
                                           .base = static_cast<Unreal::UObject*>(static_cast<void*>(params_buffer.data())),
                                           .data = param_data,
                                           .property = param
                                   };
                                   StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                               }

                               lua_param_index++;
                           }

                           delegate_value->ProcessMulticastDelegate<Unreal::UObject>(params_buffer.data());

                           signature_function->DestroyStruct(params_buffer.data());

                           return 0;
                       });

        // GetBindings - Get array of all delegate bindings
        table.add_pair("GetBindings",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);
                           const Unreal::FMulticastScriptDelegate* delegate_value = property->GetMulticastDelegate(property_value);

                           if (!delegate_value)
                           {
                               lua.set_nil();
                               return 1;
                           }

                           LuaMadeSimple::Lua::Table lua_table = lua.prepare_new_table();

                           int32_t count = delegate_value->Num();
                           for (int32_t i = 0; i < count; ++i)
                           {
                               lua_table.add_key(i + 1); // Lua is 1-indexed

                               LuaMadeSimple::Lua::Table delegate_entry = lua.prepare_new_table();

                               delegate_entry.add_key("Object");
                               auto_construct_object(lua, delegate_value->InvocationList[i].GetUObject());
                               delegate_entry.fuse_pair();

                               delegate_entry.add_key("FunctionName");
                               FName::construct(lua, delegate_value->InvocationList[i].GetFunctionName());
                               delegate_entry.fuse_pair();

                               delegate_entry.make_local();
                               lua_table.fuse_pair(); // Set array element
                           }

                           lua_table.make_local();
                           return 1;
                       });

        // Clear - Remove all delegates from the invocation list
        table.add_pair("Clear",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           // Expecting: property:Clear()
                           // Stack: 1=self

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);

                           property->ClearDelegate(container, property_value);

                           return 0;
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

    // ========================================
    // XMulticastSparseDelegateProperty (multicast sparse delegate)
    // ========================================

    XMulticastSparseDelegateProperty::XMulticastSparseDelegateProperty(Unreal::FMulticastSparseDelegateProperty* object)
        : RemoteObjectBase<Unreal::FMulticastSparseDelegateProperty, FMulticastSparseDelegatePropertyName>(object)
    {
    }

    XMulticastSparseDelegateProperty::XMulticastSparseDelegateProperty(const PusherParams& params)
        : RemoteObjectBase<Unreal::FMulticastSparseDelegateProperty, FMulticastSparseDelegatePropertyName>(
                  static_cast<Unreal::FMulticastSparseDelegateProperty*>(params.property)),
          m_base(params.base),
          m_property(static_cast<Unreal::FMulticastSparseDelegateProperty*>(params.property))
    {
    }

    auto XMulticastSparseDelegateProperty::construct(const PusherParams& params) -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XMulticastSparseDelegateProperty lua_object{params};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = params.lua.get_metatable(metatable_name);
        if (params.lua.is_nil(-1))
        {
            params.lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FMulticastSparseDelegateProperty>::construct(params.lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            params.lua.new_metatable<LuaType::XMulticastSparseDelegateProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        params.lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XMulticastSparseDelegateProperty::construct(const LuaMadeSimple::Lua& lua,
                                                     Unreal::FMulticastSparseDelegateProperty* unreal_object)
        -> const LuaMadeSimple::Lua::Table
    {
        LuaType::XMulticastSparseDelegateProperty lua_object{unreal_object};

        auto metatable_name = ClassName::ToString();

        LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
        if (lua.is_nil(-1))
        {
            lua.discard_value(-1);
            LuaMadeSimple::Type::RemoteObject<Unreal::FMulticastSparseDelegateProperty>::construct(lua, lua_object);
            setup_metamethods(lua_object);
            setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
            lua.new_metatable<LuaType::XMulticastSparseDelegateProperty>(metatable_name, lua_object.get_metamethods());
        }

        // Create object & surrender ownership to Lua
        lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

        return table;
    }

    auto XMulticastSparseDelegateProperty::construct(const LuaMadeSimple::Lua& lua, BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
    {
        LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<Unreal::FMulticastSparseDelegateProperty>::construct(lua, construct_to);

        setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        setup_metamethods(construct_to);

        return table;
    }

    auto XMulticastSparseDelegateProperty::setup_metamethods([[maybe_unused]] BaseObject& base_object) -> void
    {
    }

    template <LuaMadeSimple::Type::IsFinal is_final>
    auto XMulticastSparseDelegateProperty::setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
    {
        Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

        // Add - Add a delegate to the invocation list
        table.add_pair("Add",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastSparseDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           // Expecting: property:Add(targetObject, functionName)
                           // Parameters: 1=targetObject, 2=functionName (string or FName)
                           auto* target_object = lua.get_userdata<UObject>(1).get_remote_cpp_object();

                           Unreal::FName fname;
                           if (lua.is_string(1))
                           {
                               fname = Unreal::FName(to_wstring(lua.get_string(1)), Unreal::FNAME_Add);
                           }
                           else
                           {
                               fname = lua.get_userdata<FName>(1).get_local_cpp_object();
                           }

                           Unreal::FScriptDelegate script_delegate;
                           script_delegate.BindUFunction(target_object, fname);

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);

                           property->AddDelegate(script_delegate, container, property_value);

                           return 0;
                       });

        // Remove - Remove a delegate from the invocation list
        table.add_pair("Remove",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastSparseDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           // Expecting: property:Remove(targetObject, functionName)
                           // Parameters: 1=targetObject, 2=functionName (string or FName)
                           auto* target_object = lua.get_userdata<UObject>(1).get_remote_cpp_object();

                           Unreal::FName fname;
                           if (lua.is_string(1))
                           {
                               fname = Unreal::FName(to_wstring(lua.get_string(1)), Unreal::FNAME_Add);
                           }
                           else
                           {
                               fname = lua.get_userdata<FName>(1).get_local_cpp_object();
                           }

                           Unreal::FScriptDelegate script_delegate;
                           script_delegate.BindUFunction(target_object, fname);

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);

                           property->RemoveDelegate(script_delegate, container, property_value);

                           return 0;
                       });

        // Broadcast - Fire the delegate, calling all bound functions
        table.add_pair("Broadcast",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastSparseDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);
                           const Unreal::FMulticastScriptDelegate* delegate_value = property->GetMulticastDelegate(property_value);

                           if (!delegate_value)
                           {
                               return 0; // No bindings, nothing to broadcast
                           }

                           Unreal::TObjectPtr<Unreal::UFunction>& signature_function = property->GetSignatureFunction();
                           if (!signature_function)
                           {
                               lua.throw_error("Delegate signature function not found");
                           }

                           size_t params_size = signature_function->GetStructureSize();
                           std::vector<uint8_t> params_buffer(params_size);

                           signature_function->InitializeStruct(params_buffer.data());

                           // Process parameters from Lua stack (parameters start at position 1 after self is consumed)
                           int lua_param_index = 1;
                           for (Unreal::FProperty* param : Unreal::TFieldRange<Unreal::FProperty>(signature_function, Unreal::EFieldIterationFlags::IncludeDeprecated))
                           {
                               if (!param->HasAnyPropertyFlags(Unreal::CPF_Parm) || param->HasAnyPropertyFlags(Unreal::CPF_ReturnParm))
                               {
                                   continue;
                               }

                               int32_t offset = param->GetOffset_Internal();
                               void* param_data = &params_buffer[offset];

                               int32_t name_comparison_index = param->GetClass().GetFName().GetComparisonIndex().ToUnstableInt();

                               if (StaticState::m_property_value_pushers.contains(name_comparison_index))
                               {
                                   const PusherParams pusher_params{
                                           .operation = Operation::Set,
                                           .lua = lua,
                                           .base = static_cast<Unreal::UObject*>(static_cast<void*>(params_buffer.data())),
                                           .data = param_data,
                                           .property = param
                                   };
                                   StaticState::m_property_value_pushers[name_comparison_index](pusher_params);
                               }

                               lua_param_index++;
                           }

                           delegate_value->ProcessMulticastDelegate<Unreal::UObject>(params_buffer.data());

                           signature_function->DestroyStruct(params_buffer.data());

                           return 0;
                       });

        // GetBindings - Get array of all delegate bindings
        table.add_pair("GetBindings",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastSparseDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);
                           const Unreal::FMulticastScriptDelegate* delegate_value = property->GetMulticastDelegate(property_value);

                           if (!delegate_value)
                           {
                               lua.set_nil();
                               return 1;
                           }

                           LuaMadeSimple::Lua::Table lua_table = lua.prepare_new_table();

                           int32_t count = delegate_value->Num();
                           for (int32_t i = 0; i < count; ++i)
                           {
                               lua_table.add_key(i + 1); // Lua is 1-indexed

                               LuaMadeSimple::Lua::Table delegate_entry = lua.prepare_new_table();

                               delegate_entry.add_key("Object");
                               auto_construct_object(lua, delegate_value->InvocationList[i].GetUObject());
                               delegate_entry.fuse_pair();

                               delegate_entry.add_key("FunctionName");
                               FName::construct(lua, delegate_value->InvocationList[i].GetFunctionName());
                               delegate_entry.fuse_pair();

                               delegate_entry.make_local();
                               lua_table.fuse_pair(); // Set array element
                           }

                           lua_table.make_local();
                           return 1;
                       });

        // Clear - Remove all delegates from the invocation list
        table.add_pair("Clear",
                       [](const LuaMadeSimple::Lua& lua) -> int {
                           const auto& lua_object = lua.get_userdata<XMulticastSparseDelegateProperty>();
                           auto* property = lua_object.m_property;
                           auto* container = lua_object.m_base;

                           // Expecting: property:Clear()
                           // Stack: 1=self

                           void* property_value = property->ContainerPtrToValuePtr<void>(container);

                           property->ClearDelegate(container, property_value);

                           return 0;
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
} // namespace RC::LuaType