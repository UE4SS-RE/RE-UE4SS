#include <DynamicOutput/DynamicOutput.hpp>
#include <SDKGenerator/Common.hpp>
#pragma warning(disable : 4005)
#include <Unreal/AActor.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FDelegateProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FFieldPathProperty.hpp>
#include <Unreal/Property/FInterfaceProperty.hpp>
#include <Unreal/Property/FLazyObjectProperty.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <Unreal/Property/FMulticastInlineDelegateProperty.hpp>
#include <Unreal/Property/FMulticastSparseDelegateProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FSetProperty.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>
#include <Unreal/Property/FSoftObjectProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FWeakObjectProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <UnrealDef.hpp>
#pragma warning(default : 4005)

#define DELEGATE_SIGNATURE_POSTFIX STR("__DelegateSignature")

namespace RC::UEGenerator
{
    using namespace Unreal;

    auto get_native_class_name(UClass* uclass, bool interface_name) -> File::StringType
    {
        File::StringType result_string;

        if (interface_name)
        {
            result_string.append(STR("I"));
        }
        else if (uclass->IsChildOf<AActor>())
        {
            result_string.append(STR("A"));
        }
        else
        {
            result_string.append(STR("U"));
        }
        if ((uclass->GetClassFlags() & Unreal::CLASS_Deprecated) != 0)
        {
            result_string.append(STR("DEPRECATED_"));
        }

        result_string.append(uclass->GetName());
        return result_string;
    }

