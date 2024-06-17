#include <format>

#include <ObjectDumper/ObjectToString.hpp>
#pragma warning(disable : 4005)
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FDelegateProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FFieldPathProperty.hpp>
#include <Unreal/Property/FInterfaceProperty.hpp>
#include <Unreal/Property/FLazyObjectProperty.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <Unreal/Property/FMulticastDelegateProperty.hpp>
#include <Unreal/Property/FMulticastInlineDelegateProperty.hpp>
#include <Unreal/Property/FMulticastSparseDelegateProperty.hpp>
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/Property/FWeakObjectProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UScriptStruct.hpp>
#pragma warning(default : 4005)

namespace RC::ObjectDumper
{
    using namespace RC::Unreal;

    std::unordered_map<ToStringHash, ObjectToStringDecl> object_to_string_functions{};
    std::unordered_map<ToStringHash, ObjectToStringComplexDecl> object_to_string_complex_functions{};

    auto get_to_string(size_t hash) -> ObjectToStringDecl
    {
        return object_to_string_functions[hash];
    }

    auto get_to_string_complex(size_t hash) -> ObjectToStringComplexDecl
    {
        return object_to_string_complex_functions[hash];
    }

    auto to_string_exists(size_t hash) -> bool
    {
        return object_to_string_functions.contains(hash);
    }

    auto to_string_complex_exists(size_t hash) -> bool
    {
        return object_to_string_complex_functions.contains(hash);
    }

    auto object_trivial_dump_to_string(void* p_this, std::wstring& out_line, const wchar_t* post_delimiter) -> void
    {
        UObject* p_typed_this = static_cast<UObject*>(p_this);

        out_line.append(fmt::format(L"[{:016X}] ", reinterpret_cast<uintptr_t>(p_this)));
        out_line.append(p_typed_this->GetFullName());
        out_line.append(fmt::format(L" [n: {:X}] [c: {:016X}] [or: {:016X}]",
                                    p_typed_this->GetNamePrivate().GetComparisonIndex(),
                                    reinterpret_cast<uintptr_t>(p_typed_this->GetClassPrivate()),
                                    reinterpret_cast<uintptr_t>(p_typed_this->GetOuterPrivate())));
    }

    auto object_to_string(void* p_this, std::wstring& out_line) -> void
    {
        object_trivial_dump_to_string(p_this, out_line);
    }

    auto property_trivial_dump_to_string(void* p_this, std::wstring& out_line) -> void
    {
        FProperty* p_typed_this = static_cast<FProperty*>(p_this);

        out_line.append(fmt::format(L"[{:016X}] ", reinterpret_cast<uintptr_t>(p_this)));
        out_line.append(p_typed_this->GetFullName());
        out_line.append(fmt::format(L" [o: {:X}] ", p_typed_this->GetOffset_Internal()));

        auto property_class = p_typed_this->GetClass();
        out_line.append(fmt::format(L"[n: {:X}] [c: {:016X}]", p_typed_this->GetFName().GetComparisonIndex(), property_class.HashObject()));

        if (Version::IsAtLeast(4, 25))
        {
            out_line.append(fmt::format(L" [owr: {:016X}]", p_typed_this->GetOwnerVariant().HashObject()));
        }
    }

    auto property_to_string(void* p_this, std::wstring& out_line) -> void
    {
        property_trivial_dump_to_string(p_this, out_line);
    }

    auto arrayproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        property_trivial_dump_to_string(p_this, out_line);

