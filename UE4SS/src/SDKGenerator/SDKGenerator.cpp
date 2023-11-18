/**
 * Some types are ignored because they aren't implemented yet.
 * Types that are ignored will still have code generated for them but the code will be commented out.
 * Types that generate the following C++ types are ignored:
 * TSet<T>, TMap<T>
 *
 *
 */

#include <string>
#include <format>
#include <bit>
#include <utility>
#include <vector>
#include <type_traits>

#include <SDKGenerator/SDKGenerator.hpp>
#include <glaze/glaze.hpp>
#include <glaze/core/macros.hpp>
#include <UE4SSProgram.hpp>
#include <Constructs/Views/EnumerateView.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <Constructs/Views/EnumerateView.hpp>
#include <SDKGenerator/Common.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FWeakObjectProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/Property/FDelegateProperty.hpp>
#include <Unreal/Property/FMulticastInlineDelegateProperty.hpp>
#include <Unreal/Property/FMulticastSparseDelegateProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FNameProperty.hpp>
#include <Unreal/Property/FStrProperty.hpp>
#include <Unreal/Property/FTextProperty.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <Unreal/Property/FSetProperty.hpp>
#include <Unreal/Property/FSoftObjectProperty.hpp>
#include <Unreal/Property/FSoftClassProperty.hpp>
#include <Unreal/Property/FLazyObjectProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FInterfaceProperty.hpp>
#include <Unreal/Property/FFieldPathProperty.hpp>

namespace RC::UEGenerator
{
    using namespace RC::Unreal;

    enum class Indent
    {
        Yes,
        No
    };

    enum class IsFullPath
    {
        Yes,
        No,
    };

    struct RuntimeSDKTestPropertyData
    {
        FProperty* property{};
        StringType property_name{};
    };

    struct RuntimeSDKTestData
    {
        StringType struct_name{};
        std::vector<RuntimeSDKTestPropertyData> properties{};
    };

    class GeneratedSDKFile
    {
      private:
        FName m_id{};
        std::filesystem::path m_full_file_path{};
        std::filesystem::path m_include_path{};
        StringType m_file_name{};
        StringType m_prologue{};
        StringType m_contents{};
        StringType m_namespace_suffix{};
        UObject* m_related_uobject{};
        std::vector<std::pair<std::filesystem::path, IsFullPath>> m_file_dependencies{};
        std::vector<StringType> m_forward_declarations{};
        size_t m_end_of_struct_padding{};
        size_t m_end_of_struct_padding_incursion{};
        size_t m_end_of_struct_padding_offset{};
        size_t m_last_unique_padding_number{};
        size_t m_end_of_struct_padding_start_pos{};
        RuntimeSDKTestData m_runtime_sdk_test_data{};
        int32_t m_prologue_scope_level{};
        int32_t m_contents_scope_level{};
        bool m_has_non_boilerplate_content{};
        bool m_might_need_namespace{true};
        bool m_is_class_banned{};

      public:
        GeneratedSDKFile() = default;
        explicit GeneratedSDKFile(std::filesystem::path full_file_path)
            : m_full_file_path(std::move(full_file_path)), m_file_name(to_generic_string(m_full_file_path.filename().c_str())),
              m_id(FName(to_generic_string(full_file_path.c_str()), FNAME_Add))
        {
        }

      public:
        auto id() -> FName
        {
            return m_id;
        }

        auto full_file_path() -> const std::filesystem::path&
        {
            return m_full_file_path;
        }

        auto include_path() -> std::filesystem::path&
        {
            return m_include_path;
        }

        auto file_name() -> StringViewType
        {
            return m_file_name;
        }

        auto prologue() -> StringType&
        {
            return m_prologue;
        }

        auto contents() -> StringType&
        {
            return m_contents;
        }

        auto namespace_suffix() -> StringType&
        {
            return m_namespace_suffix;
        }

        auto related_uobject() -> UObject*&
        {
            return m_related_uobject;
        }

        auto file_dependencies() -> std::vector<std::pair<std::filesystem::path, IsFullPath>>&
        {
            return m_file_dependencies;
        }

        auto forward_declarations() -> std::vector<StringType>&
        {
            return m_forward_declarations;
        }

        auto end_of_struct_padding() -> size_t&
        {
            return m_end_of_struct_padding;
        }

        auto end_of_struct_padding_incursion() -> size_t&
        {
            return m_end_of_struct_padding_incursion;
        }

        auto end_of_struct_padding_offset() -> size_t&
        {
            return m_end_of_struct_padding_offset;
        }

        auto last_unique_padding_number() -> size_t&
        {
            return m_last_unique_padding_number;
        }

        auto end_of_struct_padding_start_pos() -> size_t&
        {
            return m_end_of_struct_padding_start_pos;
        }

        auto runtime_sdk_test_data() -> RuntimeSDKTestData&
        {
            return m_runtime_sdk_test_data;
        }

        auto contents_scope_level() -> int32_t&
        {
            return m_contents_scope_level;
        }

        auto prologue_scope_level() -> int32_t&
        {
            return m_prologue_scope_level;
        }

        auto set_has_non_boilerplate_content(bool new_value) -> void
        {
            m_has_non_boilerplate_content = new_value;
        }

        auto set_might_need_namespace(bool new_value) -> void
        {
            m_might_need_namespace = new_value;
        }

        auto might_need_namespace() -> bool
        {
            return m_might_need_namespace;
        }

        auto is_class_banned() -> bool&
        {
            return m_is_class_banned;
        }

        auto write_to_disk() -> size_t
        {
            if (!m_has_non_boilerplate_content)
            {
                return 0;
            }
            prologue().append(contents());
            auto file = File::open(full_file_path(), File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
            file.write_string_to_file(prologue());
            return 1;
        }
    };

    class SDKGenerator
    {
      private:
        std::filesystem::path m_output_dir{};
        std::vector<std::shared_ptr<GeneratedSDKFile>> m_files{};
        std::unordered_map<UObject*, std::shared_ptr<GeneratedSDKFile>> m_file_map{};
        GeneratedSDKFile* m_current_file{};
        std::unordered_set<FName> m_non_code_file_ids{};
        std::unordered_set<FName> m_unique_enum_names{};
        std::unordered_set<UStruct*> m_banned_classes{};
        std::unordered_map<StringType, StringType> m_underlying_enum_types{};
        SDKBackendSettings* m_backend{};

      public:
        SDKGenerator(std::filesystem::path output_dir, SDKBackendSettings* backend) : m_output_dir(std::move(output_dir)), m_backend(backend)
        {
        }

      private:
        auto write_internal(StringType& out, StringViewType string, Indent should_indent = Indent::No, int32_t scope_level = 0) -> StringType::size_type
        {
            if (should_indent == Indent::Yes)
            {
                indent(out, scope_level);
            }
            auto current_pos = out.size();
            out.append(string);
            return current_pos;
        }

        auto write_internal(StringType& out, StringType::size_type pos, StringViewType string, Indent should_indent = Indent::No, int32_t scope_level = 0)
                -> StringType::size_type
        {
            if (should_indent == Indent::Yes)
            {
                indent(out, scope_level);
            }
            auto current_pos = out.size();
            out.insert(pos, string);
            return current_pos;
        }

        auto write_line_internal(StringType& out, int32_t& scope_level, StringViewType line) -> StringType::size_type
        {
            indent(out, scope_level);
            auto current_pos = write_internal(out, line);
            write_internal(out, STR("\n"));
            return current_pos;
        }

        auto write_line_internal(StringType& out, int32_t& scope_level, StringType::size_type pos, StringViewType line) -> StringType::size_type
        {
            indent(out, scope_level);
            return write_internal(out, pos, std::format(STR("{}\n"), line));
        }

        auto write_line_internal(StringType& out) -> void
        {
            write_internal(out, STR("\n"));
        }

        auto indent(StringType& out, int32_t& scope_level) -> void
        {
            for (auto i = 0; i < scope_level; ++i)
            {
                out.append(STR("    "));
            }
        }

        auto generate_master_header() -> void
        {
            new_file(STR("src/UE4SS_SDK/Include"), STR("hpp"));
            current_file().set_might_need_namespace(false);
            m_non_code_file_ids.emplace(to_generic_string(current_file().full_file_path().c_str()), FNAME_Add);
            current_file().set_has_non_boilerplate_content(true);
            write_prologue_line(STR("#pragma once"));
            write_prologue_line();
            for (const auto& file : m_files)
            {
                static auto header_extension = STR(".") + m_backend->HeaderFileExtension;
                if (!file->include_path().empty() && file->full_file_path().extension() == header_extension && file->id() != current_file().id())
                {
                    write_line(std::format(STR("#include <UE4SS_SDK/{}{}>"), to_generic_string(file->include_path().c_str()), header_extension));
                }
            }
        }

        auto get_all_files_by_name(StringType file_name) -> std::vector<GeneratedSDKFile*>
        {
            std::vector<GeneratedSDKFile*> found_files{};
            for (auto& file : m_files)
            {
                if (file->file_name() == file_name)
                {
                    found_files.emplace_back(file.get());
                }
            }
            return found_files;
        }

        auto generate_namespaces_for_classes_with_identical_names() -> void
        {
            std::unordered_set<StringType> all_file_names{};
            std::vector<StringType> file_names_for_files_with_identical_names{};
            for (const auto& file : m_files)
            {
                if (m_non_code_file_ids.contains(file->id()))
                {
                    continue;
                }
                auto [_, emplaced] = all_file_names.emplace(file->file_name());
                if (!emplaced)
                {
                    file_names_for_files_with_identical_names.emplace_back(file->file_name());
                }
            }
            for (const auto& file_name : file_names_for_files_with_identical_names)
            {
                auto files_with_identical_names = get_all_files_by_name(file_name);
                for (auto& file : files_with_identical_names)
                {
                    Output::send(STR("File: {}\n"), file->full_file_path().wstring());
                    std::filesystem::path inner_namespace_name =
                            file->related_uobject() ? std::filesystem::path{file->related_uobject()->GetOutermost()->GetName()} : std::filesystem::path{};
                    if (!inner_namespace_name.empty())
                    {
                        inner_namespace_name = inner_namespace_name.parent_path().stem();
                    }
                    file->namespace_suffix() = to_generic_string(inner_namespace_name.c_str());
                }
            }
        }

      public:
        auto generate() -> void
        {
            std::vector<UEnum*> enums{};

            UObjectGlobals::ForEachUObject([&](UObject* object, ...) {
                // TODO: Change underlying type of 'ExcludedTypes' to be FName after adding FName as a custom type to glaze.
                if (auto it = m_backend->ExcludedTypes.find(object->GetNamePrivate().ToString()); it != m_backend->ExcludedTypes.end())
                {
                    return LoopAction::Continue;
                }

                if (object->IsA<UStruct>())
                {
                    if (object->IsA<UFunction>() || object->HasAnyFlags(static_cast<EObjectFlags>(RF_DefaultSubObject | RF_ArchetypeObject)))
                    {
                        return LoopAction::Continue;
                    }
                    generate_class(std::bit_cast<UClass*>(object), object->IsA<UScriptStruct>());
                }
                else if (object->IsA<UEnum>())
                {
                    // Delay generation of enums to allow FEnumProperty to define underlying types first.
                    enums.emplace_back(std::bit_cast<UEnum*>(object));
                }
                return LoopAction::Continue;
            });

            for (const auto& uenum : enums)
            {
                generate_enum(uenum);
            }

            generate_namespaces_for_classes_with_identical_names();

            generate_api_macro_file();
            generate_cmakelists();
            generate_master_header();
            generate_runtime_sdk_test();

            Output::send<LogLevel::Verbose>(
                    STR("Structs/classes not memory accurate because of hidden padding at the end of base being used by this struct/class:\n"));
            UObjectGlobals::ForEachUObject([&](UObject* object, ...) {
                if (!object->IsA<UStruct>() || object->IsA<UFunction>())
                {
                    return LoopAction::Continue;
                }
                auto as_ustruct = (UStruct*)object;
                if (!as_ustruct->GetSuperStruct() || !as_ustruct->GetFirstProperty())
                {
                    return LoopAction::Continue;
                }
                if (as_ustruct->GetFirstProperty()->GetOffset_Internal() < as_ustruct->GetSuperStruct()->GetStructureSize())
                {
                    Output::send<LogLevel::Warning>(STR("'{}' uses hidden padding from base '{}'\n"),
                                                    as_ustruct->GetName(),
                                                    as_ustruct->GetSuperStruct()->GetName());
                }
                return LoopAction::Continue;
            });
        }