    auto is_integral_type(FProperty* property) -> bool
    {
        if (property->IsA<FNumericProperty>())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    auto get_native_enum_name(UEnum* uenum, bool include_type) -> File::StringType
    {
        std::wstring result_string;

        // Seems to be not needed, because enum objects, unlike classes or structs, retain their normal E prefix
        // ResultString.append(STR("E"));
        result_string.append(uenum->GetName());

        // Namespaced enums need to have ::Type appended for the type
        if (uenum->GetCppForm() == UEnum::ECppForm::Namespaced && include_type)
        {
            result_string.append(STR("::Type"));
        }
        return result_string;
    }

    auto get_native_struct_name(UScriptStruct* script_struct) -> File::StringType
    {
        std::wstring result_string;

        result_string.append(STR("F"));
        result_string.append(script_struct->GetName());

        return result_string;
    }

    auto sanitize_property_name(const File::StringType& property_name) -> File::StringType
    {
        std::wstring resulting_name = property_name;

        // Remove heading underscore, used by private variables in some games
        if (resulting_name.length() >= 2 && resulting_name[0] == '_')
        {
            resulting_name.erase(0, 1);
        }
        // Remove heading m if it is followed by uppercase letter, used by variables in some games
        if (resulting_name.length() >= 2 && resulting_name[0] == 'm' && towupper(resulting_name[1]) == resulting_name[1])
        {
            resulting_name.erase(0, 1);
        }

        // Make sure first character is uppercase
        if (resulting_name.length() >= 1)
        {
            resulting_name[0] = towupper(resulting_name[0]);
        }
        return resulting_name;
    }

    auto generate_delegate_name(FProperty* property, const File::StringType& context_name) -> File::StringType
    {
        const std::wstring property_name = sanitize_property_name(property->GetName());
        return fmt::format(STR("F{}{}"), context_name, property_name);
    }

    auto generate_property_cxx_name(FProperty* property, bool is_top_level_declaration, UObject* class_context, EnableForwardDeclarations enable_forward_declarations)
            -> File::StringType
    {
        const std::wstring field_class_name = property->GetClass().GetName();

        // Byte Property
        if (property->IsA<FByteProperty>())
        {
            FByteProperty* byte_property = static_cast<FByteProperty*>(property);
            UEnum* enum_value = byte_property->GetEnum();

            if (enum_value != NULL)
            {
                // Non-EnumClass enumerations should be wrapped into TEnumAsByte according to UHT
                const std::wstring enum_type_name = get_native_enum_name(enum_value);
                return fmt::format(STR("TEnumAsByte<{}>"), enum_type_name);
            }
            return STR("uint8");
        }

        // Enum Property
        if (property->IsA<FEnumProperty>())
        {
            FEnumProperty* enum_property = static_cast<FEnumProperty*>(property);
            UEnum* uenum = enum_property->GetEnum();

            if (uenum == NULL)
            {
                throw std::runtime_error(RC::fmt("EnumProperty %S does not have a valid Enum value", property->GetName().c_str()));
            }

            const std::wstring enum_type_name = get_native_enum_name(uenum);
            return enum_type_name;
        }

        // Bool Property
        if (property->IsA<FBoolProperty>())
        {
            FBoolProperty* bool_property = static_cast<FBoolProperty*>(property);
            if (is_top_level_declaration && bool_property->GetFieldMask() != 255)
            {
                return STR("uint8");
            }
            return STR("bool");
        }

        // Standard Numeric Properties
        if (property->IsA<FInt8Property>())
        {
            return STR("int8");
        }
        else if (property->IsA<FInt16Property>())
        {
            return STR("int16");
        }
        else if (property->IsA<FIntProperty>())
        {
            return STR("int32");
        }
        else if (property->IsA<FInt64Property>())
        {
            return STR("int64");
        }
        else if (property->IsA<FUInt16Property>())
        {
            return STR("uint16");
        }
        else if (property->IsA<FUInt32Property>())
        {
            return STR("uint32");
        }
        else if (property->IsA<FUInt64Property>())
        {
            return STR("uint64");
        }
        else if (property->IsA<FFloatProperty>())
        {
            return STR("float");
        }
        else if (property->IsA<FDoubleProperty>())
        {
            return STR("double");
        }

        // Class Properties
        if (property->IsA<FClassProperty>() || property->IsA<FAssetClassProperty>())
        {
            FClassProperty* class_property = static_cast<FClassProperty*>(property);
            UClass* meta_class = class_property->GetMetaClass();

            if (meta_class == NULL || meta_class == UObject::StaticClass())
            {
                return STR("UClass*");
            }

            File::StringType meta_class_name{};
            if (enable_forward_declarations == EnableForwardDeclarations::Yes)
            {
                meta_class_name = STR("class ");
            }
            meta_class_name.append(get_native_class_name(meta_class, false));
            return fmt::format(STR("TSubclassOf<{}>"), meta_class_name);
        }

        if (auto* class_property = CastField<FClassPtrProperty>(property); class_property)
        {
            // TODO: Confirm that this is accurate
            return STR("TObjectPtr<UClass>");
        }

        if (property->IsA<FSoftClassProperty>())
        {
            FSoftClassProperty* soft_class_property = static_cast<FSoftClassProperty*>(property);
            UClass* meta_class = soft_class_property->GetMetaClass();

            if (meta_class == NULL)
            {
                return STR("TSoftClassPtr<UClass>");
            }
            else if (meta_class == UObject::StaticClass())
            {
                return STR("TSoftClassPtr<UObject>");
            }
            
            const std::wstring meta_class_name = get_native_class_name(meta_class, false);
            return fmt::format(STR("TSoftClassPtr<{}>"), meta_class_name);
        }

        // Object Properties
        //  TODO: Verify that the syntax for 'AssetObjectProperty' is the same as for 'ObjectProperty'.
        //        If it's not, then add another branch here after you figure out what the syntax should be.
        if (property->IsA<FObjectProperty>() || property->IsA<FAssetObjectProperty>())
        {
            FObjectProperty* object_property = static_cast<FObjectProperty*>(property);
            UClass* property_class = object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return STR("UObject*");
            }

            const std::wstring property_class_name = get_native_class_name(property_class, false);
            return fmt::format(STR("{}*"), property_class_name);
        }

        if (auto* object_property = CastField<FObjectPtrProperty>(property); object_property)
        {
            auto* property_class = object_property->GetPropertyClass();

            if (!property_class)
            {
                return STR("TObjectPtr<UObject>");
            }
            else
            {
                const auto property_class_name = get_native_class_name(property_class, false);
                return fmt::format(STR("TObjectPtr<{}>"), property_class_name);
            }
        }

        if (property->IsA<FWeakObjectProperty>())
        {
            FWeakObjectProperty* weak_object_property = static_cast<FWeakObjectProperty*>(property);
            UClass* property_class = weak_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return STR("TWeakObjectPtr<UObject>");
            }

            File::StringType property_class_name{};
            if (enable_forward_declarations == EnableForwardDeclarations::Yes)
            {
                property_class_name = fmt::format(STR("class "));
            }
            property_class_name.append(get_native_class_name(property_class, false));
            return fmt::format(STR("TWeakObjectPtr<{}>"), property_class_name);
        }