        FArrayProperty* p_typed_this = static_cast<FArrayProperty*>(p_this);
        out_line.append(fmt::format(L" [ai: {:016X}]", reinterpret_cast<uintptr_t>(p_typed_this->GetInner())));
    }

    auto arrayproperty_to_string_complex(void* p_this, std::wstring& out_line, ObjectToStringComplexDeclCallable callable) -> void
    {
        FProperty* array_inner = static_cast<FArrayProperty*>(p_this)->GetInner();
        auto array_inner_class = array_inner->GetClass().HashObject();

        if (to_string_exists(array_inner_class))
        {
            get_to_string(array_inner_class)(array_inner, out_line);

            if (to_string_complex_exists(array_inner_class))
            {
                // If this code is executed then we'll be having another line before we return to the dumper, so we need to explicitly add a new line
                // If this code is not executed then we'll not be having another line and the dumper will add the new line
                out_line.append(L"\n");

                get_to_string_complex(array_inner_class)(array_inner, out_line, [&]([[maybe_unused]] void* prop) {
                    // It's possible that a new line is supposed to be appended here
                });
            }

            callable(array_inner);
        }
        else
        {
            out_line.append(array_inner->GetFullName());
            callable(array_inner);
        }
    }

    auto mapproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        property_trivial_dump_to_string(p_this, out_line);

        FMapProperty* typed_this = static_cast<FMapProperty*>(p_this);
        FProperty* key_property = typed_this->GetKeyProp();
        FProperty* value_property = typed_this->GetValueProp();
        out_line.append(fmt::format(L" [kp: {:016X}] [vp: {:016X}]", reinterpret_cast<uintptr_t>(key_property), reinterpret_cast<uintptr_t>(value_property)));
    }

    auto mapproperty_to_string_complex(void* p_this, std::wstring& out_line, ObjectToStringComplexDeclCallable callable) -> void
    {
        FMapProperty* typed_this = static_cast<FMapProperty*>(p_this);
        FProperty* key_property = typed_this->GetKeyProp();
        FProperty* value_property = typed_this->GetValueProp();
        auto key_property_class = key_property->GetClass().HashObject();
        auto value_property_class = value_property->GetClass().HashObject();

        auto dump_property = [&](FProperty* property, ToStringHash property_class) {
            if (to_string_exists(property_class))
            {
                get_to_string(property_class)(property, out_line);

                if (to_string_complex_exists(property_class))
                {
                    // If this code is executed then we'll be having another line before we return to the dumper, so we need to explicitly add a new line
                    // If this code is not executed then we'll not be having another line and the dumper will add the new line
                    out_line.append(L"\n");

                    get_to_string_complex(property_class)(property, out_line, [&]([[maybe_unused]] void* prop) {});
                }

                callable(property);
            }
            else
            {
                out_line.append(property->GetFullName());
                callable(property);
            }
        };

        dump_property(key_property, key_property_class);
        dump_property(value_property, value_property_class);
    }

    auto classproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        FClassProperty* typed_this = static_cast<FClassProperty*>(p_this);

        property_trivial_dump_to_string(p_this, out_line);
        // mc = MetaClass
        out_line.append(fmt::format(L" [mc: {:016X}]", reinterpret_cast<uintptr_t>(typed_this->GetMetaClass())));
    }

    auto delegateproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        property_trivial_dump_to_string(p_this, out_line);

        FDelegateProperty* p_typed_this = static_cast<FDelegateProperty*>(p_this);
        out_line.append(fmt::format(L" [df: {:016X}]", reinterpret_cast<uintptr_t>(p_typed_this->GetSignatureFunction())));
    }

    auto fieldpathproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        FFieldPathProperty* typed_this = static_cast<FFieldPathProperty*>(p_this);

        property_trivial_dump_to_string(p_this, out_line);
        out_line.append(fmt::format(L" [pc: {:016X}]", reinterpret_cast<uintptr_t>(typed_this->GetPropertyClass())));
    }

    auto interfaceproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        FInterfaceProperty* typed_this = static_cast<FInterfaceProperty*>(p_this);

        property_trivial_dump_to_string(p_this, out_line);
        out_line.append(fmt::format(L" [ic: {:016X}]", reinterpret_cast<uintptr_t>(typed_this->GetInterfaceClass())));
    }

    auto multicastdelegateproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        property_trivial_dump_to_string(p_this, out_line);

        FMulticastDelegateProperty* p_typed_this = static_cast<FMulticastDelegateProperty*>(p_this);
        out_line.append(fmt::format(L" [df: {:016X}]", reinterpret_cast<uintptr_t>(p_typed_this->GetSignatureFunction())));
    }

    auto objectproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        FObjectProperty* typed_this = static_cast<FObjectProperty*>(p_this);

        property_trivial_dump_to_string(p_this, out_line);
        out_line.append(fmt::format(L" [pc: {:016X}]", reinterpret_cast<uintptr_t>(typed_this->GetPropertyClass())));
    }

    auto structproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        FStructProperty* typed_this = static_cast<FStructProperty*>(p_this);

        property_trivial_dump_to_string(p_this, out_line);
        out_line.append(fmt::format(L" [ss: {:016X}]", reinterpret_cast<uintptr_t>(typed_this->GetStruct())));
    }

    auto enumproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        property_trivial_dump_to_string(p_this, out_line);

        auto* typed_this = static_cast<FEnumProperty*>(p_this);
        out_line.append(fmt::format(L" [em: {:016X}]", reinterpret_cast<uintptr_t>(typed_this->GetEnum())));
    }

    auto boolproperty_to_string(void* p_this, std::wstring& out_line) -> void
    {
        property_trivial_dump_to_string(p_this, out_line);

        auto* typed_this = static_cast<FBoolProperty*>(p_this);
        if (typed_this->GetFieldMask() != 255)
        {
            out_line.append(fmt::format(L" [fm: {:X}] [bm: {:X}]", typed_this->GetFieldMask(), typed_this->GetByteMask()));
        }
    }

    auto enum_to_string(void* p_this, std::wstring& out_line) -> void
    {
        object_trivial_dump_to_string(p_this, out_line);

        auto* typed_this = static_cast<UEnum*>(p_this);

        for (auto& Elem : typed_this->ForEachName())
        {
            out_line.append(fmt::format(L"\n[{:016X}] {} [n: {:X}] [v: {}]", 0, Elem.Key.ToString(), Elem.Key.GetComparisonIndex(), Elem.Value));
        }
    }

    auto struct_to_string(void* p_this, std::wstring& out_line) -> void
    {
        UStruct* typed_this = static_cast<UStruct*>(p_this);

        object_trivial_dump_to_string(p_this, out_line);
        out_line.append(fmt::format(L" [sps: {:016X}]", reinterpret_cast<uintptr_t>(typed_this->GetSuperStruct())));
    }

    auto function_to_string(void* p_this, std::wstring& out_line) -> void
    {
        object_trivial_dump_to_string(p_this, out_line, L":");
    }

    auto scriptstruct_to_string_complex(void* p_this, std::wstring& out_line, ObjectToStringComplexDeclCallable callable) -> void
    {
        UScriptStruct* script_struct = static_cast<UScriptStruct*>(p_this);

        for (FProperty* prop : script_struct->ForEachProperty())
        {
            callable(prop);
        }
    }

    auto init() -> void
    {
        object_to_string_functions[UEnum::StaticClass()->HashObject()] = &enum_to_string;
        object_to_string_functions[UUserDefinedEnum::StaticClass()->HashObject()] = &enum_to_string;
        object_to_string_functions[UClass::StaticClass()->HashObject()] = &struct_to_string;
        object_to_string_functions[UBlueprintGeneratedClass::StaticClass()->HashObject()] = &struct_to_string;
        object_to_string_functions[UWidgetBlueprintGeneratedClass::StaticClass()->HashObject()] = &struct_to_string;
        object_to_string_functions[UAnimBlueprintGeneratedClass::StaticClass()->HashObject()] = &struct_to_string;
        object_to_string_functions[UFunction::StaticClass()->HashObject()] = &function_to_string;
        // object_to_string_functions[UDelegateFunction::StaticClass()->HashObject()] = &function_to_string;
        // object_to_string_functions[USparseDelegateFunction::StaticClass()->HashObject()] = &function_to_string;
        object_to_string_functions[UScriptStruct::StaticClass()->HashObject()] = &struct_to_string;
        object_to_string_complex_functions[UScriptStruct::StaticClass()->HashObject()] = &scriptstruct_to_string_complex;
        object_to_string_functions[FObjectProperty::StaticClass().HashObject()] = &objectproperty_to_string;
        object_to_string_functions[FObjectPtrProperty::StaticClass().HashObject()] = &objectproperty_to_string;
        object_to_string_functions[FAssetObjectProperty::StaticClass().HashObject()] = &objectproperty_to_string;
        object_to_string_functions[FAssetClassProperty::StaticClass().HashObject()] = &classproperty_to_string;
        object_to_string_functions[FInt8Property::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FInt16Property::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FIntProperty::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FInt64Property::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FByteProperty::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FUInt16Property::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FUInt32Property::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FUInt64Property::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FNameProperty::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FFloatProperty::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FBoolProperty::StaticClass().HashObject()] = &boolproperty_to_string;
        object_to_string_functions[FArrayProperty::StaticClass().HashObject()] = &arrayproperty_to_string;
        object_to_string_complex_functions[FArrayProperty::StaticClass().HashObject()] = &arrayproperty_to_string_complex;
        object_to_string_functions[FMapProperty::StaticClass().HashObject()] = &mapproperty_to_string;
        object_to_string_complex_functions[FMapProperty::StaticClass().HashObject()] = &mapproperty_to_string_complex;
        object_to_string_functions[FStructProperty::StaticClass().HashObject()] = &structproperty_to_string;
        object_to_string_functions[FClassProperty::StaticClass().HashObject()] = &classproperty_to_string;
        if (Version::IsAtLeast(4, 18))
        {
            object_to_string_functions[FSoftClassProperty::StaticClass().HashObject()] = &classproperty_to_string;
            object_to_string_functions[FSoftObjectProperty::StaticClass().HashObject()] = &objectproperty_to_string;
        }
        object_to_string_functions[FWeakObjectProperty::StaticClass().HashObject()] = &objectproperty_to_string;
        object_to_string_functions[FLazyObjectProperty::StaticClass().HashObject()] = &objectproperty_to_string;
        if (Version::IsAtLeast(4, 15))
        {
            object_to_string_functions[FEnumProperty::StaticClass().HashObject()] = &enumproperty_to_string;
        }
        object_to_string_functions[FTextProperty::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FStrProperty::StaticClass().HashObject()] = &property_to_string;
        object_to_string_functions[FDelegateProperty::StaticClass().HashObject()] = &delegateproperty_to_string;
        object_to_string_functions[FMulticastDelegateProperty::StaticClass().HashObject()] = &multicastdelegateproperty_to_string;
        if (Version::IsAtLeast(4, 23))
        {
            object_to_string_functions[FMulticastInlineDelegateProperty::StaticClass().HashObject()] = &multicastdelegateproperty_to_string;
            object_to_string_functions[FMulticastSparseDelegateProperty::StaticClass().HashObject()] = &multicastdelegateproperty_to_string;
        }
        object_to_string_functions[FInterfaceProperty::StaticClass().HashObject()] = &interfaceproperty_to_string;
        if (Version::IsAtLeast(4, 25))
        {
            object_to_string_functions[FFieldPathProperty::StaticClass().HashObject()] = &fieldpathproperty_to_string;
        }
    }
} // namespace RC::ObjectDumper