        auto is_class_banned() -> bool&
        {
            return current_file().is_class_banned();
        }

        auto start_prologue_scope() -> void
        {
            ++current_file().prologue_scope_level();
        }

        auto end_prologue_scope() -> void
        {
            if (current_file().prologue_scope_level() > 0)
            {
                --current_file().prologue_scope_level();
            }
        }

        auto start_scope() -> void
        {
            ++current_file().contents_scope_level();
        }

        auto end_scope() -> void
        {
            if (current_file().contents_scope_level() > 0)
            {
                --current_file().contents_scope_level();
            }
        }

        auto write_prologue(StringViewType string, Indent should_indent = Indent::No) -> StringType::size_type
        {
            return write_internal(current_file().prologue(), string, should_indent);
        }

        auto write_prologue(StringType::size_type pos, StringViewType string, Indent should_indent = Indent::No) -> StringType::size_type
        {
            return write_internal(current_file().prologue(), pos, string, should_indent);
        }

        auto write_prologue_line(StringViewType line) -> StringType::size_type
        {
            return write_line_internal(current_file().prologue(), current_file().prologue_scope_level(), line);
        }

        auto write_prologue_line(StringType::size_type pos, StringViewType line) -> StringType::size_type
        {
            return write_line_internal(current_file().prologue(), current_file().prologue_scope_level(), pos, line);
        }

        auto write_prologue_line() -> void
        {
            write_line_internal(current_file().prologue());
        }

        auto write(StringViewType string, Indent should_indent = Indent::No) -> StringType::size_type
        {
            return write_internal(current_file().contents(), string, should_indent, current_file().contents_scope_level());
        }

        auto write(StringType::size_type pos, StringViewType string, Indent should_indent = Indent::No) -> StringType::size_type
        {
            return write_internal(current_file().contents(), pos, string, should_indent, current_file().contents_scope_level());
        }

        auto write_line(StringViewType line) -> StringType::size_type
        {
            return write_line_internal(current_file().contents(), current_file().contents_scope_level(), line);
        }

        auto write_line(StringType::size_type pos, StringViewType line) -> StringType::size_type
        {
            return write_line_internal(current_file().contents(), current_file().contents_scope_level(), pos, line);
        }

        auto write_line() -> void
        {
            write_line_internal(current_file().contents());
        }

        auto generate_ue_make_string_macro() -> void
        {
            write_line(STR("#define UE_MAKE_STRING(S) L##S"));
            write_line();
        }

        auto generate_ue_begin_function_body_internal_macro() -> void
        {
            write_line(STR("#define UE_BEGIN_FUNCTION_BODY_INTERNAL(FunctionPath, ParmsSize) \\"));
            write_line(STR("if (!TheFunction) \\"));
            write_line(STR("{ \\"));
            start_scope();
            write_line(STR("throw std::runtime_error{\"Unable to find '\" FunctionPath \"'\"}; \\"));
            end_scope();
            write_line(STR("} \\"));
            write_line(STR("auto ParamData = static_cast<uint8_t*>(_malloca(ParmsSize)); \\"));
            write_line(STR("memset(ParamData, 0, ParmsSize);"));
            write_line();
        }

        auto generate_ue_begin_script_function_body_macro() -> void
        {
            write_line(STR("#define UE_BEGIN_SCRIPT_FUNCTION_BODY(FunctionPath, ParmsSize) \\"));
            // Deal with 'FindObject<T>' being inside a namespace in the UE4SS backend.
            // Add a setting to control this, and/or move export it outside the namespace in UE4SS.
            write_line(std::format(STR("auto TheFunction = {}UObjectGlobals::FindObject<{}UFunction>(nullptr, L##FunctionPath); \\"),
                                   get_implementation_namespace_name(),
                                   get_implementation_namespace_name()));
            write_line(STR("UE_BEGIN_FUNCTION_BODY_INTERNAL(FunctionPath, ParmsSize)"));
            write_line();
        }

        auto generate_ue_begin_native_function_body_macro() -> void
        {
            write_line(STR("#define UE_BEGIN_NATIVE_FUNCTION_BODY(FunctionPath, ParmsSize) \\"));
            // Deal with 'FindObject<T>' being inside a namespace in the UE4SS backend.
            // Add a setting to control this, and/or move export it outside the namespace in UE4SS.
            write_line(std::format(STR("static auto TheFunction = {}UObjectGlobals::FindObject<{}UFunction>(nullptr, L##FunctionPath); \\"),
                                   get_implementation_namespace_name(),
                                   get_implementation_namespace_name()));
            write_line(STR("UE_BEGIN_FUNCTION_BODY_INTERNAL(FunctionPath, ParmsSize)"));
            write_line();
        }

        auto generate_ue_set_static_self_macro() -> void
        {
            write_line(STR("#define UE_SET_STATIC_SELF(ObjectPath) \\"));
            // Deal with 'FindObject<T>' being inside a namespace in the UE4SS backend.
            // Add a setting to control this, and/or move export it outside the namespace in UE4SS.
            write_line(std::format(STR("static auto StaticSelf = {}UObjectGlobals::FindObject<{}UObject>(nullptr, L##ObjectPath);"),
                                   get_implementation_namespace_name(),
                                   get_implementation_namespace_name()));
            write_line();
        }

        auto generate_ue_copy_property_custom_macro() -> void
        {
            write_line(STR("#define UE_COPY_PROPERTY_CUSTOM(CXXPropertyName, TypeName, PropertyValueOffset) \\"));
            write_line(STR("*std::bit_cast<TypeName*>(&ParamData[PropertyValueOffset]) = *std::bit_cast<TypeName*>(&CXXPropertyName);"));
            write_line();
        }

        auto generate_ue_copy_struct_inner_property_custom_macro() -> void
        {
            write_line(STR("#define UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM(TypeName, CXXValue, StructStartOffset, PropertyValueOffset) \\"));
            write_line(STR("*std::bit_cast<TypeName*>(&ParamData[StructStartOffset + PropertyValueOffset]) = static_cast<TypeName>(CXXValue);"));
            write_line();
        }

        auto generate_ue_copy_out_property_custom_macro() -> void
        {
            write_line(STR("#define UE_COPY_OUT_PROPERTY_CUSTOM(CXXPropertyName, TypeName, PropertyValueOffset) \\"));
            write_line(STR("CXXPropertyName = *std::bit_cast<TypeName*>(&ParamData[PropertyValueOffset]);"));
            write_line();
        }

        auto generate_ue_call_function_macro() -> void
        {
            write_line(STR("#define UE_CALL_FUNCTION() \\"));
            write_line(STR("ProcessEvent(TheFunction, ParamData);"));
            write_line();
        }

        auto generate_ue_call_static_function_macro() -> void
        {
            write_line(STR("#define UE_CALL_STATIC_FUNCTION() \\"));
            write_line(STR("StaticSelf->ProcessEvent(TheFunction, ParamData);"));
            write_line();
        }

        auto generate_ue_return_property_custom_macro() -> void
        {
            write_line(STR("#define UE_RETURN_PROPERTY_CUSTOM(TypeName, PropertyValueOffset) \\"));
            write_line(STR("return *std::bit_cast<TypeName*>(&ParamData[PropertyValueOffset]);"));
            write_line();
        }

        auto generate_ue_return_vector_custom_macro() -> void
        {
            write_line(STR("#define UE_RETURN_VECTOR_CUSTOM(PropertyValueOffset) \\"));
            write_line(STR("return *std::bit_cast<FVector*>(&ParamData[PropertyValueOffset]); \\"));
            write_line();
        }

        auto generate_ue_return_string_custom_macro() -> void
        {
            write_line(STR("#define UE_RETURN_STRING_CUSTOM(PropertyValueOffset) \\"));
            write_line(std::format(STR("return std::bit_cast<{}FString*>(&ParamData[PropertyValueOffset])->GetCharArray(); \\"),
                                   get_implementation_namespace_name()));
            write_line();
        }

        auto generate_ue_copy_vector_custom_macro() -> void
        {
            // TODO: Better support for non-UE4SS implementations for copying FVector.
            //       This mainly means allowing a different function name than 'GetX' and also allowing non-function retrieval for backends that have a full FVector definition.
            write_line(STR("#define UE_COPY_VECTOR(FromName, StructStartOffset) \\"));
            if (Version::IsBelow(5, 0))
            {
                write_line(STR("UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM(float, FromName.X, StructStartOffset, 0x0) \\"));
                write_line(STR("UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM(float, FromName.Y, StructStartOffset, 0x4) \\"));
                write_line(STR("UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM(float, FromName.Z, StructStartOffset, 0x8) \\"));
            }
            else
            {
                write_line(STR("UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM(double, FromName.X, StructStartOffset, 0x0) \\"));
                write_line(STR("UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM(double, FromName.Y, StructStartOffset, 0x8) \\"));
                write_line(STR("UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM(double, FromName.Z, StructStartOffset, 0x10) \\"));
            }
            write_line();
        }

        auto generate_ue_with_outer_macro() -> void
        {
            write_line(STR("#define UE_WITH_OUTER(Outer, ...) Outer __VA_OPT__(<__VA_ARGS__>)"));
            write_line();
        }

        auto generate_ufunction_macros() -> void
        {
            generate_ue_make_string_macro();
            generate_ue_begin_function_body_internal_macro();
            generate_ue_begin_script_function_body_macro();
            generate_ue_begin_native_function_body_macro();
            generate_ue_set_static_self_macro();
            generate_ue_copy_property_custom_macro();
            generate_ue_copy_struct_inner_property_custom_macro();
            generate_ue_copy_out_property_custom_macro();
            generate_ue_call_function_macro();
            generate_ue_call_static_function_macro();
            generate_ue_return_property_custom_macro();
            generate_ue_return_vector_custom_macro();
            generate_ue_return_string_custom_macro();
            generate_ue_copy_vector_custom_macro();
            generate_ue_with_outer_macro();
        }

