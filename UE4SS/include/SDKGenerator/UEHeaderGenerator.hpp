#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <File/File.hpp>
#include <SDKGenerator/Common.hpp>
#pragma warning(disable : 4005)
#include <Unreal/NameTypes.hpp>
#pragma warning(default : 4005)

#include <Unreal/Core/GenericPlatform/GenericPlatformStricmp.hpp>

namespace RC::Unreal
{
    class UObject;
    class UFunction;
    class UStruct;
    class FProperty;
    class UClass;
    class FField;
    class UEnum;
    class UScriptStruct;
} // namespace RC::Unreal

namespace RC::UEGenerator
{
    using FFilePath = std::filesystem::path;
    using UObject = RC::Unreal::UObject;
    using UStruct = RC::Unreal::UStruct;
    using UClass = RC::Unreal::UClass;
    using FProperty = RC::Unreal::FProperty;
    using FField = RC::Unreal::FField;
    using UEnum = RC::Unreal::UEnum;
    using UScriptStruct = RC::Unreal::UScriptStruct;
    using UFunction = RC::Unreal::UFunction;

    enum class DependencyLevel
    {
        NoDependency,
        /** Object dependency will result in a pre-declaration statement generation */
        PreDeclaration,
        /** Object dependency will result in the header include generation */
        Include,
    };

    enum class AccessModifier
    {
        None,
        Public,
        Protected,
        Private
    };

    struct ClassBlueprintInfo
    {
        bool is_blueprint_type;
        bool is_blueprintable;

        ClassBlueprintInfo() : is_blueprint_type(false), is_blueprintable(false)
        {
        }
    };

    struct PropertyTypeDeclarationContext
    {
        StringType context_name;

        class GeneratedSourceFile* source_file;

        bool is_top_level_declaration;
        bool* out_is_bitmask_bool;

        PropertyTypeDeclarationContext(const StringType& context_name,
                                       GeneratedSourceFile* source_file = NULL,
                                       bool is_top_level_declaration = false,
                                       bool* out_is_bitmask_bool = NULL)
        {
            this->context_name = context_name;
            this->source_file = source_file;
            this->is_top_level_declaration = is_top_level_declaration;
            this->out_is_bitmask_bool = out_is_bitmask_bool;
        }

        auto inner_context() const -> PropertyTypeDeclarationContext
        {
            return PropertyTypeDeclarationContext(context_name, source_file);
        }
    };

    struct StringInsensitiveCompare
    {
        auto operator()(const StringType& a, const StringType& b) const -> bool
        {
            return RC::Unreal::FGenericPlatformStricmp::Stricmp((RC::Unreal::UTF16CHAR*) a.c_str(), (RC::Unreal::UTF16CHAR*)b .c_str()) < 0;
        }
    };

    using CaseInsensitiveSet = std::set<StringType, StringInsensitiveCompare>;

    class GeneratedFile
    {
      protected:
        StringType m_file_base_name;
        FFilePath m_full_file_path;

        StringType m_file_contents_buffer;
        int32_t m_current_indent_count;

      public:
        GeneratedFile(const FFilePath& full_file_path);
        virtual ~GeneratedFile() = default;

        // Delete copy and move constructors and assignment operator
        GeneratedFile(const GeneratedFile&) = delete;
        GeneratedFile(GeneratedFile&&) = default;
        auto operator=(const GeneratedFile&) -> void = delete;

        auto append_line(const StringType& line) -> void;
        auto append_line_no_indent(const StringType& line) -> void;
        auto begin_indent_level() -> void;
        auto end_indent_level() -> void;
        auto serialize_file_content_to_disk() -> bool;

        virtual auto has_content_to_save() const -> bool;
        virtual auto generate_file_contents() -> StringType;
    };

    class GeneratedSourceFile : public GeneratedFile
    {
      private:
        StringType m_file_module_name;
        std::map<UObject*, DependencyLevel> m_dependencies;
        std::set<StringType> m_extra_includes;
        mutable std::set<StringType> m_dependency_module_names;
        UObject* m_object;
        GeneratedSourceFile* m_header_file;
        bool m_is_implementation_file;
        bool m_needs_get_type_hash;

      public:
        StringType m_implementation_constructor;
        std::unordered_set<StringType> parent_property_names{};
        std::map<FProperty*, std::tuple<StringType /*property type*/, StringType /*attach string*/, bool /*access type*/>> attachments{};

