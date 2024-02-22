#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <vector>

#include <File/File.hpp>
#include <SDKGenerator/Common.hpp>
#pragma warning(disable : 4005)
#include <Unreal/NameTypes.hpp>
#pragma warning(default : 4005)

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
    using FFilePath = ::std::filesystem::path;
    using UObject = ::RC::Unreal::UObject;
    using UStruct = ::RC::Unreal::UStruct;
    using UClass = ::RC::Unreal::UClass;
    using FProperty = ::RC::Unreal::FProperty;
    using FField = ::RC::Unreal::FField;
    using UEnum = ::RC::Unreal::UEnum;
    using UScriptStruct = ::RC::Unreal::UScriptStruct;
    using UFunction = ::RC::Unreal::UFunction;

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
        SystemStringType context_name;

        class GeneratedSourceFile* source_file;

        bool is_top_level_declaration;
        bool* out_is_bitmask_bool;

        PropertyTypeDeclarationContext(const SystemStringType& context_name,
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
        auto operator()(const SystemStringType& a, const SystemStringType& b) const -> bool
        {
            #ifdef LINUX
            // Create a case insensitive string compare for Linux
            return strcasecmp(a.c_str(), b.c_str()) < 0;
            #else
            return _wcsicmp(a.c_str(), b.c_str()) < 0;
            #endif
        }
    };

    using CaseInsensitiveSet = ::std::set<SystemStringType, StringInsensitiveCompare>;

    class UEGeneratedFile
    {
      protected:
        SystemStringType m_file_base_name;
        FFilePath m_full_file_path;

        SystemStringType m_file_contents_buffer;
        int32_t m_current_indent_count;

      public:
        UEGeneratedFile(const FFilePath& full_file_path);
        virtual ~UEGeneratedFile() = default;

        // Delete copy and move constructors and assignment operator
        UEGeneratedFile(const UEGeneratedFile&) = delete;
        UEGeneratedFile(UEGeneratedFile&&) = default;
        auto operator=(const UEGeneratedFile&) -> void = delete;

        auto append_line(const SystemStringType& line) -> void;
        auto append_line_no_indent(const SystemStringType& line) -> void;
        auto begin_indent_level() -> void;
        auto end_indent_level() -> void;
        auto serialize_file_content_to_disk() -> bool;

        virtual auto has_content_to_save() const -> bool;
        virtual auto generate_file_contents() -> SystemStringType;
    };

    class GeneratedSourceFile : public UEGeneratedFile
    {
      private:
        SystemStringType m_file_module_name;
        ::std::map<UObject*, DependencyLevel> m_dependencies;
        ::std::set<SystemStringType> m_extra_includes;
        mutable ::std::set<SystemStringType> m_dependency_module_names;
        UObject* m_object;
        GeneratedSourceFile* m_header_file;
        bool m_is_implementation_file;
        bool m_needs_get_type_hash;

      public:
        // workaround for clang tuple bug 
        // https://github.com/llvm/llvm-project/issues/17042
        struct attachment_data {
            SystemStringType property_type; // <0>
            SystemStringType attach_string; // <1>
            bool access_type; // <2>

            attachment_data(const SystemStringType& property_type, const SystemStringType& attach_string, bool access_type)
                : property_type(property_type), attach_string(attach_string), access_type(access_type)
            {
            }

            attachment_data(const attachment_data& other) = default;
            attachment_data(attachment_data&& other) = default;
            auto operator=(const attachment_data&) -> attachment_data& = default;
            auto operator=(attachment_data&&) -> attachment_data& = default;

        };
        SystemStringType m_implementation_constructor;
        ::std::unordered_set<SystemStringType> parent_property_names{};
        ::std::map<FProperty*, attachment_data> attachments{};

        GeneratedSourceFile(const FFilePath& file_path, const SystemStringType& file_module_name, bool is_implementation_file, UObject* object);

        // Delete copy and move constructors and assignment operator
        GeneratedSourceFile(const GeneratedSourceFile&) = delete;
        GeneratedSourceFile(GeneratedSourceFile&&) = default;
        auto operator=(const GeneratedSourceFile&) -> void = delete;

        auto set_header_file(GeneratedSourceFile* header_file) -> void;
        auto add_dependency_object(UObject* object, DependencyLevel dependency_level) -> void;
        auto add_extra_include(const SystemStringType& included_file_name) -> void;

        auto get_header_module_name() const -> const SystemStringType&
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

        auto copy_dependency_module_names(::std::set<SystemStringType>& out_dependency_module_names) const -> void
        {
            out_dependency_module_names.insert(m_dependency_module_names.begin(), m_dependency_module_names.end());
        }

        auto static create_source_file(const FFilePath& root_dir,
                                       const SystemStringType& module_name,
                                       const SystemStringType& base_name,
                                       bool is_implementation_file,
                                       UObject* object) -> GeneratedSourceFile;
        virtual auto generate_file_contents() -> SystemStringType override;

      protected:
        auto has_dependency(UObject* object, DependencyLevel dependency_level) -> bool;

        auto generate_pre_declarations_string() const -> SystemStringType;
        auto generate_includes_string() const -> SystemStringType;
    };

    struct UniqueName
    {
        static constexpr int32_t HAS_NO_DUPLICATES = 1;

        SystemStringType name{};
        int32_t usable_id{HAS_NO_DUPLICATES};
    };

    class UEHeaderGenerator
    {
      private:
        FFilePath m_root_directory;
        SystemStringType m_primary_module_name;

        ::std::set<SystemStringType> m_forced_module_dependencies;
        ::std::set<SystemStringType> m_ignored_module_names;
        ::std::set<SystemStringType> m_classes_with_object_initializer;

        ::std::unordered_map<SystemStringType, SystemStringType> m_underlying_enum_types;
        ::std::set<SystemStringType> m_blueprint_visible_enums;
        ::std::set<SystemStringType> m_blueprint_visible_structs;
        ::std::map<SystemStringType, ::std::shared_ptr<::std::set<SystemStringType>>> m_module_dependencies;

        ::std::vector<GeneratedSourceFile> m_header_files;
        ::std::unordered_set<UStruct*> m_structs_that_need_get_type_hash;

        // Storage to ensure that we don't have duplicate file names
        static ::std::map<SystemStringType, UniqueName> m_used_file_names;
        static ::std::map<UObject*, int32_t> m_dependency_object_to_unique_id;

        // Storage for class defaultsubojects when populating property initializers
        ::std::unordered_map<UEStringType, UEStringType> m_class_subobjects;

      public:
        UEHeaderGenerator(const FFilePath& root_directory);

        // Delete copy, move and assignment operators
        UEHeaderGenerator(const UEHeaderGenerator&) = delete;
        UEHeaderGenerator(UEHeaderGenerator&&) = delete;
        auto operator=(const UEHeaderGenerator&) -> void = delete;

        auto ignore_selected_modules() -> void;

        auto dump_native_packages() -> void;
        auto generate_object_description_file(UObject* object) -> bool;
        auto generate_module_build_file(const SystemStringType& module_name) -> void;
        auto generate_module_implementation_file(const SystemStringType& module_name) -> void;

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

        auto generate_property_value(UStruct* ustruct, FProperty* property, void* object, GeneratedSourceFile& implementation_file, const SystemStringType& property_scope)
                -> void;
        auto generate_function_implementation(UClass* uclass,
                                              UFunction* function,
                                              GeneratedSourceFile& implementation_file,
                                              bool is_generating_interface,
                                              const CaseInsensitiveSet& blacklisted_property_names) -> void;

        auto generate_interface_flags(UClass* uinterface) const -> SystemStringType;
        auto generate_class_flags(UClass* uclass) const -> SystemStringType;
        auto generate_struct_flags(UScriptStruct* script_struct) const -> SystemStringType;
        auto generate_enum_flags(UEnum* uenum) const -> SystemStringType;
        auto generate_property_type_declaration(FProperty* property, const PropertyTypeDeclarationContext& context) -> SystemStringType;
        auto generate_property_flags(FProperty* property) const -> SystemStringType;
        auto generate_function_argument_flags(FProperty* property) const -> SystemStringType;
        auto generate_function_flags(UFunction* function, bool is_function_pure_virtual = false) const -> SystemStringType;
        auto generate_function_parameter_list(UClass* property,
                                              UFunction* function,
                                              GeneratedSourceFile& header_data,
                                              bool generate_comma_before_name,
                                              const SystemStringType& context_name,
                                              const CaseInsensitiveSet& blacklisted_property_names,
                                              int32_t* out_num_params = NULL) -> SystemStringType;
        auto generate_default_property_value(FProperty* property, GeneratedSourceFile& header_data, const SystemStringType& ContextName) -> SystemStringType;

        auto generate_enum_value(UEnum* uenum, int64_t enum_value) -> SystemStringType;
        auto generate_simple_assignment_expression(FProperty* property,
                                                   const SystemStringType& value,
                                                   GeneratedSourceFile& implementation_file,
                                                   const SystemStringType& property_scope,
                                                   const SystemStringType& operator_type = SYSSTR(" = ")) -> void;
        auto generate_advanced_assignment_expression(FProperty* property,
                                                     const SystemStringType& value,
                                                     GeneratedSourceFile& implementation_file,
                                                     const SystemStringType& property_scope,
                                                     const SystemStringType& property_type,
                                                     const SystemStringType& operator_type = SYSSTR(" = ")) -> void;

        auto static generate_parameter_count_string(int32_t parameter_count) -> SystemStringType;
        auto static determine_primary_game_module_name() -> SystemStringType;

      public:
        auto add_module_and_sub_module_dependencies(::std::set<SystemStringType>& out_module_dependencies, const SystemStringType& module_name, bool add_self_module = true)
                -> void;
        auto static collect_blacklisted_property_names(UObject* property) -> CaseInsensitiveSet;

        auto static generate_object_pre_declaration(UObject* object) -> ::std::vector<::std::vector<SystemStringType>>;

        auto static convert_module_name_to_api_name(const SystemStringType& module_name) -> SystemStringType;
        auto static get_module_name_for_package(UObject* package) -> SystemStringType;
        auto static sanitize_enumeration_name(const SystemStringType& enumeration_name) -> SystemStringType;
        auto static get_highest_enum(UEnum* uenum) -> int64_t;
        auto static get_lowest_enum(UEnum* uenum) -> int64_t;

        auto static get_class_blueprint_info(UClass* function) -> ClassBlueprintInfo;
        auto static is_struct_blueprint_type(UScriptStruct* property) -> bool;
        auto static is_function_parameter_shadowing(UClass* property, FProperty* function_parameter) -> bool;

        auto static append_access_modifier(GeneratedSourceFile& header_data, AccessModifier needed_access, AccessModifier& current_access) -> void;
        auto static get_property_access_modifier(FProperty* property) -> AccessModifier;
        auto static get_function_access_modifier(UFunction* function) -> AccessModifier;
        auto static create_string_literal(const SystemStringType& string) -> SystemStringType;
        auto static get_header_name_for_object(UObject* object, bool get_existing_header = false) -> SystemStringType;
        auto static generate_cross_module_include(UObject* object, const SystemStringType& module_name, const SystemStringType& fallback_name) -> SystemStringType;
    };
} // namespace RC::UEGenerator