        auto generate_includes_for_platform_generic_types() -> void
        {
            bool all_files_are_identical = true;
            StringType last_file{};
            std::vector<StringType> all_includes{};
            auto get_dependency = [&](const StringType& dependency_name) {
                if (auto it = m_backend->UnreflectedTypes.find(dependency_name); it != m_backend->UnreflectedTypes.end())
                {
                    if (last_file != it->second)
                    {
                        all_files_are_identical = false;
                    }
                }
            };

            get_dependency(STR("uint8"));
            get_dependency(STR("uint16"));
            get_dependency(STR("uint32"));
            get_dependency(STR("uint64"));
            get_dependency(STR("int8"));
            get_dependency(STR("int16"));
            get_dependency(STR("int32"));
            get_dependency(STR("int64"));
            get_dependency(STR("TCHAR"));
            get_dependency(STR("SIZE_T"));
            get_dependency(STR("bool32"));
            get_dependency(STR("bool64"));
            get_dependency(STR("ANSICHAR"));
            get_dependency(STR("WIDECHAR"));
            get_dependency(STR("UCS2CHAR"));

            if (all_files_are_identical && !all_includes.empty())
            {
                write_prologue_line(std::format(STR("#include <{}>"), all_includes.front()));
            }
            else
            {
                for (const auto& include : all_includes)
                {
                    write_prologue_line(std::format(STR("#include <{}>"), include));
                }
            }
        }

        auto generate_api_macro_file() -> void
        {
            new_file(STR("src/UE4SS_SDK/Macros"), STR("hpp"));
            m_non_code_file_ids.emplace(to_generic_string(current_file().full_file_path().c_str()), FNAME_Add);
            current_file().set_has_non_boilerplate_content(true);
            write_prologue_line(STR("#pragma once"));
            write_prologue_line();
            write_prologue_line(STR("#include <cstring>"));
            write_prologue_line(STR("#include <cstdint>"));
            write_prologue_line(STR("#include <unordered_map>"));
            write_prologue_line(STR("#include <malloc.h>"));
            // These should probably be changed to either use the ini file for the file name, or they should always use the exact same file name as in UE.
            write_prologue_line(std::format(STR("#include <{}UObjectGlobals.{}>"), get_header_prefix(), m_backend->HeaderFileExtension));
            write_prologue_line(std::format(STR("#include <{}NameTypes.{}>"), get_header_prefix(), m_backend->HeaderFileExtension));
            write_prologue_line(std::format(STR("#include <{}UScriptStruct.{}>"), get_header_prefix(), m_backend->HeaderFileExtension));
            write_prologue_line(std::format(STR("#include <{}UClass.{}>"), get_header_prefix(), m_backend->HeaderFileExtension));
            write_prologue_line(std::format(STR("#include <{}FProperty.{}>"), get_header_prefix(), m_backend->HeaderFileExtension));
            generate_includes_for_platform_generic_types();
            write_prologue_line();
            start_scope(); // Namespace
            generate_using_namespace_statement_for_unreal_implementation();
            write_line();
            write_line(STR("#ifndef RC_UE4SS_SDK_EXPORTS"));
            write_line(STR("#ifndef RC_UE4SS_SDK_BUILD_STATIC"));
            write_line(STR("#ifndef RC_UE4SS_SDK_API"));
            write_line(STR("#define RC_UE4SS_SDK_API __declspec(dllimport)"));
            write_line(STR("#endif"));
            write_line(STR("#else"));
            write_line(STR("#ifndef RC_UE4SS_SDK_API"));
            write_line(STR("#define RC_UE4SS_SDK_API"));
            write_line(STR("#endif"));
            write_line(STR("#endif"));
            write_line(STR("#else"));
            write_line(STR("#ifndef RC_UE4SS_SDK_API"));
            write_line(STR("#define RC_UE4SS_SDK_API __declspec(dllexport)"));
            write_line(STR("#endif"));
            write_line(STR("#endif"));
            write_line();
            generate_ufunction_macros();
            end_scope(); // Namespace
        }

        auto generate_cmakelists() -> void
        {
            new_file(STR("CMakeLists"), STR("txt"));
            current_file().set_might_need_namespace(false);
            m_non_code_file_ids.emplace(to_generic_string(current_file().full_file_path().c_str()), FNAME_Add);
            current_file().set_has_non_boilerplate_content(true);
            write_line(STR("cmake_minimum_required(VERSION 3.18)"));
            write_line();
            write_line(STR("set(TARGET UE4SS_SDK)"));
            write_line(STR("project(${TARGET})"));
            write_line();
            write_line(STR("add_library(${TARGET} INTERFACE)"));
            write_line(STR("target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)"));
        }

        auto delete_old_files_from_disk() -> void
        {
            if (!std::filesystem::exists(m_output_dir))
            {
                return;
            }

            if (std::filesystem::exists(m_output_dir))
            {
                std::filesystem::remove_all(m_output_dir);
            }
        }

        auto generate_forward_declarations(GeneratedSDKFile* file) -> void
        {
            std::sort(file->forward_declarations().begin(), file->forward_declarations().end());
            auto last = std::unique(file->forward_declarations().begin(), file->forward_declarations().end());
            file->forward_declarations().erase(last, file->forward_declarations().end());
            if (!file->forward_declarations().empty())
            {
                ++file->prologue_scope_level();
            }
            for (const auto& forward_declaration : file->forward_declarations())
            {
                write_line_internal(file->prologue(), file->prologue_scope_level(), std::format(STR("{}"), forward_declaration));
            }
            if (!file->forward_declarations().empty())
            {
                --file->prologue_scope_level();
                write_line_internal(file->prologue());
            }
        }

        auto get_namespace(UStruct* ustruct, bool is_script_struct, GeneratedSDKFile* file_in = nullptr) -> StringType
        {
            auto file = file_in ? file_in : &current_file();
            StringType namespace_to_use{};
            static auto coreuobject_object = UObjectGlobals::FindObject<UStruct>(nullptr, STR("/Script/CoreUObject.Object"));
            if (ustruct == coreuobject_object)
            {
                namespace_to_use = m_backend->UnrealImplementationNamespace;
            }
            else if (auto it = m_backend->UnreflectedTypes.find(get_native_class_or_struct_name(ustruct, is_script_struct));
                     it != m_backend->UnreflectedTypes.end())
            {
                namespace_to_use = m_backend->UnrealImplementationNamespace;
            }
            else if (auto it2 = m_backend->ExcludedTypes.find(ustruct->GetName()); it2 != m_backend->ExcludedTypes.end())
            {
                namespace_to_use = m_backend->UnrealImplementationNamespace;
            }
            else
            {
                namespace_to_use = m_backend->SDKNamespace;
            }
            if (!file->namespace_suffix().empty())
            {
                namespace_to_use.append(std::format(STR("::{}"), file->namespace_suffix()));
            }
            namespace_to_use.append(STR("::"));
            return namespace_to_use;
        }

        auto generate_runtime_sdk_test() -> void
        {
            new_file(STR("src/UE4SS_SDK/RuntimeSDKTest"), STR("hpp"));
            current_file().set_has_non_boilerplate_content(true);
            start_scope(); // Namespace
            write_line(STR("void run_test()"));
            write_line(STR("{"));
            start_scope();
            write_line(STR(R"(Output::send<LogLevel::Verbose>(STR("Running runtime SDK test\n"));)"));
            for (const auto& file : m_files)
            {
                if (!file->related_uobject() || !file->related_uobject()->IsA<UStruct>())
                {
                    continue;
                }
                auto as_ustruct = static_cast<UStruct*>(file->related_uobject());
                StringType namespace_to_use = std::format(STR("{}"), get_namespace(as_ustruct, as_ustruct->IsA<UScriptStruct>(), file.get()));
                StringType struct_name = namespace_to_use + file->runtime_sdk_test_data().struct_name;
                for (const auto& property_data : file->runtime_sdk_test_data().properties)
                {
                    if (auto as_bool_property = CastField<FBoolProperty>(property_data.property); as_bool_property)
                    {
                        if (as_bool_property->GetFieldMask() != 255)
                        {
                            continue;
                        }
                    }
                    write_line(std::format(STR("if (offsetof({}, {}) != 0x{:X})"),
                                           struct_name,
                                           property_data.property_name,
                                           property_data.property->GetOffset_Internal()));
                    write_line(STR("{"));
                    start_scope();
                    write_line(std::format(STR("Output::send<LogLevel::Error>(STR(\"Class '{}' not memory accurate because of property '{}', 0x{{:X}} != "
                                               "0x{:X}\\n\"), offsetof({}, {}));"),
                                           struct_name,
                                           property_data.property_name,
                                           property_data.property->GetOffset_Internal(),
                                           struct_name,
                                           property_data.property_name));
                    end_scope();
                    write_line(STR("}"));
                }
            }
            write_line(STR(R"(Output::send<LogLevel::Verbose>(STR("Runtime SDK test done!\n"));)"));
            end_scope();
            write_line(STR("}"));
            end_scope(); // Namespace
        }

        struct FilesWritten
        {
            size_t number_of_files_total{};
            size_t number_of_files_actually_written{};
        };
        auto write_to_disk() -> FilesWritten
        {
            FilesWritten files_written{};
            files_written.number_of_files_total = m_files.size();
            Output::send(STR("Writing {} files...\n"), files_written.number_of_files_total);
            for (const auto& file : m_files)
            {
                if (file->is_class_banned())
                {
                    write_line_internal(file->prologue(),
                                        file->prologue_scope_level(),
                                        STR("// Class/struct is commented out because of unsupported member variable types."));
                    write_line_internal(
                            file->prologue(),
                            file->prologue_scope_level(),
                            STR("// The class/struct cannot exist until these types are support or the class/struct would not be memory accurate."));
                    write_line_internal(file->prologue(), file->prologue_scope_level(), STR("/*"));
                }
                if (file->end_of_struct_padding() && file->end_of_struct_padding() - file->end_of_struct_padding_incursion() > 0)
                {
                    StringType buffer{file->might_need_namespace() ? STR("        ") : STR("    ")};
                    if (!file->namespace_suffix().empty())
                    {
                        buffer.append(STR("    "));
                    }
                    buffer.append(std::format(STR("uint8 padding_{}[0x{:X}]{{}}; // 0x{:X}"),
                                              ++file->last_unique_padding_number(),
                                              file->end_of_struct_padding() - file->end_of_struct_padding_incursion(),
                                              file->end_of_struct_padding_offset()));
                    file->contents().insert(file->end_of_struct_padding_start_pos(), std::format(STR("{}\n"), buffer));
                }
                if (file->might_need_namespace())
                {
                    start_namespace(file.get());
                    end_namespace(file.get());
                }
                generate_forward_declarations(file.get());
                if (file->is_class_banned())
                {
                    write_line_internal(file->contents(), file->contents_scope_level(), STR("//*/"));
                }
                files_written.number_of_files_actually_written += file->write_to_disk();
            }
            Output::send(STR("Done!\n"));
            return files_written;
        }

        auto new_file(const std::filesystem::path& file_path, StringViewType file_extension, UObject* related_uobject = nullptr) -> GeneratedSDKFile&
        {
            std::filesystem::path full_file_path = m_output_dir / file_path;
            full_file_path.replace_extension(file_extension);
            full_file_path = full_file_path.make_preferred();
            auto shared_file = m_files.emplace_back(std::make_shared<GeneratedSDKFile>(full_file_path));
            m_current_file = shared_file.get();
            if (related_uobject)
            {
                m_file_map.emplace(related_uobject, shared_file);
            }
            m_current_file->related_uobject() = related_uobject;
            return *m_current_file;
        }

        auto get_file_path_from_class(UObject* uobject) -> std::filesystem::path
        {
            if (!uobject)
            {
                return {};
            }

            UObject* package = uobject->GetOuterPrivate();
            while (package)
            {
                if (package->IsA<UPackage>())
                {
                    break;
                }
                else
                {
                    package = package->GetOuterPrivate();
                }
            }

            auto file_path = std::filesystem::path{&package->GetName()[1]};
            file_path /= uobject->GetName();
            file_path = file_path.make_preferred();
            return file_path;
        }