        GeneratedSourceFile(const FFilePath& file_path, const StringType& file_module_name, bool is_implementation_file, UObject* object);

        // Delete copy and move constructors and assignment operator
        GeneratedSourceFile(const GeneratedSourceFile&) = delete;
        GeneratedSourceFile(GeneratedSourceFile&&) = default;
        auto operator=(const GeneratedSourceFile&) -> void = delete;

        auto set_header_file(GeneratedSourceFile* header_file) -> void;
        auto add_dependency_object(UObject* object, DependencyLevel dependency_level) -> void;
        auto add_extra_include(const StringType& included_file_name) -> void;

        auto get_header_module_name() const -> const StringType&
        {
            return m_file_module_name;
        }
        auto is_implementation_file() const -> bool
        {
            return m_is_implementation_file;
        }

        auto get_current_string_position() -> size_t
        {
            return m_file_contents_buffer.size();
        }
        auto set_need_get_type_hash(bool new_value) -> void
        {
            m_needs_get_type_hash = new_value;
        }
        auto get_corresponding_object() -> UObject*
        {
            return m_object;
        }

        virtual auto has_content_to_save() const -> bool override;

        auto copy_dependency_module_names(std::set<StringType>& out_dependency_module_names) const -> void
        {
            out_dependency_module_names.insert(m_dependency_module_names.begin(), m_dependency_module_names.end());
        }

        auto static create_source_file(const FFilePath& root_dir,
                                       const StringType& module_name,
                                       const StringType& base_name,
                                       bool is_implementation_file,
                                       UObject* object) -> GeneratedSourceFile;
        virtual auto generate_file_contents() -> StringType override;

      protected:
        auto has_dependency(UObject* object, DependencyLevel dependency_level) -> bool;

        auto generate_pre_declarations_string() const -> StringType;
        auto generate_includes_string() const -> StringType;
    };

    struct UniqueName
    {
        static constexpr int32_t HAS_NO_DUPLICATES = 1;

        File::StringType name{};
        int32_t usable_id{HAS_NO_DUPLICATES};
    };

    class UEHeaderGenerator
    {
      private:
        FFilePath m_root_directory;
        StringType m_primary_module_name;

        std::set<StringType> m_forced_module_dependencies;
        std::set<StringType> m_ignored_module_names;
        std::set<StringType> m_classes_with_object_initializer;

        std::unordered_map<StringType, StringType> m_underlying_enum_types;
        std::set<StringType> m_blueprint_visible_enums;
        std::set<StringType> m_blueprint_visible_structs;
        std::map<StringType, std::shared_ptr<std::set<StringType>>> m_module_dependencies;

        std::vector<GeneratedSourceFile> m_header_files;
        std::unordered_set<UStruct*> m_structs_that_need_get_type_hash;

        // Storage to ensure that we don't have duplicate file names
        static std::map<File::StringType, UniqueName> m_used_file_names;
        static std::map<UObject*, int32_t> m_dependency_object_to_unique_id;

        // Storage for class defaultsubojects when populating property initializers
        std::unordered_map<StringType, StringType> m_class_subobjects;

      public:
        UEHeaderGenerator(const FFilePath& root_directory);

        // Delete copy, move and assignment operators
        UEHeaderGenerator(const UEHeaderGenerator&) = delete;
        UEHeaderGenerator(UEHeaderGenerator&&) = delete;
        auto operator=(const UEHeaderGenerator&) -> void = delete;

        auto ignore_selected_modules() -> void;

        auto dump_native_packages() -> void;
        auto generate_object_description_file(UObject* object) -> bool;
        auto generate_module_build_file(const StringType& module_name) -> void;
        auto generate_module_implementation_file(const StringType& module_name) -> void;

      private:
        auto generate_interface_definition(UClass* function, GeneratedSourceFile& header_data) -> void;
        auto generate_object_definition(UClass* interface_function, GeneratedSourceFile& header_data) -> void;
        auto generate_struct_definition(UScriptStruct* property, GeneratedSourceFile& header_data) -> void;
        auto generate_enum_definition(UEnum* name_pair, GeneratedSourceFile& header_data) -> void;
        auto generate_global_delegate_declaration(UFunction* signature_function, UClass* delegate_class, GeneratedSourceFile& header_data) -> void;
        auto generate_delegate_type_declaration(UFunction* signature_function, UClass* delagate_class, GeneratedSourceFile& header_data) -> void;
        auto generate_object_implementation(UClass* property, GeneratedSourceFile& implementation_file) -> void;
        auto generate_struct_implementation(UScriptStruct* property, GeneratedSourceFile& implementation_file) -> void;

