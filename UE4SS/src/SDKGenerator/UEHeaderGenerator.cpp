#ifdef WIN32

#define NOMINMAX
#include <Windows.h>
#ifdef TEXT
#undef TEXT
#endif
#endif

#include <algorithm>
#include <format>
#include <fstream>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <SDKGenerator/UEHeaderGenerator.hpp>
#pragma warning(disable : 4005)
#include <DynamicOutput/DynamicOutput.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/FScriptArray.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/FText.hpp>
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
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FSetProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>
#include <Unreal/Property/FSoftObjectProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/Property/FWeakObjectProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/UActorComponent.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UnrealFlags.hpp>
#include <Helpers/Format.hpp>
#pragma warning(default : 4005)

namespace RC::UEGenerator
{
    using namespace RC::Unreal;

    std::map<File::StringType, UniqueName> UEHeaderGenerator::m_used_file_names{};
    std::map<UObject*, int32_t> UEHeaderGenerator::m_dependency_object_to_unique_id{};

    auto static is_subtype_struct_valid(UScriptStruct* subtype) -> bool
    {

        static const std::vector<FName> invalid_subtype_structs = {
                FName(STR("FloatInterval"), FNAME_Add),
                FName(STR("SplineCurves"), FNAME_Add),
                FName(STR("Int32Interval"), FNAME_Add),
                FName(STR("BoneReference"), FNAME_Add),
                FName(STR("OverlapResult"), FNAME_Add),
                FName(STR("RichCurve"), FNAME_Add),
                FName(STR("MovieScenePropertyTrack"), FNAME_Add),
                FName(STR("MovieSceneSection"), FNAME_Add),
                FName(STR("MovieSceneFloatChannel"), FNAME_Add),
                FName(STR("MovieSceneActorReferenceData"), FNAME_Add),
                FName(STR("ExpressionInput"), FNAME_Add),
                FName(STR("ScalarParameterNameAndCurve"), FNAME_Add),
                FName(STR("PredictionKey"), FNAME_Add),
                FName(STR("CustomPrimitiveData"), FNAME_Add),
                FName(STR("EQSParametrizedQueryExecutionRequest"), FNAME_Add),
                FName(STR("BlendFilter"), FNAME_Add),
                FName(STR("AIDataProviderFloatValue"), FNAME_Add),
                FName(STR("AIDataProviderIntValue"), FNAME_Add),
                FName(STR("AIDataProviderBoolValue"), FNAME_Add),
        };

        auto subtype_name = subtype->GetNamePrivate();
        for (const auto& subtype_struct_name : invalid_subtype_structs)
        {
            if (subtype_name == subtype_struct_name)
            {
                return false;
            }
        }
        return true;
    }

    auto static is_subtype_valid(FProperty* subtype) -> bool
    {
        if ((subtype->IsA<FNumericProperty>() && !subtype->IsA<FIntProperty>() && !subtype->IsA<FFloatProperty>() && !subtype->IsA<FByteProperty>()))
        {
            return false;
        }
        else if (auto* as_array = CastField<FArrayProperty>(subtype); as_array)
        {
            auto array_inner = as_array->GetInner();
            if (array_inner->IsA<FWeakObjectProperty>())
            {
                return false;
            }
            return is_subtype_valid(array_inner);
        }
        else if (auto* as_map = CastField<FMapProperty>(subtype); as_map)
        {
            auto map_key_prop = as_map->GetKeyProp();
            auto map_value_prop = as_map->GetValueProp();
            if (map_key_prop->IsA<FWeakObjectProperty>() || map_value_prop->IsA<FWeakObjectProperty>())
            {
                return false;
            }
            return is_subtype_valid(map_key_prop) && is_subtype_valid(map_value_prop);
        }
        else if (auto* as_struct = CastField<FStructProperty>(subtype); as_struct)
        {
            return is_subtype_struct_valid(as_struct->GetStruct());
        }
        else if (auto* as_enum = CastField<FEnumProperty>(subtype); as_enum)
        {
            return as_enum->GetUnderlyingProperty()->IsA<FByteProperty>();
        }
        else
        {
            return true;
        }
    }