        auto new_header_file(UObject* uobject) -> GeneratedSDKFile&
        {
            new_file("src/UE4SS_SDK" / get_file_path_from_class(uobject), m_backend->HeaderFileExtension, uobject);
            if (uobject)
            {
                current_file().include_path() = convert_file_path_slashes_to_forward_slashes(get_file_path_from_class(uobject));
            }
            write_prologue_line(STR("#pragma once"));
            write_prologue_line();
            return current_file();
        }

        auto current_file() -> GeneratedSDKFile&
        {
            if (!m_current_file)
            {
                throw std::runtime_error{"SDKGenerator::current_file(): m_current_file is nullptr"};
            }
            return *m_current_file;
        }

        auto current_file_name() -> std::filesystem::path
        {
            return current_file().file_name();
        }

        auto convert_file_path_slashes_to_forward_slashes(const std::filesystem::path& file_path) -> std::filesystem::path
        {
            auto file_path_with_forward_slashes = file_path.string();
            std::replace(file_path_with_forward_slashes.begin(), file_path_with_forward_slashes.end(), '\\', '/');
            return file_path_with_forward_slashes;
        }

        auto add_file_dependency(const std::filesystem::path& file_path, IsFullPath is_full_path = IsFullPath::No) -> void
        {
            if (file_path.empty())
            {
                return;
            }
            auto file_path_with_forward_slashes = convert_file_path_slashes_to_forward_slashes(file_path);
            current_file().file_dependencies().emplace_back(file_path_with_forward_slashes, is_full_path);
        }

        auto generate_dependency_based_on_banned_deps(UObject* uobject) -> void
        {
            FName name{};
            if (!uobject)
            {
                Output::send<LogLevel::Warning>(STR("generate_dependency_based_on_banned_deps: UObject is nullptr.\n"));
                return;
            }
            else
            {
                name = uobject->GetNamePrivate();
            }

            if (auto it = m_backend->ExcludedTypes.find(name.ToString()); it == m_backend->ExcludedTypes.end())
            {
                add_file_dependency(get_file_path_from_class(uobject));
            }
        }

        auto get_unreflected_file_dependency(const StringType& dependency_name) -> const std::filesystem::path
        {
            if (auto it = m_backend->UnreflectedTypes.find(dependency_name); it != m_backend->UnreflectedTypes.end())
            {
                return it->second;
            }
            else
            {
                throw std::runtime_error{
                        std::format("SDKGenerator: get_unreflected_file_dependency: Unable to find dependency path for '{}'", to_string(dependency_name))};
            }
        }