        if (property->IsA<FLazyObjectProperty>())
        {
            FLazyObjectProperty* lazy_object_property = static_cast<FLazyObjectProperty*>(property);
            UClass* property_class = lazy_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return STR("TLazyObjectPtr<UObject>");
            }

            File::StringType property_class_name{};
            if (enable_forward_declarations == EnableForwardDeclarations::Yes)
            {
                property_class_name = STR("class ");
            }
            property_class_name.append(get_native_class_name(property_class, false));
            return fmt::format(STR("TLazyObjectPtr<{}>"), property_class_name);
        }

        if (property->IsA<FSoftObjectProperty>())
        {
            FSoftObjectProperty* soft_object_property = static_cast<FSoftObjectProperty*>(property);
            UClass* property_class = soft_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return STR("TSoftObjectPtr<UObject>");
            }

            const std::wstring property_class_name = get_native_class_name(property_class, false);
            return fmt::format(STR("TSoftObjectPtr<{}>"), property_class_name);
        }

        // Interface Property
        if (property->IsA<FInterfaceProperty>())
        {
            FInterfaceProperty* interface_property = static_cast<FInterfaceProperty*>(property);
            UClass* interface_class = interface_property->GetInterfaceClass();

            if (interface_class == NULL || interface_class == UInterface::StaticClass())
            {
                return STR("FScriptInterface");
            }

            File::StringType interface_class_name{};
            if (enable_forward_declarations == EnableForwardDeclarations::Yes)
            {
                interface_class_name = STR("class ");
            }
            interface_class_name.append(get_native_class_name(interface_class, true));
            return fmt::format(STR("TScriptInterface<{}>"), interface_class_name);
        }

        // Struct Property
        if (property->IsA<FStructProperty>())
        {
            FStructProperty* struct_property = static_cast<FStructProperty*>(property);
            UScriptStruct* script_struct = struct_property->GetStruct();

            if (script_struct == NULL)
            {
                throw std::runtime_error(RC::fmt("Struct is NULL for StructProperty %S", property->GetName().c_str()));
            }

            const std::wstring native_struct_name = get_native_struct_name(script_struct);
            return native_struct_name;
        }

        // Delegate Properties
        if (property->IsA<FDelegateProperty>())
        {
            FDelegateProperty* delegate_property = static_cast<FDelegateProperty*>(property);

            const std::wstring delegate_type_name = generate_delegate_name(delegate_property, class_context->GetName());
            return delegate_type_name;
        }

        // In 4.23, they replaced 'MulticastDelegateProperty' with 'Inline' & 'Sparse' variants
        // It looks like the delegate macro might be the same as the 'Inline' variant in later versions, so we'll use the same branch here
        if (property->IsA<FMulticastInlineDelegateProperty>() || property->IsA<FMulticastDelegateProperty>())
        {
            FMulticastInlineDelegateProperty* delegate_property = static_cast<FMulticastInlineDelegateProperty*>(property);

            const std::wstring delegate_type_name = generate_delegate_name(delegate_property, class_context->GetName());
            return delegate_type_name;
        }

        if (property->IsA<FMulticastSparseDelegateProperty>())
        {
            FMulticastSparseDelegateProperty* delegate_property = static_cast<FMulticastSparseDelegateProperty*>(property);

            const std::wstring delegate_type_name = generate_delegate_name(delegate_property, class_context->GetName());
            return delegate_type_name;
        }

        // Field path property
        if (property->IsA<FFieldPathProperty>())
        {
            FFieldPathProperty* field_path_property = static_cast<FFieldPathProperty*>(property);
            const std::wstring property_class_name = field_path_property->GetPropertyClass()->GetName();
            return fmt::format(STR("TFieldPath<F{}>"), property_class_name);
        }

        // Collection and Map Properties
        //  TODO: This is missing support for freeze image array properties because XArrayProperty is incomplete. (low priority)
        if (property->IsA<FArrayProperty>())
        {
            FArrayProperty* array_property = static_cast<FArrayProperty*>(property);
            FProperty* inner_property = array_property->GetInner();

            File::StringType inner_property_type{};
            if (enable_forward_declarations == EnableForwardDeclarations::Yes && !is_integral_type(inner_property))
            {
                if (inner_property->IsA<FObjectProperty>())
                {
                    inner_property_type = STR("class ");
                }
            }
            inner_property_type.append(generate_property_cxx_name(inner_property, is_top_level_declaration, class_context));
            return fmt::format(STR("TArray<{}>"), inner_property_type);
        }

        if (property->IsA<FSetProperty>())
        {
            FSetProperty* set_property = static_cast<FSetProperty*>(property);
            FProperty* element_prop = set_property->GetElementProp();

            const std::wstring element_property_type = generate_property_cxx_name(element_prop, is_top_level_declaration, class_context);
            return fmt::format(STR("TSet<{}>"), element_property_type);
        }

        // TODO: This is missing support for freeze image map properties because XMapProperty is incomplete. (low priority)
        if (property->IsA<FMapProperty>())
        {
            FMapProperty* map_property = static_cast<FMapProperty*>(property);
            FProperty* key_property = map_property->GetKeyProp();
            FProperty* value_property = map_property->GetValueProp();

            File::StringType key_type{};
            File::StringType value_type{};
            if (enable_forward_declarations == EnableForwardDeclarations::Yes && !is_integral_type(key_property) && !is_integral_type(value_property))
            {
                if (!key_property->IsA<FClassPtrProperty>())
                {
                    key_type = STR("class ");
                }

                if (!value_property->IsA<FClassPtrProperty>())
                {
                    value_type = STR("class ");
                }
            }
            key_type.append(generate_property_cxx_name(key_property, is_top_level_declaration, class_context));
            value_type.append(generate_property_cxx_name(value_property, is_top_level_declaration, class_context));

            return fmt::format(STR("TMap<{}, {}>"), key_type, value_type);
        }

        // Standard properties that do not have any special attributes
        if (property->IsA<FNameProperty>())
        {
            return STR("FName");
        }
        else if (property->IsA<FStrProperty>())
        {
            return STR("FString");
        }
        else if (property->IsA<FTextProperty>())
        {
            return STR("FText");
        }
        throw std::runtime_error(RC::fmt("Unsupported property class %S", field_class_name.c_str()));
    }

    auto generate_property_lua_name(FProperty* property, bool is_top_level_declaration, UObject* class_context) -> File::StringType
    {
        const std::wstring field_class_name = property->GetClass().GetName();

        // Byte Property
        if (field_class_name == STR("ByteProperty"))
        {
            FByteProperty* byte_property = static_cast<FByteProperty*>(property);
            UEnum* enum_value = byte_property->GetEnum();

            if (enum_value != NULL)
            {
                // Non-EnumClass enumerations should be wrapped into TEnumAsByte according to UHT
                const std::wstring enum_type_name = get_native_enum_name(enum_value);
                return fmt::format(STR("{}"), enum_type_name);
            }
            return STR("uint8");
        }

        // Enum Property
        if (field_class_name == STR("EnumProperty"))
        {
            FEnumProperty* enum_property = static_cast<FEnumProperty*>(property);
            UEnum* uenum = enum_property->GetEnum();

            if (uenum == NULL)
            {
                throw std::runtime_error(RC::fmt("EnumProperty %S does not have a valid Enum value", property->GetName().c_str()));
            }

            const std::wstring enum_type_name = get_native_enum_name(uenum);
            return enum_type_name;
        }

        // Bool Property
        if (field_class_name == STR("BoolProperty"))
        {
            return STR("boolean");
        }

        // Standard Numeric Properties
        if (field_class_name == STR("Int8Property"))
        {
            return STR("int8");
        }
        else if (field_class_name == STR("Int16Property"))
        {
            return STR("int16");
        }
        else if (field_class_name == STR("IntProperty"))
        {
            return STR("int32");
        }
        else if (field_class_name == STR("Int64Property"))
        {
            return STR("int64");
        }
        else if (field_class_name == STR("UInt16Property"))
        {
            return STR("uint16");
        }
        else if (field_class_name == STR("UInt32Property"))
        {
            return STR("uint32");
        }
        else if (field_class_name == STR("UInt64Property"))
        {
            return STR("uint64");
        }
        else if (field_class_name == STR("FloatProperty"))
        {
            return STR("float");
        }
        else if (field_class_name == STR("DoubleProperty"))
        {
            return STR("double");
        }

        // Object Properties
        //  TODO: Verify that the syntax for 'AssetObjectProperty' is the same as for 'ObjectProperty'.
        //        If it's not, then add another branch here after you figure out what the syntax should be.
        if (field_class_name == STR("ObjectProperty") || field_class_name == STR("AssetObjectProperty"))
        {
            FObjectProperty* object_property = static_cast<FObjectProperty*>(property);
            UClass* property_class = object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return STR("UObject");
            }

            const std::wstring property_class_name = get_native_class_name(property_class, false);
            return fmt::format(STR("{}"), property_class_name);
        }

        if (auto* object_property = CastField<FObjectPtrProperty>(property); object_property)
        {
            auto* property_class = object_property->GetPropertyClass();

            if (!property_class)
            {
                return STR("TObjectPtr<UObject>");
            }
            else
            {
                const auto property_class_name = get_native_class_name(property_class, false);
                return fmt::format(STR("TObjectPtr<{}>"), property_class_name);
            }
        }

        if (field_class_name == STR("WeakObjectProperty"))
        {
            FWeakObjectProperty* weak_object_property = static_cast<FWeakObjectProperty*>(property);
            UClass* property_class = weak_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return STR("TWeakObjectPtr<UObject>");
            }

            File::StringType property_class_name{};
            property_class_name.append(get_native_class_name(property_class, false));
            return fmt::format(STR("TWeakObjectPtr<{}>"), property_class_name);
        }

        if (field_class_name == STR("LazyObjectProperty"))
        {
            FLazyObjectProperty* lazy_object_property = static_cast<FLazyObjectProperty*>(property);
            UClass* property_class = lazy_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return STR("TLazyObjectPtr<UObject>");
            }

            File::StringType property_class_name{};
            property_class_name.append(get_native_class_name(property_class, false));
            return fmt::format(STR("TLazyObjectPtr<{}>"), property_class_name);
        }

        if (field_class_name == STR("SoftObjectProperty"))
        {
            FSoftObjectProperty* soft_object_property = static_cast<FSoftObjectProperty*>(property);
            UClass* property_class = soft_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return STR("TSoftObjectPtr<UObject>");
            }

            const std::wstring property_class_name = get_native_class_name(property_class, false);
            return fmt::format(STR("TSoftObjectPtr<{}>"), property_class_name);
        }

        // Class Properties
        if (field_class_name == STR("ClassProperty") || field_class_name == STR("AssetClassProperty"))
        {
            FClassProperty* class_property = static_cast<FClassProperty*>(property);
            UClass* meta_class = class_property->GetMetaClass();

            if (meta_class == NULL || meta_class == UObject::StaticClass())
            {
                return STR("UClass");
            }

            File::StringType meta_class_name{};
            meta_class_name.append(get_native_class_name(meta_class, false));
            return fmt::format(STR("TSubclassOf<{}>"), meta_class_name);
        }

        if (auto* class_property = CastField<FClassPtrProperty>(property); class_property)
        {
            // TODO: Confirm that this is accurate
            return STR("TObjectPtr<UClass>");
        }

        if (field_class_name == STR("SoftClassProperty"))
        {
            FSoftClassProperty* soft_class_property = static_cast<FSoftClassProperty*>(property);
            UClass* meta_class = soft_class_property->GetMetaClass();

            if (meta_class == NULL || meta_class == UObject::StaticClass())
            {
                return STR("TSoftClassPtr<UObject>");
            }

            const std::wstring meta_class_name = get_native_class_name(meta_class, false);
            return fmt::format(STR("TSoftClassPtr<{}>"), meta_class_name);
        }

        // Interface Property
        if (field_class_name == STR("InterfaceProperty"))
        {
            FInterfaceProperty* interface_property = static_cast<FInterfaceProperty*>(property);
            UClass* interface_class = interface_property->GetInterfaceClass();

            if (interface_class == NULL || interface_class == UInterface::StaticClass())
            {
                return STR("FScriptInterface");
            }

            File::StringType interface_class_name{};
            interface_class_name.append(get_native_class_name(interface_class, true));
            return fmt::format(STR("TScriptInterface<{}>"), interface_class_name);
        }

        // Struct Property
        if (field_class_name == STR("StructProperty"))
        {
            FStructProperty* struct_property = static_cast<FStructProperty*>(property);
            UScriptStruct* script_struct = struct_property->GetStruct();

            if (script_struct == NULL)
            {
                throw std::runtime_error(RC::fmt("Struct is NULL for StructProperty %S", property->GetName().c_str()));
            }

            const std::wstring native_struct_name = get_native_struct_name(script_struct);
            return native_struct_name;
        }

        // Delegate Properties
        if (field_class_name == STR("DelegateProperty"))
        {
            FDelegateProperty* delegate_property = static_cast<FDelegateProperty*>(property);

            const std::wstring delegate_type_name = generate_delegate_name(delegate_property, class_context->GetName());
            return delegate_type_name;
        }

        // In 4.23, they replaced 'MulticastDelegateProperty' with 'Inline' & 'Sparse' variants
        // It looks like the delegate macro might be the same as the 'Inline' variant in later versions, so we'll use the same branch here
        if (field_class_name == STR("MulticastInlineDelegateProperty") || field_class_name == STR("MulticastDelegateProperty"))
        {
            FMulticastInlineDelegateProperty* delegate_property = static_cast<FMulticastInlineDelegateProperty*>(property);

            const std::wstring delegate_type_name = generate_delegate_name(delegate_property, class_context->GetName());
            return delegate_type_name;
        }

        if (field_class_name == STR("MulticastSparseDelegateProperty"))
        {
            FMulticastSparseDelegateProperty* delegate_property = static_cast<FMulticastSparseDelegateProperty*>(property);

            const std::wstring delegate_type_name = generate_delegate_name(delegate_property, class_context->GetName());
            return delegate_type_name;
        }

        // Field path property
        if (field_class_name == STR("FieldPathProperty"))
        {
            FFieldPathProperty* field_path_property = static_cast<FFieldPathProperty*>(property);
            const std::wstring property_class_name = field_path_property->GetPropertyClass()->GetName();
            return fmt::format(STR("TFieldPath<F{}>"), property_class_name);
        }

        // Collection and Map Properties
        //  TODO: This is missing support for freeze image array properties because XArrayProperty is incomplete. (low priority)
        if (field_class_name == STR("ArrayProperty"))
        {
            FArrayProperty* array_property = static_cast<FArrayProperty*>(property);
            FProperty* inner_property = array_property->GetInner();

            File::StringType inner_property_type{};
            inner_property_type.append(generate_property_lua_name(inner_property, is_top_level_declaration, class_context));
            return fmt::format(STR("TArray<{}>"), inner_property_type);
        }

        if (field_class_name == STR("SetProperty"))
        {
            FSetProperty* set_property = static_cast<FSetProperty*>(property);
            FProperty* element_prop = set_property->GetElementProp();

            const std::wstring element_property_type = generate_property_lua_name(element_prop, is_top_level_declaration, class_context);
            return fmt::format(STR("TSet<{}>"), element_property_type);
        }

        // TODO: This is missing support for freeze image map properties because XMapProperty is incomplete. (low priority)
        if (field_class_name == STR("MapProperty"))
        {
            FMapProperty* map_property = static_cast<FMapProperty*>(property);
            FProperty* key_property = map_property->GetKeyProp();
            FProperty* value_property = map_property->GetValueProp();

            File::StringType key_type{};
            File::StringType value_type{};
            key_type.append(generate_property_lua_name(key_property, is_top_level_declaration, class_context));
            value_type.append(generate_property_lua_name(value_property, is_top_level_declaration, class_context));

            return fmt::format(STR("TMap<{}, {}>"), key_type, value_type);
        }

        // Standard properties that do not have any special attributes
        if (field_class_name == STR("NameProperty"))
        {
            return STR("FName");
        }
        else if (field_class_name == STR("StrProperty"))
        {
            return STR("FString");
        }
        else if (field_class_name == STR("TextProperty"))
        {
            return STR("FText");
        }
        throw std::runtime_error(RC::fmt("Unsupported property class %S", field_class_name.c_str()));
    }

    auto get_native_delegate_type_name(Unreal::UFunction* signature_function, Unreal::UClass* current_class, bool strip_outer_name) -> File::StringType
    {
        if (!is_delegate_signature_function(signature_function))
        {
            throw std::runtime_error(RC::fmt("Function %S is not a delegate signature function", signature_function->GetName().c_str()));
        }

        // Delegate names always start with F and have __DelegateSignature postfix
        File::StringType delegate_type_name = strip_delegate_signature_postfix(signature_function);
        delegate_type_name.insert(0, STR("F"));

        // Return the delegate name without the outer name if we have been requested to strip it
        if (strip_outer_name)
        {
            return delegate_type_name;
        }

        UObject* delegate_outer = signature_function->GetOuterPrivate();

        // If delegate is declared inside the class, we need to retrieve the class name and use it as the outer type
        if (UClass* delegate_outer_class = Unreal::Cast<UClass>(delegate_outer))
        {

            // Skip the scope declaration if delegate is declared inside the current class
            if (delegate_outer_class != current_class)
            {
                // For interface, delegates are declared inside the interface definition
                bool is_class_interface = delegate_outer_class->IsChildOf<UInterface>();
                const File::StringType outer_class_name = get_native_class_name(delegate_outer_class, is_class_interface);

                delegate_type_name.insert(0, STR("::"));
                delegate_type_name.insert(0, outer_class_name);
            }
        }
        else if (!delegate_outer->IsA<Unreal::UPackage>())
        {
            // Delegate signature functions should never exist outside the UPackage or UClass
            throw std::runtime_error(RC::fmt("Delegate signature function %S does not have class or package as outer", delegate_outer->GetName().c_str()));
        }
        return delegate_type_name;
    }

    auto is_delegate_signature_function(Unreal::UFunction* function) -> bool
    {
        return (function->GetFunctionFlags() & Unreal::FUNC_Delegate) != 0 && function->GetName().ends_with(DELEGATE_SIGNATURE_POSTFIX);
    }

    auto strip_delegate_signature_postfix(Unreal::UFunction* signature_function) -> File::StringType
    {
        if (!is_delegate_signature_function(signature_function))
        {
            throw std::runtime_error(RC::fmt("Function %S is not a delegate signature function", signature_function->GetName().c_str()));
        }

        // Delegate names always start with F and have __DelegateSignature postfix
        const File::StringType delegate_signature_postfix = DELEGATE_SIGNATURE_POSTFIX;

        File::StringType delegate_name = signature_function->GetName();
        delegate_name.erase(delegate_name.length() - delegate_signature_postfix.length());
        return delegate_name;
    }
} // namespace RC::UEGenerator