        auto generate_property(UObject* uclass, FProperty* property, GeneratedSourceFile& header_data) -> void;
        auto generate_function(UClass* uclass,
                               UFunction* function,
                               GeneratedSourceFile& header_data,
                               bool is_generating_interface,
                               const CaseInsensitiveSet& blacklisted_property_names,
                               bool generate_as_override = false) -> void;

        auto generate_property_value(UStruct* ustruct, FProperty* property, void* object, GeneratedSourceFile& implementation_file, const StringType& property_scope)
                -> void;
        auto generate_function_implementation(UClass* uclass,
                                              UFunction* function,
                                              GeneratedSourceFile& implementation_file,
                                              bool is_generating_interface,
                                              const CaseInsensitiveSet& blacklisted_property_names) -> void;

        auto generate_interface_flags(UClass* uinterface) const -> StringType;
        auto generate_class_flags(UClass* uclass) const -> StringType;
        auto generate_struct_flags(UScriptStruct* script_struct) const -> StringType;
        auto generate_enum_flags(UEnum* uenum) const -> StringType;
        auto generate_property_type_declaration(FProperty* property, const PropertyTypeDeclarationContext& context) -> StringType;
        auto generate_property_flags(FProperty* property) const -> StringType;
        auto generate_function_argument_flags(FProperty* property) const -> StringType;
        auto generate_function_flags(UFunction* function, bool is_function_pure_virtual = false) const -> StringType;
        auto generate_function_parameter_list(UClass* property,
                                              UFunction* function,
                                              GeneratedSourceFile& header_data,
                                              bool generate_comma_before_name,
                                              const StringType& context_name,
                                              const CaseInsensitiveSet& blacklisted_property_names,
                                              int32_t* out_num_params = NULL) -> StringType;
        auto generate_default_property_value(FProperty* property, GeneratedSourceFile& header_data, const StringType& ContextName) -> StringType;

        auto generate_enum_value(UEnum* uenum, int64_t enum_value) -> StringType;
        auto generate_simple_assignment_expression(FProperty* property,
                                                   const StringType& value,
                                                   GeneratedSourceFile& implementation_file,
                                                   const StringType& property_scope,
                                                   const StringType& operator_type = STR(" = ")) -> void;
        auto generate_advanced_assignment_expression(FProperty* property,
                                                     const StringType& value,
                                                     GeneratedSourceFile& implementation_file,
                                                     const StringType& property_scope,
                                                     const StringType& property_type,
                                                     const StringType& operator_type = STR(" = ")) -> void;

        auto static generate_parameter_count_string(int32_t parameter_count) -> StringType;
        auto static determine_primary_game_module_name() -> StringType;

      public:
        auto add_module_and_sub_module_dependencies(std::set<StringType>& out_module_dependencies, const StringType& module_name, bool add_self_module = true)
                -> void;
        auto static collect_blacklisted_property_names(UObject* property) -> CaseInsensitiveSet;

        auto static generate_object_pre_declaration(UObject* object) -> std::vector<std::vector<StringType>>;

        auto static convert_module_name_to_api_name(const StringType& module_name) -> StringType;
        auto static get_module_name_for_package(UObject* package) -> StringType;
        auto static sanitize_enumeration_name(const StringType& enumeration_name) -> StringType;
        auto static get_highest_enum(UEnum* uenum) -> int64_t;
        auto static get_lowest_enum(UEnum* uenum) -> int64_t;

        auto static get_class_blueprint_info(UClass* function) -> ClassBlueprintInfo;
        auto static is_struct_blueprint_type(UScriptStruct* property) -> bool;
        auto static is_function_parameter_shadowing(UClass* property, FProperty* function_parameter) -> bool;

        auto static append_access_modifier(GeneratedSourceFile& header_data, AccessModifier needed_access, AccessModifier& current_access) -> void;
        auto static get_property_access_modifier(FProperty* property) -> AccessModifier;
        auto static get_function_access_modifier(UFunction* function) -> AccessModifier;
        auto static create_string_literal(const StringType& string) -> StringType;
        auto static get_header_name_for_object(UObject* object, bool get_existing_header = false) -> StringType;
        auto static generate_cross_module_include(UObject* object, const StringType& module_name, const StringType& fallback_name) -> StringType;
    };
} // namespace RC::UEGenerator
