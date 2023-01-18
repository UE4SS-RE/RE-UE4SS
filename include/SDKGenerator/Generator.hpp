#ifndef UE4SS_REWRITTEN_SDKGENERATOR_GENERATOR_HPP
#define UE4SS_REWRITTEN_SDKGENERATOR_GENERATOR_HPP

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

#include <File/File.hpp>
#include <Unreal/NameTypes.hpp>

namespace RC::Unreal
{
    class UObject;
    class UFunction;
    class UStruct;
    class UClass;
    class UScriptStruct;
    class FProperty;
}

namespace RC::UEGenerator
{
    enum class IsDelegateFunction
    {
        Yes,
        No,
    };

    class CXXGenerator
    {
    private:
        const std::filesystem::path m_directory_to_generate_in;

    public:
        struct ObjectInfo
        {
            Unreal::UObject* object{};
            ObjectInfo* first_encountered_at{};
        };

        struct PropertyInfo
        {
            Unreal::FProperty* property{};
            bool should_forward_declare{};
        };

        struct FunctionInfo
        {
            Unreal::UFunction* function{};
            ObjectInfo& owner;
            std::vector<PropertyInfo> params{};
        };

        struct GeneratedFile
        {
            std::filesystem::path primary_file_name;
            std::filesystem::path secondary_file_name;
            std::vector<File::StringType> ordered_primary_file_contents;
            std::vector<File::StringType> ordered_secondary_file_contents;
            File::StringType package_name;
            File::Handle primary_file;
            File::Handle secondary_file;
            bool primary_file_has_no_contents;
            bool secondary_file_has_no_contents;
        };

        struct FileName
        {
            uint32_t num_collisions{};
        };

        // Map of FName.ComparisonIndex -> File::Handle
        std::unordered_map<Unreal::FName, GeneratedFile> m_files{};
        std::unordered_map<File::StringType, FileName> m_file_names{};
        std::unordered_map<Unreal::UObject*, ObjectInfo> m_classes_dumped{};

    public:
        CXXGenerator() = delete;
        CXXGenerator(const std::filesystem::path directory_to_generate_in);

    private:
        auto create_all_files() -> void;
        auto object_is_package(Unreal::UObject* object) -> bool;

        auto generate_offset_comment(Unreal::FProperty*, File::StringType& file_contents) -> File::StringType;
        auto generate_header_start(GeneratedFile& generated_file, File::StringViewType file_name) -> void;
        auto generate_header_end(GeneratedFile& generated_file) -> void;
        auto generate_tab(size_t num_tabs = 1) -> File::StringType;
        auto make_function_info(ObjectInfo& owner, Unreal::UFunction* function, File::StringType& current_class_content) -> FunctionInfo;
        auto generate_function_declaration(ObjectInfo&, const FunctionInfo&, GeneratedFile&, File::StringType& current_class_content, IsDelegateFunction = IsDelegateFunction::No) -> void;
        auto generate_prefix(Unreal::UStruct* obj_class) -> File::StringType;
        auto generate_class_dependency(ObjectInfo& owner, Unreal::UStruct* native_class, File::StringType& current_class_content) -> void;
        auto generate_class_dependency_from_property(ObjectInfo& owner, Unreal::FProperty* property, File::StringType& current_class_content) -> bool;
        auto generate_class(ObjectInfo object_info, GeneratedFile& generated_file, File::StringType& current_class_content) -> Unreal::FProperty*;
        auto generate_enum(Unreal::UObject* native_object, GeneratedFile& generated_file) -> void;
        auto generate_package(Unreal::UObject* package, File::StringType& out) -> void;
        auto generate_package_if_non_existent(Unreal::UObject* object) -> GeneratedFile*;
        auto cleanup_old_sdk() -> void;
        auto sort_files(std::vector<File::StringType>& content) -> void;
        auto check_structure_type(const auto& x) -> std::wstring;

    public:
        auto generate() -> void;
    };
}


#endif //UE4SS_REWRITTEN_SDKGENERATOR_GENERATOR_HPP