    auto string_to_uppercase(SystemStringType s) -> SystemStringType
    {
        std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) {
            return towupper(c);
        });
        return s;
    }

    class FlagFormatHelper
    {
        std::set<SystemStringType> m_switches;
        std::map<SystemStringType, std::set<SystemStringType>> m_parameters;
        std::shared_ptr<FlagFormatHelper> m_meta_helper;

        FlagFormatHelper(bool is_root_helper)
        {
            if (is_root_helper)
            {
                m_meta_helper = std::shared_ptr<FlagFormatHelper>(new FlagFormatHelper(false));
            }
        }

      public:
        FlagFormatHelper() : FlagFormatHelper(true)
        {
        }

        auto add_switch(const SystemStringType& switch_name) -> void
        {
            m_switches.insert(switch_name);
        }

        auto add_parameter(const SystemStringType& parameter_name, const SystemStringType& parameter_value) -> void
        {
            if (parameter_name == SYSSTR("meta"))
            {
                throw std::invalid_argument("Use get_meta() to add metadata to the flag declaration");
            }

            auto map_iterator = m_parameters.find(parameter_name);
            if (map_iterator != m_parameters.end())
            {
                map_iterator->second.insert(parameter_name);
            }
            else
            {
                m_parameters.insert({parameter_name, {parameter_value}});
            }
        }

        auto get_meta() const -> FlagFormatHelper*
        {
            return m_meta_helper.get();
        }

        auto build_flag_string() const -> SystemStringType
        {
            SystemStringType resulting_string;

            for (const SystemStringType& switch_name : m_switches)
            {
                resulting_string.append(switch_name);
                resulting_string.append(SYSSTR(", "));
            }

            for (const auto& parameter_pair : m_parameters)
            {
                resulting_string.append(parameter_pair.first);
                resulting_string.append(SYSSTR("="));
                const std::set<SystemStringType>& parameter_values = parameter_pair.second;

                if (parameter_values.size() != 1)
                {
                    resulting_string.append(SYSSTR("("));

                    for (const SystemStringType& parameter_value : parameter_values)
                    {
                        resulting_string.append(parameter_value);
                        resulting_string.append(SYSSTR(", "));
                    }

                    if (parameter_values.size() != 0)
                    {
                        resulting_string.erase(resulting_string.size() - 1, 1);
                    }
                    resulting_string.append(SYSSTR(")"));
                }
                else
                {
                    resulting_string.append(*parameter_values.begin());
                }
                resulting_string.append(SYSSTR(", "));
            }

            if (m_meta_helper)
            {
                const SystemStringType meta_flag_string = m_meta_helper->build_flag_string();
                if (!meta_flag_string.empty())
                {
                    resulting_string.append(SYSSTR("meta=("));
                    resulting_string.append(meta_flag_string);
                    resulting_string.append(SYSSTR(")"));
                    resulting_string.append(SYSSTR(", "));
                }
            }

            if (!resulting_string.empty())
            {
                resulting_string.erase(resulting_string.size() - 2, 2);
            }
            return resulting_string;
        }
    };

    auto UEHeaderGenerator::generate_module_build_file(const SystemStringType& module_name) -> void
    {
        const FFilePath module_file_path = m_root_directory / module_name / std::format(SYSSTR("{}.Build.cs"), module_name);
        GeneratedFile module_build_file = GeneratedFile(module_file_path);

        module_build_file.append_line(SYSSTR("using UnrealBuildTool;"));
        module_build_file.append_line(SYSSTR(""));

        module_build_file.append_line(std::format(SYSSTR("public class {} : ModuleRules {{"), module_name));
        module_build_file.begin_indent_level();

        module_build_file.append_line(std::format(SYSSTR("public {}(ReadOnlyTargetRules Target) : base(Target) {{"), module_name));
        module_build_file.begin_indent_level();

        module_build_file.append_line(SYSSTR("PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;"));

        if (Version::IsAtLeast(4, 24))
        {
            module_build_file.append_line(SYSSTR("bLegacyPublicIncludePaths = false;"));
            module_build_file.append_line(SYSSTR("ShadowVariableWarningLevel = WarningLevel.Warning;"));
        }

        module_build_file.append_line(SYSSTR(""));
        module_build_file.append_line(SYSSTR("PublicDependencyModuleNames.AddRange(new string[] {"));
        module_build_file.begin_indent_level();

        std::set<SystemStringType> all_module_dependencies = this->m_forced_module_dependencies;
        std::set<SystemStringType> clean_module_dependencies{};
        add_module_and_sub_module_dependencies(clean_module_dependencies, module_name, false);

        all_module_dependencies.insert(clean_module_dependencies.begin(), clean_module_dependencies.end());

        for (const SystemStringType& other_module_name : all_module_dependencies)
        {
            module_build_file.append_line(std::format(SYSSTR("\"{}\","), other_module_name));
        }

        module_build_file.end_indent_level();
        module_build_file.append_line(SYSSTR("});"));

        module_build_file.end_indent_level();
        module_build_file.append_line(SYSSTR("}"));

        module_build_file.end_indent_level();
        module_build_file.append_line(SYSSTR("}"));

        module_build_file.serialize_file_content_to_disk();
    }

    auto UEHeaderGenerator::generate_module_implementation_file(const SystemStringType& module_name) -> void
    {
        const FFilePath module_file_path = m_root_directory / module_name / SYSSTR("Private") / std::format(SYSSTR("{}Module.cpp"), module_name);
        GeneratedFile module_impl_file = GeneratedFile(module_file_path);

        module_impl_file.append_line(SYSSTR("#include \"Modules/ModuleManager.h\""));
        module_impl_file.append_line(SYSSTR(""));
        if (module_name != m_primary_module_name)
        {
            module_impl_file.append_line(std::format(SYSSTR("IMPLEMENT_MODULE(FDefaultGameModuleImpl, {});"), module_name));
        }
        else
        {
            module_impl_file.append_line(std::format(SYSSTR("IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, {}, {});"), module_name, module_name));
        }

        module_impl_file.serialize_file_content_to_disk();
    }

    auto UEHeaderGenerator::generate_interface_definition(UClass* uclass, GeneratedSourceFile& header_data) -> void
    {
        const SystemStringType interface_class_native_name = get_native_class_name(uclass);
        const SystemStringType interface_flags_string = generate_interface_flags(uclass);

        SystemStringType maybe_api_name;
        if ((uclass->GetClassFlags() & CLASS_RequiredAPI) != 0)
        {
            maybe_api_name.append(convert_module_name_to_api_name(header_data.get_header_module_name()));
            maybe_api_name.append(SYSSTR(" "));
        }

        UClass* super_class = uclass->GetSuperClass();
        header_data.add_dependency_object(super_class, DependencyLevel::Include);

        SystemStringType parent_interface_class_name = get_native_class_name(super_class);

        // Generate interface UCLASS declaration
        header_data.append_line(std::format(SYSSTR("UINTERFACE({})"), interface_flags_string));
        header_data.append_line(std::format(SYSSTR("class {}{} : public {} {{"), maybe_api_name, interface_class_native_name, parent_interface_class_name));

        header_data.begin_indent_level();
        header_data.append_line(SYSSTR("GENERATED_BODY()"));
        header_data.end_indent_level();

        header_data.append_line(SYSSTR("};"));
        header_data.append_line(SYSSTR(""));

        // Generate interface real class declaration
        const SystemStringType interface_native_name = get_native_class_name(uclass, true);
        const SystemStringType parent_interface_name = get_native_class_name(super_class, true);

        header_data.append_line(std::format(SYSSTR("class {}{} : public {} {{"), maybe_api_name, interface_native_name, parent_interface_name));
        header_data.begin_indent_level();

        header_data.append_line(SYSSTR("GENERATED_BODY()"));

        AccessModifier current_access_modifier = AccessModifier::None;
        append_access_modifier(header_data, AccessModifier::Public, current_access_modifier);

        // Generate delegate type declarations for the current class
        int32_t NumDelegatesGenerated = 0;
        for (UFunction* function : uclass->ForEachFunction())
        {
            if (is_delegate_signature_function(function))
            {
                generate_delegate_type_declaration(function, uclass, header_data);
                NumDelegatesGenerated++;
            }
        }
        if (NumDelegatesGenerated)
        {
            header_data.append_line(SYSSTR(""));
        }

        // Generate interface functions
        for (UFunction* function : uclass->ForEachFunction())
        {
            if (!is_delegate_signature_function(function))
            {
                append_access_modifier(header_data, get_function_access_modifier(function), current_access_modifier);
                generate_function(uclass, function, header_data, true, CaseInsensitiveSet{});
            }
        }

        header_data.end_indent_level();
        header_data.append_line(SYSSTR("};"));
    }

    auto UEHeaderGenerator::generate_object_definition(UClass* uclass, GeneratedSourceFile& header_data) -> void
    {
        const SystemStringType class_native_name = get_native_class_name(uclass);
        const SystemStringType class_flags_string = generate_class_flags(uclass);

        SystemStringType maybe_api_name;
        if ((uclass->GetClassFlags() & CLASS_RequiredAPI) != 0)
        {
            maybe_api_name.append(convert_module_name_to_api_name(header_data.get_header_module_name()));
            maybe_api_name.append(SYSSTR(" "));
        }

        UClass* super_class = uclass->GetSuperClass();
        SystemStringType parent_class_name;
        if (super_class)
        {
            parent_class_name = get_native_class_name(super_class);
        }
        else
        {
            parent_class_name = SYSSTR("UObjectBaseUtility");
        }

        if (super_class)
        {
            header_data.add_dependency_object(super_class, DependencyLevel::Include);
        }

        SystemStringType interface_list_string;
        auto implemented_interfaces = uclass->GetInterfaces();

        for (const RC::Unreal::FImplementedInterface& uinterface : implemented_interfaces)
        {
            header_data.add_dependency_object(uinterface.Class, DependencyLevel::Include);
            const SystemStringType interface_name = get_native_class_name(uinterface.Class, true);

            interface_list_string.append(SYSSTR(", public "));
            interface_list_string.append(interface_name);
        }

        header_data.append_line(std::format(SYSSTR("UCLASS({})"), class_flags_string));
        header_data.append_line(std::format(SYSSTR("class {}{} : public {}{} {{"), maybe_api_name, class_native_name, parent_class_name, interface_list_string));
        header_data.begin_indent_level();

        header_data.append_line(SYSSTR("GENERATED_BODY()"));

        AccessModifier current_access_modifier = AccessModifier::None;
        append_access_modifier(header_data, AccessModifier::Public, current_access_modifier);

        CaseInsensitiveSet blacklisted_property_names = collect_blacklisted_property_names(uclass);
        bool encountered_replicated_properties = false;

        // Generate delegate type declarations for the current class
        int32_t NumDelegatesGenerated = 0;
        for (UFunction* function : uclass->ForEachFunction())
        {
            if (is_delegate_signature_function(function))
            {
                generate_delegate_type_declaration(function, uclass, header_data);
                NumDelegatesGenerated++;
            }
        }
        if (NumDelegatesGenerated)
        {
            header_data.append_line(SYSSTR(""));
        }

        // Generate properties
        for (FProperty* property : uclass->ForEachProperty())
        {
            encountered_replicated_properties |= (property->GetPropertyFlags() & CPF_Net) != 0;
            append_access_modifier(header_data, get_property_access_modifier(property), current_access_modifier);
            generate_property(uclass, property, header_data);

            if (auto as_map_property = CastField<FMapProperty>(property))
            {
                if (auto as_struct_property = CastField<FStructProperty>(as_map_property->GetKeyProp()))
                {
                    // Add GetKeyProp() to vector for second pass.
                    m_structs_that_need_get_type_hash.emplace(as_struct_property->GetStruct());
                }
            }
        }

        // Make sure constructor and replicated properties methods are public
        append_access_modifier(header_data, AccessModifier::Public, current_access_modifier);

        // Generate constructor
        SystemStringType constructor_string;
        if (uclass->IsChildOf<AActor>() || uclass->IsChildOf<UActorComponent>())
        {
            constructor_string.append(SYSSTR("const FObjectInitializer& ObjectInitializer"));
        }
        header_data.append_line(std::format(SYSSTR("{}({});"), class_native_name, constructor_string));
        header_data.append_line_no_indent(SYSSTR(""));

        // Generate GetLifetimeReplicatedProps override if we have encountered replicated properties
        if (encountered_replicated_properties)
        {
            header_data.append_line(SYSSTR("virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;"));
            header_data.append_line_no_indent(SYSSTR(""));
        }

        // Generate functions
        std::unordered_set<FName> implemented_functions;
        for (UFunction* function : uclass->ForEachFunction())
        {
            if (!is_delegate_signature_function(function))
            {
                append_access_modifier(header_data, get_function_access_modifier(function), current_access_modifier);
                generate_function(uclass, function, header_data, false, blacklisted_property_names);
                implemented_functions.emplace(function->GetNamePrivate());
            }
        }

        // Generate overrides for all inherited virtual functions
        if (implemented_interfaces.Num() > 0)
        {
            header_data.append_line_no_indent(SYSSTR(""));
            header_data.append_line(SYSSTR("// Fix for true pure virtual functions not being implemented"));
        }
        for (const RC::Unreal::FImplementedInterface& uinterface : implemented_interfaces)
        {
            for (UFunction* interface_function : uinterface.Class->ForEachFunction())
            {
                bool should_skip = (interface_function->GetFunctionFlags() & FUNC_BlueprintEvent);

                if (!implemented_functions.contains(interface_function->GetNamePrivate()) && !should_skip)
                {
                    append_access_modifier(header_data, get_function_access_modifier(interface_function), current_access_modifier);
                    generate_function(uclass, interface_function, header_data, false, blacklisted_property_names, true);
                }
            }
        }

        header_data.end_indent_level();
        header_data.append_line(SYSSTR("};"));
    }

    auto UEHeaderGenerator::generate_struct_definition(UScriptStruct* script_struct, GeneratedSourceFile& header_data) -> void
    {
        const SystemStringType struct_native_name = get_native_struct_name(script_struct);
        const SystemStringType struct_flags_string = generate_struct_flags(script_struct);

        SystemStringType api_macro_name = convert_module_name_to_api_name(header_data.get_header_module_name());
        api_macro_name.append(SYSSTR(" "));
        bool is_struct_exported = (script_struct->GetStructFlags() & STRUCT_RequiredAPI) != 0;

        UScriptStruct* super_struct = script_struct->GetSuperScriptStruct();
        SystemStringType parent_struct_declaration;
        if (super_struct)
        {
            header_data.add_dependency_object(super_struct, DependencyLevel::Include);

            const SystemStringType super_struct_native_name = get_native_struct_name(super_struct);
            parent_struct_declaration.append(std::format(SYSSTR(" : public {}"), super_struct_native_name));
        }

        header_data.append_line(std::format(SYSSTR("USTRUCT({})"), struct_flags_string));
        header_data.append_line(std::format(SYSSTR("struct {}{}{} {{"), is_struct_exported ? api_macro_name : SYSSTR(""), struct_native_name, parent_struct_declaration));
        header_data.begin_indent_level();

        header_data.append_line(SYSSTR("GENERATED_BODY()"));

        AccessModifier current_access_modifier = AccessModifier::None;
        append_access_modifier(header_data, AccessModifier::Public, current_access_modifier);

        // Generate struct properties
        for (FProperty* property : script_struct->ForEachProperty())
        {
            append_access_modifier(header_data, get_property_access_modifier(property), current_access_modifier);
            generate_property(script_struct, property, header_data);

            if (auto as_map_property = CastField<FMapProperty>(property))
            {
                if (auto as_struct_property = CastField<FStructProperty>(as_map_property->GetKeyProp()))
                {
                    // Add GetKeyProp() to vector for second pass.
                    m_structs_that_need_get_type_hash.emplace(as_struct_property->GetStruct());
                }
            }
        }

        // Generate constructor and make sure it's public
        append_access_modifier(header_data, AccessModifier::Public, current_access_modifier);
        header_data.append_line(std::format(SYSSTR("{}{}();"), !is_struct_exported ? api_macro_name : SYSSTR(""), struct_native_name));

        header_data.end_indent_level();
        header_data.append_line(SYSSTR("};"));
    }

    auto UEHeaderGenerator::generate_enum_definition(UEnum* uenum, GeneratedSourceFile& header_data) -> void
    {
        const SystemStringType native_enum_name = get_native_enum_name(uenum, false);
        const int64 highest_enum_value = get_highest_enum(uenum);
        const bool can_use_uint8_override = (highest_enum_value <= 255 && get_lowest_enum(uenum) >= 0);
        const SystemStringType enum_flags_string = generate_enum_flags(uenum);
        const auto underlying_type = m_underlying_enum_types.find(native_enum_name);
        const bool has_known_underlying_type = underlying_type != m_underlying_enum_types.end();
        UEnum::ECppForm cpp_form = uenum->GetCppForm();
        bool enum_is_uint8{false};

        header_data.append_line(std::format(SYSSTR("UENUM({})"), enum_flags_string));

        if (cpp_form == UEnum::ECppForm::Namespaced)
        {
            header_data.append_line(std::format(SYSSTR("namespace {} {{"), native_enum_name));
            header_data.begin_indent_level();
            header_data.append_line(SYSSTR("enum Type {"));
        }
        else if (cpp_form == UEnum::ECppForm::Regular)
        {
            header_data.append_line(std::format(SYSSTR("enum {} {{"), native_enum_name));
        }
        else if (cpp_form == UEnum::ECppForm::EnumClass)
        {
            if (!has_known_underlying_type)
            {
                if (UE4SSProgram::settings_manager.UHTHeaderGenerator.MakeEnumClassesBlueprintType && can_use_uint8_override)
                {
                    header_data.append_line(std::format(SYSSTR("enum class {} : uint8 {{"), native_enum_name));
                    enum_is_uint8 = true;
                }
                else
                {
                    // Enum has never been used in any native classes or structures, go with implicit type
                    header_data.append_line(std::format(SYSSTR("enum class {} {{"), native_enum_name));
                }
            }
            else
            {
                SystemStringType underlying_type_string = underlying_type->second;

                header_data.append_line(std::format(SYSSTR("enum class {} : {} {{"), native_enum_name, underlying_type_string));
            }
        }

        header_data.begin_indent_level();

        UEStringType enum_prefix = uenum->GenerateEnumPrefix();
        int64 expected_next_enum_value = 0;
        bool last_value_was_negative_one{false};
        std::set<UEStringType> enum_name_set{};
        for (auto [Name, Value] : uenum->ForEachName())
        {
            UEStringType enum_name = Name.ToString();
            auto result_enumeration_line = sanitize_enumeration_name(to_system(enum_name));
            auto pre_append_result_line = result_enumeration_line;

            // If an enum name is listed in the array twice, that likely means it is used as the value for another enum.  Long story short, don't print it.
            if (enum_name_set.contains(enum_name))
            {
                continue;
            }
            else
            {
                enum_name_set.emplace(enum_name);
            }
            
            // Taking advantage of GetNameByValue returning the first result for the value to determine if there are any enumerator names that
            // reference an already declared value/name.
            auto first_name_with_value = uenum->GetNameByValue(Value).ToString();
            if (first_name_with_value != Name.ToString())
            {
                result_enumeration_line.append(std::format(SYSSTR(" = {}"), sanitize_enumeration_name(to_system(first_name_with_value))));
            }
            else if (Value != expected_next_enum_value || last_value_was_negative_one)
            {
                const auto CastString = (enum_is_uint8 && Value < 0) ? SYSSTR("(uint8)") : SYSSTR("");
                const auto MinusSign = Value < 0 ? SYSSTR("-") : SYSSTR("");
                result_enumeration_line.append(std::format(SYSSTR(" = {}{}{}"), CastString, MinusSign, Value < 0 ? -Value : Value));
            }
            expected_next_enum_value = Value + 1;
            last_value_was_negative_one = (Value == -1);

            SystemStringType pre_append_result_line_lower = pre_append_result_line;
            std::transform(pre_append_result_line_lower.begin(), pre_append_result_line_lower.end(), pre_append_result_line_lower.begin(), ::towlower);
            if (pre_append_result_line_lower.ends_with(SYSSTR("_max")))
            {
                const auto expected_full_constant_name = std::format(SYSSTR("{}_MAX"), to_system(enum_prefix));
                SystemStringType expected_full_constant_name_lower = expected_full_constant_name;
                std::transform(expected_full_constant_name_lower.begin(), expected_full_constant_name_lower.end(), expected_full_constant_name_lower.begin(), ::towlower);
                
                int64_t expected_max_value = highest_enum_value + 1;

                // Skip enum _MAX constant if it has a matching name and is 1 greater than the highest value used, which means it has been autogenerated
                if ((pre_append_result_line_lower == expected_full_constant_name_lower || pre_append_result_line_lower == sanitize_enumeration_name(expected_full_constant_name_lower)) &&
                    Value == expected_max_value)
                {
                    continue;
                }
                // Otherwise, just make sure it's hidden and not visible to the end user
                result_enumeration_line.append(SYSSTR(" UMETA(Hidden)"));
            }

            result_enumeration_line.append(SYSSTR(","));
            header_data.append_line(result_enumeration_line);
        }

        header_data.end_indent_level();
        header_data.append_line(SYSSTR("};"));

        if (cpp_form == UEnum::ECppForm::Namespaced)
        {
            header_data.end_indent_level();
            header_data.append_line(SYSSTR("}"));
        }
    }

    auto UEHeaderGenerator::generate_delegate_type_declaration(UFunction* signature_function, UClass* delegate_class, GeneratedSourceFile& header_data) -> void
    {
        SystemStringType owning_class;
        if (delegate_class == nullptr)
        {
            owning_class = SYSSTR("UObject*");
        }
        else
        {
            owning_class = to_system(delegate_class->GetNamePrivate().ToString());
        }

        auto function_flags = signature_function->GetFunctionFlags();
        if ((function_flags & Unreal::FUNC_Delegate) == 0)
        {
            throw std::runtime_error(RC::fmt("Delegate Signature function {} is missing FUNC_Delegate flag", signature_function->GetName()));
        }

        // TODO not particularly nice or reliable, but will do for now
        const bool is_sparse = signature_function->GetClassPrivate()->GetName() == STR("SparseDelegateFunction");

        const bool is_multicast = (function_flags & Unreal::FUNC_MulticastDelegate) != 0;
        const bool declared_const = (function_flags & FUNC_Const) != 0;

        const SystemStringType delegate_type_name = get_native_delegate_type_name(signature_function, nullptr, true);
        FProperty* return_value_property = signature_function->GetReturnProperty();

        SystemStringType delegate_macro_string;

        // Delegate macro declaration is only allowed on the top level delegates, class-based types are limited to being implicit
        if (signature_function->GetOuterPrivate()->IsA<UPackage>())
        {
            delegate_macro_string.append(SYSSTR("UDELEGATE("));
            delegate_macro_string.append(generate_function_flags(signature_function));
            delegate_macro_string.append(SYSSTR(") "));
        }

        PropertyTypeDeclarationContext context(delegate_type_name, &header_data);

        int32_t num_delegate_parameters = 0;
        SystemStringType delegate_parameter_list =
                generate_function_parameter_list(nullptr, signature_function, header_data, true, context.context_name, {}, &num_delegate_parameters);
        if (num_delegate_parameters > 0)
        {
            delegate_parameter_list.insert(0, SYSSTR(", "));
        }

        if (num_delegate_parameters > 9)
        {
            Output::send<LogLevel::Error>(SYSSTR("Invalid delegate parameter count in Delegate: {}. Using _TooMany\n"), delegate_type_name);
        }

        SystemStringType return_value_declaration;
        if (return_value_property != NULL)
        {
            return_value_declaration = generate_property_type_declaration(return_value_property, context);
            return_value_declaration.append(SYSSTR(", "));
        }

        SystemStringType delegate_declaration_string = std::format(SYSSTR("{}DECLARE_DYNAMIC{}{}_DELEGATE{}{}{}({}{}{}{});"),
                                                               delegate_macro_string,
                                                               is_multicast ? SYSSTR("_MULTICAST") : SYSSTR(""),
                                                               is_sparse ? SYSSTR("_SPARSE") : SYSSTR(""),
                                                               return_value_property ? SYSSTR("_RetVal") : SYSSTR(""),
                                                               generate_parameter_count_string(num_delegate_parameters),
                                                               declared_const ? SYSSTR("_Const") : SYSSTR(""),
                                                               return_value_declaration,
                                                               delegate_type_name,
                                                               // TODO: Actually get delegate property name.
                                                               is_sparse ? std::format(SYSSTR("{}, {}"), owning_class, SYSSTR("EnterPropertyName")) : SYSSTR(""),
                                                               delegate_parameter_list);

        header_data.append_line(delegate_declaration_string);
    }

    auto UEHeaderGenerator::generate_object_implementation(UClass* uclass, GeneratedSourceFile& implementation_file) -> void
    {
        const SystemStringType class_native_name = get_native_class_name(uclass);

        SystemStringType constructor_content_string;
        SystemStringType constructor_postfix_string;
        UClass* super_class = uclass->GetSuperClass();
        const SystemStringType native_parent_class_name = super_class ? get_native_class_name(super_class) : SYSSTR("UObjectUtility");

        // Generate constructor implementation except for overrides.

        // If class is a child of AActor we add the UObjectInitializer constructor.
        // This may not be required in all cases, but is necessary to override subcomponents and does not hurt anything.
        SystemStringType object_initializer_overrides;
        if (uclass->IsChildOf<AActor>() || uclass->IsChildOf<UActorComponent>())
        {
            constructor_content_string.append(SYSSTR("const FObjectInitializer& ObjectInitializer"));
            constructor_postfix_string.append(std::format(SYSSTR(") : Super(ObjectInitializer{}"), object_initializer_overrides));
        }
        // If parent class contains the UObjectInitializer constructor without default value,
        // we need to create the explicit call to such constructor and pass UObjectInitializer::Get() as the argument.
        else if (m_classes_with_object_initializer.contains(native_parent_class_name))
        {
            constructor_postfix_string.append(std::format(SYSSTR(") : {}(FObjectInitializer::Get()"), native_parent_class_name));
        }

        implementation_file.m_implementation_constructor.append(
                std::format(SYSSTR("{}::{}({}{}"), class_native_name, class_native_name, constructor_content_string, constructor_postfix_string));

        implementation_file.begin_indent_level();

        // Generate and initialize properties
        UObject* class_default_object = uclass->GetClassDefaultObject();

        if (class_default_object != nullptr)
        {
            for (FProperty* property : uclass->OrderedForEachPropertyInChain())
            {
                generate_property_value(uclass, property, class_default_object, implementation_file, SYSSTR("this->"));
            }
        }
        else
        {
            implementation_file.append_line(SYSSTR("// Null default object."));
        }
        m_class_subobjects.clear();

        // Generate component attachments
        for (auto attachment : implementation_file.attachments)
        {
            if ((attachment.second).access_type == false)
            {
                generate_simple_assignment_expression(attachment.first, (attachment.second).attach_string, implementation_file, SYSSTR("this->"), SYSSTR("->"));
            }
            else
            {
                generate_advanced_assignment_expression(attachment.first, (attachment.second).attach_string, implementation_file, SYSSTR("this->"), (attachment.second).property_type, SYSSTR("->"));
            }
        }
        
        implementation_file.end_indent_level();
        implementation_file.append_line(SYSSTR("}\n"));

        // Finalize constructor.  We do this after the property generation because we need information from the properties
        // to determine the required overrides within the constructor.
        implementation_file.m_implementation_constructor.append(SYSSTR(") {"));

        CaseInsensitiveSet blacklisted_property_names = collect_blacklisted_property_names(uclass);

        // Generate functions
        for (UFunction* function : uclass->ForEachFunction())
        {
            if (!is_delegate_signature_function(function))
            {
                generate_function_implementation(uclass, function, implementation_file, false, blacklisted_property_names);
                implementation_file.append_line(SYSSTR(""));
            }
        }

        bool encountered_replicated_properties = false;

        for (FProperty* property : uclass->ForEachProperty())
        {
            encountered_replicated_properties |= (property->GetPropertyFlags() & CPF_Net) != 0;
        }

        // Generate replicated properties implementation if we really need it
        if (encountered_replicated_properties)
        {
            implementation_file.add_extra_include(SYSSTR("Net/UnrealNetwork.h"));

            implementation_file.append_line(
                    std::format(SYSSTR("void {}::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {{"), class_native_name));
            implementation_file.begin_indent_level();

            implementation_file.append_line(SYSSTR("Super::GetLifetimeReplicatedProps(OutLifetimeProps);"));
            implementation_file.append_line(SYSSTR(""));

            for (FProperty* property : uclass->ForEachProperty())
            {
                if ((property->GetPropertyFlags() & CPF_Net) != 0)
                {
                    implementation_file.append_line(std::format(SYSSTR("DOREPLIFETIME({}, {});"), class_native_name, to_system(property->GetName())));
                }
            }

            implementation_file.end_indent_level();
            implementation_file.append_line(SYSSTR("}"));
            implementation_file.append_line(SYSSTR(""));
        }
    }

    auto UEHeaderGenerator::generate_struct_implementation(UScriptStruct* script_struct, GeneratedSourceFile& implementation_file) -> void
    {
        const SystemStringType struct_native_name = get_native_struct_name(script_struct);

        // Generate constructor implementation and initialize properties inside
        implementation_file.m_implementation_constructor.append(std::format(SYSSTR("{}::{}() {{"), struct_native_name, struct_native_name));
        implementation_file.begin_indent_level();

        // Generate properties
        void* struct_default_object = malloc(script_struct->GetPropertiesSize());

        // TODO: ScriptStruct->InitializeStruct(StructDefaultObject);
        memset(struct_default_object, 0, script_struct->GetPropertiesSize());

        for (FProperty* property : script_struct->OrderedForEachPropertyInChain())
        {
            generate_property_value(script_struct, property, struct_default_object, implementation_file, SYSSTR("this->"));
        }

        // TODO: ScriptStruct->DestroyStruct(StructDefaultObject);
        free(struct_default_object);

        implementation_file.end_indent_level();
        implementation_file.append_line(SYSSTR("}"));
    }

    auto UEHeaderGenerator::generate_property(UObject* uclass, FProperty* property, GeneratedSourceFile& header_data) -> void
    {
        const SystemStringType property_flags_string = generate_property_flags(property);

        bool is_bitmask_bool = false;
        PropertyTypeDeclarationContext Context(to_system(uclass->GetName()), &header_data, true, &is_bitmask_bool);

        SystemStringType property_type_string{};
        bool type_is_valid = true;
        SystemStringType error_string{};
        try
        {
            property_type_string = generate_property_type_declaration(property, Context);
        }
        catch (std::exception& e)
        {
            type_is_valid = false;
            error_string = to_system(std::string(e.what()));
        }

        if (!type_is_valid)
        {
            Output::send<LogLevel::Warning>(SYSSTR("Warning: {}\n"), error_string);
            header_data.append_line(std::format(SYSSTR("// UPROPERTY({})"), property_flags_string));
            header_data.append_line(std::format(SYSSTR("// Missed Property: {}"), to_system(property->GetName())));
            header_data.append_line(std::format(SYSSTR("// {}"), error_string));
            header_data.append_line(SYSSTR(""));
            return;
        }

        SystemStringType property_extra_declaration;
        if (property->GetArrayDim() != 1)
        {
            property_extra_declaration.append(SYSSTR("["));
            property_extra_declaration.append(to_system(std::to_string(property->GetArrayDim())));
            property_extra_declaration.append(SYSSTR("]"));
        }
        else if (is_bitmask_bool)
        {
            property_extra_declaration.append(SYSSTR(": 1"));
        }

        header_data.append_line(std::format(SYSSTR("UPROPERTY({})"), property_flags_string));
        header_data.append_line(std::format(SYSSTR("{} {}{};"), property_type_string, to_system(property->GetName()), property_extra_declaration));
        header_data.append_line(SYSSTR(""));
    }

    // TODO FUNC_Final is not properly handled (should be always set except some weird cases)
    auto UEHeaderGenerator::generate_function(UClass* uclass,
                                              UFunction* function,
                                              GeneratedSourceFile& header_data,
                                              bool is_generating_interface,
                                              const CaseInsensitiveSet& blacklisted_property_names,
                                              bool generate_as_override) -> void
    {
        auto function_flags = function->GetFunctionFlags();
        const UEStringType context_name = uclass->GetName();
        bool is_function_pure_virtual = generate_as_override;

        SystemStringType function_modifier_string;
        if ((function_flags & FUNC_Static) != 0)
        {
            function_modifier_string.append(SYSSTR("static "));
        }
        else if ((function_flags & FUNC_BlueprintEvent) == 0 && is_generating_interface)
        {
            // When we have a blueprint function that is not blueprint event inside the interface,
            // it means we are dealing with the native interface that cannot be implemented via blueprints
            // and uses pure virtual functions implemented through native code
            function_modifier_string.append(SYSSTR("virtual "));
            is_function_pure_virtual = true;
        }

        FProperty* return_property = function->GetReturnProperty();
        SystemStringType return_property_string;
        if (return_property != NULL)
        {
            PropertyTypeDeclarationContext context(to_system(uclass->GetName()), &header_data);
            return_property_string = generate_property_type_declaration(return_property, context);
        }
        else
        {
            return_property_string = SYSSTR("void");
        }

        SystemStringType function_extra_postfix_string;
        if ((function_flags & FUNC_Const) != 0)
        {
            function_extra_postfix_string.append(SYSSTR(" const"));
        }
        if (is_function_pure_virtual)
        {
            SystemStringType return_statement_string;
            if (return_property != NULL)
            {
                const auto default_property_value = generate_default_property_value(return_property, header_data, to_system(context_name));
                return_statement_string = std::format(SYSSTR(" return {};"), default_property_value);
            }

            if (generate_as_override)
            {
                function_extra_postfix_string.append(SYSSTR(" override"));
            }
            function_extra_postfix_string.append(std::format(SYSSTR(" PURE_VIRTUAL({},{})"), to_system(function->GetName()), return_statement_string));
        }

        auto function_argument_list = generate_function_parameter_list(uclass, function, header_data, false, to_system(context_name), blacklisted_property_names);

        const auto function_flags_string = generate_function_flags(function, is_function_pure_virtual);
        header_data.append_line(std::format(SYSSTR("UFUNCTION({})"), function_flags_string));
        // Format for virtual functions
        // virtual <return type> <function_name>(<params>) PURE_VIRTUAL(<function name>, <return statement>)
        header_data.append_line(std::format(SYSSTR("{}{} {}({}){};"),
                                            function_modifier_string,
                                            return_property_string,
                                            to_system(function->GetName()),
                                            function_argument_list,
                                            function_extra_postfix_string));
        header_data.append_line(SYSSTR(""));
    }

    auto UEHeaderGenerator::generate_enum_value(UEnum* uenum, int64_t enum_value) -> SystemStringType
    {
        UEnum::ECppForm cpp_form = uenum->GetCppForm();
        const SystemStringType enum_native_name = get_native_enum_name(uenum, false);

        SystemStringType enum_constant_name;
        for (auto [Name, Value] : uenum->ForEachName())
        {
            if (Value == enum_value)
            {
                enum_constant_name = sanitize_enumeration_name(to_system(Name.ToString()));
            }
        }
        if (enum_constant_name.empty())
        {
            Output::send(SYSSTR("Warning: Invalid value for enum '{}', casting instead of using enum name. Value '{}' will be cast to the enum.\n"),
                         enum_native_name,
                         enum_value);
            return std::format(SYSSTR("({}){}"), enum_native_name, enum_value);
        }
        else
        {
            // Regular enumerations do not really need an enum name prefix
            if (cpp_form == UEnum::ECppForm::Regular)
            {
                return enum_constant_name;
            }
            return std::format(SYSSTR("{}::{}"), enum_native_name, enum_constant_name);
        }
    }

    auto UEHeaderGenerator::generate_simple_assignment_expression(FProperty* property,
                                                                  const SystemStringType& value,
                                                                  GeneratedSourceFile& implementation_file,
                                                                  const SystemStringType& property_scope,
                                                                  const SystemStringType& operator_type) -> void
    {
        const auto field_class_name = to_system(property->GetName());
        if (property->GetArrayDim() == 1)
        {
            implementation_file.append_line(std::format(SYSSTR("{}{}{}{};"), property_scope, field_class_name, operator_type, value));
        }
        else
        {
            for (int32_t i = 0; i < property->GetArrayDim(); i++)
            {
                implementation_file.append_line(std::format(SYSSTR("{}{}[{}]{}{};"), property_scope, field_class_name, i, operator_type, value));
            }
        }
    }

    auto UEHeaderGenerator::generate_advanced_assignment_expression(FProperty* property,
                                                                    const SystemStringType& value,
                                                                    GeneratedSourceFile& implementation_file,
                                                                    const SystemStringType& property_scope,
                                                                    const SystemStringType& property_type,
                                                                    const SystemStringType& operator_type) -> void
    {
        const auto field_class_name = to_system(property->GetName());
        implementation_file.append_line(std::format(SYSSTR("const FProperty* p_{} = GetClass()->FindPropertyByName(\"{}\");"), field_class_name, field_class_name));
        if (property->GetArrayDim() == 1)
        {
            implementation_file.append_line(std::format(SYSSTR("(*p_{}->ContainerPtrToValuePtr<{}>(this)){}{};"), field_class_name, property_type, operator_type, value));
        }
        else
        {
            for (int32_t i = 0; i < property->GetArrayDim(); i++)
            {
                implementation_file.append_line(
                        std::format(SYSSTR("*p_{}->ContainerPtrToValuePtr<{}>(this){}[{}]{}{};"), field_class_name, property_scope, field_class_name, i, operator_type, value));
            }
        }
    }

    auto UEHeaderGenerator::generate_property_value(
            UStruct* ustruct, FProperty* property, void* object, GeneratedSourceFile& implementation_file, const SystemStringType& property_scope) -> void
    {
        const UEStringType property_name = property->GetName();
        if (property_name == STR("NativeClass") || property_name == STR("hudClass")) { return; }
        const bool private_access_modifier = get_property_access_modifier(property) == AccessModifier::Private;
        bool super_and_no_access = false;
        // Populate super for checks to ensure values are only initialized if they are overriden in the child class
        UStruct* super;
        void* super_object = nullptr;
        FProperty* super_property = nullptr;
        const SystemStringType property_type = generate_property_cxx_name(property, true, ustruct);
        auto as_class = Cast<UClass>(ustruct);
        if (as_class)
        {
            super = as_class->GetSuperClass();
            super_object = Cast<UClass>(super)->GetClassDefaultObject();
            if (super_object != nullptr)
            {
                super_property = super->GetPropertyByNameInChain(property_name.data());
            }
        }
        else
        {
            super = ustruct->GetSuperStruct();
            if (super != nullptr)
            {
                super_object = malloc(super->GetPropertiesSize());
                memset(super_object, 0, super->GetPropertiesSize());
                super_property = super->GetPropertyByNameInChain(property_name.data());
            }
        }

        // Byte Property
        if (property->IsA<FByteProperty>())
        {
            FByteProperty* byte_property = static_cast<FByteProperty*>(property);
            uint8_t* byte_property_value = byte_property->ContainerPtrToValuePtr<uint8_t>(object);

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FByteProperty* super_byte_property = static_cast<FByteProperty*>(super_property);
                uint8_t* super_byte_property_value = super_byte_property->ContainerPtrToValuePtr<uint8_t>(super_object);
                if (*byte_property_value == *super_byte_property_value)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            UEnum* uenum = byte_property->GetEnum();
            SystemStringType result_property_value;
            if (uenum != NULL)
            {
                const SystemStringType enum_type_name = get_native_enum_name(uenum);
                result_property_value = generate_enum_value(uenum, *byte_property_value);
            }
            else
            {
                #ifdef WIN32
                result_property_value = std::to_wstring(*byte_property_value);
                #else
                result_property_value = std::to_string(*byte_property_value);
                #endif
            }

            if (!super_and_no_access)
            {
                generate_simple_assignment_expression(property, result_property_value, implementation_file, property_scope);
            }
            else
            {
                generate_advanced_assignment_expression(property, result_property_value, implementation_file, property_scope, property_type);
            }
            return;
        }

        // Enum Property
        if (property->IsA<FEnumProperty>())
        {
            FEnumProperty* p_enum_property = static_cast<FEnumProperty*>(property);
            UEnum* uenum = p_enum_property->GetEnum();
            if (uenum == NULL)
            {
                throw std::runtime_error(RC::fmt("EnumProperty {} does not have a valid Enum value", property->GetName()));
            }

            FNumericProperty* underlying_property = p_enum_property->GetUnderlyingProperty();
            int64 value = underlying_property->GetSignedIntPropertyValue(p_enum_property->ContainerPtrToValuePtr<int64>(object));

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FNumericProperty* super_underlying_property = static_cast<FEnumProperty*>(super_property)->GetUnderlyingProperty();
                int64 super_value = super_underlying_property->GetSignedIntPropertyValue(super_property->ContainerPtrToValuePtr<int64>(super_object));
                if (value == super_value)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            implementation_file.add_dependency_object(uenum, DependencyLevel::Include);
            SystemStringType result_property_value = generate_enum_value(uenum, value);
            if (!super_and_no_access)
            {
                generate_simple_assignment_expression(property, result_property_value, implementation_file, property_scope);
            }
            else
            {
                generate_advanced_assignment_expression(property, result_property_value, implementation_file, property_scope, property_type);
            }
            return;
        }

        // Bool Property
        if (property->IsA<FBoolProperty>())
        {
            FBoolProperty* bool_property = static_cast<FBoolProperty*>(property);
            uint8_t* raw_property_value = bool_property->ContainerPtrToValuePtr<uint8>(object);

            uint8_t field_mask = bool_property->GetFieldMask();
            uint8_t byte_offset = bool_property->GetByteOffset();

            uint8_t* byte_value = raw_property_value + byte_offset;
            bool result_bool_value = !!(*byte_value & field_mask);

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FBoolProperty* super_bool_property = static_cast<FBoolProperty*>(super_property);
                uint8_t* super_raw_property_value = super_bool_property->ContainerPtrToValuePtr<uint8>(super_object);

                uint8_t super_field_mask = super_bool_property->GetFieldMask();
                uint8_t super_byte_offset = super_bool_property->GetByteOffset();

                uint8_t* super_byte_value = super_raw_property_value + super_byte_offset;
                bool super_result_bool_value = !!(*super_byte_value & super_field_mask);
                if (result_bool_value == super_result_bool_value)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            const SystemStringType result_property_value = result_bool_value ? SYSSTR("true") : SYSSTR("false");
            if (!super_and_no_access)
            {
                generate_simple_assignment_expression(property, result_property_value, implementation_file, property_scope);
            }
            else
            {
                // Skip private bitmask bools TODO: Fix setting these
                if (bool_property->GetFieldMask() != 255) { return; }
                generate_advanced_assignment_expression(property, result_property_value, implementation_file, property_scope, property_type);
            }
            return;
        }

        // Name property is generated as normal string assignment
        if (property->IsA<FNameProperty>())
        {
            FName* name_value = property->ContainerPtrToValuePtr<FName>(object);
            const auto name_value_string = to_system(name_value->ToString());

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FName* super_name_value = super_property->ContainerPtrToValuePtr<FName>(super_object);
                const auto super_name_value_string = to_system(super_name_value->ToString());
                if (name_value_string == super_name_value_string)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            if (name_value_string != SYSSTR("None"))
            {
                const SystemStringType result_property_value = std::format(SYSSTR("TEXT(\"{}\")"), name_value_string);
                if (!super_and_no_access)
                {
                    generate_simple_assignment_expression(property, result_property_value, implementation_file, property_scope);
                }
                else
                {
                    generate_advanced_assignment_expression(property, result_property_value, implementation_file, property_scope, property_type);
                }
            }
            return;
        }

        // String properties are just normal strings
        if (property->IsA<FStrProperty>())
        {
            FString* string_value = property->ContainerPtrToValuePtr<FString>(object);
            const auto string_value_string = to_system(UEStringType (string_value->GetCharArray()));

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FString* super_string_value = super_property->ContainerPtrToValuePtr<FString>(super_object);
                const auto super_string_value_string = to_system(UEStringType (super_string_value->GetCharArray()));
                if (string_value_string == super_string_value_string)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            if (string_value_string != SYSSTR(""))
            {
                const SystemStringType result_value = create_string_literal(string_value_string);
                if (!super_and_no_access)
                {
                    generate_simple_assignment_expression(property, result_value, implementation_file, property_scope);
                }
                else
                {
                    generate_advanced_assignment_expression(property, result_value, implementation_file, property_scope, property_type);
                }
            }
            return;
        }

        // Text properties are treated as FText::FromString, although it's not ideal
        if (property->IsA<FTextProperty>())
        {
            FText* text_value = property->ContainerPtrToValuePtr<FText>(object);
            const auto text_value_string = text_value->ToString();

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FText* super_text_value = super_property->ContainerPtrToValuePtr<FText>(super_object);
                const auto super_text_value_string = super_text_value->ToString();
                if (text_value_string == super_text_value_string)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            if (text_value_string != STR(""))
            {
                const auto result_property_value = std::format(SYSSTR("FText::FromString({})"), create_string_literal(to_system(text_value_string)));
                if (!super_and_no_access)
                {
                    generate_simple_assignment_expression(property, result_property_value, implementation_file, property_scope);
                }
                else
                {
                    generate_advanced_assignment_expression(property, result_property_value, implementation_file, property_scope, property_type);
                }
            }
            return;
        }

        // Class Properties are handled either as NULL or StaticClass references
        if (property->IsA<FClassProperty>())
        {
            FClassProperty* class_property = static_cast<FClassProperty*>(property);
            UClass* class_value = *class_property->ContainerPtrToValuePtr<UClass*>(object);

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FClassProperty* super_class_property = static_cast<FClassProperty*>(super_property);
                UClass* super_class_value = *super_class_property->ContainerPtrToValuePtr<UClass*>(super_object);
                if (class_value == super_class_value)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            if (class_value == NULL)
            {
                // If class value is NULL, generate a simple NULL assignment
                if (!super_and_no_access)
                {
                    generate_simple_assignment_expression(property, SYSSTR("NULL"), implementation_file, property_scope);
                }
                else
                {
                    generate_advanced_assignment_expression(property, SYSSTR("NULL"), implementation_file, property_scope, property_type);
                }
            }
            else if ((class_value->GetClassFlags() & CLASS_Native) != 0)
            {
                implementation_file.add_dependency_object(class_value, DependencyLevel::Include);

                // Otherwise, generate StaticClass call, assuming the class is native
                const SystemStringType object_class_name = get_native_class_name(class_value);
                const SystemStringType initializer = std::format(SYSSTR("{}::StaticClass()"), object_class_name);

                if (!super_and_no_access)
                {
                    generate_simple_assignment_expression(property, initializer, implementation_file, property_scope);
                }
                else
                {
                    generate_advanced_assignment_expression(property, initializer, implementation_file, property_scope, property_type);
                }
            }
            else
            {
                // Unhandled case, reference to the non-native blueprint class potentially?
                Output::send(SYSSTR("Unhandled default value of the FClassProperty {}: {}\n"), to_system(property->GetFullName()), to_system(class_value->GetFullName()));
            }
            return;
        }

        // Object Properties are handled either as NULL, default subobjects, or UClass objects
        if (property->IsA<FObjectProperty>())
        {
            FObjectProperty* object_property = static_cast<FObjectProperty*>(property);
            UObject* sub_object_value = *object_property->ContainerPtrToValuePtr<UObject*>(object);
            FObjectProperty* super_object_property;
            UObject* super_sub_object_value = NULL;

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                super_object_property = static_cast<FObjectProperty*>(super_property);
                super_sub_object_value = *super_object_property->ContainerPtrToValuePtr<UObject*>(super_object);
                if (sub_object_value == NULL && super_sub_object_value == NULL)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            if (sub_object_value == NULL)
            {
                // If object value is NULL, generate a simple NULL constant
                // TODO: Needs additional checks to see if the class is abstract to potentially change this to a default object init
                if (!super_and_no_access)
                {
                    generate_simple_assignment_expression(property, SYSSTR("NULL"), implementation_file, property_scope);
                }
                else
                {
                    generate_advanced_assignment_expression(property, SYSSTR("NULL"), implementation_file, property_scope, property_type);
                }
                return;
            }

            if (sub_object_value->HasAnyFlags(EObjectFlags::RF_DefaultSubObject))
            {
                UClass* object_class_type = sub_object_value->GetClassPrivate();
                const auto object_name = to_system(sub_object_value->GetName());

                UClass* super_object_class_type{};
                // Additional checks to ensure this property needs to be initialized in the current class
                if (super_sub_object_value)
                {
                    super_object_class_type = super_sub_object_value->GetClassPrivate();
                    const auto super_object_name = to_system(super_sub_object_value->GetName());
                    if ((object_class_type == super_object_class_type) && (object_name == super_object_name))
                    {
                        return;
                    }
                    super_and_no_access = private_access_modifier;
                }

                bool parent_component_found = false;
                SystemStringType prior_property_variable{};

                // Check to see if any other property in the super initialized a component with the same name to ensure
                // we are not creating the subobject in a child class unnecessarily.
                if (super_object && !super_property)
                {
                    for (FProperty* check_super_property : super->OrderedForEachPropertyInChain())
                    {
                        if (check_super_property->IsA<FObjectProperty>())
                        {
                            FObjectProperty* check_super_object_property = static_cast<FObjectProperty*>(check_super_property);
                            UObject* check_super_sub_object_value = *check_super_object_property->ContainerPtrToValuePtr<UObject*>(super_object);
                            if (check_super_sub_object_value)
                            {
                                auto check_super_object_name = to_system(check_super_sub_object_value->GetName());
                                if (check_super_object_name == object_name)
                                {
                                    parent_component_found = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                // Generate an initializer by either setting this property to a pre-existing property
                // overriding the object class of an existing component, or creating a new default subobject
                SystemStringType initializer{};
                if (auto it = m_class_subobjects.find(object_name); it != m_class_subobjects.end())
                {
                    // Set property to equal previous property referencing the same object
                    initializer = it->second;
                    FProperty* prior_property = ustruct->GetPropertyByNameInChain(to_ue(initializer).c_str());
                    bool prior_private = get_property_access_modifier(prior_property) == AccessModifier::Private;
                    if (prior_private)
                    {
                        UObject* check_sub_object_value = *prior_property->ContainerPtrToValuePtr<UObject*>(object);
                        SystemStringType prior_prop_class_name = SYSSTR("NULL");
                        if (check_sub_object_value->GetClassPrivate() != nullptr)
                        {
                            prior_prop_class_name = get_native_class_name(check_sub_object_value->GetClassPrivate());
                        }
                        implementation_file.append_line(
                                std::format(SYSSTR("FProperty* p_{}_Prior = GetClass()->FindPropertyByName(\"{}\");"), initializer, initializer));
                        initializer = std::format(SYSSTR("*p_{}_Prior->ContainerPtrToValuePtr<{}*>(this)"), initializer, prior_prop_class_name);
                    }
                    if (!super_and_no_access)
                    {
                        initializer = std::format(SYSSTR("({}*){}"), get_native_class_name(object_property->GetPropertyClass()), initializer);
                        generate_simple_assignment_expression(property, initializer, implementation_file, property_scope);
                    }
                    else
                    {
                        initializer = std::format(SYSSTR("({}*){}"), get_native_class_name(object_property->GetPropertyClass()), initializer);
                        generate_advanced_assignment_expression(property, initializer, implementation_file, property_scope, property_type);
                    }
                }
                else if ((super_object_class_type && object_class_type != super_object_class_type) || parent_component_found)
                {
                    // Add an objectinitializer default subobject class override to the constructor
                    implementation_file.add_dependency_object(object_class_type, DependencyLevel::Include);
                    implementation_file.m_implementation_constructor.append(
                            std::format(SYSSTR(".SetDefaultSubobjectClass<{}>(TEXT(\"{}\"))"), get_native_class_name(object_class_type), object_name));
                            m_class_subobjects.try_emplace(object_name, to_system(property->GetName()));
                }
                else
                {
                    // Generate a CreateDefaultSubobject function call
                    implementation_file.add_dependency_object(object_class_type, DependencyLevel::Include);
                    const SystemStringType object_class_name = get_native_class_name(object_class_type);
                    initializer = std::format(SYSSTR("CreateDefaultSubobject<{}>(TEXT(\"{}\"))"), object_class_name, object_name);
                    m_class_subobjects.try_emplace(object_name, to_system(property->GetName()));
                    if (!super_and_no_access)
                    {
                        generate_simple_assignment_expression(property, initializer, implementation_file, property_scope);
                    }
                    else
                    {
                        generate_advanced_assignment_expression(property, initializer, implementation_file, property_scope, property_type);
                    }
                }

                FObjectProperty* attach_parent_property = static_cast<FObjectProperty*>(sub_object_value->GetPropertyByNameInChain(STR("AttachParent")));
                UObject* attach_parent_object_value{};
                if (attach_parent_property)
                {
                    attach_parent_object_value = *attach_parent_property->ContainerPtrToValuePtr<UObject*>(sub_object_value);
                }
                if (attach_parent_object_value != NULL)
                {
                    const auto attach_parent_object_name = to_system(attach_parent_object_value->GetName());
                    const SystemStringType operator_type = SYSSTR("->");
                    bool parent_found = false;
                    SystemStringType attach_string;
                    if (auto it = m_class_subobjects.find(attach_parent_object_name); it != m_class_subobjects.end())
                    {
                        // Set property to equal previous property referencing the same object
                        attach_string = std::format(SYSSTR("SetupAttachment({})"), it->second);
                        parent_found = true;
                    }
                    else if (as_class)
                    {
                        for (FProperty* check_property : as_class->OrderedForEachPropertyInChain())
                        {
                            if (check_property->IsA<FObjectProperty>())
                            {
                                FObjectProperty* check_object_property = static_cast<FObjectProperty*>(check_property);
                                UObject* check_sub_object_value = *check_object_property->ContainerPtrToValuePtr<UObject*>(object);
                                if (check_sub_object_value)
                                {
                                    auto check_object_name = to_system(check_sub_object_value->GetName());
                                    if (check_object_name == attach_parent_object_name)
                                    {
                                        if (get_property_access_modifier(check_object_property) != AccessModifier::Private)
                                        {
                                            attach_string = std::format(SYSSTR("SetupAttachment({})"), to_system(check_property->GetName()));
                                        }
                                        else
                                        {
                                            auto parent_property_name = std::format(SYSSTR("const FProperty* p_{}_Parent = GetClass()->FindPropertyByName(\"{}\");"),
                                                                                       to_system(check_property->GetName()),
                                                                                       to_system(check_property->GetName()));
                                            if (!implementation_file.parent_property_names.contains(parent_property_name))
                                            {
                                                implementation_file.parent_property_names.emplace(parent_property_name);
                                                implementation_file.append_line(parent_property_name);
                                            }
                                            attach_string = std::format(SYSSTR("SetupAttachment(p_{}_Parent->ContainerPtrToValuePtr<{}>(this))"),
                                                                        to_system(check_property->GetName()),
                                                                        get_native_class_name(check_sub_object_value->GetClassPrivate()));
                                            implementation_file.add_dependency_object(check_sub_object_value->GetClassPrivate(), DependencyLevel::Include);
                                        }
                                        parent_found = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (parent_found)
                    {
                        if (!super_and_no_access)
                        {
                            implementation_file.attachments.try_emplace(property, GeneratedSourceFile::attachment_data {property_type, attach_string, false});
                        }
                        else
                        {
                            implementation_file.attachments.try_emplace(property, GeneratedSourceFile::attachment_data {property_type, attach_string, true});
                        }
                    }
                }
                return;
            }

            if (UClass* sub_object_as_class = Cast<UClass>(sub_object_value))
            {
                // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
                if (super_sub_object_value != nullptr)
                {
                    UClass* super_sub_object_as_class = Cast<UClass>(sub_object_value);
                    if (sub_object_as_class == super_sub_object_as_class)
                    {
                        return;
                    }
                    super_and_no_access = private_access_modifier;
                }

                // Generate a ::StaticClass call if this object represents a class
                implementation_file.add_dependency_object(sub_object_as_class, DependencyLevel::Include);

                const SystemStringType object_class_name = get_native_class_name(sub_object_as_class);
                const SystemStringType initializer = std::format(SYSSTR("{}::StaticClass()"), object_class_name);
                if (!super_and_no_access)
                {
                    generate_simple_assignment_expression(property, initializer, implementation_file, property_scope);
                }
                else
                {
                    generate_advanced_assignment_expression(property, initializer, implementation_file, property_scope, property_type);
                }
                return;
            }

            // Unhandled case, might be some external object reference
            Output::send(SYSSTR("Unhandled default value of the FObjectProperty {}: {}\n"), to_system(property->GetFullName()), to_system(sub_object_value->GetFullName()));

            return;
        }

        // Struct properties are serialization as normal struct constructors with custom scope
        // TODO there are a lot of issues with that regarding member access, unnecessary assignments and so on
        /*if (FieldClassName == SYSSTR("StructProperty")) {
            XStructProperty* StructProperty = static_cast<XStructProperty*>(Property);
            UScriptStruct* ScriptStruct = StructProperty->get_script_struct();

            if (ScriptStruct == NULL) {
                throw std::runtime_error(RC::fmt("Struct is NULL for StructProperty %S", Property->GetName().c_str()));
            }

            void* StructDataPointer = StructProperty->container_ptr_to_value_ptr<void>(Object);
            const SystemStringType NewPropertyScope = std::format(SYSSTR("{}{}."), PropertyScope, StructProperty->GetName());

            //Generate values for each struct property
            //TODO we do not really need to generate assignments for each struct member, we only really need members that are different from the constructor set
        defaults ScriptStruct->ForEachProperty([&](FProperty* NestedStructProperty) { GeneratePropertyValue(NestedStructProperty, StructDataPointer,
        ImplementationFile, NewPropertyScope); return RC::LoopAction::Continue;
            });
            return;
        }*/

        if (property->IsA<FArrayProperty>())
        {
            FScriptArray* property_value = property->ContainerPtrToValuePtr<FScriptArray>(object);

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FScriptArray* super_property_value = super_property->ContainerPtrToValuePtr<FScriptArray>(super_object);
                if (property_value == super_property_value)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            // Reserve the needed amount of elements
            if (property_value->Num() > 0)
            {
                if (!super_and_no_access)
                {
                    implementation_file.append_line(std::format(SYSSTR("{}{}.AddDefaulted({});"), property_scope, to_system(property->GetName()), (int32_t)property_value->Num()));
                }
                else
                {
                    // TODO
                }
            }
            return;
        }

        // TODO: Support collection/map properties initialization later
        if (property->IsA<FSetProperty>())
        {
            return;
        }
        if (property->IsA<FMapProperty>())
        {
            return;
        }

        if (property->IsA<FNumericProperty>())
        {
            FNumericProperty* numeric_property = static_cast<FNumericProperty*>(property);

            // Ensure property either does not exist in parent class or is overriden in the CDO for the child class
            if (super_property != nullptr)
            {
                FNumericProperty* super_numeric_property = static_cast<FNumericProperty*>(super_property);
                if (numeric_property == super_numeric_property)
                {
                    return;
                }
                super_and_no_access = private_access_modifier;
            }

            SystemStringType number_constant_string;
            if (!numeric_property->IsFloatingPoint())
            {
                int64 value = numeric_property->GetSignedIntPropertyValue(numeric_property->ContainerPtrToValuePtr<int64>(object));
                number_constant_string = to_system( std::to_string(value));
            }
            else
            {
                double value = numeric_property->GetFloatingPointPropertyValue(numeric_property->ContainerPtrToValuePtr<double>(object));
                number_constant_string = std::format(SYSSTR("{:.2f}f"), value);
            }
            if (!super_and_no_access)
            {
                generate_simple_assignment_expression(property, number_constant_string, implementation_file, property_scope);
            }
            else
            {
                generate_advanced_assignment_expression(property, number_constant_string, implementation_file, property_scope, property_type);
            }
            return;
        }
    }

    auto UEHeaderGenerator::generate_function_implementation(UClass* uclass,
                                                             UFunction* function,
                                                             GeneratedSourceFile& implementation_file,
                                                             bool is_generating_interface,
                                                             const CaseInsensitiveSet& blacklisted_property_names) -> void
    {
        const auto class_native_name = get_native_class_name(uclass, is_generating_interface);
        const auto raw_function_name = to_system(function->GetName());
        auto function_flags = function->GetFunctionFlags();
        PropertyTypeDeclarationContext context(to_system(uclass->GetName()), &implementation_file);

        SystemStringType function_implementation_name;
        SystemStringType net_validate_function_name;
        bool is_input_function_const = ((function_flags)&FUNC_Const) != 0;

        if ((function_flags & FUNC_Net) != 0)
        {
            // Network functions always have the implementation inside the _Implementation function
            function_implementation_name = std::format(SYSSTR("{}::{}_Implementation"), class_native_name, raw_function_name);

            // Validated network functions by default have their validation function name set to _Validate
            if ((function_flags & FUNC_NetValidate) != 0)
            {
                net_validate_function_name = std::format(SYSSTR("{}::{}_Validate"), class_native_name, raw_function_name);
            }
        }
        else if ((function_flags & FUNC_BlueprintEvent) != 0)
        {
            // Blueprint Events use _Implementation by default too, but only BlueprintNativeEvents.
            // BlueprintImplementableEvents do not have any native functions at all, they're just thunks
            if ((function_flags & FUNC_Native) != 0)
            {
                function_implementation_name = std::format(SYSSTR("{}::{}_Implementation"), class_native_name, raw_function_name);
            }
        }
        else
        {
            // Otherwise, normal UFunctions get a standard name matching the function in question
            function_implementation_name = std::format(SYSSTR("{}::{}"), class_native_name, raw_function_name);
        }

        SystemStringType function_parameter_list;
        if (!function_implementation_name.empty() || !net_validate_function_name.empty())
        {
            function_parameter_list =
                    generate_function_parameter_list(uclass, function, implementation_file, false, context.context_name, blacklisted_property_names);
        }

        if (!function_implementation_name.empty())
        {
            FProperty* return_value_property = function->GetReturnProperty();

            const SystemStringType return_value_type = return_value_property ? generate_property_type_declaration(return_value_property, context) : SYSSTR("void");

            implementation_file.append_line(std::format(SYSSTR("{} {}({}){} {{"),
                                                        return_value_type,
                                                        function_implementation_name,
                                                        function_parameter_list,
                                                        is_input_function_const ? SYSSTR(" const") : SYSSTR("")));
            implementation_file.begin_indent_level();

            if (return_value_property != NULL)
            {
                const SystemStringType default_value = generate_default_property_value(return_value_property, implementation_file, context.context_name);
                implementation_file.append_line(std::format(SYSSTR("return {};"), default_value));
            }

            implementation_file.end_indent_level();
            implementation_file.append_line(SYSSTR("}"));
        }

        if (!net_validate_function_name.empty())
        {
            implementation_file.append_line(std::format(SYSSTR("bool {}({}) {{"), net_validate_function_name, function_parameter_list));
            implementation_file.begin_indent_level();

            implementation_file.append_line(SYSSTR("return true;"));

            implementation_file.end_indent_level();
            implementation_file.append_line(SYSSTR("}"));
        }
    }

    auto UEHeaderGenerator::generate_parameter_count_string(int32_t parameter_count) -> SystemStringType
    {
        switch (parameter_count)
        {
        case 0:
            return SYSSTR("");
        case 1:
            return SYSSTR("_OneParam");
        case 2:
            return SYSSTR("_TwoParams");
        case 3:
            return SYSSTR("_ThreeParams");
        case 4:
            return SYSSTR("_FourParams");
        case 5:
            return SYSSTR("_FiveParams");
        case 6:
            return SYSSTR("_SixParams");
        case 7:
            return SYSSTR("_SevenParams");
        case 8:
            return SYSSTR("_EightParams");
        case 9:
            return SYSSTR("_NineParams");
        default:
            return SYSSTR("_TooMany");
        }
    }

    auto UEHeaderGenerator::append_access_modifier(GeneratedSourceFile& header_data, AccessModifier needed_access, AccessModifier& current_access) -> void
    {
        if (current_access != needed_access)
        {
            current_access = needed_access;

            if (needed_access == AccessModifier::Public)
            {
                header_data.append_line_no_indent(SYSSTR("public:"));
            }
            else if (needed_access == AccessModifier::Protected)
            {
                header_data.append_line_no_indent(SYSSTR("protected:"));
            }
            else if (needed_access == AccessModifier::Private)
            {
                header_data.append_line_no_indent(SYSSTR("private:"));
            }
        }
    }

    auto UEHeaderGenerator::get_function_access_modifier(UFunction* function) -> AccessModifier
    {
        auto function_flags = function->GetFunctionFlags();

        if ((function_flags & FUNC_Private) != 0)
        {
            return AccessModifier::Private;
        }
        else if ((function_flags & FUNC_Protected) != 0)
        {
            return AccessModifier::Protected;
        }
        else if ((function_flags & FUNC_Public) != 0)
        {
            return AccessModifier::Public;
        }
        return AccessModifier::Public;
    }

    auto UEHeaderGenerator::get_property_access_modifier(FProperty* property) -> AccessModifier
    {
        auto property_flags = property->GetPropertyFlags();

        if ((property_flags & CPF_NativeAccessSpecifierPublic) != 0)
        {
            return AccessModifier::Public;
        }
        else if ((property_flags & CPF_NativeAccessSpecifierProtected) != 0)
        {
            return AccessModifier::Protected;
        }
        else if ((property_flags & CPF_NativeAccessSpecifierPrivate) != 0)
        {
            return AccessModifier::Private;
        }
        return AccessModifier::Public;
    }

    auto UEHeaderGenerator::create_string_literal(const SystemStringType& string) -> SystemStringType
    {
        SystemStringType result;
        result.append(SYSSTR("TEXT(\""));

        bool previous_character_was_hex = false;

        const SystemCharType* ptr = string.c_str();
        while (SystemCharType ch = *ptr++)
        {
            switch (ch)
            {
            case SYSSTR('\r'): {
                continue;
            }
            case SYSSTR('\n'): {
                result.append(SYSSTR("\\n"));
                previous_character_was_hex = false;
                break;
            }
            case SYSSTR('\\'): {
                result.append(SYSSTR("\\\\"));
                previous_character_was_hex = false;
                break;
            }
            case SYSSTR('\"'): {
                result.append(SYSSTR("\\\""));
                previous_character_was_hex = false;
                break;
            }
            default: {
                if ((unsigned char)ch < 31 || (unsigned char)ch >= 128)
                {
                    result.append(std::format(SYSSTR("\\x{:04X}"), ch));
                    previous_character_was_hex = true;
                }
                else
                {
                    // We close and open the literal (with TEXT) here in order to ensure that successive hex characters aren't
                    // appended to the hex sequence, causing a different number
                    if (previous_character_was_hex && iswxdigit(ch) != 0)
                    {
                        result.append(SYSSTR("\")TEXT(\""));
                    }
                    previous_character_was_hex = false;
                    result.push_back(ch);
                }
                break;
            }
            }
        }
        result.append(SYSSTR("\")"));
        return result;
    }

    auto UEHeaderGenerator::convert_module_name_to_api_name(const SystemStringType& module_name) -> SystemStringType
    {
        SystemStringType uppercase_string = string_to_uppercase(module_name);
        uppercase_string.append(SYSSTR("_API"));
        return uppercase_string;
    }

    auto UEHeaderGenerator::add_module_and_sub_module_dependencies(std::set<SystemStringType>& out_module_dependencies,
                                                                   const SystemStringType& module_name,
                                                                   bool add_self_module) -> void
    {
        // Prevent infinite recursion
        if (out_module_dependencies.contains(module_name))
        {
            return;
        }

        if (add_self_module)
        {
            out_module_dependencies.insert(module_name);
        }
        const auto iterator = m_module_dependencies.find(module_name);
        if (iterator != m_module_dependencies.end())
        {
            for (const SystemStringType& DependencyModuleName : *iterator->second)
            {
                out_module_dependencies.insert(DependencyModuleName);
            }
        }
    }

    auto UEHeaderGenerator::collect_blacklisted_property_names(UObject* uclass) -> CaseInsensitiveSet
    {
        CaseInsensitiveSet result_set;

        if (uclass->GetClassPrivate()->IsChildOf(UClass::StaticClass()))
        {
            UClass* class_object = static_cast<UClass*>(uclass);

            for (FProperty* property : class_object->ForEachProperty())
            {
                result_set.insert(to_system(property->GetName()));
            }

            for (UFunction* function : class_object->ForEachFunction())
            {
                result_set.insert(to_system(function->GetName()));
            }
        }
        else if (uclass->GetClassPrivate()->IsChildOf(UScriptStruct::StaticClass()))
        {
            UScriptStruct* script_struct = static_cast<UScriptStruct*>(uclass);

            for (FProperty* property : script_struct->ForEachProperty())
            {
                result_set.insert(to_system(property->GetName()));
            }
        }
        return result_set;
    }

    // TODO CannotImplementInterfaceInBlueprint is not exactly right,
    // TODO you can have interface with no implementable blueprint methods but that you can still implement in blueprint
    auto UEHeaderGenerator::generate_interface_flags(UClass* uinterface) const -> SystemStringType
    {
        FlagFormatHelper flag_format_helper{};

        auto class_flags = uinterface->GetClassFlags();
        UClass* super_interface = uinterface->GetSuperClass();
        auto parent_class_flags = super_interface->GetClassFlags();

        auto class_own_flags = (EClassFlags)(class_flags & (~(parent_class_flags & CLASS_Inherit)));

        if ((class_own_flags & CLASS_MinimalAPI) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("MinimalAPI"));
        }

        ClassBlueprintInfo blueprint_info = get_class_blueprint_info(uinterface);
        ClassBlueprintInfo parent_blueprint_info = get_class_blueprint_info(super_interface);

        if (blueprint_info.is_blueprintable)
        {
            if (!parent_blueprint_info.is_blueprintable)
            {
                flag_format_helper.add_switch(SYSSTR("Blueprintable"));
            }
        }
        else if (blueprint_info.is_blueprint_type)
        {
            if (!parent_blueprint_info.is_blueprint_type)
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintType"));
            }
            flag_format_helper.get_meta()->add_switch(SYSSTR("CannotImplementInterfaceInBlueprint"));
        }

        return flag_format_helper.build_flag_string();
    }

    auto UEHeaderGenerator::generate_class_flags(UClass* uclass) const -> SystemStringType
    {
        FlagFormatHelper flag_format_helper{};

        auto class_flags = uclass->GetClassFlags();
        UClass* super_class = uclass->GetSuperClass();
        auto parent_class_flags = super_class ? super_class->GetClassFlags() : EClassFlags::CLASS_None;

        auto class_own_flags = (EClassFlags)(class_flags & (~(parent_class_flags & CLASS_Inherit)));

        if ((class_own_flags & CLASS_MinimalAPI) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("MinimalAPI"));
        }

        ClassBlueprintInfo blueprint_info = get_class_blueprint_info(uclass);
        ClassBlueprintInfo parent_blueprint_info{};
        if (super_class != NULL)
        {
            parent_blueprint_info = get_class_blueprint_info(super_class);
        }

        if (UE4SSProgram::settings_manager.UHTHeaderGenerator.MakeAllPropertyBlueprintsReadWrite)
        {
            flag_format_helper.add_switch(SYSSTR("Blueprintable"));
        }
        else
        {
            if (blueprint_info.is_blueprintable)
            {
                if (!parent_blueprint_info.is_blueprintable)
                {
                    flag_format_helper.add_switch(SYSSTR("Blueprintable"));
                }
            }
            else if (blueprint_info.is_blueprint_type)
            {
                if (!parent_blueprint_info.is_blueprint_type)
                {
                    flag_format_helper.add_switch(SYSSTR("BlueprintType"));
                }
            }
        }

        if ((class_own_flags & CLASS_Deprecated) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Deprecated"));
        }
        if ((class_own_flags & CLASS_Abstract) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Abstract"));
        }

        if ((class_own_flags & CLASS_MinimalAPI) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("MinimalAPI"));
        }
        if ((class_own_flags & CLASS_NoExport) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NoExport"));
        }
        // TODO not quite the case, because UHT boilerplate implicitly marks every native class as CLASS_Intrinsic
        // if ((ClassOwnFlags & CLASS_Intrinsic) != 0) {
        //     FlagFormatHelper.AddSwitch(SYSSTR("Intrinsic"));
        // }

        if ((class_own_flags & CLASS_Const) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Const"));
        }
        if ((class_own_flags & CLASS_DefaultToInstanced) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("DefaultToInstanced"));
        }

        UClass* class_within = uclass->GetClassWithin();
        if (class_within != NULL && class_within != UObject::StaticClass() && (super_class == NULL || class_within != super_class->GetClassWithin()))
        {
            flag_format_helper.add_parameter(SYSSTR("Within"), to_system(class_within->GetName()));
        }

        if ((class_own_flags & CLASS_Transient) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Transient"));
        }
        else if ((parent_class_flags & CLASS_Transient) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NonTransient"));
        }

        if ((class_own_flags & CLASS_EditInlineNew) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("EditInlineNew"));
        }
        else if ((class_flags & CLASS_EditInlineNew) == 0 && (parent_class_flags & CLASS_EditInlineNew) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NotEditInlineNew"));
        }

        if ((class_own_flags & CLASS_NotPlaceable) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NotPlaceable"));
        }
        else if ((class_flags & CLASS_NotPlaceable) == 0 && (parent_class_flags & CLASS_NotPlaceable) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Placeable"));
        }

        bool add_config_name{false};

        if ((class_own_flags & CLASS_DefaultConfig) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("DefaultConfig"));
            add_config_name = true;
        }
        if ((class_own_flags & CLASS_GlobalUserConfig) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("GlobalUserConfig"));
            add_config_name = true;
        }
        if ((class_own_flags & CLASS_ProjectUserConfig) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("ProjectUserConfig"));
            add_config_name = true;
        }

        for (FProperty* property : uclass->ForEachProperty())
        {
            if ((property->GetPropertyFlags() & CPF_Config) != 0 || (property->GetPropertyFlags() & CPF_GlobalConfig) != 0)
            {
                add_config_name = true;
            }
        }

        const UEStringType class_config_name = uclass->GetClassConfigName().ToString();
        if (super_class == NULL || class_config_name != super_class->GetClassConfigName().ToString())
        {
            flag_format_helper.add_parameter(SYSSTR("Config"), to_system(class_config_name));
            // Don't add our override config if we add the real one here
            add_config_name = false;
        }

        if (UE4SSProgram::settings_manager.UHTHeaderGenerator.MakeAllConfigsEngineConfig && add_config_name)
        {
            flag_format_helper.add_parameter(SYSSTR("Config"), SYSSTR("Engine"));
        }

        if ((class_own_flags & CLASS_PerObjectConfig) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("PerObjectConfig"));
        }
        if ((class_own_flags & CLASS_ConfigDoNotCheckDefaults) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("ConfigDoNotCheckDefaults"));
        }

        if ((class_own_flags & CLASS_HideDropDown) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("HideDropdown"));
        }

        if ((class_own_flags & CLASS_CollapseCategories) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("CollapseCategories"));
        }
        else if ((parent_class_flags & CLASS_CollapseCategories) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("DontCollapseCategories"));
        }

        // Mark all UActorComponent derived classes as BlueprintSpawnableComponent by default
        // This will allow using them inside the Simple Construction Script of the blueprint assets
        if (uclass->IsChildOf<UActorComponent>())
        {
            flag_format_helper.get_meta()->add_switch(SYSSTR("BlueprintSpawnableComponent"));
            flag_format_helper.add_parameter(SYSSTR("ClassGroup"), SYSSTR("Custom"));
        }

        return flag_format_helper.build_flag_string();
    }

    /**/
    auto UEHeaderGenerator::generate_property_type_declaration(FProperty* property, const PropertyTypeDeclarationContext& context) -> SystemStringType
    {
        UClass* current_class = Unreal::Cast<UClass>(property->GetOutermostOwner());
        const UEStringType field_class_name = property->GetClass().GetName();

        // Byte Property
        if (property->IsA<FByteProperty>())
        {
            FByteProperty* byte_property = static_cast<FByteProperty*>(property);
            UEnum* enum_value = byte_property->GetEnum();
            
            if (enum_value != NULL)
            {
                if (context.source_file != NULL)
                {
                    context.source_file->add_dependency_object(enum_value, DependencyLevel::Include);
                }

                const SystemStringType enum_type_name = get_native_enum_name(enum_value);

                if ((property->GetPropertyFlags() & CPF_BlueprintVisible) != 0)
                {
                    this->m_blueprint_visible_enums.insert(enum_type_name);
                }

                // Non-EnumClass enumerations should be wrapped into TEnumAsByte according to UHT, but implicit uint8s should not use TEnumAsByte
                return std::format(SYSSTR("TEnumAsByte<{}>"), enum_type_name);
            }
            return SYSSTR("uint8");
        }

        // Enum Property
        if (property->IsA<FEnumProperty>())
        {
            FEnumProperty* enum_property = static_cast<FEnumProperty*>(property);
            FNumericProperty* underlying_property = enum_property->GetUnderlyingProperty();
            UEnum* uenum = enum_property->GetEnum();
            if (uenum == NULL)
            {
                throw std::runtime_error(RC::fmt("EnumProperty {} does not have a valid Enum value", property->GetName()));
            }
            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(uenum, DependencyLevel::Include);
            }
            const SystemStringType enum_type_name = get_native_enum_name(uenum);

            if ((property->GetPropertyFlags() & CPF_BlueprintVisible) != 0)
            {
                this->m_blueprint_visible_enums.insert(enum_type_name);
            }

            const SystemStringType underlying_enum_type = generate_property_type_declaration(underlying_property, context);
            this->m_underlying_enum_types.insert({enum_type_name, underlying_enum_type});
            return enum_type_name;
        }

        // Bool Property
        if (property->IsA<FBoolProperty>())
        {
            FBoolProperty* bool_property = static_cast<FBoolProperty*>(property);
            if (context.is_top_level_declaration && context.out_is_bitmask_bool != NULL)
            {
                if (bool_property->GetFieldMask() != 255)
                {
                    *context.out_is_bitmask_bool = true;
                    return SYSSTR("uint8");
                }
            }
            return SYSSTR("bool");
        }

        // Standard Numeric Properties
        if (property->IsA<FInt8Property>())
        {
            return SYSSTR("int8");
        }
        else if (property->IsA<FInt16Property>())
        {
            return SYSSTR("int16");
        }
        else if (property->IsA<FIntProperty>())
        {
            return SYSSTR("int32");
        }
        else if (property->IsA<FInt64Property>())
        {
            return SYSSTR("int64");
        }
        else if (property->IsA<FUInt16Property>())
        {
            return SYSSTR("uint16");
        }
        else if (property->IsA<FUInt32Property>())
        {
            return SYSSTR("uint32");
        }
        else if (property->IsA<FUInt64Property>())
        {
            return SYSSTR("uint64");
        }
        else if (property->IsA<FFloatProperty>())
        {
            return SYSSTR("float");
        }
        else if (property->IsA<FDoubleProperty>())
        {
            return SYSSTR("double");
        }

        // Class Properties
        if (property->IsA<FClassProperty>() || property->IsA<FAssetClassProperty>())
        {
            FClassProperty* class_property = static_cast<FClassProperty*>(property);
            UClass* meta_class = class_property->GetMetaClass();

            if (meta_class == NULL || meta_class == UObject::StaticClass())
            {
                return SYSSTR("UClass*");
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(meta_class, DependencyLevel::PreDeclaration);
                context.source_file->add_extra_include(SYSSTR("Templates/SubclassOf.h"));
            }
            const SystemStringType meta_class_name = get_native_class_name(meta_class, false);

            return std::format(SYSSTR("TSubclassOf<{}>"), meta_class_name);
        }

        if (auto* class_property = CastField<FClassPtrProperty>(property); class_property)
        {
            // TODO: Confirm that this is accurate
            return SYSSTR("TClassPtr<UClass>");
        }

        if (property->IsA<FSoftClassProperty>())
        {
            FSoftClassProperty* soft_class_property = static_cast<FSoftClassProperty*>(property);
            UClass* meta_class = soft_class_property->GetMetaClass();

            if (meta_class == NULL || meta_class == UObject::StaticClass())
            {
                return SYSSTR("TSoftClassPtr<UObject>");
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(meta_class, DependencyLevel::PreDeclaration);
            }
            const SystemStringType meta_class_name = get_native_class_name(meta_class, false);

            return std::format(SYSSTR("TSoftClassPtr<{}>"), meta_class_name);
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
                return SYSSTR("UObject*");
            }
            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(property_class, DependencyLevel::PreDeclaration);
            }
            const SystemStringType property_class_name = get_native_class_name(property_class, false);

            return std::format(SYSSTR("{}*"), property_class_name);
        }

        if (auto* object_property = CastField<FObjectPtrProperty>(property); object_property)
        {
            auto* property_class = object_property->GetPropertyClass();

            if (!property_class)
            {
                return SYSSTR("TObjectPtr<UObject>");
            }
            else
            {
                if (context.source_file)
                {
                    context.source_file->add_dependency_object(property_class, DependencyLevel::PreDeclaration);
                }

                const auto property_class_name = get_native_class_name(property_class, false);
                return std::format(SYSSTR("TObjectPtr<{}>"), property_class_name);
            }
        }

        if (property->IsA<FWeakObjectProperty>())
        {
            FWeakObjectProperty* weak_object_property = static_cast<FWeakObjectProperty*>(property);
            UClass* property_class = weak_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return SYSSTR("TWeakObjectPtr<UObject>");
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(property_class, DependencyLevel::PreDeclaration);
            }
            const SystemStringType property_class_name = get_native_class_name(property_class, false);

            return std::format(SYSSTR("TWeakObjectPtr<{}>"), property_class_name);
        }

        if (property->IsA<FLazyObjectProperty>())
        {
            FLazyObjectProperty* lazy_object_property = static_cast<FLazyObjectProperty*>(property);
            UClass* property_class = lazy_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return SYSSTR("TLazyObjectPtr<UObject>");
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(property_class, DependencyLevel::PreDeclaration);
            }
            const SystemStringType property_class_name = get_native_class_name(property_class, false);

            return std::format(SYSSTR("TLazyObjectPtr<{}>"), property_class_name);
        }

        if (property->IsA<FSoftObjectProperty>())
        {
            FSoftObjectProperty* soft_object_property = static_cast<FSoftObjectProperty*>(property);
            UClass* property_class = soft_object_property->GetPropertyClass();

            if (property_class == NULL)
            {
                return SYSSTR("TSoftObjectPtr<UObject>");
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(property_class, DependencyLevel::PreDeclaration);
            }
            const SystemStringType property_class_name = get_native_class_name(property_class, false);

            return std::format(SYSSTR("TSoftObjectPtr<{}>"), property_class_name);
        }

        // Interface Property
        if (property->IsA<FInterfaceProperty>())
        {
            FInterfaceProperty* interface_property = static_cast<FInterfaceProperty*>(property);
            UClass* interface_class = interface_property->GetInterfaceClass();

            if (interface_class == NULL || interface_class == UInterface::StaticClass())
            {
                return SYSSTR("FScriptInterface");
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(interface_class, DependencyLevel::PreDeclaration);
            }
            const SystemStringType interface_class_name = get_native_class_name(interface_class, true);

            return std::format(SYSSTR("TScriptInterface<{}>"), interface_class_name);
        }

        // Struct Property
        if (property->IsA<FStructProperty>())
        {
            FStructProperty* struct_property = static_cast<FStructProperty*>(property);
            UScriptStruct* script_struct = struct_property->GetStruct();

            if (script_struct == NULL)
            {
                throw std::runtime_error(RC::fmt("Struct is NULL for StructProperty {}", property->GetName()));
            }
            const SystemStringType native_struct_name = get_native_struct_name(script_struct);

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(script_struct, DependencyLevel::Include);
            }
            this->m_blueprint_visible_structs.insert(native_struct_name);

            return native_struct_name;
        }

        // Delegate Properties
        if (property->IsA<FDelegateProperty>())
        {
            FDelegateProperty* delegate_property = static_cast<FDelegateProperty*>(property);
            UFunction* delegate_signature_function = delegate_property->GetSignatureFunction();
            if (!delegate_signature_function)
            {
                throw std::runtime_error{
                        std::format("FunctionSignature is nullptr, cannot deduce function for '{}'\n", to_string(delegate_property->GetFullName()))};
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(delegate_signature_function, DependencyLevel::Include);
            }
            return get_native_delegate_type_name(delegate_signature_function, current_class);
        }

        // In 4.23, they replaced 'MulticastDelegateProperty' with 'Inline' & 'Sparse' variants
        // It looks like the delegate macro might be the same as the 'Inline' variant in later versions, so we'll use the same branch here
        if (property->IsA<FMulticastInlineDelegateProperty>() || property->IsA<FMulticastDelegateProperty>())
        {
            FMulticastInlineDelegateProperty* delegate_property = static_cast<FMulticastInlineDelegateProperty*>(property);
            UFunction* delegate_signature_function = delegate_property->GetSignatureFunction();
            if (!delegate_signature_function)
            {
                throw std::runtime_error{
                        std::format("FunctionSignature is nullptr, cannot deduce function for '{}'\n", to_string(delegate_property->GetFullName()))};
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(delegate_signature_function, DependencyLevel::Include);
            }
            return get_native_delegate_type_name(delegate_signature_function, current_class);
        }

        if (property->IsA<FMulticastSparseDelegateProperty>())
        {
            FMulticastSparseDelegateProperty* delegate_property = static_cast<FMulticastSparseDelegateProperty*>(property);
            UFunction* delegate_signature_function = delegate_property->GetSignatureFunction();
            if (!delegate_signature_function)
            {
                throw std::runtime_error{
                        std::format("FunctionSignature is nullptr, cannot deduce function for '{}'\n", to_string(delegate_property->GetFullName()))};
            }

            if (context.source_file != NULL)
            {
                context.source_file->add_dependency_object(delegate_signature_function, DependencyLevel::Include);
            }
            return get_native_delegate_type_name(delegate_signature_function, current_class);
        }

        // Field path property
        if (property->IsA<FFieldPathProperty>())
        {
            FFieldPathProperty* field_path_property = static_cast<FFieldPathProperty*>(property);
            const auto property_class_name = to_system(field_path_property->GetPropertyClass()->GetName());
            return std::format(SYSSTR("TFieldPath<F{}>"), property_class_name);
        }

        // Collection and Map Properties
        //  TODO: This is missing support for freeze image array properties because XArrayProperty is incomplete. (low priority)
        if (property->IsA<FArrayProperty>())
        {
            FArrayProperty* array_property = static_cast<FArrayProperty*>(property);
            FProperty* inner_property = array_property->GetInner();

            const SystemStringType inner_property_type = generate_property_type_declaration(inner_property, context.inner_context());
            return std::format(SYSSTR("TArray<{}>"), inner_property_type);
        }

        if (property->IsA<FSetProperty>())
        {
            FSetProperty* set_property = static_cast<FSetProperty*>(property);
            FProperty* element_prop = set_property->GetElementProp();

            const SystemStringType element_property_type = generate_property_type_declaration(element_prop, context.inner_context());
            return std::format(SYSSTR("TSet<{}>"), element_property_type);
        }

        // TODO: This is missing support for freeze image map properties because XMapProperty is incomplete. (low priority)
        if (property->IsA<FMapProperty>())
        {
            FMapProperty* map_property = static_cast<FMapProperty*>(property);
            FProperty* key_property = map_property->GetKeyProp();
            FProperty* value_property = map_property->GetValueProp();

            const SystemStringType key_type = generate_property_type_declaration(key_property, context.inner_context());
            const SystemStringType value_type = generate_property_type_declaration(value_property, context.inner_context());

            return std::format(SYSSTR("TMap<{}, {}>"), key_type, value_type);
        }

        // Standard properties that do not have any special attributes
        if (property->IsA<FNameProperty>())
        {
            return SYSSTR("FName");
        }
        else if (property->IsA<FStrProperty>())
        {
            return SYSSTR("FString");
        }
        else if (property->IsA<FTextProperty>())
        {
            return SYSSTR("FText");
        }
        throw std::runtime_error(RC::fmt("[generate_property_type_declaration] Unsupported property class '{}', full name: '{}'",
                                         field_class_name,
                                         property->GetFullName()));
    }
    //*/

    auto UEHeaderGenerator::generate_function_argument_flags(FProperty* property) const -> SystemStringType
    {
        FlagFormatHelper flag_format_helper{};
        auto property_flags = property->GetPropertyFlags();

        // CPF_ConstParm is handled explicitly in the parameter list generator, it will generate const before parameter
        // if ((PropertyFlags & CPF_ConstParm) != 0) {
        //     FlagFormatHelper.AddSwitch(SYSSTR("Const"));
        // }

        // We only want to add UPARAM(Ref) when parameter is marked as reference AND output,
        // while not being marked as constant, because if it's marked as constant, it might be a parameter passed by const reference
        if ((property_flags & CPF_ReferenceParm) != 0 && (property_flags & CPF_OutParm) != 0 && (property_flags & CPF_ConstParm) == 0)
        {
            flag_format_helper.add_switch(SYSSTR("Ref"));
        }

        if ((property_flags & CPF_RepSkip) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NotReplicated"));
        }
        return flag_format_helper.build_flag_string();
    }

    auto UEHeaderGenerator::generate_property_flags(FProperty* property) const -> SystemStringType
    {
        FlagFormatHelper flag_format_helper{};
        auto property_flags = property->GetPropertyFlags();

        if (UE4SSProgram::settings_manager.UHTHeaderGenerator.MakeAllPropertyBlueprintsReadWrite)
        {
            flag_format_helper.add_switch(SYSSTR("EditAnywhere"));
        }
        else if ((property_flags & CPF_Edit) != 0)
        {
            if ((property_flags & CPF_EditConst) != 0)
            {
                if ((property_flags & CPF_DisableEditOnTemplate) != 0)
                {
                    flag_format_helper.add_switch(SYSSTR("VisibleInstanceOnly"));
                }
                else if ((property_flags & CPF_DisableEditOnInstance) != 0)
                {
                    flag_format_helper.add_switch(SYSSTR("VisibleDefaultsOnly"));
                }
                else
                {
                    flag_format_helper.add_switch(SYSSTR("VisibleAnywhere"));
                }
            }
            else
            {
                if ((property_flags & CPF_DisableEditOnTemplate) != 0)
                {
                    flag_format_helper.add_switch(SYSSTR("EditInstanceOnly"));
                }
                else if ((property_flags & CPF_DisableEditOnInstance) != 0)
                {
                    flag_format_helper.add_switch(SYSSTR("EditDefaultsOnly"));
                }
                else
                {
                    flag_format_helper.add_switch(SYSSTR("EditAnywhere"));
                }
            }
        }

        if ((property_flags & CPF_NoClear) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NoClear"));
        }
        if ((property_flags & CPF_EditFixedSize) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("EditFixedSize"));
        }
        if ((property_flags & CPF_SimpleDisplay) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("SimpleDisplay"));
        }
        if ((property_flags & CPF_AdvancedDisplay) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("AdvancedDisplay"));
        }

        if (UE4SSProgram::settings_manager.UHTHeaderGenerator.MakeAllPropertyBlueprintsReadWrite)
        {
            if (property->GetArrayDim() == 1 && is_subtype_valid(property))
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintReadWrite"));
            }
            flag_format_helper.get_meta()->add_parameter(SYSSTR("AllowPrivateAccess"), SYSSTR("true"));
        }
        else if ((property_flags & CPF_BlueprintVisible) != 0)
        {
            if ((property_flags & CPF_BlueprintReadOnly) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintReadOnly"));
            }
            else
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintReadWrite"));
            }
            if ((property_flags & CPF_NativeAccessSpecifierPrivate) != 0)
            {
                flag_format_helper.get_meta()->add_parameter(SYSSTR("AllowPrivateAccess"), SYSSTR("true"));
            }
        }

        if ((property_flags & CPF_BlueprintAssignable) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("BlueprintAssignable"));
        }
        if ((property_flags & CPF_BlueprintCallable) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("BlueprintCallable"));
        }
        if ((property_flags & CPF_BlueprintAuthorityOnly) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("BlueprintAuthorityOnly"));
        }

        if ((property_flags & CPF_Config) != 0)
        {
            if ((property_flags & CPF_GlobalConfig) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("GlobalConfig"));
            }
            else
            {
                flag_format_helper.add_switch(SYSSTR("Config"));
            }
        }

        if ((property_flags & CPF_Net) != 0)
        {
            if ((property_flags & CPF_RepNotify) != 0)
            {
                const auto rep_notify_func_name = to_system(property->GetRepNotifyFunc().ToString());
                flag_format_helper.add_parameter(SYSSTR("ReplicatedUsing"), rep_notify_func_name);
            }
            else
            {
                flag_format_helper.add_switch(SYSSTR("Replicated"));
            }
        }
        if ((property_flags & CPF_RepSkip) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NotReplicated"));
        }

        if ((property_flags & CPF_AssetRegistrySearchable) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("AssetRegistrySearchable"));
        }
        if ((property_flags & CPF_Interp) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Interp"));
        }
        if ((property_flags & CPF_SaveGame) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("SaveGame"));
        }
        if ((property_flags & CPF_NonTransactional) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NonTransactional"));
        }

        if ((property_flags & CPF_Transient) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Transient"));
        }
        if ((property_flags & CPF_DuplicateTransient) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("DuplicateTransient"));
        }
        if ((property_flags & CPF_TextExportTransient) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("TextExportTransient"));
        }
        if ((property_flags & CPF_NonPIEDuplicateTransient) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NonPIEDuplicateTransient"));
        }
        if ((property_flags & CPF_SkipSerialization) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("SkipSerialization"));
        }

        // Need to have all of these flags, otherwise we might accidentally get Instanced of delegate properties; CPF_ExportObject is not set for delegate properties
        uint64_t instanced_flags = CPF_ExportObject | CPF_InstancedReference;

        // Instanced Arrays use CPF_ContainsInstancedReference instead of CPF_InstancedReference
        uint64_t instanced_array_flags = CPF_ExportObject | CPF_ContainsInstancedReference;

        if (((property_flags & instanced_flags) == instanced_flags || (property_flags & instanced_array_flags) == instanced_array_flags) &&
            (property->IsA<FObjectProperty>() || (property->IsA<FArrayProperty>() && static_cast<FArrayProperty*>(property)->GetInner()->IsA<FObjectProperty>()) ||
             (property->IsA<FMapProperty>() && static_cast<FMapProperty*>(property)->GetValueProp()->IsA<FObjectProperty>())))
        {
            flag_format_helper.add_switch(SYSSTR("Instanced"));
        }
        else if ((property_flags & CPF_ExportObject) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Export"));
        }
        return flag_format_helper.build_flag_string();
    }

    auto UEHeaderGenerator::generate_struct_flags(UScriptStruct* script_struct) const -> SystemStringType
    {
        FlagFormatHelper flag_format_helper{};

        UScriptStruct* super_struct = script_struct->GetSuperScriptStruct();
        EStructFlags parent_struct_flags = (EStructFlags)(super_struct ? (super_struct->GetStructFlags() & (~STRUCT_ComputedFlags)) : STRUCT_NoFlags);
        EStructFlags struct_flags = (EStructFlags)(script_struct->GetStructFlags() & (~STRUCT_ComputedFlags));

        EStructFlags struct_own_flags = (EStructFlags)(struct_flags & (~(parent_struct_flags & STRUCT_Inherit)));

        const SystemStringType native_struct_name = get_native_struct_name(script_struct);
        if (is_struct_blueprint_type(script_struct) || m_blueprint_visible_structs.contains(native_struct_name) ||
            UE4SSProgram::settings_manager.UHTHeaderGenerator.MakeAllPropertyBlueprintsReadWrite)
        {
            flag_format_helper.add_switch(SYSSTR("BlueprintType"));
        }

        if ((struct_own_flags & STRUCT_NoExport) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("NoExport"));
        }
        if ((struct_own_flags & STRUCT_Atomic) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Atomic"));
        }
        if ((struct_own_flags & STRUCT_Immutable) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Immutable"));
        }
        return flag_format_helper.build_flag_string();
    }

    auto UEHeaderGenerator::generate_enum_flags(UEnum* uenum) const -> SystemStringType
    {
        
        FlagFormatHelper flag_format_helper{};

        auto enum_flags = uenum->GetEnumFlags();

        if ((((int32_t)enum_flags) & ((int32_t)EEnumFlags::Flags)) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Flags"));
        }
        const SystemStringType enum_native_name = get_native_enum_name(uenum);

        if (UE4SSProgram::settings_manager.UHTHeaderGenerator.MakeEnumClassesBlueprintType)
        {
            auto cpp_form = uenum->GetCppForm();
            if (cpp_form == UEnum::ECppForm::EnumClass)
            {
                const auto underlying_type = m_underlying_enum_types.find(enum_native_name);
                if (underlying_type->second == SYSSTR("uint8") ||
                    (underlying_type == m_underlying_enum_types.end() &&
                    (get_highest_enum(uenum) <= 255 &&
                    get_lowest_enum(uenum) >= 0)))
                {
                    // Underlying type is implicit or explicitly uint8.
                    flag_format_helper.add_switch(SYSSTR("BlueprintType"));
                }
            }
            else
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintType"));
            }
        }
        return flag_format_helper.build_flag_string();
    }

    auto UEHeaderGenerator::sanitize_enumeration_name(const SystemStringType& enumeration_name) -> SystemStringType
    {
        SystemStringType result_enum_name = enumeration_name;

        // Remove enumeration name from the string
        size_t enum_name_string_split = enumeration_name.find(SYSSTR("::"));
        if (enum_name_string_split != SystemStringType::npos)
        {
            result_enum_name.erase(0, enum_name_string_split + 2);
        }
        return result_enum_name;
    }

    auto UEHeaderGenerator::get_highest_enum(UEnum* uenum) -> int64_t
    {
        if (!uenum || uenum->NumEnums() <= 0)
        {
            return 0;
        }

        int64 highest_enum_value = 0;
        const auto enum_prefix = to_system(uenum->GenerateEnumPrefix());
        const auto expected_max_name = std::format(SYSSTR("{}_MAX"), enum_prefix);
        auto expected_max_name_lower = expected_max_name;
        std::transform(expected_max_name_lower.begin(), expected_max_name_lower.end(), expected_max_name_lower.begin(), ::towlower);
        
        for (auto [Name, Value] : uenum->ForEachName())
        {
            auto enum_name = sanitize_enumeration_name(to_system(Name.ToString()));
            auto enum_name_lower = enum_name;
            std::transform(enum_name_lower.begin(), enum_name_lower.end(), enum_name_lower.begin(), ::towlower);
            if ((enum_name_lower != expected_max_name_lower && enum_name_lower != sanitize_enumeration_name(expected_max_name_lower)) && Value > highest_enum_value)
            {
                highest_enum_value = Value;
            }
        }
        return highest_enum_value;
    }

    auto UEHeaderGenerator::get_lowest_enum(UEnum* uenum) -> int64_t
    {
        if (!uenum || uenum->NumEnums() <= 0)
        {
            return 0;
        }

        int64 lowest_enum_value = 0;
        for (auto [Name, Value] : uenum->ForEachName())
        {
            if (Value < lowest_enum_value)
            {
                lowest_enum_value = Value;
            }
        }
        return lowest_enum_value;
    }

    auto UEHeaderGenerator::generate_function_flags(UFunction* function, bool is_function_pure_virtual) const -> SystemStringType
    {
        FlagFormatHelper flag_format_helper{};

        auto function_flags = function->GetFunctionFlags();

        UObject* outer_object = function->GetOuterPrivate();
        bool is_interface_function = false;

        if (outer_object->GetClassPrivate()->IsChildOf(UClass::StaticClass()))
        {
            UClass* outer_class = (UClass*)outer_object;
            is_interface_function = (outer_class->GetClassFlags() & CLASS_Interface) != 0;
        }

        bool blueprint_callable_added = false;
        if ((function_flags & FUNC_BlueprintCallable) != 0)
        {
            // Interface functions cannot be BlueprintPure
            if ((function_flags & FUNC_BlueprintPure) != 0 && !is_interface_function)
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintPure"));
            }
            else
            {
                // If function is marked as FUNC_Const but not as BlueprintPure,
                // it has been explicitly marked as blueprint impure, and we need to preserve this behavior
                if ((function_flags & FUNC_Const) != 0 && !is_interface_function)
                {
                    flag_format_helper.add_parameter(SYSSTR("BlueprintPure"), SYSSTR("false"));
                }
                flag_format_helper.add_switch(SYSSTR("BlueprintCallable"));
                blueprint_callable_added = true;
            }
        }
        if (!blueprint_callable_added && UE4SSProgram::settings_manager.UHTHeaderGenerator.MakeAllFunctionsBlueprintCallable && !is_function_pure_virtual)
        {
            bool has_invalid_param{};
            for (FProperty* param : function->ForEachProperty())
            {
                if (!is_subtype_valid(param))
                {
                    has_invalid_param = true;
                    break;
                }
            }

            if (!has_invalid_param)
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintCallable"));
            }
        }

        if ((function_flags & FUNC_BlueprintEvent) != 0)
        {
            if ((function_flags & FUNC_Native) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintNativeEvent"));
            }
            else
            {
                flag_format_helper.add_switch(SYSSTR("BlueprintImplementableEvent"));
            }
        }

        if ((function_flags & FUNC_Net) != 0)
        {
            if ((function_flags & FUNC_NetServer) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("Server"));
            }
            else if ((function_flags & FUNC_NetClient) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("Client"));
            }
            else if ((function_flags & FUNC_NetMulticast) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("NetMulticast"));
            }
            else if ((function_flags & FUNC_NetRequest) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("ServiceRequest"));
            }
            else if ((function_flags & FUNC_NetResponse) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("ServiceResponse"));
            }

            if ((function_flags & FUNC_NetReliable) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("Reliable"));
            }
            else
            {
                flag_format_helper.add_switch(SYSSTR("Unreliable"));
            }
            if ((function_flags & FUNC_NetValidate) != 0)
            {
                flag_format_helper.add_switch(SYSSTR("WithValidation"));
            }
        }

        if ((function_flags & FUNC_Exec) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("Exec"));
        }
        if ((function_flags & FUNC_BlueprintAuthorityOnly) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("BlueprintAuthorityOnly"));
        }
        if ((function_flags & FUNC_BlueprintCosmetic) != 0)
        {
            flag_format_helper.add_switch(SYSSTR("BlueprintCosmetic"));
        }

        static auto latent_action_info = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.LatentActionInfo"));
        bool bWCFound = false;
        bool bLAFound = false;
        for (FProperty* param : function->ForEachProperty())
        {
            auto param_name = to_system(param->GetName());
            auto param_uc_name = string_to_uppercase(param_name);
            if (param_uc_name.find(SYSSTR("WORLDCONTEXT")) != param_uc_name.npos)
            {
                flag_format_helper.get_meta()->add_parameter(SYSSTR("WorldContext"), std::format(SYSSTR("\"{}\""), param_name));
                bWCFound = true;
            }
            if (auto as_struct_property = CastField<FStructProperty>(param); as_struct_property)
            {
                // We now know this is a StructProperty.
                if (as_struct_property->GetStruct()->IsChildOf(latent_action_info))
                {
                    flag_format_helper.get_meta()->add_parameter(SYSSTR("LatentInfo"), std::format(SYSSTR("\"{}\""), param_name));
                    flag_format_helper.get_meta()->add_switch(SYSSTR("Latent"));
                    bLAFound = true;
                }
            }
            if (bWCFound && bLAFound)
            {
                break;
            }
        }

        return flag_format_helper.build_flag_string();
    }

    auto UEHeaderGenerator::generate_function_parameter_list(UClass* uclass,
                                                             UFunction* function,
                                                             GeneratedSourceFile& header_data,
                                                             bool generate_comma_before_name,
                                                             const SystemStringType& context_name,
                                                             const CaseInsensitiveSet& blacklisted_property_names,
                                                             int32_t* out_num_params) -> SystemStringType
    {
        SystemStringType function_arguments_string;

        for (FProperty* property : function->ForEachProperty())
        {
            auto property_flags = property->GetPropertyFlags();
            if ((property_flags & CPF_Parm) != 0 && (property_flags & CPF_ReturnParm) == 0)
            {
                SystemStringType param_declaration;

                // We only generate UPARAM declarations if we are not generating the implementation file
                if (!header_data.is_implementation_file())
                {
                    const SystemStringType parameter_flags_string = generate_function_argument_flags(property);
                    if (!parameter_flags_string.empty())
                    {
                        param_declaration.append(SYSSTR("UPARAM("));
                        param_declaration.append(parameter_flags_string);
                        param_declaration.append(SYSSTR(") "));
                    }
                }

                // Force const reference when we're dealing with strings, and they are not passed by reference
                // UHT for whatever reason completely strips const and reference flags from string properties, but happily generates them in boilerplate code
                const bool should_force_const_ref =
                        ((property_flags & (CPF_ReferenceParm | CPF_OutParm)) == 0) && (property->GetClass().GetName() == STR("StrProperty"));

                // Append const keyword to the parameter declaration if it is marked as const parameter
                if ((property_flags & CPF_ConstParm) != 0 || should_force_const_ref)
                {
                    param_declaration.append(SYSSTR("const "));
                }

                PropertyTypeDeclarationContext context(context_name, &header_data);
                param_declaration.append(generate_property_type_declaration(property, context));

                // Reference will be appended if the parameter has a CPF_ReferenceParm flag set,
                // which would also be always set for output parameters
                if ((property_flags & (CPF_ReferenceParm | CPF_OutParm)) != 0 || should_force_const_ref)
                {
                    param_declaration.append(SYSSTR("&"));
                }

                if (generate_comma_before_name)
                {
                    param_declaration.append(SYSSTR(","));
                }
                param_declaration.append(SYSSTR(" "));

                auto property_name = to_system(property->GetName());

                // If property name is blacklisted, capitalize first letter and prepend New
                if ((uclass && is_function_parameter_shadowing(uclass, property)) || blacklisted_property_names.contains(property_name))
                {
                    property_name[0] = towupper(property_name[0]);
                    property_name.insert(0, SYSSTR("New"));
                }
                param_declaration.append(property_name);

                function_arguments_string.append(param_declaration);
                function_arguments_string.append(SYSSTR(", "));
                if (out_num_params)
                {
                    (*out_num_params)++;
                }
            }
        }

        // remove trailing comma and space from the arguments string
        if (!function_arguments_string.empty())
        {
            function_arguments_string.erase(function_arguments_string.size() - 2);
        }
        return function_arguments_string;
    }

    auto UEHeaderGenerator::generate_default_property_value(FProperty* property, GeneratedSourceFile& header_data, const SystemStringType& ContextName) -> SystemStringType
    {
        const auto field_class_name = to_system(property->GetClass().GetName());
        PropertyTypeDeclarationContext context(ContextName, &header_data);

        // Byte Property
        if (field_class_name == SYSSTR("ByteProperty"))
        {
            FByteProperty* byte_property = static_cast<FByteProperty*>(property);
            UEnum* Enum = byte_property->GetEnum();

            if (Enum != NULL)
            {
                const int64_t first_enum_constant_value = Enum->GetEnumNameByIndex(0).Value;
                return generate_enum_value(Enum, first_enum_constant_value);
            }
            return SYSSTR("0");
        }

        // Enum Property
        if (field_class_name == SYSSTR("EnumProperty"))
        {
            FEnumProperty* enum_property = static_cast<FEnumProperty*>(property);
            UEnum* uenum = enum_property->GetEnum();

            if (uenum == NULL)
            {
                throw std::runtime_error(RC::fmt("EnumProperty {} does not have a valid Enum value", property->GetName()));
            }
            const int64_t first_enum_constant_value = uenum->GetEnumNameByIndex(0).Value;
            return generate_enum_value(uenum, first_enum_constant_value);
        }

        // Bool Property
        if (field_class_name == SYSSTR("BoolProperty"))
        {
            return SYSSTR("false");
        }
        // Standard Numeric Properties
        if (field_class_name == SYSSTR("Int8Property") || field_class_name == SYSSTR("Int16Property") || field_class_name == SYSSTR("IntProperty") ||
            field_class_name == SYSSTR("Int64Property") || field_class_name == SYSSTR("UInt16Property") || field_class_name == SYSSTR("UInt32Property") ||
            field_class_name == SYSSTR("UInt64Property"))
        {
            return SYSSTR("0");
        }
        if (field_class_name == SYSSTR("FloatProperty"))
        {
            return SYSSTR("0.0f");
        }
        if (field_class_name == SYSSTR("DoubleProperty"))
        {
            return SYSSTR("0.0");
        }

        // Object Properties
        if (field_class_name == SYSSTR("ObjectProperty") || field_class_name == SYSSTR("WeakObjectProperty") || field_class_name == SYSSTR("LazyObjectProperty") ||
            field_class_name == SYSSTR("SoftObjectProperty") || field_class_name == SYSSTR("AssetObjectProperty") || property->IsA<FObjectPtrProperty>())
        {
            return SYSSTR("NULL");
        }

        // Class Properties
        if (field_class_name == SYSSTR("ClassProperty") || field_class_name == SYSSTR("SoftClassProperty") || field_class_name == SYSSTR("InterfaceProperty") ||
            field_class_name == SYSSTR("AssetClassProperty") || property->IsA<FClassPtrProperty>())
        {
            return SYSSTR("NULL");
        }

        // Struct Property
        if (field_class_name == SYSSTR("StructProperty"))
        {
            FStructProperty* struct_property = static_cast<FStructProperty*>(property);
            UScriptStruct* script_struct = struct_property->GetStruct();

            if (script_struct == NULL)
            {
                throw std::runtime_error(RC::fmt("Struct is NULL for StructProperty {}", property->GetName()));
            }
            const SystemStringType native_struct_name = get_native_struct_name(script_struct);
            return std::format(SYSSTR("{}{{}}"), native_struct_name);
        }

        // Delegate Properties
        if (field_class_name == SYSSTR("DelegateProperty") || field_class_name == SYSSTR("MulticastInlineDelegateProperty") ||
            field_class_name == SYSSTR("MulticastSparseDelegateProperty"))
        {
            const SystemStringType delegate_type_name = generate_delegate_name(property, context.context_name);
            return std::format(SYSSTR("{}()"), delegate_type_name);
        }

        // Field path property
        if (field_class_name == SYSSTR("FieldPathProperty"))
        {
            return SYSSTR("FFieldPath()");
        }

        // Collection and Map Properties
        if (field_class_name == SYSSTR("ArrayProperty"))
        {
            FArrayProperty* array_property = static_cast<FArrayProperty*>(property);
            FProperty* inner_property = array_property->GetInner();

            const SystemStringType inner_property_type = generate_property_type_declaration(inner_property, context);
            return std::format(SYSSTR("TArray<{}>()"), inner_property_type);
        }

        if (field_class_name == SYSSTR("SetProperty"))
        {
            FSetProperty* set_property = static_cast<FSetProperty*>(property);
            FProperty* element_prop = set_property->GetElementProp();

            const SystemStringType element_property_type = generate_property_type_declaration(element_prop, context);
            return std::format(SYSSTR("TSet<{}>()"), element_property_type);
        }

        if (field_class_name == SYSSTR("MapProperty"))
        {
            FMapProperty* map_property = static_cast<FMapProperty*>(property);
            FProperty* key_property = map_property->GetKeyProp();
            FProperty* value_property = map_property->GetValueProp();

            const SystemStringType key_type = generate_property_type_declaration(key_property, context);
            const SystemStringType value_type = generate_property_type_declaration(value_property, context);

            return std::format(SYSSTR("TMap<{}, {}>()"), key_type, value_type);
        }

        // Various string, name and text properties
        if (field_class_name == SYSSTR("NameProperty"))
        {
            return SYSSTR("NAME_None");
        }
        if (field_class_name == SYSSTR("StrProperty"))
        {
            return SYSSTR("TEXT(\"\")");
        }
        if (field_class_name == SYSSTR("TextProperty"))
        {
            return SYSSTR("FText::GetEmpty()");
        }
        throw std::runtime_error(RC::fmt("[generate_default_property_value] Unsupported property class '{}', full name: '{}'",
                                         field_class_name,
                                         property->GetFullName()));
    }

    auto UEHeaderGenerator::get_class_blueprint_info(UClass* uclass) -> ClassBlueprintInfo
    {
        // These 3 classes are "Intrinsically" blueprintable - they are blueprintable and BlueprintType themselves,
        // but this modifier is not passed through the class hierarchy like normally done
        // So to force generation of correct Blueprintable/BlueprintType statements, we report them as non-blueprint-types
        if (uclass == UObject::StaticClass() || uclass == UActorComponent::StaticClass() || uclass == USceneComponent::StaticClass())
        {
            return ClassBlueprintInfo();
        }

        ClassBlueprintInfo blueprint_info{};
        UClass* super_class = uclass->GetSuperClass();
        if (super_class != NULL)
        {
            blueprint_info = get_class_blueprint_info(super_class);
        }

        for (FProperty* property : uclass->ForEachProperty())
        {
            auto property_flags = property->GetPropertyFlags();

            if ((property_flags & CPF_BlueprintVisible) != 0)
            {
                blueprint_info.is_blueprint_type = true;
                break;
            }
        }

        for (UFunction* function : uclass->ForEachFunction())
        {
            auto function_flags = function->GetFunctionFlags();

            if ((function_flags & FUNC_BlueprintEvent) != 0)
            {
                blueprint_info.is_blueprintable = true;
                blueprint_info.is_blueprint_type = true;
                break;
            }
            if ((function_flags & FUNC_BlueprintCallable) != 0)
            {
                blueprint_info.is_blueprint_type = true;
            }
        }
        return blueprint_info;
    }

    auto UEHeaderGenerator::is_struct_blueprint_type(UScriptStruct* script_struct) -> bool
    {
        UScriptStruct* super_struct = script_struct->GetSuperScriptStruct();
        if (super_struct != NULL)
        {
            bool is_super_struct_blueprint_type = is_struct_blueprint_type(super_struct);
            if (is_super_struct_blueprint_type)
            {
                return true;
            }
        }
        bool is_blueprint_type = false;

        for (FProperty* property : script_struct->ForEachProperty())
        {
            auto property_flags = property->GetPropertyFlags();

            if ((property_flags & CPF_BlueprintVisible) != 0)
            {
                is_blueprint_type = true;
                break;
            }
        }
        return is_blueprint_type;
    }

    auto UEHeaderGenerator::is_function_parameter_shadowing(UClass* uclass, FProperty* function_parameter) -> bool
    {
        bool is_shadowing = false;
        for (FProperty* property : uclass->ForEachPropertyInChain())
        {
            if (property->GetFName().Equals(function_parameter->GetFName()))
            {
                is_shadowing = true;
                break;
            }
        }

        return is_shadowing;
    }

    auto UEHeaderGenerator::get_module_name_for_package(UObject* package) -> SystemStringType
    {
        if (package->GetOuterPrivate() != NULL)
        {
            throw std::invalid_argument("Encountered a package with an outer object set");
        }
        auto package_name = to_system(package->GetName());
        if (!package_name.starts_with(SYSSTR("/Script/")))
        {
            return SYSSTR("");
        }
        package_name.erase(0, SystemStringType(SYSSTR("/Script/")).length());
        return package_name;
    }

    UEHeaderGenerator::UEHeaderGenerator(const FFilePath& root_directory)
    {
        this->m_root_directory = root_directory;
        this->m_primary_module_name = determine_primary_game_module_name();

        // Force inclusion of Core and CoreUObject into all the generated module build files
        this->m_forced_module_dependencies.insert(SYSSTR("Core"));
        this->m_forced_module_dependencies.insert(SYSSTR("CoreUObject"));
        // TODO not optimal, but still needed for the majority of the cases
        this->m_forced_module_dependencies.insert(SYSSTR("Engine"));

        // Add few classes that require explicit UObjectInitializer constructor call, excluding classes inheriting from AActor.
        this->m_classes_with_object_initializer.insert(SYSSTR("UUserWidget"));
        this->m_classes_with_object_initializer.insert(SYSSTR("UListView"));
        this->m_classes_with_object_initializer.insert(SYSSTR("UMovieSceneTrack"));
        this->m_classes_with_object_initializer.insert(SYSSTR("USoundWaveProcedural"));
        this->m_classes_with_object_initializer.insert(SYSSTR("URichTextBlock"));
        this->m_classes_with_object_initializer.insert(SYSSTR("URichTextBlockImageDecorator"));
        this->m_classes_with_object_initializer.insert(SYSSTR("URichTextBlockDecorator"));
        this->m_classes_with_object_initializer.insert(SYSSTR("USkeletalMeshComponentBudgeted"));
        this->m_classes_with_object_initializer.insert(SYSSTR("UIpNetDriver"));
        this->m_classes_with_object_initializer.insert(SYSSTR("UAITask"));
    }

    auto UEHeaderGenerator::ignore_selected_modules() -> void
    {
        // Never generate CoreUObject, since it contains a lot of intrinsic classes and is generally always the same
        // Also skip engine initially because engine contents should not change much either

        // Skip "Engine" and "CoreUObject" if requested
        if (UE4SSProgram::settings_manager.UHTHeaderGenerator.IgnoreEngineAndCoreUObject ||
            UE4SSProgram::settings_manager.UHTHeaderGenerator.IgnoreAllCoreEngineModules)
        {
            this->m_ignored_module_names.insert(SYSSTR("Engine"));
            this->m_ignored_module_names.insert(SYSSTR("CoreUObject"));
        }

        // Skip all core engine packages if requested
        if (UE4SSProgram::settings_manager.UHTHeaderGenerator.IgnoreAllCoreEngineModules)
        {
            this->m_ignored_module_names.insert(SYSSTR("ActorLayerUtilities"));
            this->m_ignored_module_names.insert(SYSSTR("ActorSequence"));
            this->m_ignored_module_names.insert(SYSSTR("AIModule"));
            this->m_ignored_module_names.insert(SYSSTR("AndroidPermission"));
            this->m_ignored_module_names.insert(SYSSTR("AnimationCore"));
            this->m_ignored_module_names.insert(SYSSTR("AnimationSharing"));
            this->m_ignored_module_names.insert(SYSSTR("AnimGraphRuntime"));
            this->m_ignored_module_names.insert(SYSSTR("AppleImageUtils"));
            this->m_ignored_module_names.insert(SYSSTR("ArchVisCharacter"));
            this->m_ignored_module_names.insert(SYSSTR("AssetRegistry"));
            this->m_ignored_module_names.insert(SYSSTR("AssetTags"));
            this->m_ignored_module_names.insert(SYSSTR("AudioAnalyzer"));
            this->m_ignored_module_names.insert(SYSSTR("AudioCapture"));
            this->m_ignored_module_names.insert(SYSSTR("AudioExtensions"));
            this->m_ignored_module_names.insert(SYSSTR("AudioMixer"));
            this->m_ignored_module_names.insert(SYSSTR("AudioPlatformConfiguration"));
            this->m_ignored_module_names.insert(SYSSTR("AudioSynesthesia"));
            this->m_ignored_module_names.insert(SYSSTR("AugmentedReality"));
            this->m_ignored_module_names.insert(SYSSTR("AutomationUtils"));
            this->m_ignored_module_names.insert(SYSSTR("AvfMediaFactory"));
            this->m_ignored_module_names.insert(SYSSTR("BuildPatchServices"));
            this->m_ignored_module_names.insert(SYSSTR("CableComponent"));
            this->m_ignored_module_names.insert(SYSSTR("Chaos"));
            this->m_ignored_module_names.insert(SYSSTR("ChaosCloth"));
            this->m_ignored_module_names.insert(SYSSTR("ChaosNiagara"));
            this->m_ignored_module_names.insert(SYSSTR("ChaosSolvers"));
            this->m_ignored_module_names.insert(SYSSTR("ChaosSolverEngine"));
            this->m_ignored_module_names.insert(SYSSTR("CinematicCamera"));
            this->m_ignored_module_names.insert(SYSSTR("ClothingSystemRuntimeCommon"));
            this->m_ignored_module_names.insert(SYSSTR("ClothingSystemRuntimeInterface"));
            this->m_ignored_module_names.insert(SYSSTR("ClothingSystemRuntimeNv"));
            this->m_ignored_module_names.insert(SYSSTR("CustomMeshComponent"));
            this->m_ignored_module_names.insert(SYSSTR("DatasmithContent"));
            this->m_ignored_module_names.insert(SYSSTR("DeveloperSettings"));
            this->m_ignored_module_names.insert(SYSSTR("EditableMesh"));
            this->m_ignored_module_names.insert(SYSSTR("EngineMessages"));
            this->m_ignored_module_names.insert(SYSSTR("EngineSettings"));
            this->m_ignored_module_names.insert(SYSSTR("EyeTracker"));
            this->m_ignored_module_names.insert(SYSSTR("FacialAnimation"));
            this->m_ignored_module_names.insert(SYSSTR("FieldSystemCore"));
            this->m_ignored_module_names.insert(SYSSTR("FieldSystemEngine"));
            this->m_ignored_module_names.insert(SYSSTR("Foliage"));
            this->m_ignored_module_names.insert(SYSSTR("GameplayTags"));
            this->m_ignored_module_names.insert(SYSSTR("GameplayTasks"));
            this->m_ignored_module_names.insert(SYSSTR("GeometryCache"));
            this->m_ignored_module_names.insert(SYSSTR("GeometryCacheTracks"));
            this->m_ignored_module_names.insert(SYSSTR("GeometryCollectionCore"));
            this->m_ignored_module_names.insert(SYSSTR("GeometryCollectionSimulationCore"));
            this->m_ignored_module_names.insert(SYSSTR("GeometryCollectionEngine"));
            this->m_ignored_module_names.insert(SYSSTR("GeometryCollectionTracks"));
            this->m_ignored_module_names.insert(SYSSTR("GooglePAD"));
            this->m_ignored_module_names.insert(SYSSTR("HeadMountedDisplay"));
            this->m_ignored_module_names.insert(SYSSTR("ImageWrapper"));
            this->m_ignored_module_names.insert(SYSSTR("ImageWriteQueue"));
            this->m_ignored_module_names.insert(SYSSTR("ImgMedia"));
            this->m_ignored_module_names.insert(SYSSTR("ImgMediaFactory"));
            this->m_ignored_module_names.insert(SYSSTR("InputCore"));
            this->m_ignored_module_names.insert(SYSSTR("InteractiveToolsFramework"));
            this->m_ignored_module_names.insert(SYSSTR("JsonUtilities"));
            this->m_ignored_module_names.insert(SYSSTR("Landscape"));
            this->m_ignored_module_names.insert(SYSSTR("LevelSequence"));
            this->m_ignored_module_names.insert(SYSSTR("LightPropagationVolumeRuntime"));
            this->m_ignored_module_names.insert(SYSSTR("LiveLinkInterface"));
            this->m_ignored_module_names.insert(SYSSTR("LocationServicesBPLibrary"));
            this->m_ignored_module_names.insert(SYSSTR("LuminRuntimeSettings"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeap"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapAR"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapARPin"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapAudio"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapController"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapEyeTracker"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapHandMeshing"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapHandTracking"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapIdentity"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapImageTracker"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapLightEstimation"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapPlanes"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapPrivileges"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapSecureStorage"));
            this->m_ignored_module_names.insert(SYSSTR("MagicLeapSharedWorld"));
            this->m_ignored_module_names.insert(SYSSTR("MaterialShaderQualitySettings"));
            this->m_ignored_module_names.insert(SYSSTR("MediaAssets"));
            this->m_ignored_module_names.insert(SYSSTR("MediaCompositing"));
            this->m_ignored_module_names.insert(SYSSTR("MediaUtils"));
            this->m_ignored_module_names.insert(SYSSTR("MeshDescription"));
            this->m_ignored_module_names.insert(SYSSTR("MobilePatchingUtils"));
            this->m_ignored_module_names.insert(SYSSTR("MotoSynth"));
            this->m_ignored_module_names.insert(SYSSTR("MoviePlayer"));
            this->m_ignored_module_names.insert(SYSSTR("MovieScene"));
            this->m_ignored_module_names.insert(SYSSTR("MovieSceneCapture"));
            this->m_ignored_module_names.insert(SYSSTR("MovieSceneTracks"));
            this->m_ignored_module_names.insert(SYSSTR("MRMesh"));
            this->m_ignored_module_names.insert(SYSSTR("NavigationSystem"));
            this->m_ignored_module_names.insert(SYSSTR("NetCore"));
            this->m_ignored_module_names.insert(SYSSTR("Niagara"));
            this->m_ignored_module_names.insert(SYSSTR("NiagaraAnimNotifies"));
            this->m_ignored_module_names.insert(SYSSTR("NiagaraCore"));
            this->m_ignored_module_names.insert(SYSSTR("NiagaraShader"));
            this->m_ignored_module_names.insert(SYSSTR("OculusHMD"));
            this->m_ignored_module_names.insert(SYSSTR("OculusInput"));
            this->m_ignored_module_names.insert(SYSSTR("OculusMR"));
            this->m_ignored_module_names.insert(SYSSTR("OnlineSubsystem"));
            this->m_ignored_module_names.insert(SYSSTR("OnlineSubsystemUtils"));
            this->m_ignored_module_names.insert(SYSSTR("Overlay"));
            this->m_ignored_module_names.insert(SYSSTR("PacketHandler"));
            this->m_ignored_module_names.insert(SYSSTR("Paper2D"));
            this->m_ignored_module_names.insert(SYSSTR("PhysicsCore"));
            this->m_ignored_module_names.insert(SYSSTR("PhysXVehicles"));
            this->m_ignored_module_names.insert(SYSSTR("ProceduralMeshComponent"));
            this->m_ignored_module_names.insert(SYSSTR("PropertyAccess"));
            this->m_ignored_module_names.insert(SYSSTR("PropertyPath"));
            this->m_ignored_module_names.insert(SYSSTR("Renderer"));
            this->m_ignored_module_names.insert(SYSSTR("Serialization"));
            this->m_ignored_module_names.insert(SYSSTR("SessionMessages"));
            this->m_ignored_module_names.insert(SYSSTR("SignificanceManager"));
            this->m_ignored_module_names.insert(SYSSTR("Slate"));
            this->m_ignored_module_names.insert(SYSSTR("SlateCore"));
            this->m_ignored_module_names.insert(SYSSTR("SoundFields"));
            this->m_ignored_module_names.insert(SYSSTR("StaticMeshDescription"));
            this->m_ignored_module_names.insert(SYSSTR("SteamVR"));
            this->m_ignored_module_names.insert(SYSSTR("SteamVRInputDevice"));
            this->m_ignored_module_names.insert(SYSSTR("Synthesis"));
            this->m_ignored_module_names.insert(SYSSTR("TcpMessaging"));
            this->m_ignored_module_names.insert(SYSSTR("TemplateSequence"));
            this->m_ignored_module_names.insert(SYSSTR("TimeManagement"));
            this->m_ignored_module_names.insert(SYSSTR("UdpMessaging"));
            this->m_ignored_module_names.insert(SYSSTR("UMG"));
            this->m_ignored_module_names.insert(SYSSTR("UObjectPlugin"));
            this->m_ignored_module_names.insert(SYSSTR("VariantManagerContent"));
            this->m_ignored_module_names.insert(SYSSTR("VectorVM"));
            this->m_ignored_module_names.insert(SYSSTR("WmfMediaFactory"));
        }
    }

    auto UEHeaderGenerator::dump_native_packages() -> void
    {
        ignore_selected_modules();

        Output::send(SYSSTR("Cleaning up previously generated SDK (if one exists)\n"));
        if (std::filesystem::exists(m_root_directory))
        {
            std::filesystem::remove_all(m_root_directory);
        }

        Output::send(SYSSTR("Initializing native packages dump\n"));

        std::vector<UClass*> native_classes_to_dump;
        std::vector<UScriptStruct*> native_structs_to_dump;
        std::vector<UEnum*> native_enums_to_dump;
        std::vector<UFunction*> native_delegates_to_dump;

        Output::send(SYSSTR("Gathering native objects for dumping\n"));
        UObjectGlobals::ForEachUObject([&](void* raw_object, int32_t chunk_index, int32_t object_index) {
            UObject* typed_object = static_cast<UObject*>(raw_object);

            if (UClass* casted_object = Cast<UClass>(typed_object))
            {
                if ((casted_object->GetClassFlags() & CLASS_Native) != 0)
                {
                    native_classes_to_dump.push_back(casted_object);
                }
            }
            else if (UScriptStruct* casted_struct = Cast<UScriptStruct>(typed_object))
            {
                if ((casted_struct->GetStructFlags() & STRUCT_Native) != 0)
                {
                    native_structs_to_dump.push_back(casted_struct);
                }
            }
            else if (UEnum* uenum = Cast<UEnum>(typed_object))
            {
                if (!typed_object->IsA<UUserDefinedEnum>())
                {
                    native_enums_to_dump.push_back(uenum);
                }
            }
            else if (UFunction* function = Cast<UFunction>(typed_object))
            {
                // We are looking for delegate signature functions located inside the native packages
                // When they are located directly on the top level, they will result in a separate header, otherwise they will
                // be included into their respective outer header
                if (is_delegate_signature_function(function) && function->GetOuterPrivate()->IsA<UPackage>())
                {
                    UPackage* outer_package = Cast<UPackage>(function->GetOuterPrivate());

                    // Make sure the function package actually corresponds to the real native module
                    if (!get_module_name_for_package(outer_package).empty())
                    {
                        native_delegates_to_dump.push_back(function);
                    }
                }
            }

            return RC::LoopAction::Continue;
        });

        Output::send(SYSSTR("Attempting to dump {} native classes\n"), native_classes_to_dump.size());

        for (UFunction* delegate_signature_function : native_delegates_to_dump)
        {
            // Output::send(SYSSTR("Dumping native delegate type {}\n"), global_delegate_signature->GetName());
            generate_object_description_file(delegate_signature_function);
        }

        for (UClass* class_to_dump : native_classes_to_dump)
        {
            // Output::send(SYSSTR("Dumping native class {}\n"), class_to_dump->GetName());
            generate_object_description_file(class_to_dump);
        }

        Output::send(SYSSTR("Attempting to dump {} native structs\n"), native_structs_to_dump.size());

        for (UScriptStruct* struct_to_dump : native_structs_to_dump)
        {
            // Output::send(SYSSTR("Dumping native struct {}\n"), struct_to_dump->GetName());
            generate_object_description_file(struct_to_dump);
        }

        Output::send(SYSSTR("Attempting to dump {} native enums\n"), native_enums_to_dump.size());

        for (UEnum* enum_to_dump : native_enums_to_dump)
        {
            // Output::send(SYSSTR("Dumping native enum {}\n"), enum_to_dump->GetName());
            generate_object_description_file(enum_to_dump);
        }

        Output::send(SYSSTR("Writing stub module build files for {} modules\n"), m_module_dependencies.size());
        for (const auto& module_pair : m_module_dependencies)
        {
            generate_module_implementation_file(module_pair.first);
            generate_module_build_file(module_pair.first);
        }

        // Pass #2
        for (auto& header_file : m_header_files)
        {
            auto object = header_file.get_corresponding_object();
            bool is_struct = object->IsA<UStruct>();
            bool is_class = object->IsA<UClass>();
            if ((is_struct || is_class) && m_structs_that_need_get_type_hash.find(std::bit_cast<UStruct*>(object)) != m_structs_that_need_get_type_hash.end())
            {
                File::StringType name{};
                if (is_class)
                {
                    name = get_native_class_name(std::bit_cast<UClass*>(object));
                }
                else if (is_struct)
                {
                    name = get_native_struct_name(std::bit_cast<UScriptStruct*>(object));
                }
                header_file.append_line(std::format(SYSSTR("FORCEINLINE uint32 GetTypeHash(const {}) {{ return 0; }}"), name));
            }

            // Case for FTickFunction struct
            if (is_struct)
            {
                auto struct_object = std::bit_cast<UScriptStruct*>(object);
                auto super_struct = struct_object->GetSuperScriptStruct();
                if (super_struct)
                {
                    if (get_native_struct_name(super_struct) == SYSSTR("FTickFunction"))
                    {
                        File::StringType name{};
                        name = get_native_struct_name(struct_object);
                        header_file.append_line(SYSSTR(""));
                        header_file.append_line(SYSSTR("template<>"));
                        header_file.append_line(std::format(SYSSTR("struct TStructOpsTypeTraits<{}> : public TStructOpsTypeTraitsBase2<{}>"), name, name));
                        header_file.append_line(SYSSTR("{"));
                        header_file.append_line(SYSSTR("    enum"));
                        header_file.append_line(SYSSTR("    {"));
                        header_file.append_line(SYSSTR("        WithCopy = false"));
                        header_file.append_line(SYSSTR("    };"));
                        header_file.append_line(SYSSTR("};"));
                    }
                }
            }
            header_file.serialize_file_content_to_disk();
        }

        Output::send(SYSSTR("Done!\n"));
    }

    auto UEHeaderGenerator::generate_object_description_file(UObject* object) -> bool
    {
        const SystemStringType module_name = get_module_name_for_package(object->GetOutermost());
        const SystemStringType file_base_name = get_header_name_for_object(object);

        if (module_name.empty())
        {
            return false;
        }
        if (m_ignored_module_names.contains(module_name))
        {
            return false;
        }

        GeneratedSourceFile& header_file =
                m_header_files.emplace_back(GeneratedSourceFile::create_source_file(m_root_directory, module_name, file_base_name, false, object));
        GeneratedSourceFile implementation_file = GeneratedSourceFile::create_source_file(m_root_directory, module_name, file_base_name, true, object);
        implementation_file.set_header_file(&header_file);

        if (UClass* uclass = Cast<UClass>(object))
        {
            if (uclass->IsChildOf<UInterface>())
            {
                generate_interface_definition(uclass, header_file);
            }
            else
            {
                generate_object_definition(uclass, header_file);
                generate_object_implementation(uclass, implementation_file);
            }
        }
        else if (UScriptStruct* script_struct = Cast<UScriptStruct>(object))
        {
            generate_struct_definition(script_struct, header_file);
            generate_struct_implementation(script_struct, implementation_file);
        }
        else if (UEnum* uenum = Cast<UEnum>(object))
        {
            generate_enum_definition(uenum, header_file);
        }
        else if (UFunction* function = Cast<UFunction>(object))
        {
            if (!is_delegate_signature_function(function))
            {
                throw std::runtime_error(RC::fmt("Function {} is not a delegate signature function", function->GetName()));
            }
            if (!function->GetOuterPrivate()->IsA<UPackage>())
            {
                throw std::runtime_error(RC::fmt("Delegate Signature Function {} does not have a UPackage as it's owner", function->GetName()));
            }
            generate_global_delegate_declaration(function, NULL, header_file);
        }
        else
        {
            throw std::runtime_error(
                    RC::fmt("Provided object {} is not of a supported type: {}", object->GetName(), object->GetClassPrivate()->GetName()));
        }

        auto iterator = this->m_module_dependencies.find(module_name);
        if (iterator == this->m_module_dependencies.end())
        {
            iterator = this->m_module_dependencies.insert({module_name, std::make_shared<std::set<SystemStringType>>()}).first;
        }

        if (!header_file.has_content_to_save())
        {
            return false;
        }
        implementation_file.serialize_file_content_to_disk();

        // This is necessary because header_file.serialize_file_content_to_disk() is not called anymore
        // so we need to call all the necessary internal code to generate the dependency list
        // otherwise the below code for copy_dependency_module_names will not work.
        header_file.generate_file_contents();

        // Record module names used in the headers
        std::shared_ptr<std::set<SystemStringType>> out_dependency_module_names = iterator->second;
        header_file.copy_dependency_module_names(*out_dependency_module_names);
        implementation_file.copy_dependency_module_names(*out_dependency_module_names);

        return true;
    }

    auto UEHeaderGenerator::generate_object_pre_declaration(UObject* object) -> std::vector<std::vector<SystemStringType>>
    {
        std::vector<std::vector<SystemStringType>> pre_declarations;

        UClass* object_class = object->GetClassPrivate();

        if (object_class->IsChildOf(UClass::StaticClass()))
        {
            UClass* uclass = static_cast<UClass*>(object);

            if (uclass->IsChildOf(UInterface::StaticClass()))
            {
                pre_declarations.push_back({SYSSTR("class "), get_native_class_name(uclass, true), SYSSTR(";\n")});
            }
            pre_declarations.push_back({SYSSTR("class "), get_native_class_name(uclass, false), SYSSTR(";\n")});
        }
        else if (object_class->IsChildOf(UScriptStruct::StaticClass()))
        {
            UScriptStruct* script_struct = static_cast<UScriptStruct*>(object);

            pre_declarations.push_back({SYSSTR("struct "), get_native_struct_name(script_struct), SYSSTR(";\n")});
        }
        else if (object_class->IsChildOf(UEnum::StaticClass()))
        {
            // TODO do we want them? They're not that easy since we do not know enum types precisely in advance
            throw std::invalid_argument("Enum pre-declarations are not supported");
        }
        else
        {
            throw std::invalid_argument("Provided object is not of a supported type, should be UClass/UScriptStruct/UEnum");
        }

        return pre_declarations;
    }

    auto UEHeaderGenerator::get_header_name_for_object(UObject* object, bool get_existing_header) -> SystemStringType
    {
        File::StringType header_name{};
        UObject* final_object{};

        if (object->IsA<UClass>() || object->IsA<UScriptStruct>())
        {
            // Class and struct headers follow the relevant object name
            header_name = to_system(object->GetName());
            final_object = object;
        }
        else if (object->IsA<UEnum>())
        {
            // Enumeration usually have the E prefix which will be present in the header names
            // We do not strip it because there are some broken headers that do not follow that convention (e.g. funny Wwise)
            header_name = to_system(object->GetName());
            final_object = object;
        }
        else
        {
            // Delegate signature functions need to have their postfix removed
            UFunction* signature_function = Unreal::Cast<UFunction>(object);
            if (signature_function != nullptr && is_delegate_signature_function(signature_function))
            {

                // If delegate is not declared inside the package, it is declared in the header of it's outer
                if (!signature_function->GetOuterPrivate()->IsA<UPackage>())
                {
                    final_object = signature_function->GetOuterPrivate();
                    header_name = get_header_name_for_object(final_object, get_existing_header);
                }
                else
                {
                    // Otherwise, remove the postfix and use the function name as the header name
                    // Also append the delegate postfix because apparently there can be conflicts
                    SystemStringType DelegateName = strip_delegate_signature_postfix(signature_function);
                    DelegateName.append(SYSSTR("Delegate"));
                    header_name = DelegateName;
                    final_object = object;
                }
            }
        }

        if (header_name.empty())
        {
            // Unsupported dependency object type
            throw std::runtime_error(RC::fmt("Unsupported dependency object type {}: {}", object->GetClassPrivate()->GetName(), object->GetName()));
        }

        if (get_existing_header)
        {
            if (auto it2 = m_dependency_object_to_unique_id.find(final_object); it2 != m_dependency_object_to_unique_id.end())
            {
                header_name.append(std::format(SYSSTR("{}"), it2->second));
            }
        }
        else
        {
            if (auto it = m_used_file_names.find(header_name); it != m_used_file_names.end())
            {
                header_name.append(std::format(SYSSTR("{}"), ++it->second.usable_id));
                m_dependency_object_to_unique_id.emplace(final_object, it->second.usable_id);
            }
            else
            {
                m_used_file_names.emplace(header_name, UniqueName{header_name});
            }
        }

        return header_name;
    }

    auto UEHeaderGenerator::generate_global_delegate_declaration(UFunction* signature_function, UClass* delegate_class, GeneratedSourceFile& header_data) -> void
    {
        generate_delegate_type_declaration(signature_function, delegate_class, header_data);
    }

    auto UEHeaderGenerator::determine_primary_game_module_name() -> SystemStringType
    {
        /*
        HMODULE primary_executable_module = GetModuleHandleW(NULL);
        wchar_t module_name_buffer[1024]{'\0'};
        GetModuleFileNameW(primary_executable_module, module_name_buffer, ARRAYSIZE(module_name_buffer));

        // Retrieve the filename from the full path, strip down the extension
        FFilePath root_executable_path((SystemStringType(module_name_buffer)));
        SystemStringType filename = root_executable_path.filename().replace_extension().wstring();

        // Remove the shipping file postfix
        SystemStringType shipping_postfix = SYSSTR("-Win64-Shipping");
        if (filename.ends_with(shipping_postfix))
        {
            filename.erase(filename.length() - shipping_postfix.length());
        }
        return filename;
        */
       return "Main";
    }

    auto UEHeaderGenerator::generate_cross_module_include(UObject* object, const SystemStringType& module_name, const SystemStringType& fallback_name) -> SystemStringType
    {
        // Retrieve the most top level object located inside the native package
        UObject* top_level_object = object;

        while (!top_level_object->GetOuterPrivate()->IsA<UPackage>())
        {
            top_level_object = top_level_object->GetOuterPrivate();
        }

        const auto object_name = to_system(top_level_object->GetName());
        return std::format(SYSSTR("//CROSS-MODULE INCLUDE V2: -ModuleName={} -ObjectName={} -FallbackName={}\n"), module_name, object_name, fallback_name);
    }

    GeneratedFile::GeneratedFile(const FFilePath& full_file_path)
    {
        this->m_full_file_path = full_file_path;
        this->m_file_base_name = to_system_string(full_file_path.filename().replace_extension());
        this->m_current_indent_count = 0;
    }

    auto GeneratedFile::append_line(const SystemStringType& line) -> void
    {
        for (int32_t i = 0; i < m_current_indent_count; i++)
        {
            m_file_contents_buffer.append(SYSSTR("    "));
        }
        m_file_contents_buffer.append(line);
        m_file_contents_buffer.append(SYSSTR("\n"));
    }

    auto GeneratedFile::append_line_no_indent(const SystemStringType& line) -> void
    {
        m_file_contents_buffer.append(line);
        m_file_contents_buffer.append(SYSSTR("\n"));
    }

    auto GeneratedFile::begin_indent_level() -> void
    {
        m_current_indent_count++;
    }

    auto GeneratedFile::end_indent_level() -> void
    {
        m_current_indent_count--;
        if (m_current_indent_count < 0)
        {
            throw std::invalid_argument("Attempt to pop empty indent level stack");
        }
    }

    auto GeneratedFile::serialize_file_content_to_disk() -> bool
    {
        if (!has_content_to_save())
        {
            return false;
        }
        // TODO might be slow, maybe move it out into the header generator?
        std::filesystem::create_directories(this->m_full_file_path.parent_path());

        SystemStreamType file_output_stream;
        file_output_stream.open(m_full_file_path);
        if (!file_output_stream.is_open())
        {
            throw std::invalid_argument("Failed to open the header file");
        }
        // TODO: FIX STREAM
//        file_output_stream << generate_file_contents().c_str();
        file_output_stream.close();
        return true;
    }

    auto GeneratedFile::generate_file_contents() -> SystemStringType
    {
        return m_file_contents_buffer;
    }

    auto GeneratedFile::has_content_to_save() const -> bool
    {
        return !m_file_contents_buffer.empty();
    }

    auto GeneratedSourceFile::create_source_file(const FFilePath& root_dir,
                                                 const SystemStringType& module_name,
                                                 const SystemStringType& base_name,
                                                 bool is_implementation_file,
                                                 UObject* object) -> GeneratedSourceFile
    {
        FFilePath full_file_path;
        if (is_implementation_file)
        {
            full_file_path = root_dir / module_name / SYSSTR("Private") / (base_name + SYSSTR(".cpp"));
        }
        else
        {
            full_file_path = root_dir / module_name / SYSSTR("Public") / (base_name + SYSSTR(".h"));
        }
        return GeneratedSourceFile(full_file_path, module_name, is_implementation_file, object);
    }

    GeneratedSourceFile::GeneratedSourceFile(const FFilePath& file_path, const SystemStringType& file_module_name, bool is_implementation_file, UObject* object)
        : GeneratedFile(file_path)
    {
        this->m_file_module_name = file_module_name;
        this->m_is_implementation_file = is_implementation_file;
        this->m_object = object;
    }

    auto GeneratedSourceFile::set_header_file(GeneratedSourceFile* header_file) -> void
    {
        this->m_header_file = header_file;
    }

    auto GeneratedSourceFile::add_extra_include(const SystemStringType& included_file_name) -> void
    {
        this->m_extra_includes.insert(included_file_name);
    }

    auto GeneratedSourceFile::add_dependency_object(UObject* object, DependencyLevel dependency_level) -> void
    {
        const auto iterator = m_dependencies.find(object);

        // Only add the dependency if we don't have one already or requested level is higher than the current one
        if (iterator == m_dependencies.end() || (int32_t)iterator->second <= (int32_t)dependency_level)
        {
            m_dependencies.insert_or_assign(object, dependency_level);
        }
    }

    auto GeneratedSourceFile::generate_file_contents() -> SystemStringType
    {
        SystemStringType result_header_contents;
        result_header_contents.append(generate_includes_string());
        result_header_contents.append(SYSSTR("\n"));

        SystemStringType pre_declarations_string = generate_pre_declarations_string();
        if (!pre_declarations_string.empty())
        {
            result_header_contents.append(pre_declarations_string);
            result_header_contents.append(SYSSTR("\n"));
        }

        if (!m_implementation_constructor.empty())
        {
            result_header_contents.append(m_implementation_constructor);
            result_header_contents.append(SYSSTR("\n"));
        }

        if (!m_file_contents_buffer.empty())
        {
            result_header_contents.append(m_file_contents_buffer);
            result_header_contents.append(SYSSTR("\n"));
        }
        return result_header_contents;
    }

    auto GeneratedSourceFile::generate_includes_string() const -> SystemStringType
    {
        SystemStringType result_include_string;
        std::vector<std::vector<SystemStringType>> include_lines;
        std::vector<SystemStringType> cross_module_includes;

        // For the header file, we generate the pragma and minimal core includes
        if (!m_is_implementation_file)
        {
            result_include_string.append(SYSSTR("#pragma once\n"));
            result_include_string.append(SYSSTR("#include \"CoreMinimal.h\"\n"));
        }
        // For CPP implementation file, we need to generate the header include
        if (m_is_implementation_file)
        {
            if (m_header_file != NULL)
            {
                // Generate it if we have the correct header file set
                result_include_string.append(std::format(SYSSTR("#include \"{}.h\"\n"), m_header_file->m_file_base_name));
            }
            else
            {
                // Otherwise, we generate a simple minimal core include
                result_include_string.append(SYSSTR("#include \"CoreMinimal.h\"\n"));
            }
        }

        // Generate extra includes we might need that do not represent objects
        for (const SystemStringType& extra_included_file : m_extra_includes)
        {
            include_lines.push_back({SYSSTR("#include \""), extra_included_file, SYSSTR("\"\n")});
        }

        // Generate includes for the relevant object files
        for (const auto& dependency_pair : m_dependencies)
        {
            UObject* dependency_object = dependency_pair.first;

            // Only want to generate include dependencies
            if (dependency_pair.second != DependencyLevel::Include)
            {
                continue;
            }

            const SystemStringType object_header_name = UEHeaderGenerator::get_header_name_for_object(dependency_object, true);

            // Definitely skip include if object in question is placed into this header
            if (object_header_name == m_file_base_name)
            {
                continue;
            }

            // Skip includes that have already been generated on the header file
            if (m_is_implementation_file && m_header_file != NULL)
            {
                if (m_header_file->has_dependency(dependency_object, DependencyLevel::Include))
                {
                    continue;
                }
            }
            UObject* package = dependency_object->GetOutermost();
            SystemStringType native_module_name = UEHeaderGenerator::get_module_name_for_package(package);

            if (!native_module_name.empty())
            {
                // If this package corresponds to the file inside this module, we generate the normal include,
                // since generated headers are always located in the module root and follow one file per object convention
                if (m_file_module_name == native_module_name)
                {
                    include_lines.push_back({SYSSTR("#include \""), object_header_name, SYSSTR(".h\"\n")});
                }
                else
                {
                    // Otherwise, we generate an include stub which will be handled by the unreal engine commandlet later
                    m_dependency_module_names.insert(native_module_name);
                    cross_module_includes.push_back(UEHeaderGenerator::generate_cross_module_include(dependency_object, native_module_name, object_header_name));
                }
            }
        }

        // Sort the cross module includes and add them to the result so that they are always above the rest of the includes
        std::sort(cross_module_includes.begin(), cross_module_includes.end());

        // Remove duplicates - there are sometimes multiple instances of the same cross module include
        cross_module_includes.erase(std::unique(cross_module_includes.begin(), cross_module_includes.end()), cross_module_includes.end());

        for (const SystemStringType& cross_module_include : cross_module_includes)
        {
            result_include_string.append(cross_module_include);
        }

        // Sort the includes by module name, since we want to make sure that they are always in the same order
        std::sort(include_lines.begin(), include_lines.end(), [](const std::vector<SystemStringType>& a, const std::vector<SystemStringType>& b) {
            return a[1] < b[1];
        });

        for (const auto& line : include_lines)
        {
            for (const auto& part : line)
            {
                result_include_string.append(part);
            }
        }

        // Last include of the header file should always be a generated one
        if (!m_is_implementation_file)
        {
            result_include_string.append(std::format(SYSSTR("#include \"{}.generated.h\"\n"), m_file_base_name));
        }
        return result_include_string;
    }

    auto GeneratedSourceFile::generate_pre_declarations_string() const -> SystemStringType
    {
        SystemStringType result_declarations;
        std::vector<std::vector<std::vector<SystemStringType>>> pre_declarations;

        // Generate pre-declarations for the relevant object files
        for (const auto& dependency_pair : m_dependencies)
        {
            UObject* dependency_object = dependency_pair.first;

            // Only want to generate pre-declarations
            if (dependency_pair.second != DependencyLevel::PreDeclaration)
            {
                continue;
            }

            // We still need to reference the object's owner module
            UObject* package = dependency_object->GetOutermost();
            SystemStringType native_module_name = UEHeaderGenerator::get_module_name_for_package(package);

            if (!native_module_name.empty() && m_file_module_name != native_module_name)
            {
                m_dependency_module_names.insert(native_module_name);
            }

            if (!m_is_implementation_file) pre_declarations.push_back(UEHeaderGenerator::generate_object_pre_declaration(dependency_object));
        }

        // Sort the entries alphabetically by the class name
        std::sort(pre_declarations.begin(),
                  pre_declarations.end(),
                  [](const std::vector<std::vector<SystemStringType>>& a, const std::vector<std::vector<SystemStringType>>& b) {
                      return a[0][1] < b[0][1];
                  });

        // Add pre_declarations to result_declarations
        for (const auto& declaration : pre_declarations)
        {
            for (const auto& line : declaration)
            {
                for (const auto& part : line)
                {
                    result_declarations.append(part);
                }
            }
        }

        return result_declarations;
    }

    auto GeneratedSourceFile::has_content_to_save() const -> bool
    {
        return !m_file_contents_buffer.empty();
    }

    auto GeneratedSourceFile::has_dependency(UObject* object, DependencyLevel dependency_level) -> bool
    {
        const auto iterator = m_dependencies.find(object);
        return iterator != m_dependencies.end() && ((int32_t)iterator->second >= (int32_t)dependency_level);
    }
} // namespace RC::UEGenerator