        auto generate_dependency_requirements_for_property(FProperty* property, UStruct* struct_context) -> void
        {
            if (!property)
            {
                return;
            }

            if (property->IsA<FClassProperty>() || property->IsA<FAssetClassProperty>())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TSubclassOf"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                current_file().forward_declarations().emplace_back(
                        std::format(STR("class {};"), get_native_class_or_struct_name(std::bit_cast<FClassProperty*>(property)->GetMetaClass(), false)));
            }
            else if (auto as_soft_class_property = CastField<FSoftClassProperty>(property); as_soft_class_property)
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TSoftClassPtr"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                generate_dependency_based_on_banned_deps(as_soft_class_property->GetMetaClass());
                current_file().forward_declarations().emplace_back(
                        std::format(STR("class {};"), get_native_class_or_struct_name(as_soft_class_property->GetMetaClass(), false)));
            }
            else if (auto as_lazy_object_property = CastField<FLazyObjectProperty>(property); as_lazy_object_property)
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TLazyObjectPtr"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                generate_dependency_based_on_banned_deps(as_lazy_object_property->GetPropertyClass());
                current_file().forward_declarations().emplace_back(
                        std::format(STR("class {};"), get_native_class_or_struct_name(as_lazy_object_property->GetPropertyClass(), false)));
            }
            else if (auto as_soft_object_property = CastField<FSoftObjectProperty>(property); as_soft_object_property)
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TSoftObjectPtr"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                generate_dependency_based_on_banned_deps(as_soft_object_property->GetPropertyClass());
                current_file().forward_declarations().emplace_back(
                        std::format(STR("class {};"), get_native_class_or_struct_name(as_soft_object_property->GetPropertyClass(), false)));
            }
            else if (auto as_weak_object_ptr_property = CastField<FWeakObjectProperty>(property); as_weak_object_ptr_property)
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TWeakObjectPtr"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                current_file().forward_declarations().emplace_back(
                        std::format(STR("class {};"), get_native_class_or_struct_name(as_weak_object_ptr_property->GetPropertyClass(), false)));
            }
            else if (auto as_object_ptr_property = CastField<FObjectPtrProperty>(property); as_object_ptr_property)
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TObjectPtr"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                current_file().forward_declarations().emplace_back(
                        std::format(STR("class {};"), get_native_class_or_struct_name(as_object_ptr_property->GetPropertyClass(), false)));
            }
            else if (auto as_object_property_base = CastField<FObjectPropertyBase>(property); as_object_property_base)
            {
                current_file().forward_declarations().emplace_back(
                        std::format(STR("class {};"), get_native_class_or_struct_name(as_object_property_base->GetPropertyClass(), false)));
            }
            else if (auto as_struct_property = CastField<FStructProperty>(property); as_struct_property)
            {
                auto the_struct = as_struct_property->GetStruct();
                generate_dependency_based_on_banned_deps(the_struct);
            }
            else if (auto as_array_property = CastField<FArrayProperty>(property); as_array_property)
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TArray"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                generate_dependency_requirements_for_property(as_array_property->GetInner(), struct_context);
            }
            else if (auto as_map_property = CastField<FMapProperty>(property); as_map_property)
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TMap"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                generate_dependency_requirements_for_property(as_map_property->GetKeyProp(), struct_context);
                generate_dependency_requirements_for_property(as_map_property->GetValueProp(), struct_context);
            }
            else if (auto as_set_property = CastField<FSetProperty>(property); as_set_property)
            {
                // Commented until we have TSet support.
                // static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TSet"));
                // add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                generate_dependency_requirements_for_property(as_set_property->GetElementProp(), struct_context);
            }
            else if (auto as_byte_property = CastField<FByteProperty>(property); as_byte_property && as_byte_property->GetEnum())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TEnumAsByte"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                add_file_dependency(get_file_path_from_class(as_byte_property->GetEnum()));
            }
            else if (auto as_enum_property = CastField<FEnumProperty>(property); as_enum_property)
            {
                add_file_dependency(get_file_path_from_class(as_enum_property->GetEnum()));
                m_underlying_enum_types.emplace(get_native_enum_name(as_enum_property->GetEnum(), false),
                                                get_property_type_name(as_enum_property->GetUnderlyingProp(), struct_context, false));
            }
            else if (property->IsA<FDelegateProperty>())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("FScriptDelegate"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
            }
            else if (property->IsA<FMulticastInlineDelegateProperty>())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("FMulticastScriptDelegate"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
            }
            else if (property->IsA<FMulticastSparseDelegateProperty>())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("FSparseDelegate"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
            }
            else if (auto as_interface_property = CastField<FInterfaceProperty>(property); as_interface_property)
            {
                auto interface_class = as_interface_property->GetInterfaceClass();
                if (!interface_class || interface_class == UInterface::StaticClass())
                {
                    static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("FScriptInterface"));
                    add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                }
                else
                {
                    static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TScriptInterface"));
                    add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
                }
                current_file().forward_declarations().emplace_back(
                        std::format(STR("class {};"), get_native_class_or_struct_name(as_interface_property->GetInterfaceClass(), false)));
            }
            else if (property->IsA<FNameProperty>())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("FName"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
            }
            else if (property->IsA<FTextProperty>())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("FText"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
            }
            else if (property->IsA<FFieldPathProperty>())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("TFieldPath"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
            }
            else if (property->IsA<FStrProperty>())
            {
                static const std::filesystem::path unreflected_file_dependency = get_unreflected_file_dependency(STR("FString"));
                add_file_dependency(unreflected_file_dependency, IsFullPath::Yes);
            }
        }

        auto get_super_class_or_script_struct_name(UStruct* class_or_struct, bool is_script_struct) -> StringType
        {
            auto as_struct = std::bit_cast<UStruct*>(class_or_struct);
            StringType super_name{};
            if (auto super_struct = as_struct->GetSuperStruct(); super_struct)
            {
                super_name =
                        std::format(STR("{}{}"), get_namespace(super_struct, is_script_struct), get_native_class_or_struct_name(super_struct, is_script_struct));
            }
            else
            {
                super_name = std::format(STR("{}UObject"), get_namespace(as_struct, is_script_struct));
            }
            return super_name;
        }

        auto get_header_prefix() -> StringType
        {
            if (m_backend->IncludePrefix.empty())
            {
                return STR("");
            }
            else
            {
                return std::format(STR("{}/"), m_backend->IncludePrefix);
            }
        }

        auto get_implementation_namespace_name() -> StringType
        {
            if (m_backend->UnrealImplementationNamespace.empty())
            {
                return {};
            }
            else
            {
                static StringType s_calculated_namespace{};
                if (!s_calculated_namespace.empty())
                {
                    return s_calculated_namespace;
                }
                else
                {
                    s_calculated_namespace = m_backend->UnrealImplementationNamespace;
                    s_calculated_namespace.append(STR("::"));
                    return s_calculated_namespace;
                }
            }
        }

        auto generate_using_namespace_statement_for_unreal_implementation() -> void
        {
            static StringType s_calculated_statement{};
            if (!s_calculated_statement.empty())
            {
                write_line(s_calculated_statement);
            }
            else if (!m_backend->UnrealImplementationNamespace.empty())
            {
                s_calculated_statement = std::format(STR("using namespace {};"), m_backend->UnrealImplementationNamespace);
                write_line(s_calculated_statement);
            }
        }

        auto start_namespace(GeneratedSDKFile* file) -> void
        {
            if (!m_backend->SDKNamespace.empty())
            {
                write_line_internal(file->prologue(),
                                    file->prologue_scope_level(),
                                    std::format(STR("namespace {}{}"),
                                                m_backend->SDKNamespace,
                                                file->namespace_suffix().empty() ? STR("") : std::format(STR("::{}"), file->namespace_suffix())));
                write_line_internal(file->prologue(), file->prologue_scope_level(), STR("{"));
            }
        }

        auto end_namespace(GeneratedSDKFile* file) -> void
        {
            if (!m_backend->SDKNamespace.empty())
            {
                write_line_internal(file->contents(), file->contents_scope_level(), STR("}"));
            }
        }

        auto generate_regular_enum_definition(UEnum* uenum) -> void
        {
            write_line(std::format(STR("enum {}"), get_native_enum_name(uenum, false)));
            write_line(STR("{"));
            start_scope();
            generate_enum_value_definitions(
                    uenum,
                    [&](const auto& value) {
                        write_line(std::format(STR("{}"), value));
                    },
                    ShouldUseMacros::No,
                    UseFriendlyEnumNames::Yes,
                    &m_unique_enum_names);
            end_scope();
            write_line(STR("};"));
        }

        auto generate_namespaced_enum_definition(UEnum* uenum) -> void
        {
            write_line(std::format(STR("namespace {}"), get_native_enum_name(uenum, false)));
            write_line(STR("{"));
            start_scope();
            write_line(STR("enum Type"));
            write_line(STR("{"));
            start_scope();
            generate_enum_value_definitions(
                    uenum,
                    [&](const auto& value) {
                        write_line(std::format(STR("{}"), value));
                    },
                    ShouldUseMacros::No,
                    UseFriendlyEnumNames::Yes,
                    &m_unique_enum_names);
            end_scope();
            write_line(STR("};"));
            end_scope();
            write_line(STR("}"));
        }

        auto generate_class_enum_definition(UEnum* uenum) -> void
        {
            const auto underlying_type = m_underlying_enum_types.find(get_native_enum_name(uenum, false));
            const auto has_known_underlying_type = underlying_type != m_underlying_enum_types.end();
            if (!has_known_underlying_type)
            {
                bool enum_is_greater_than_signed_32_bit_integer{};
                bool enum_is_greater_than_signed_64_bit_integer{};
                for (const auto& [Name, Value] : uenum->ForEachName())
                {
                    if (Value >= std::numeric_limits<int32_t>::max())
                    {
                        enum_is_greater_than_signed_32_bit_integer = true;
                        break;
                    }
                    if (Value >= std::numeric_limits<int64_t>::max())
                    {
                        enum_is_greater_than_signed_64_bit_integer = true;
                        break;
                    }
                }
                if (enum_is_greater_than_signed_32_bit_integer)
                {
                    if (enum_is_greater_than_signed_64_bit_integer)
                    {
                        write_line(std::format(STR("enum class {} : uint64_t"), get_native_enum_name(uenum, false)));
                    }
                    else
                    {
                        write_line(std::format(STR("enum class {} : int64_t"), get_native_enum_name(uenum, false)));
                    }
                }
                else
                {
                    write_line(std::format(STR("enum class {}"), get_native_enum_name(uenum, false)));
                }
            }
            else
            {
                write_line(std::format(STR("enum class {} : {}"), get_native_enum_name(uenum, false), underlying_type->second));
            }
            write_line(STR("{"));
            start_scope();
            generate_enum_value_definitions(
                    uenum,
                    [&](const StringType& value) {
                        write_line(std::format(STR("{}"), value));
                    },
                    ShouldUseMacros::No,
                    UseFriendlyEnumNames::Yes);
            end_scope();
            write_line(STR("};"));
        }

        auto generate_enum_header(UEnum* uenum) -> void
        {
            new_header_file(uenum);
            current_file().set_has_non_boilerplate_content(true);

            write_prologue_line(STR("#include <UE4SS_SDK/Macros.hpp>"));
            write_prologue_line();

            start_scope(); // Namespace

            switch (uenum->GetCppForm())
            {
            case UEnum::ECppForm::Regular:
                generate_regular_enum_definition(uenum);
                break;
            case UEnum::ECppForm::Namespaced:
                generate_namespaced_enum_definition(uenum);
                break;
            case UEnum::ECppForm::EnumClass:
                generate_class_enum_definition(uenum);
                break;
            default:
                throw std::runtime_error(std::format("SDKGenerator::generate_enum_header: Unknown enum CppForm: {}",
                                                     static_cast<std::underlying_type_t<UEnum::ECppForm>>(uenum->GetCppForm())));
            }

            end_scope(); // Namespace
        }

        auto generate_includes_for_file_dependencies() -> void
        {
            if (current_file().file_dependencies().empty())
            {
                return;
            }
            std::sort(current_file().file_dependencies().begin(), current_file().file_dependencies().end());
            auto last = std::unique(current_file().file_dependencies().begin(), current_file().file_dependencies().end());
            current_file().file_dependencies().erase(last, current_file().file_dependencies().end());
            for (auto& [file_path, is_full_path] : current_file().file_dependencies())
            {
                if (is_full_path == IsFullPath::Yes)
                {
                    write_prologue_line(std::format(STR("#include <{}>"), to_generic_string(file_path.c_str())));
                }
                else
                {
                    write_prologue_line(std::format(STR("#include <UE4SS_SDK/{}.{}>"), to_generic_string(file_path.c_str()), m_backend->HeaderFileExtension));
                }
            }
        }

        struct StructContext
        {
            StructContext* super_context{};
            UStruct* current_struct{};
            FProperty* last_property{};
            std::unordered_map<StringType, uint8_t> unique_property_number{};
            size_t unique_padding_number{};
            int32_t last_offset{-1};
            int32_t last_size{-1};

            auto get_last_property() -> FProperty*
            {
                FProperty* found_property{};
                auto context = this;
                while (context)
                {
                    found_property = context->last_property;
                    if (found_property)
                    {
                        break;
                    }
                    context = context->super_context;
                }
                return found_property;
            }

            auto get_last_offset() -> int32_t
            {
                int32_t found_last_offset{};
                auto context = this;
                while (context)
                {
                    found_last_offset = context->last_offset;
                    if (found_last_offset != -1)
                    {
                        break;
                    }
                    context = context->super_context;
                }
                return found_last_offset == -1 ? 0 : found_last_offset;
            }

            auto get_last_size() -> int32_t
            {
                int32_t found_last_size{};
                auto context = this;
                while (context)
                {
                    found_last_size = context->last_size;
                    if (found_last_size != -1)
                    {
                        break;
                    }
                    context = context->super_context;
                }
                return found_last_size == -1 ? 0 : found_last_size;
            }
        };
        std::unordered_map<UStruct*, StructContext> s_struct_contexts{};

        auto get_struct_context(UStruct* ustruct) -> StructContext&
        {
            if (auto it = s_struct_contexts.find(ustruct); it != s_struct_contexts.end())
            {
                // Existing context found, use it.
                return it->second;
            }

            // If we get to this point, no context was found, so let's create a new context.
            auto& context = s_struct_contexts.emplace(ustruct, StructContext{}).first->second;
            context.current_struct = ustruct;
            if (ustruct->GetSuperStruct())
            {
                context.super_context = &get_struct_context(ustruct->GetSuperStruct());
            }
            return context;
        }

        template <typename T>
        auto get_sanitized_object_or_property_name(T* generic_type, StructContext* struct_context = nullptr) -> StringType
        {
            StringType name{};
            if constexpr (std::is_convertible_v<T, FProperty>)
            {
                // Variable names might be identical to builtin C++ type with the only difference being the casing.
                // FNames are typically not stored with casing preserved, so we have to make a guess that if 'var' is encountered, the name is actually 'Var'.
                static auto bool_name_lowercase = StringViewType{STR("bool")};
                static auto float_name_lowercase = StringViewType{STR("float")};
                static auto double_name_lowercase = StringViewType{STR("double")};
                static auto int8_name_lowercase = StringViewType{STR("int8")};
                static auto int16_name_lowercase = StringViewType{STR("int16")};
                static auto int32_name_lowercase = StringViewType{STR("int32")};
                static auto int64_name_lowercase = StringViewType{STR("int64")};
                static auto uint8_name_lowercase = StringViewType{STR("uint8")};
                static auto uint16_name_lowercase = StringViewType{STR("uint16")};
                static auto uint32_name_lowercase = StringViewType{STR("uint32")};
                static auto uint64_name_lowercase = StringViewType{STR("uint64")};
                if (generic_type->template IsA<FBoolProperty>() && generic_type->GetName() == bool_name_lowercase)
                {
                    name = STR("Bool");
                }
                else if (generic_type->template IsA<FFloatProperty>() && generic_type->GetName() == float_name_lowercase)
                {
                    name = STR("Float");
                }
                else if (generic_type->template IsA<FDoubleProperty>() && generic_type->GetName() == bool_name_lowercase)
                {
                    name = STR("Double");
                }
                else if (generic_type->template IsA<FInt8Property>() && generic_type->GetName() == int8_name_lowercase)
                {
                    name = STR("Int8");
                }
                else if (generic_type->template IsA<FInt16Property>() && generic_type->GetName() == int16_name_lowercase)
                {
                    name = STR("Int16");
                }
                else if (generic_type->template IsA<FIntProperty>() && generic_type->GetName() == int32_name_lowercase)
                {
                    name = STR("Int32");
                }
                else if (generic_type->template IsA<FInt64Property>() && generic_type->GetName() == int64_name_lowercase)
                {
                    name = STR("Int64");
                }
                else if (generic_type->template IsA<FByteProperty>() && generic_type->GetName() == uint8_name_lowercase)
                {
                    name = STR("UInt8");
                }
                else if (generic_type->template IsA<FUInt16Property>() && generic_type->GetName() == uint16_name_lowercase)
                {
                    name = STR("UInt16");
                }
                else if (generic_type->template IsA<FUInt32Property>() && generic_type->GetName() == uint32_name_lowercase)
                {
                    name = STR("UInt32");
                }
                else if (generic_type->template IsA<FUInt64Property>() && generic_type->GetName() == uint64_name_lowercase)
                {
                    name = STR("UInt64");
                }
                else
                {
                    static auto true_name_uppercase = StringViewType{STR("TRUE")};
                    static auto false_name_uppercase = StringViewType{STR("FALSE")};
                    if (generic_type->GetName() == true_name_uppercase)
                    {
                        name = STR("True");
                    }
                    else if (generic_type->GetName() == false_name_uppercase)
                    {
                        name = STR("False");
                    }
                    else if (std::iswdigit(generic_type->GetName()[0]))
                    {
                        name = std::format(STR("AutoNamedProp_{}"), generic_type->GetName());
                    }
                    else
                    {
                        name = generic_type->GetName();
                        name.erase(std::remove(name.begin(), name.end(), STR('(')), name.end());
                        name.erase(std::remove(name.begin(), name.end(), STR(')')), name.end());
                    }
                }
            }
            else
            {
                name = generic_type->GetName();
            }
            name.erase(std::remove(name.begin(), name.end(), STR(' ')), name.end());
            if constexpr (std::is_convertible_v<T, FProperty>)
            {
                if (struct_context)
                {
                    auto [it, was_emplaced] = struct_context->unique_property_number.emplace(name, 0);
                    ++it->second;
                    if (!was_emplaced && it->second > 0)
                    {
                        name.append(std::format(STR("_{}"), it->second));
                    }
                }
            }
            return name;
        }

        auto get_delegate_type_if_property_is_delegate(FProperty* property) -> std::pair<StringType, bool>
        {
            if (property->IsA<FDelegateProperty>())
            {
                return {STR("FScriptDelegate"), true};
            }
            else if (property->IsA<FMulticastInlineDelegateProperty>())
            {
                return {STR("FMulticastScriptDelegate"), true};
            }
            else if (property->IsA<FMulticastSparseDelegateProperty>())
            {
                return {STR("FSparseDelegate"), true};
            }
            else
            {
                return {{}, false};
            }
        }

        auto get_property_type_name(FProperty* property, UObject* class_context, bool can_be_ref) -> StringType
        {
            if (property)
            {
                StringType name{};
                auto has_out_parm_or_reference_parm = property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm);
                auto force_const_ref = !has_out_parm_or_reference_parm && property->IsA<FStrProperty>();
                if (force_const_ref)
                {
                    name.append(STR("const "));
                }
                auto [delegate_type, is_delegate] = get_delegate_type_if_property_is_delegate(property);
                if (is_delegate)
                {
                    name = delegate_type;
                }
                else
                {
                    if (auto as_array_property = CastField<FArrayProperty>(property); as_array_property)
                    {
                        auto [inner_delegate_type, is_inner_delegate] = get_delegate_type_if_property_is_delegate(as_array_property->GetInner());
                        if (is_inner_delegate)
                        {
                            name = std::format(STR("TArray<{}>"), inner_delegate_type);
                        }
                        else
                        {
                            name = generate_property_cxx_name(property, true, class_context);
                        }
                    }
                    else if (auto as_map_property = CastField<FMapProperty>(property); as_map_property)
                    {
                        auto [key_delegate_type, is_key_delegate] = get_delegate_type_if_property_is_delegate(as_map_property->GetKeyProp());
                        name.append(STR("TMap<"));
                        if (is_key_delegate)
                        {
                            name.append(key_delegate_type);
                        }
                        else
                        {
                            name.append(generate_property_cxx_name(as_map_property->GetKeyProp(), true, class_context));
                        }
                        auto [value_delegate_type, is_value_delegate] = get_delegate_type_if_property_is_delegate(as_map_property->GetValueProp());
                        if (is_value_delegate)
                        {
                            name.append(value_delegate_type);
                        }
                        else
                        {
                            name.append(generate_property_cxx_name(as_map_property->GetKeyProp(), true, class_context));
                        }
                        name.append(STR(">"));
                    }
                    else if (auto as_set_property = CastField<FSetProperty>(property); as_set_property)
                    {
                        auto [inner_delegate_type, is_inner_delegate] = get_delegate_type_if_property_is_delegate(as_set_property->GetElementProp());
                        if (is_inner_delegate)
                        {
                            name = std::format(STR("TSet<{}>"), inner_delegate_type);
                        }
                        else
                        {
                            name = generate_property_cxx_name(property, true, class_context);
                        }
                    }
                    else
                    {
                        name = generate_property_cxx_name(property, true, class_context);
                    }
                }
                if (can_be_ref && (has_out_parm_or_reference_parm || force_const_ref))
                {
                    name.append(STR("&"));
                }
                return name;
            }
            else
            {
                return STR("void");
            }
        }

        auto generate_ufunction_params(UFunction* ufunction) -> void
        {
            auto num_params = ufunction->GetNumParms();
            auto return_value_offset = ufunction->GetReturnValueOffset();

            // When return_value_offset is 0xFFFF it means that there is no return value so GetNumParms() is accurate
            // Otherwise you must subtract 1 to account for the return value (stored in the same struct and counts as a param)
            auto has_return_value = return_value_offset != 0xFFFF;
            auto num_actual_params = has_return_value ? (num_params == 0 ? 0 : num_params - 1) : num_params;

            for (const auto [param, i] : ufunction->ForEachProperty() | views::enumerate)
            {
                if (param->HasAnyPropertyFlags(CPF_ReturnParm) || !param->HasAnyPropertyFlags(CPF_Parm))
                {
                    continue;
                }

                generate_dependency_requirements_for_property(param, ufunction);

                write(std::format(STR("{} {}"), get_property_type_name(param, ufunction, true), get_sanitized_object_or_property_name(param)));
                if (i + 1 < num_actual_params)
                {
                    write(STR(", "));
                }
            }
        }

        auto get_member_type(UFunction* ufunction) -> StringType
        {
            return ufunction->HasAnyFunctionFlags(FUNC_Static) ? STR("static ") : STR("");
        }

        auto get_typeless_object_name(UObject* object) -> StringType
        {
            auto object_name = object->GetFullName();
            auto object_name_type_space_location = object_name.find(STR(' '));
            if (object_name_type_space_location == object_name.npos)
            {
                Output::send<LogLevel::Warning>(STR("Could not copy name of PlayerController, was unable to find space in full PlayerController name: '{}'."),
                                                object_name);
                return {};
            }
            else
            {
                if (object_name_type_space_location > static_cast<unsigned long long>(std::numeric_limits<long long>::max()))
                {
                    Output::send<LogLevel::Warning>(STR("integer overflow when converting pc_name_type_space_location to signed"));
                    return {};
                }
                else
                {
                    return StringType{object_name.begin() + static_cast<long long>(object_name_type_space_location) + 1, object_name.end()};
                }
            }
        }

        auto generate_begin_function_macro_call(UClass* uclass, UFunction* ufunction, bool class_is_native, bool class_contains_only_static_functions) -> void
        {
            if (class_is_native)
            {
                write_line(std::format(STR("UE_BEGIN_NATIVE_FUNCTION_BODY(\"{}\", {})"), get_typeless_object_name(ufunction), ufunction->GetParmsSize()));
            }
            else
            {
                write_line(std::format(STR("UE_BEGIN_SCRIPT_FUNCTION_BODY(\"{}\", {})"), get_typeless_object_name(ufunction), ufunction->GetParmsSize()));
            }

            if (class_contains_only_static_functions || ufunction->HasAnyFunctionFlags(FUNC_Static))
            {
                write_line(std::format(STR("UE_SET_STATIC_SELF(\"{}\")"), get_typeless_object_name(uclass->GetClassDefaultObject())));
            }
        }

        auto generate_copy_property_macro_calls(UFunction* ufunction) -> void
        {
            // TODO: ArrayProperty!
            //       Either use TArray<T> or convert to std::vector<T>.
            //       Right now, TArray<T> is implicitly used.
            for (const auto& param : ufunction->ForEachProperty())
            {
                if (!param->HasAnyPropertyFlags(CPF_Parm) || param->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
                {
                    continue;
                }

                if (auto as_struct_property = CastField<FStructProperty>(param); as_struct_property)
                {
                    static auto vector_name = FName(STR("Vector"));
                    auto the_struct = as_struct_property->GetStruct();
                    if (the_struct->GetNamePrivate() == vector_name)
                    {
                        write_line(std::format(STR("UE_COPY_VECTOR({}, 0x{:X})"), get_sanitized_object_or_property_name(param), param->GetOffset_Internal()));
                    }
                    else
                    {
                        for (const auto& inner_param : the_struct->ForEachProperty())
                        {
                            write_line(std::format(STR("UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM({}, {}.{}, 0x{:X}, 0x{:X})"),
                                                   get_property_type_name(inner_param, the_struct, false),
                                                   get_sanitized_object_or_property_name(param),
                                                   get_sanitized_object_or_property_name(inner_param),
                                                   param->GetOffset_Internal(),
                                                   inner_param->GetOffset_Internal()));
                        }
                    }
                }
                else
                {
                    write_line(std::format(STR("UE_COPY_PROPERTY_CUSTOM({}, {}, 0x{:X})"),
                                           get_sanitized_object_or_property_name(param),
                                           get_property_type_name(param, ufunction, false),
                                           param->GetOffset_Internal()));
                }
            }
        }

        auto generate_call_function_macro_call(UFunction* ufunction, bool class_contains_only_static_functions) -> void
        {
            if (class_contains_only_static_functions || ufunction->HasAnyFunctionFlags(FUNC_Static))
            {
                write_line(STR("UE_CALL_STATIC_FUNCTION()"));
            }
            else
            {
                write_line(STR("UE_CALL_FUNCTION()"));
            }
        }

        auto get_outered_type_name(FProperty* property, UObject* class_context) -> StringType
        {
            StringType type_name{};
            if (auto as_map_property = CastField<FMapProperty>(property); as_map_property)
            {
                type_name = std::format(STR("UE_WITH_OUTER(TMap, {}, {})"),
                                        get_property_type_name(as_map_property->GetKeyProp(), class_context, false),
                                        get_property_type_name(as_map_property->GetValueProp(), class_context, false));
                return type_name;
            }
            else
            {
                return get_property_type_name(property, class_context, false);
            }
        }

        auto generate_copy_out_property_macro_calls(UFunction* ufunction) -> void
        {
            for (const auto& param : ufunction->ForEachProperty())
            {
                if (param->HasAnyPropertyFlags(CPF_ReturnParm) || !param->HasAnyPropertyFlags(CPF_OutParm))
                {
                    continue;
                }

                write_line(std::format(STR("UE_COPY_OUT_PROPERTY_CUSTOM({}, {}, 0x{:X})"),
                                       get_sanitized_object_or_property_name(param),
                                       get_outered_type_name(param, ufunction),
                                       param->GetOffset_Internal()));
            }
        }

        auto generate_return_property_macro_call(UFunction* ufunction) -> void
        {
            auto return_property = ufunction->GetReturnProperty();
            if (!return_property)
            {
                return;
            }
            write_line(std::format(STR("UE_RETURN_PROPERTY_CUSTOM({}, 0x{:X})"),
                                   get_outered_type_name(return_property, ufunction),
                                   return_property->GetOffset_Internal()));
        }

        auto generate_ufunction_body(UClass* uclass, UFunction* ufunction, bool class_is_native, bool class_contains_only_static_functions) -> void
        {
            generate_begin_function_macro_call(uclass, ufunction, class_is_native, class_contains_only_static_functions);
            generate_copy_property_macro_calls(ufunction);
            generate_call_function_macro_call(ufunction, class_contains_only_static_functions);
            generate_copy_out_property_macro_calls(ufunction);
            generate_return_property_macro_call(ufunction);
        }

        auto generate_ufunction_definition(
                UClass* uclass, UFunction* ufunction, bool class_is_native, bool class_contains_only_static_functions, bool owner_is_script_struct) -> void
        {
            auto return_property = ufunction->GetReturnProperty();
            generate_dependency_requirements_for_property(return_property, ufunction);
            write(std::format(STR("{}{} {}("),
                              get_member_type(ufunction),
                              get_property_type_name(return_property, ufunction, false),
                              get_sanitized_object_or_property_name(ufunction)),
                  Indent::Yes);
            generate_ufunction_params(ufunction);
            write(STR(")"));
            write_line();
            write_line(STR("{"));
            start_scope();
            generate_ufunction_body(uclass, ufunction, class_is_native, class_contains_only_static_functions);
            end_scope();
            write_line(STR("}"));
        }

        auto get_native_class_or_struct_name(UStruct* class_or_struct, bool is_script_struct) -> StringType
        {
            if (is_script_struct)
            {
                return get_native_struct_name(std::bit_cast<UScriptStruct*>(class_or_struct));
            }
            else
            {
                bool should_use_interface_prefix = class_or_struct != UInterface::StaticClass() && class_or_struct->IsChildOf<UInterface>();
                return get_native_class_name(std::bit_cast<UClass*>(class_or_struct), should_use_interface_prefix);
            }
        }

        auto generate_static_size_for_struct(UStruct* ustruct) -> void
        {
            write_line(STR("static constexpr size_t StaticSize()"));
            write_line(STR("{"));
            start_scope();
            write_line(std::format(STR("return 0x{:X};"), ustruct->GetStructureSize()));
            end_scope();
            write_line(STR("}"));
            write_line();
        }

        auto generate_copy_assignment_operator(UStruct* ustruct) -> void
        {
            write_line(std::format(STR("{}& operator=(const {}& Other)"),
                                   get_native_class_or_struct_name(ustruct, true),
                                   get_native_class_or_struct_name(ustruct, true)));
            write_line(STR("{"));
            start_scope();
            write_line(STR("if (this == &Other) { return *this; }"));
            // Choosing to use memcpy to make sure every byte is copied instead of triggering operator= on each member.
            // When triggering operator= on each member, we become dependent on having support for all types.
            // If we don't support them all, then at best we can guarantee that unsupported types will be zerod instead of copied.
            // That wouldn't be ideal as we should allow backends to retrieve this data if they support the types even if the generator doesn't support them.
            write_line(STR("memcpy(this, &Other, StaticSize());"));
            write_line(STR("return *this;"));
            end_scope();
            write_line(STR("}"));
            write_line();
        }

        auto get_last_property_in_bitfield(UStruct* owner, FBoolProperty* property) -> std::pair<FBoolProperty*, uint8_t>
        {
            FBoolProperty* last_property_in_field{};
            uint8_t num_bits_in_field{};
            for (const auto& owner_property : owner->ForEachProperty())
            {
                if (owner_property->GetOffset_Internal() == property->GetOffset_Internal() && !std::bit_cast<FBoolProperty*>(owner_property)->IsNativeBool())
                {
                    last_property_in_field = std::bit_cast<FBoolProperty*>(owner_property);
                    ++num_bits_in_field;
                }
            }
            return {last_property_in_field, num_bits_in_field};
        }

        auto get_next_power_of_two(uint32_t number) -> uint32_t
        {
            number--;
            number |= number >> 1;
            number |= number >> 2;
            number |= number >> 4;
            number |= number >> 8;
            number |= number >> 16;
            number++;
            return number;
        }

        auto generate_member_variable_bit_padding(StructContext& struct_context, uint8_t next_bit, FBoolProperty* current_bit, uint8_t start_bit) -> void
        {
            if (next_bit < start_bit)
            {
                // An unknown number of bits of padding is needed before generating this bit.
                write_line(std::format(STR("uint8 padding_{} : 1{{}}; // 0x{:X} (0x{:X})"),
                                       ++struct_context.unique_padding_number,
                                       current_bit->GetOffset_Internal(),
                                       next_bit));
                next_bit = get_next_power_of_two(next_bit + 1);
                generate_member_variable_bit_padding(struct_context, next_bit, current_bit, start_bit);
            }
        }

        auto generate_member_variable_bitfield_bit(StructContext& struct_context, FBoolProperty* bit, StringViewType sanitized_property_name) -> void
        {
            /*
             *  Bitfields.
             *  The last number is the FieldMask, and the value is a power of two.
             *  We can use this to figure out where unreflected bits (if they exist) are in the bitfield.
             *  ----------------------------------------------------------------------
             *  Bitfield A: 7/8 bits are reflected
             *  ----------------------------------------------------------------------
             *  bNetTemporary 0 1 1 1
             *  bOnlyRelevantToOwner 0 1 4 4
             *  bAlwaysRelevant 0 1 8 8
             *  bReplicateMovement 0 1 16 16
             *  bCallPreReplication 0 1 32 32
             *  bCallPreReplicationForReplay 0 1 64 64
             *  bHidden 0 1 128 128
             *  ----------------------------------------------------------------------
             *  Bitfield B: 8/8 bits are reflected
             *  bTearOff 0 1 1 1
             *  bForceNetAddressable 0 1 2 2
             *  bExchangedRoles 0 1 4 4
             *  bNetLoadOnClient 0 1 8 8
             *  bNetUseOwnerRelevancy 0 1 16 16
             *  bRelevantForNetworkReplays 0 1 32 32
             *  bRelevantForLevelBounds 0 1 64 64
             *  bReplayRewindable 0 1 128 128
             *  ----------------------------------------------------------------------
             */

            auto last_bit = CastField<FBoolProperty>(struct_context.get_last_property());
            auto bits_are_in_same_field = last_bit && last_bit->GetOffset_Internal() == bit->GetOffset_Internal();
            auto current_field_mask = bit->GetFieldMask();
            auto last_bit_exists_and_is_not_immediately_previous_bit = last_bit && current_field_mask != last_bit->GetFieldMask() * 2;
            auto unreflected_bits_exists_before_this_bit = bits_are_in_same_field && last_bit_exists_and_is_not_immediately_previous_bit;
            if (unreflected_bits_exists_before_this_bit)
            {
                // Generate padding if we already have some bits in this field.
                generate_member_variable_bit_padding(struct_context, bits_are_in_same_field ? last_bit->GetFieldMask() * 2 : 1, bit, current_field_mask);
            }
            else if ((!bits_are_in_same_field || !last_bit) && current_field_mask != 1)
            {
                // Generate padding if this is the first reflected bit but there is at least one earlier unreflected bit.
                for (auto i = 1; i < current_field_mask; i *= 2)
                {
                    write_line(
                            std::format(STR("uint8 padding_{} : 1{{}}; // 0x{:X} (0x{:X})"), ++struct_context.unique_padding_number, bit->GetOffset_Internal(), i));
                }
            }

            write_line(std::format(STR("{} {} : 1{{}}; // 0x{:X} (0x{:X})"),
                                   get_property_type_name(bit, struct_context.current_struct, false),
                                   sanitized_property_name,
                                   bit->GetOffset_Internal(),
                                   bit->GetFieldMask()));
        }

        auto get_last_property_in_chain(UStruct* ustruct) -> FProperty*
        {
            FProperty* last_property{};
            for (const auto& property : ustruct->ForEachProperty())
            {
                last_property = property;
            }
            if (!last_property)
            {
                for (const auto& super_struct : ustruct->ForEachSuperStruct())
                {
                    for (const auto& property : super_struct->ForEachProperty())
                    {
                        last_property = property;
                    }
                    if (last_property)
                    {
                        break;
                    }
                }
            }

            return last_property;
        }

        auto generate_member_variable_padding(StructContext& struct_context, FProperty* current_property) -> void
        {
            auto last_property_in_this_struct = struct_context.get_last_property();
            auto [last_property, found_property_in_sub_struct] = [&] {
                auto last_property_context = last_property_in_this_struct;
                if (auto as_struct_property = CastField<FStructProperty>(last_property_context); as_struct_property)
                {
                    auto last_property_in_struct = get_last_property_in_chain(as_struct_property->GetStruct());
                    return std::pair{last_property_in_struct ? last_property_in_struct : last_property_context, true};
                }
                else
                {
                    return std::pair{last_property_context, false};
                }
            }();
            int32_t current_member_offset_if_all_reflected{};
            int32_t num_bytes_to_pad_by{};
            if (!last_property)
            {
                num_bytes_to_pad_by =
                        struct_context.current_struct->GetFirstProperty() && struct_context.current_struct->GetFirstProperty()->GetOffset_Internal() > 0
                                ? struct_context.current_struct->GetFirstProperty()->GetOffset_Internal()
                                : 0;
            }
            else
            {
                auto current_property_offset = current_property->GetOffset_Internal();
                auto last_member_offset = found_property_in_sub_struct ? last_property_in_this_struct->GetOffset_Internal() +
                                                                                 last_property->GetOffset_Internal() + last_property->GetSize()
                                                                       : last_property->GetOffset_Internal();
                auto last_member_size = last_property->IsA<FStructProperty>() && !std::bit_cast<FStructProperty*>(last_property)->GetStruct()->GetFirstProperty()
                                                ? 1
                                                : last_property_in_this_struct->GetSize();
                current_member_offset_if_all_reflected = found_property_in_sub_struct ? last_member_offset : last_member_offset + last_member_size;
                num_bytes_to_pad_by = current_property_offset - current_member_offset_if_all_reflected;
            }
            num_bytes_to_pad_by -= num_bytes_to_pad_by % current_property->GetMinAlignment();
            if (num_bytes_to_pad_by > 0)
            {
                write_line(std::format(STR("uint8 padding_{}[0x{:X}]{{}}; // 0x{:X}"),
                                       ++struct_context.unique_padding_number,
                                       num_bytes_to_pad_by,
                                       current_member_offset_if_all_reflected));
                struct_context.last_size = num_bytes_to_pad_by;
                struct_context.last_offset = current_member_offset_if_all_reflected;
            }
        }

        auto generate_member_variable_declaration(StructContext& struct_context, FProperty* property) -> void
        {
            /*
             *  Class with both reflected and unreflected variables.
             *  No virtual functions, therefore, no extra VTable.
             *  ----------------------------------------------------------------------
             *  /Script/CoreUObject.Object                                      : 0x00
             *  UObject Members                                                 :<0x28 (UObject Members)
             *  /Script/UE4SS_Main52.MyBaseCxxObject                            : 0x28 (Start)
             *  /Script/UE4SS_Main52.MyBaseCxxObject:Prop1_Base_Reflected       : 0x28
             *  /Script/UE4SS_Main52.MyBaseCxxObject:Prop3_Base_Reflected       : 0x30
             *  /Script/UE4SS_Main52.MyDerivedCxxObject                         : 0x??
             *  /Script/UE4SS_Main52.MyDerivedCxxObject:Prop2_Derived_Reflected : 0x3C
             */

            generate_dependency_requirements_for_property(property, struct_context.current_struct);

            // Add padding to finish the bitfield if the last property was the last bit in the field but there are more bits available.
            auto last_property = struct_context.get_last_property();
            if (last_property && last_property->IsA<FBoolProperty>())
            {
                auto last_property_in_bitfield = get_last_property_in_bitfield(struct_context.current_struct, static_cast<FBoolProperty*>(last_property));
                if (last_property == last_property_in_bitfield.first && last_property_in_bitfield.first->GetFieldMask() != 128)
                {
                    for (uint8_t field_mask = last_property_in_bitfield.first->GetFieldMask(); field_mask < 128; field_mask *= 2)
                    {
                        write_line(std::format(STR("uint8 padding_{} : 1{{}}; // 0x{:X} (0x{:X})"),
                                               ++struct_context.unique_padding_number,
                                               last_property->GetOffset_Internal(),
                                               field_mask * 2));
                    }
                }
            }
            generate_member_variable_padding(struct_context, property);
            auto sanitized_property_name = get_sanitized_object_or_property_name(property, &struct_context);
            auto current_member_offset = property->GetOffset_Internal();
            if (auto as_bool_property = CastField<FBoolProperty>(property); as_bool_property && as_bool_property->GetFieldMask() != 255)
            {
                // Add padding before this bit if there are unreflected bits between this bit and the last bit.
                generate_member_variable_bitfield_bit(struct_context, as_bool_property, sanitized_property_name);
            }
            else if (property->IsA<FMapProperty>() || property->IsA<FSetProperty>())
            {
                write_line(std::format(STR("// Type '{}' unsupported! Generated as padding instead."),
                                       get_property_type_name(property, struct_context.current_struct, false)));
                write_line(std::format(STR("uint8 {}[0x{:X}]{{}}; // 0x{:X}"), sanitized_property_name, property->GetSize(), current_member_offset));
            }
            else
            {
                StringType buffer{};
                static auto transform_struct = UObjectGlobals::FindObject<UScriptStruct>(nullptr, STR("/Script/CoreUObject.Transform"));
                if (struct_context.current_struct == transform_struct)
                {
                    buffer.append(STR("alignas(16) "));
                }
                buffer.append(std::format(STR("{} {}"), get_property_type_name(property, struct_context.current_struct, false), sanitized_property_name));
                if (auto array_dim = property->GetArrayDim(); array_dim > 1)
                {
                    buffer.append(STR("["));
                    buffer.append(std::format(STR("{}"), array_dim));
                    buffer.append(STR("]"));
                }
                buffer.append(std::format(STR("{{}}; // 0x{:X}"), current_member_offset));
                write_line(buffer);
            }

            struct_context.last_offset = current_member_offset;
            struct_context.last_size = property->GetSize();
            current_file().runtime_sdk_test_data().properties.emplace_back(property, sanitized_property_name);
        }

        auto generate_end_of_struct_padding(StructContext& struct_context) -> void
        {
            auto struct_size = struct_context.current_struct->GetStructureSize();
            if (!struct_context.current_struct->GetFirstProperty())
            {
                // There are no properties, but there might be unreflected data.
                auto num_bytes_to_pad_by = struct_size - (struct_context.super_context ? struct_context.super_context->current_struct->GetStructureSize() : 0);
                if (num_bytes_to_pad_by <= 0)
                {
                    return;
                }
                current_file().end_of_struct_padding() = num_bytes_to_pad_by;
                current_file().end_of_struct_padding_offset() = struct_context.get_last_offset() + struct_context.get_last_size();
                struct_context.last_offset = current_file().end_of_struct_padding_offset();
                struct_context.last_size = num_bytes_to_pad_by;
            }
            else if (auto predicted_struct_size = struct_context.last_offset + struct_context.last_size; predicted_struct_size != struct_size)
            {
                // There are unreflected member variables at the end of this struct.
                // Generated padding to ensure memory layout accuracy.
                auto num_bytes_to_pad_by = struct_size - struct_context.get_last_offset() - struct_context.get_last_size();
                if (num_bytes_to_pad_by <= 0)
                {
                    return;
                }
                auto padding_offset = struct_context.get_last_offset() + struct_context.get_last_size();
                current_file().end_of_struct_padding() = num_bytes_to_pad_by;
                current_file().end_of_struct_padding_offset() = padding_offset;
                struct_context.last_offset = padding_offset;
                struct_context.last_size = num_bytes_to_pad_by;
            }
            current_file().end_of_struct_padding_start_pos() = current_file().contents().size();
            current_file().last_unique_padding_number() = struct_context.unique_padding_number;
        }

        auto generate_core_uobject_class() -> void
        {
            auto implementation_namespace_name = get_implementation_namespace_name();
            if (implementation_namespace_name.empty())
            {
                implementation_namespace_name = STR("::");
            }
            write_line(std::format(STR("class RC_UE4SS_SDK_API UObject : public {}UObject"), implementation_namespace_name));
            write_line(STR("{"));
            start_scope();
            write_line(STR("void* VTable{};"));
            write_line(std::format(STR("{}EObjectFlags ObjectFlags{{}};"), implementation_namespace_name));
            write_line(STR("int32 InternalIndex{};"));
            write_line(STR("class UClass* ClassPrivate{};"));
            write_line(STR("FName NamePrivate{};"));
            write_line(STR("class UObject* OuterPrivate{};"));
            end_scope();
            write_line(STR("};"));
            end_scope(); // Namespace, from 'generate_class_header'
        }

        enum class BanType
        {
            NotBanned,
            Full,
            MemberVariableOnly,
            MemberFunctionOnly,
        };
        auto is_member_variable_type_banned(BanType ban_type) -> bool
        {
            return ban_type == BanType::Full || ban_type == BanType::MemberVariableOnly;
        }
        auto is_member_function_type_banned(BanType ban_type) -> bool
        {
            return ban_type == BanType::Full || ban_type == BanType::MemberFunctionOnly;
        }
        auto get_banned_type(FProperty* property) -> BanType
        {
            if (property && property->GetArrayDim() > 1)
            {
                return BanType::MemberFunctionOnly;
            }

            if (!property || property->IsA<FObjectProperty>())
            {
                return BanType::NotBanned;
            }
            else if (auto as_array_property = CastField<FArrayProperty>(property); as_array_property)
            {
                return (get_banned_type(as_array_property->GetInner()));
            }
            else if (auto as_map_property = CastField<FMapProperty>(property); as_map_property)
            {
                // Commented out until we support MapProperty.
                auto key_ban_type = get_banned_type(as_map_property->GetKeyProp());
                auto value_ban_type = get_banned_type(as_map_property->GetValueProp());
                if (key_ban_type == BanType::Full || value_ban_type == BanType::Full ||
                    (value_ban_type == BanType::MemberVariableOnly && key_ban_type == BanType::MemberFunctionOnly) ||
                    (value_ban_type == BanType::MemberFunctionOnly && key_ban_type == BanType::MemberVariableOnly))
                {
                    return BanType::Full;
                }
                else if (key_ban_type == BanType::MemberVariableOnly || value_ban_type == BanType::MemberVariableOnly)
                {
                    return BanType::MemberVariableOnly;
                }
                else if (key_ban_type == BanType::MemberFunctionOnly || value_ban_type == BanType::MemberFunctionOnly)
                {
                    return BanType::MemberFunctionOnly;
                }
                else
                {
                    return BanType::MemberFunctionOnly;
                }
            }
            else if (auto as_set_property = CastField<FSetProperty>(property); as_set_property)
            {
                // Commented out until we support SetProperty.
                // return get_banned_type(as_set_property->GetElementProp());
                return BanType::MemberFunctionOnly;
            }
            else if (auto as_struct_property = CastField<FStructProperty>(property); as_struct_property)
            {
                if (m_banned_classes.contains(as_struct_property->GetStruct()))
                {
                    return BanType::Full;
                }
                else
                {
                    BanType inner_ban_type{};
                    for (const auto& inner_property : as_struct_property->GetStruct()->ForEachPropertyInChain())
                    {
                        auto next_inner_ban_type = get_banned_type(inner_property);
                        if (next_inner_ban_type == BanType::Full)
                        {
                            return BanType::Full;
                        }
                        else if (next_inner_ban_type != BanType::NotBanned)
                        {
                            inner_ban_type = next_inner_ban_type;
                        }
                    }
                    return inner_ban_type;
                }
            }
            return BanType::NotBanned;
        }

        auto generate_class_header(UObject* class_or_struct, bool is_script_struct) -> void
        {
            new_header_file(class_or_struct);

            current_file().set_has_non_boilerplate_content(true);

            write_prologue_line(STR("#include <bit>"));
            write_prologue_line();
            write_prologue_line(STR("#include <UE4SS_SDK/Macros.hpp>"));
            write_prologue_line(std::format(STR("#include <{}UClass.{}>"), get_header_prefix(), m_backend->HeaderFileExtension));
            write_prologue_line(std::format(STR("#include <{}UFunction.{}>"), get_header_prefix(), m_backend->HeaderFileExtension));

            start_scope(); // Namespace

            auto as_struct = std::bit_cast<UStruct*>(class_or_struct);
            auto& current_struct_context = get_struct_context(as_struct);
            auto super_struct = as_struct->GetSuperStruct();

            if (super_struct)
            {
                if (m_banned_classes.contains(super_struct))
                {
                    Output::send(STR("Banning type '{}' because super is banned\n"), as_struct->GetFullName());
                    is_class_banned() = true;
                    m_banned_classes.emplace(as_struct);
                }
            }

            auto class_is_native = std::bit_cast<UClass*>(class_or_struct)->HasAnyClassFlags(CLASS_Native);
            write_line(std::format(STR("// Super Size: 0x{:X}"), super_struct ? super_struct->GetStructureSize() : 0));
            write_line(std::format(STR("// Size: 0x{:X}"), as_struct->GetStructureSize()));

            auto struct_or_class_name = get_native_class_or_struct_name(as_struct, is_script_struct);
            current_file().runtime_sdk_test_data().struct_name = struct_or_class_name;

            auto has_functions = as_struct->ForEachFunction().begin() != as_struct->ForEachFunction().end();
            bool has_properties = as_struct->GetFirstProperty();
            if (is_script_struct)
            {
                write_line(std::format(STR("struct RC_UE4SS_SDK_API {}{}"),
                                       struct_or_class_name,
                                       super_struct ? std::format(STR(" : public {}"), get_super_class_or_script_struct_name(as_struct, is_script_struct))
                                                    : STR("")));
            }
            else
            {
                write_line(std::format(STR("class RC_UE4SS_SDK_API {} : public {}"),
                                       struct_or_class_name,
                                       get_super_class_or_script_struct_name(as_struct, is_script_struct)));
            }
            write_line(STR("{"));
            write_line(STR("public:"));
            start_scope();
            if (is_script_struct)
            {
                generate_static_size_for_struct(as_struct);
                generate_copy_assignment_operator(as_struct);
            }
            write_line(STR("// Properties."));
            std::unordered_set<FName> getter_functions_generated{};
            for (const auto& property : as_struct->ForEachProperty())
            {
                auto banned_type = get_banned_type(property);
                if (is_member_variable_type_banned(banned_type))
                {
                    Output::send(STR("Banning type '{}' because type '{}' is banned\n"), as_struct->GetFullName(), property->GetFullName());
                    is_class_banned() = true;
                    m_banned_classes.emplace(as_struct);
                }
                if (is_member_variable_type_banned(banned_type) && !is_class_banned())
                {
                    write_line(STR("/*"));
                }
                generate_member_variable_declaration(current_struct_context, property);
                current_struct_context.last_property = property;
                if (is_member_variable_type_banned(banned_type) && !is_class_banned())
                {
                    write_line(STR("//*/"));
                }
            }
            // generate_end_of_struct_padding(current_struct_context);
            write_line();
            write_line(STR("// Functions."));
            bool class_contains_only_static_functions = true;
            for (const auto& ufunction : as_struct->ForEachFunction())
            {
                if (!ufunction->HasAnyFunctionFlags(FUNC_Static))
                {
                    class_contains_only_static_functions = false;
                    break;
                }
            }
            for (const auto& ufunction : as_struct->ForEachFunction())
            {
                auto banned_type = get_banned_type(ufunction->GetReturnProperty());
                if (!is_member_function_type_banned(banned_type))
                {
                    for (const auto& param : ufunction->ForEachProperty())
                    {
                        banned_type = get_banned_type(param);
                        if (is_member_function_type_banned(banned_type))
                        {
                            break;
                        }
                    }
                }
                if (getter_functions_generated.contains(ufunction->GetNamePrivate()))
                {
                    continue;
                }
                if (is_member_function_type_banned(banned_type) && !is_class_banned())
                {
                    write_line(STR("/*"));
                }
                generate_ufunction_definition(std::bit_cast<UClass*>(class_or_struct), ufunction, class_is_native, class_contains_only_static_functions, is_script_struct);
                if (is_member_function_type_banned(banned_type) && !is_class_banned())
                {
                    write_line(STR("//*/"));
                }
            }
            end_scope();
            write_line(STR("};"));
            write_line();
            end_scope(); // Namespace
            write_line();

            if (super_struct)
            {
                generate_dependency_based_on_banned_deps(super_struct);
            }
            generate_includes_for_file_dependencies();

            write_prologue_line();
        }

        auto generate_class(UClass* uclass, bool is_script_struct) -> void
        {
            generate_class_header(uclass, is_script_struct);
        }

        auto generate_enum(UEnum* uenum) -> void
        {
            generate_enum_header(uenum);
        }
    };

    auto generate_sdk(const std::filesystem::path& output_dir, SDKBackendSettings& backend_settings) -> void
    {
        if (backend_settings.SDKNamespace == STR("UE4SS_Default"))
        {
            backend_settings.SDKNamespace = STR("WSDK");
        }
        std::filesystem::path final_output_dir{};
        if (UE4SSProgram::settings_manager.SDKGenerator.OutputPath.empty())
        {
            final_output_dir = output_dir;
        }
        else
        {
            final_output_dir = UE4SSProgram::settings_manager.SDKGenerator.OutputPath;
            final_output_dir /= "UE4SS_SDK";
        }
        SDKGenerator generator{final_output_dir, &backend_settings};

        Output::send(STR("Generating SDK...\n"));
        generator.generate();
        Output::send(STR("SDK disk location: '{}'\n"), to_generic_string(final_output_dir.c_str()));
        Output::send(STR("Saving SDK to disk...\n"));
        generator.delete_old_files_from_disk();
        auto files_written = generator.write_to_disk();
        Output::send(STR("Total files written: {}\n"), files_written.number_of_files_actually_written);
        Output::send(STR("SDK generator all done!\n"));
    }
} // namespace RC::UEGenerator
