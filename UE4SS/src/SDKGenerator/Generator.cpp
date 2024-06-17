#include <cwctype>
#include <format>
#include <locale>
#include <set>

#include <SDKGenerator/Common.hpp>
#include <SDKGenerator/Generator.hpp>
#include <UE4SSProgram.hpp>
#pragma warning(disable : 4005)
#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>
#include <Unreal/Property/FDelegateProperty.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <Unreal/Property/FMulticastInlineDelegateProperty.hpp>
#include <Unreal/Property/FMulticastSparseDelegateProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/TypeChecker.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UScriptStruct.hpp>
#pragma warning(default : 4005)

namespace RC::UEGenerator
{
    using TypeChecker = RC::Unreal::TypeChecker;
    namespace UObjectGlobals = RC::Unreal::UObjectGlobals;
    using EObjectFlags = RC::Unreal::EObjectFlags;
    using FName = RC::Unreal::FName;
    using UObject = RC::Unreal::UObject;
    using UClass = RC::Unreal::UClass;
    using UInterface = RC::Unreal::UInterface;
    using UScriptStruct = RC::Unreal::UScriptStruct;
    using UFunction = RC::Unreal::UFunction;
    using XProperty = RC::Unreal::FProperty;
    using UStruct = RC::Unreal::UStruct;
    using UEnum = Unreal::UEnum;
    using FObjectProperty = RC::Unreal::FObjectProperty;
    using FClassProperty = RC::Unreal::FClassProperty;
    using FStructProperty = RC::Unreal::FStructProperty;
    using FArrayProperty = RC::Unreal::FArrayProperty;
    using FMapProperty = RC::Unreal::FMapProperty;
    using FDelegateProperty = RC::Unreal::FDelegateProperty;
    using FMulticastInlineDelegateProperty = RC::Unreal::FMulticastInlineDelegateProperty;
    using FMulticastSparseDelegateProperty = RC::Unreal::FMulticastSparseDelegateProperty;

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

    auto generate_tab(size_t num_tabs = 1) -> File::StringType
    {
        File::StringType tab_storage{};
        for (size_t i = 0; i < num_tabs; ++i)
        {
            tab_storage += STR("    ");
        }

        return tab_storage;
    }
    auto generate_prefix(UStruct* obj) -> File::StringType
    {
        UClass* obj_class = obj->GetClassPrivate();
        if (obj_class->IsChildOf<UScriptStruct>())
        {
            return STR("struct");
        }
        else
        {
            return STR("class");
        }
    }
    auto generate_class_name(UStruct* class_to_generate) -> File::StringType
    {
        if (class_to_generate->GetClassPrivate()->IsChildOf<UScriptStruct>())
        {
            return get_native_struct_name(std::bit_cast<UScriptStruct*>(class_to_generate));
        }
        else if (class_to_generate->IsChildOf<UInterface>())
        {
            return get_native_class_name(static_cast<UClass*>(class_to_generate), true);
        }
        else
        {
            // Assume that it's a UClass
            return get_native_class_name(static_cast<UClass*>(class_to_generate));
        }
    }

    template <typename T>
    class TypeGenerator
    {
      private:
        T specification{};
        const std::filesystem::path m_directory_to_generate_in;

      public:
        struct FileName
        {
            uint32_t num_collisions{};
        };

        // Map of FName.ComparisonIndex -> File::Handle
        std::unordered_map<Unreal::FName, GeneratedFile> m_files{};
        std::unordered_map<File::StringType, FileName> m_file_names{};
        std::unordered_map<Unreal::UObject*, ObjectInfo> m_classes_dumped{};

      public:
        TypeGenerator() = delete;
        TypeGenerator(const std::filesystem::path directory_to_generate_in) : m_directory_to_generate_in(directory_to_generate_in)
        {
            m_files.reserve(512);
        }

        auto create_all_files() -> void
        {
            Output::send(STR("Creating all files...\n"));
            for (auto& [comparison_index, generated_file] : m_files)
            {
                if (!generated_file.ordered_primary_file_contents.empty())
                {
                    generated_file.primary_file =
                            File::open(generated_file.primary_file_name, File::OpenFor::Appending, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);

                    specification.generate_file_header(generated_file);

                    sort_files(generated_file.ordered_primary_file_contents);

                    File::StringType combined_file_contents;
                    for (auto& line : generated_file.ordered_primary_file_contents)
                    {
                        combined_file_contents.append(line);
                    }

                    if (combined_file_contents.empty())
                    {
                        Output::send(STR("Empty primary file contents in '{}'\n"), generated_file.package_name);
                    }
                    else
                    {
                        generated_file.primary_file.write_string_to_file(combined_file_contents);
                    }

                    specification.generate_file_footer(generated_file);

                    generated_file.primary_file.close();
                }

                if (!generated_file.ordered_secondary_file_contents.empty())
                {
                    generated_file.secondary_file =
                            File::open(generated_file.secondary_file_name, File::OpenFor::Appending, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);

                    sort_files(generated_file.ordered_secondary_file_contents);

                    File::StringType combined_file_contents;
                    for (auto& line : generated_file.ordered_secondary_file_contents)
                    {
                        combined_file_contents.append(line);
                    }

                    if (combined_file_contents.empty())
                    {
                        Output::send(STR("Empty secondary file contents in '{}'\n"), generated_file.package_name);
                    }
                    else
                    {
                        generated_file.secondary_file.write_string_to_file(combined_file_contents);
                    }

                    generated_file.secondary_file.close();
                }
            }
        }

        auto sort_files(std::vector<File::StringType>& content) -> void
        {
            std::vector<File::StringType> struct_content;
            std::vector<File::StringType> class_content;
            std::vector<File::StringType> other_content;
            for (auto& line : content)
            {
                if (line.starts_with(STR("struct")))
                {
                    struct_content.push_back(line);
                }
                else if (line.starts_with(STR("class")))
                {
                    class_content.push_back(line);
                }
                else
                {
                    other_content.push_back(line);
                }
            }

            sort_types(struct_content);
            sort_types(class_content);
            sort_types(other_content);

            content.clear();
            content.reserve(struct_content.size() + class_content.size() + other_content.size());
            content.insert(content.end(), struct_content.begin(), struct_content.end());
            content.insert(content.end(), class_content.begin(), class_content.end());
            content.insert(content.end(), other_content.begin(), other_content.end());
        }

        auto sort_types(std::vector<File::StringType>& content) -> void
        {
            std::sort(content.begin(), content.end(), [&](const auto& a, const auto& b) {
                auto a_class_name = get_class_name(a);
                auto b_class_name = get_class_name(b);
                return a_class_name < b_class_name;
            });
        }

        auto get_class_name(const auto& x) -> std::wstring
        {
            // Using this method instead of regex because it is extremely slow
            auto class_name = x.substr(x.find(STR(' ')) + 1);
            class_name = class_name.substr(0, class_name.find(STR(' ')));
            if (class_name == STR("class"))
            {
                // Case for enum class
                class_name = x.substr(x.find(STR(' ')) + 7);
                class_name = class_name.substr(0, class_name.find(STR(' ')));
            }
            return class_name;
        }

        auto object_is_package(UObject* object) -> bool
        {
            return object->GetClassPrivate()->GetNamePrivate().Equals(Unreal::GPackageName);
        }

        auto generate_offset_comment(XProperty* property, File::StringType& line) -> File::StringType
        {
            if (UE4SSProgram::settings_manager.CXXHeaderGenerator.DumpOffsetsAndSizes)
            {
                return fmt::format(STR("{:85} // 0x{:04X} (size: 0x{:X})"), line, property->GetOffset_Internal(), property->GetSize());
            }
            else
            {
                return line;
            }
        }

        auto check_ignore_forward_declaration(const ObjectInfo& owner, XProperty* return_property) -> bool
        {
            if (return_property->IsA<FStructProperty>())
            {
                // Can StructProperty even be forward declared ? I don't know if it's ever a pointer to a struct
                auto* script_struct = static_cast<FStructProperty*>(return_property)->GetStruct();
                if (m_classes_dumped.contains(script_struct))
                {
                    auto& property_class_info = m_classes_dumped[script_struct];
                    if (property_class_info.first_encountered_at)
                    {
                        return property_class_info.first_encountered_at->object == owner.object;
                    }
                }
            }
            else if (return_property->IsA<FClassProperty>())
            {
                // Can ClassProperty be forward declared ? Maybe ?
                auto* meta_class = static_cast<FClassProperty*>(return_property)->GetMetaClass();
                if (m_classes_dumped.contains(meta_class))
                {
                    auto& property_class_info = m_classes_dumped[meta_class];
                    if (property_class_info.first_encountered_at)
                    {
                        return property_class_info.first_encountered_at->object == owner.object;
                    }
                }
            }
            else if (return_property->IsA<FObjectProperty>())
            {
                auto* property_class = static_cast<FObjectProperty*>(return_property)->GetPropertyClass();
                if (m_classes_dumped.contains(property_class))
                {
                    auto& property_class_info = m_classes_dumped[property_class];
                    if (property_class_info.first_encountered_at)
                    {
                        return property_class_info.first_encountered_at->object == owner.object;
                    }
                }
            }
            return false;
        }

        auto generate_function_declaration(ObjectInfo& owner,
                                           const FunctionInfo& function_info,
                                           GeneratedFile& generated_file,
                                           File::StringType& out_current_class_content,
                                           IsDelegateFunction is_delegate_function = IsDelegateFunction::No) -> void
        {
            std::optional<PropertyInfo> return_property_info = [&]() -> std::optional<PropertyInfo> {
                for (const auto& property_info : function_info.params)
                {
                    if (property_info.property->HasAnyPropertyFlags(Unreal::CPF_ReturnParm))
                    {
                        return property_info;
                    }
                }

                return {};
            }();

            XProperty* return_property = [&]() {
                return return_property_info.has_value() ? return_property_info.value().property : nullptr;
            }();

            File::StringType function_name{function_info.function->GetName()};
            if (is_delegate_function == IsDelegateFunction::Yes)
            {
                // Remove the last 19 characters, which is always '__DelegateSignature' for delegates
                function_name.erase(function_name.size() - 19, 19);
            }

            StringType current_class_content{};
            specification.generate_function_declaration(this, current_class_content, owner, function_info, function_name, return_property, return_property_info);

            // Commenting out this code because all network replicated functions are events
            // Therefore this is not an accurate way to check if a function is an event
            /*
            if ((function->get_function_flags() & Unreal::EFunctionFlags::FUNC_Event) != 0)
            {
                current_class_content.append(STR(" // EVENT"));
            }
            //*/
            current_class_content.append(STR("\n"));
            out_current_class_content.append(current_class_content);
        }

        auto generate_class_dependency(ObjectInfo& owner, UStruct* inherited_class, File::StringType& current_class_content) -> void
        {
            if (!inherited_class)
            {
                return;
            }

            if (!m_classes_dumped.contains(inherited_class))
            {
                GeneratedFile* package_file_for_inherited_class = generate_package_if_non_existent(inherited_class);
                if (package_file_for_inherited_class)
                {
                    auto& inherited_object_info = m_classes_dumped.emplace(inherited_class, ObjectInfo{inherited_class, &owner}).first->second;
                    File::StringType new_class_content{};
                    generate_class(inherited_object_info, *package_file_for_inherited_class, new_class_content);
                    if (!package_file_for_inherited_class->primary_file_has_no_contents)
                    {
                        package_file_for_inherited_class->ordered_primary_file_contents.push_back(new_class_content);
                    }
                }

                return;
            }
        }

        auto generate_class_dependency_from_property(ObjectInfo& owner, XProperty* property, File::StringType& current_class_content) -> bool
        {
            if (property->IsA<FStructProperty>())
            {
                generate_class_dependency(owner, static_cast<FStructProperty*>(property)->GetStruct(), current_class_content);
                return false;
            }
            else if (property->IsA<FClassProperty>())
            {
                // return generate_class_dependency(owner, static_cast<XClassProperty*>(property)->get_meta_class());
                return false;
            }
            else if (property->IsA<FObjectProperty>())
            {
                // return generate_class_dependency(owner, static_cast<XObjectProperty*>(property)->get_property_class());
                return true;
            }
            else if (property->IsA<FArrayProperty>())
            {
                // return generate_class_dependency_from_property(owner, static_cast<XArrayProperty*>(property)->get_inner());
                // if (static_cast<XArrayProperty*>(property)->get_inner()->is_child_of<XObjectProperty>())
                //{
                //     return true;
                // }
                XProperty* inner = static_cast<FArrayProperty*>(property)->GetInner();
                if (inner->IsA<FStructProperty>())
                {
                    generate_class_dependency(owner, static_cast<FStructProperty*>(inner)->GetStruct(), current_class_content);
                    return false;
                }
            }
            else if (property->IsA<FMapProperty>())
            {
                XProperty* key_property = static_cast<FMapProperty*>(property)->GetKeyProp();
                XProperty* value_property = static_cast<FMapProperty*>(property)->GetValueProp();

                if (key_property->IsA<FStructProperty>())
                {
                    generate_class_dependency(owner, static_cast<FStructProperty*>(key_property)->GetStruct(), current_class_content);
                }

                if (value_property->IsA<FStructProperty>())
                {
                    generate_class_dependency(owner, static_cast<FStructProperty*>(value_property)->GetStruct(), current_class_content);
                }

                return false;
            }

            return false;
        }

        auto make_function_info(ObjectInfo& owner, UFunction* function, File::StringType& current_class_content) -> FunctionInfo
        {
            FunctionInfo function_info{
                    .function = function,
                    .owner = owner,
            };

            for (XProperty* param : function->ForEachProperty())
            {
                if (!param->HasAnyPropertyFlags(Unreal::CPF_Parm | Unreal::CPF_ReturnParm))
                {
                    continue;
                }

                function_info.params.emplace_back(PropertyInfo{param, generate_class_dependency_from_property(owner, param, current_class_content)});
            }

            return function_info;
        }

        auto generate_class(ObjectInfo object_info, GeneratedFile& generated_file, File::StringType& current_class_content) -> XProperty*
        {
            UStruct* native_class = static_cast<UStruct*>(object_info.object);
            if (specification.should_generate_class(native_class))
            {
                generated_file.primary_file_has_no_contents = false;
                specification.generate_class(this, object_info, generated_file, current_class_content);
            }

            return native_class->GetFirstProperty();
        }

        auto generate_enum(UObject* native_object, GeneratedFile& generated_file) -> void
        {
            generated_file.secondary_file_has_no_contents = false;

            File::StringType content_buffer;
            UEnum* uenum = static_cast<UEnum*>(native_object);

            specification.generate_enum_declaration(content_buffer, uenum);

            for (const auto& elem : uenum->ForEachName())
            {
                auto enum_value_full_name = elem.Key.ToString();
                size_t colon_pos = enum_value_full_name.rfind(STR(":"));
                auto enum_value_name = colon_pos == enum_value_full_name.npos ? enum_value_full_name : enum_value_full_name.substr(colon_pos + 1);

                specification.generate_enum_member(content_buffer, uenum, enum_value_name, elem);
            }

            specification.generate_enum_end(content_buffer, uenum);

            content_buffer.append(STR("\n\n"));

            generated_file.ordered_secondary_file_contents.push_back(content_buffer);
        }

        auto generate_package(UObject* package, File::StringType& out) -> void
        {
            UObjectGlobals::ForEachUObject([&](void* object, [[maybe_unused]] int32_t chunk_index, [[maybe_unused]] int32_t object_index) {
                return LoopAction::Continue;
            });
        }

        auto generate_package_if_non_existent(UObject* object) -> GeneratedFile*
        {
            UObject* package{};
            UObject* outer = object;
            if (!outer)
            {
                return nullptr;
            }

            do
            {
                if (object_is_package(outer))
                {
                    package = outer;
                    break;
                }

                outer = outer->GetOuterPrivate();
            } while (outer);

            if (!package)
            {
                throw std::runtime_error{"[generate_package_if_non_existent] Was unable to find UPackage for this object"};
            }

            FName package_fname = package->GetNamePrivate();
            if (m_files.contains(package_fname))
            {
                return &m_files.at(package_fname);
            }
            else
            {
                // Get rid of everything before the last slash + the last slash, leaving only the actual name
                File::StringType package_name = package->GetNamePrivate().ToString();
                package_name = package_name.substr(package_name.rfind(STR("/")) + 1);
                File::StringType package_name_all_lower = package_name;
                std::transform(package_name_all_lower.begin(), package_name_all_lower.end(), package_name_all_lower.begin(), [](File::CharType c) {
                    return std::towlower(c);
                });

                if (m_file_names.contains(package_name_all_lower))
                {
                    // File name collision
                    auto& file_name = m_file_names[package_name_all_lower];
                    package_name.append(fmt::format(STR("_DUPL_{}"), ++file_name.num_collisions));
                    Output::send(STR("File name collision, renamed to '{}'\n"), package_name);
                }
                else
                {
                    m_file_names.emplace(package_name_all_lower, FileName{});
                }

                // The '\\?\' at the beginning of the string unlocks path size restriction from MAX_PATH to 32k
                std::filesystem::path directory_to_generate_in = std::filesystem::path("\\\\?\\");
                directory_to_generate_in += (m_directory_to_generate_in);

                File::StringType ext = specification.get_file_extension();
                std::filesystem::path primary_file_path_and_name = directory_to_generate_in;
                primary_file_path_and_name.append(package_name);
                primary_file_path_and_name.replace_extension(ext);

                std::filesystem::path secondary_file_path_and_name = directory_to_generate_in;
                secondary_file_path_and_name.append(package_name + STR("_enums"));
                secondary_file_path_and_name.replace_extension(ext);

                GeneratedFile generated_file{
                        .primary_file_name = primary_file_path_and_name,
                        .secondary_file_name = secondary_file_path_and_name,
                        .ordered_primary_file_contents = {},
                        .ordered_secondary_file_contents = {},
                        .package_name = package_name,
                        .primary_file = {},
                        .secondary_file = {},
                        .primary_file_has_no_contents = true,
                        .secondary_file_has_no_contents = true,
                };

                auto& file_in_map = m_files.emplace(std::move(package_fname), std::move(generated_file)).first->second;
                return &file_in_map;
            }
        }

        auto cleanup_old_sdk() -> void
        {
            if (!std::filesystem::exists(m_directory_to_generate_in))
            {
                return;
            }

            for (const auto& item : std::filesystem::directory_iterator(m_directory_to_generate_in))
            {
                if (item.is_directory())
                {
                    continue;
                }
                if (item.path().extension() != specification.get_file_extension())
                {
                    continue;
                }

                File::delete_file(item.path());
            }
        }

      public:
        auto generate() -> void
        {
            Output::send(STR("Cleaning up old SDK files...\n"));
            cleanup_old_sdk();
            Output::send(STR("Generating SDK...\n"));

            // 400k should be enough for most games, and it's highly unlikely to cause more than one reallocation even if the game is huge
            m_classes_dumped.reserve(400000);

            size_t num_objects_generated{};
            UObjectGlobals::ForEachUObject([&](void* untyped_object, [[maybe_unused]] int32_t chunk_index, [[maybe_unused]] int32_t object_index) {
                UObject* object = static_cast<UObject*>(untyped_object);
                UClass* object_class = object->GetClassPrivate();

                // Generate file for package if it doesn't already exist
                GeneratedFile* package_file = generate_package_if_non_existent(object);

                if (!package_file)
                {
                    // Object should not be dumped
                    return LoopAction::Continue;
                }
                else if (object->IsA<UEnum>())
                {
                    generate_enum(object, *package_file);
                    ++num_objects_generated;

                    return LoopAction::Continue;
                }
                else if ((object_class->IsChildOf<UClass>() || object_class->IsChildOf<UScriptStruct>()) && !m_classes_dumped.contains(object))
                {
                    // Generate a class for this object
                    auto& object_info = m_classes_dumped.emplace(object, ObjectInfo{object}).first->second;
                    File::StringType class_content{};
                    generate_class(object_info, *package_file, class_content);
                    if (!package_file->primary_file_has_no_contents)
                    {
                        package_file->ordered_primary_file_contents.push_back(class_content);
                    }
                    ++num_objects_generated;

                    return LoopAction::Continue;
                }
                else
                {
                    // Object should not be dumped
                    return LoopAction::Continue;
                }
            });

            create_all_files();
        }
    };

    class CXXHeaderGenerator
    {
      public:
        auto get_file_extension() -> File::StringType
        {
            return STR(".hpp");
        }
        auto generate_file_header(GeneratedFile& generated_file) -> void
        {
            generated_file.primary_file.write_string_to_file(
                    fmt::format(STR("#ifndef UE4SS_SDK_{}_HPP\n#define UE4SS_SDK_{}_HPP\n\n"), generated_file.package_name, generated_file.package_name));

            if (!generated_file.secondary_file_has_no_contents)
            {
                generated_file.primary_file.write_string_to_file(fmt::format(STR("#include \"{}\"\n\n"), generated_file.secondary_file_name.filename().c_str()));
            }
        }
        auto generate_file_footer(GeneratedFile& generated_file) -> void
        {
            generated_file.primary_file.write_string_to_file(fmt::format(STR("#endif\n")));
        }
        auto generate_enum_declaration(File::StringType& content_buffer, UEnum* uenum) -> void
        {
            const auto cpp_form = uenum->GetCppForm();
            if (cpp_form == UEnum::ECppForm::Regular)
            {
                content_buffer.append(fmt::format(STR("enum {} {{\n"), get_native_enum_name(uenum, false)));
            }
            else if (cpp_form == UEnum::ECppForm::Namespaced)
            {
                content_buffer.append(fmt::format(STR("namespace {} {{\n{}enum Type {{\n"), get_native_enum_name(uenum, false), generate_tab()));
            }
            else if (cpp_form == UEnum::ECppForm::EnumClass)
            {
                content_buffer.append(fmt::format(STR("enum class {} {{\n"), get_native_enum_name(uenum, false)));
            }
        }
        auto generate_enum_member(File::StringType& content_buffer, UEnum* uenum, const File::StringType& enum_value_name, const Unreal::FEnumNamePair& elem) -> void
        {
            content_buffer.append(fmt::format(STR("{}{}{} = {},\n"),
                                              generate_tab(),
                                              uenum->GetCppForm() == UEnum::ECppForm::Namespaced ? generate_tab() : STR(""),
                                              enum_value_name,
                                              elem.Value));
        }
        auto generate_enum_end(File::StringType& content_buffer, UEnum* uenum) -> void
        {
            const auto cpp_form = uenum->GetCppForm();
            content_buffer.append(fmt::format(STR("{}}};"), cpp_form == UEnum::ECppForm::Namespaced ? generate_tab() : STR("")));

            if (cpp_form == UEnum::ECppForm::Namespaced)
            {
                content_buffer.append(STR("\n}"));
            }
        }
        auto should_generate_class(UStruct* native_class)
        {
            return true;
        }
        auto generate_class(TypeGenerator<CXXHeaderGenerator>* generator, ObjectInfo& object_info, GeneratedFile& generated_file, File::StringType& current_class_content)
        {
            UStruct* native_class = static_cast<UStruct*>(object_info.object);
            File::StringType content_buffer{};

            UStruct* inherits_from_class = native_class->GetSuperStruct();

            // Make sure that the base class is defined
            generator->generate_class_dependency(object_info, inherits_from_class, current_class_content);

            // If any properties have dependencies, make sure that they are defined
            // This makes sure that we don't have member variables with undefined types (if the types are local, otherwise we need to include the file that the struct exists in)
            std::vector<PropertyInfo> properties_to_generate{};
            for (XProperty* property : native_class->ForEachProperty())
            {
                properties_to_generate.emplace_back(
                        PropertyInfo{property, generator->generate_class_dependency_from_property(object_info, property, current_class_content)});
            }

            std::vector<FunctionInfo> functions_to_generate{};
            for (UFunction* function : native_class->ForEachFunction())
            {
                auto& function_info = functions_to_generate.emplace_back(FunctionInfo{function, object_info});

                for (XProperty* param : function->ForEachProperty())
                {
                    if (!param->HasAnyPropertyFlags(Unreal::CPF_Parm | Unreal::CPF_ReturnParm))
                    {
                        continue;
                    }

                    function_info.params.emplace_back(
                            PropertyInfo{param, generator->generate_class_dependency_from_property(object_info, param, current_class_content)});
                }
            }

            auto class_name = generate_class_name(native_class);

            generate_class_declaration(content_buffer, native_class, inherits_from_class);

            int32_t num_padding_elements{0};
            XProperty* last_property_in_this_class{nullptr};

            for (const auto& property_info : properties_to_generate)
            {
                XProperty* property = property_info.property;
                int32_t current_property_offset = property->GetOffset_Internal();
                int32_t current_property_size = property->GetSize();

                StringType part_one{};
                try
                {
                    part_one = fmt::format(STR("{}{}{} {};"),
                                           generate_tab(),
                                           property_info.should_forward_declare ? STR("class ") : STR(""),
                                           generate_property_cxx_name(property, true, native_class, EnableForwardDeclarations::Yes),
                                           property->GetName());
                }
                catch (std::exception& e)
                {
                    Output::send<LogLevel::Warning>(STR("Could not generate property '{}' because: {}\n"), property->GetFullName(), to_wstring(e.what()));
                    continue;
                }

                content_buffer.append(fmt::format(STR("{}\n"), generator->generate_offset_comment(property, part_one)));

                if (property->IsA<FDelegateProperty>())
                {
                    generator->generate_function_declaration(object_info,
                                                             generator->make_function_info(object_info,
                                                                                           static_cast<FDelegateProperty*>(property)->GetSignatureFunction(),
                                                                                           current_class_content),
                                                             generated_file,
                                                             content_buffer,
                                                             IsDelegateFunction::Yes);
                }
                else if (property->IsA<FMulticastInlineDelegateProperty>())
                {
                    generator->generate_function_declaration(
                            object_info,
                            generator->make_function_info(object_info,
                                                          static_cast<FMulticastInlineDelegateProperty*>(property)->GetSignatureFunction(),
                                                          current_class_content),
                            generated_file,
                            content_buffer,
                            IsDelegateFunction::Yes);
                }
                else if (property->IsA<FMulticastSparseDelegateProperty>())
                {
                    generator->generate_function_declaration(
                            object_info,
                            generator->make_function_info(object_info,
                                                          static_cast<FMulticastSparseDelegateProperty*>(property)->GetSignatureFunction(),
                                                          current_class_content),
                            generated_file,
                            content_buffer,
                            IsDelegateFunction::Yes);
                }

                if (UE4SSProgram::settings_manager.CXXHeaderGenerator.KeepMemoryLayout)
                {
                    // Check if next member-var is reflected
                    // If it's not, add padding so that everything in the struct is aligned properly
                    auto* next_property = property->GetNextFieldAsProperty();
                    if (next_property)
                    {
                        int32_t current_property_end_location = current_property_offset + current_property_size;

                        int32_t next_property_offset = next_property->GetOffset_Internal();

                        if (current_property_offset != next_property_offset && current_property_end_location != next_property_offset)
                        {
                            // Add padding
                            int32_t padding_property_offset = current_property_end_location;
                            int32_t padding_property_size = next_property_offset - padding_property_offset;

                            auto padding_part_one = fmt::format(STR("{}char {}[0x{:X}];"),
                                                                generate_tab(),
                                                                fmt::format(STR("padding_{}"), num_padding_elements++),
                                                                padding_property_size);
                            content_buffer.append(
                                    fmt::format(STR("{:85} // 0x{:04X} (size: 0x{:X})\n"), padding_part_one, padding_property_offset, padding_property_size));
                        }
                    }
                }

                last_property_in_this_class = property;
            }

            int32_t class_size = native_class->GetPropertiesSize();
            generate_class_struct_end(content_buffer, class_name, class_size, num_padding_elements, last_property_in_this_class);

            // Functions
            if (native_class->HasChildren())
            {
                content_buffer.append(STR("\n"));
                for (const auto& function_info : functions_to_generate)
                {
                    generator->generate_function_declaration(object_info, function_info, generated_file, content_buffer);
                }
            }

            generate_class_end(content_buffer, class_size);

            content_buffer.append(STR("\n\n"));

            current_class_content.append(content_buffer);
        }

        auto generate_class_declaration(File::StringType& content_buffer, UStruct* native_class, UStruct* inherits_from_class) -> void
        {
            auto class_name = generate_class_name(native_class);
            if (inherits_from_class)
            {
                content_buffer.append(
                        fmt::format(STR("{} {} : public {}\n{{\n"), generate_prefix(native_class), class_name, generate_class_name(inherits_from_class)));
            }
            else
            {
                content_buffer.append(fmt::format(STR("{} {}\n{{\n"), generate_prefix(native_class), class_name));
            }
        }
        auto generate_class_struct_end(File::StringType& content_buffer,
                                       const File::StringType& class_name,
                                       size_t class_size,
                                       int32_t num_padding_elements,
                                       XProperty* last_property_in_this_class) -> void
        {
            if (UE4SSProgram::settings_manager.CXXHeaderGenerator.KeepMemoryLayout)
            {
                if (last_property_in_this_class)
                {
                    // TODO: Fix this, the padding is required for alignment when using the SDK for code injection
                    // This commented-out code was here to provide the correct padding between classes so that everything would line up correctly
                    // But it no longer works because it was dependent on the "generate_class_chain" function that no longer eixsts
                    /*
                    int32_t last_property_offset = last_property_in_this_class->get_offset_for_internal();
                    if (first_property)
                    {
                        int32_t first_property_offset = first_property->get_offset_for_internal();
                        if (last_property_offset != first_property_offset)
                        {
                            int32_t last_property_size = last_property_in_this_class->get_size();
                            int32_t padding_size = first_property_offset - (last_property_offset + last_property_size);
                            printf_s("class_size: %X\n", class_size);
                            printf_s("first_property->get_size(): %X\n", first_property->get_size());
                            printf_s("last_property_offset: %X\n", last_property_offset);
                            printf_s("first_property_offset: %X\n", first_property_offset);

                            auto padding_part_one = fmt::format(STR("{}char {}[0x{:X}];"), generate_tab(), fmt::format(STR("padding_{}"), num_padding_elements), padding_size);
                            out.append(fmt::format(STR("{:85} // 0x{:04X} (size: 0x{:X})\n"), padding_part_one, last_property_offset + last_property_size, padding_size));
                        }
                    }
                    //*/
                }
                else if (class_size > 0)
                {
                    // No reflected member variables exist but there are non-reflected member variables
                    // Add padding for non-reflected member variables, for alignment purposes
                    auto padding_part_one =
                            fmt::format(STR("{}char {}[0x{:X}];"), generate_tab(), fmt::format(STR("padding_{}"), num_padding_elements), class_size);
                    content_buffer.append(fmt::format(STR("{:85} // 0x0000 (size: 0x{:X})\n"), padding_part_one, 0x0));
                }
            }
        }
        auto generate_class_end(File::StringType& content_buffer, size_t class_size) -> void
        {
            if (UE4SSProgram::settings_manager.CXXHeaderGenerator.DumpOffsetsAndSizes)
            {
                content_buffer.append(fmt::format(STR("}}; // Size: 0x{:X}"), class_size));
            }
            else
            {
                content_buffer.append(STR("};"));
            }
        }

        auto generate_function_declaration(TypeGenerator<CXXHeaderGenerator>* generator,
                                           File::StringType& current_class_content,
                                           ObjectInfo& owner,
                                           const FunctionInfo& function_info,
                                           File::StringType function_name,
                                           XProperty* return_property,
                                           std::optional<PropertyInfo> return_property_info) -> void
        {
            File::StringType function_type_name{};
            if (return_property)
            {
                try
                {
                    function_type_name = generate_property_cxx_name(return_property, true, function_info.function, EnableForwardDeclarations::Yes);
                }
                catch (std::exception& e)
                {
                    Output::send<LogLevel::Warning>(STR("Could not generate function '{}' because: {}\n"),
                                                    function_info.function->GetFullName(),
                                                    to_wstring(e.what()));
                    return;
                }

                if (return_property_info.value().should_forward_declare && !generator->check_ignore_forward_declaration(owner, return_property))
                {
                    function_type_name.insert(0, STR("class "));
                }
            }
            else
            {
                function_type_name = STR("void");
            }

            current_class_content.append(fmt::format(STR("{}{} {}("), generate_tab(), function_type_name, function_name));

            for (size_t i = 0; i < function_info.params.size(); ++i)
            {
                const auto& param_info = function_info.params[i];
                if (!param_info.property->HasAnyPropertyFlags(Unreal::CPF_ReturnParm))
                {
                    try
                    {
                        current_class_content.append(
                                fmt::format(STR("{}{}{}{} {}"),
                                            param_info.property->HasAnyPropertyFlags(Unreal::CPF_ConstParm) ? STR("const ") : STR(""),
                                            param_info.should_forward_declare ? STR("class ") : STR(""),
                                            generate_property_cxx_name(param_info.property, true, function_info.function, EnableForwardDeclarations::Yes),
                                            param_info.property->HasAnyPropertyFlags(Unreal::CPF_ReferenceParm | Unreal::CPF_OutParm) ? STR("&") : STR(""),
                                            param_info.property->GetName()));
                    }
                    catch (std::exception& e)
                    {
                        Output::send<LogLevel::Warning>(STR("Could not generate function '{}' because: {}\n"),
                                                        function_info.function->GetFullName(),
                                                        to_wstring(e.what()));
                        return;
                    }

                    if (i + 1 < function_info.params.size())
                    {
                        auto* next_param = function_info.params[i + 1].property;
                        if (next_param && (!next_param->HasAnyPropertyFlags(Unreal::CPF_ReturnParm) || i + 2 < function_info.params.size()))
                        {
                            current_class_content.append(STR(", "));
                        }
                    }
                }
            }
            current_class_content.append(STR(");"));
        }
    };

    class LuaTypesGenerator
    {
      private:
        auto is_valid_lua_symbol(const File::StringType& str) -> bool
        {
            static const std::set<File::StringType> keywords = {STR("and"),   STR("break"), STR("do"),       STR("else"),   STR("elseif"), STR("end"),
                                                                STR("false"), STR("for"),   STR("function"), STR("if"),     STR("in"),     STR("local"),
                                                                STR("nil"),   STR("not"),   STR("or"),       STR("repeat"), STR("return"), STR("then"),
                                                                STR("true"),  STR("until"), STR("while")};
            if (keywords.contains(str))
            {
                return false;
            }
            auto it = str.begin();
            if (it == str.end() || std::isdigit(*it))
            {
                // string empty or first char is digit
                return false;
            }
            for (; it != str.end(); ++it)
            {
                auto c = *it;
                if (c != '_' && !std::isalnum(c))
                {
                    // is not underscore or alphanumeric in current locale
                    return false;
                }
            }
            return true;
        }
        auto quote_lua_symbol(const File::StringType& symbol) -> File::StringType
        {
            File::StringType quoted;
            quoted.reserve(symbol.size() + 2);
            quoted.push_back('\'');
            for (auto it = symbol.begin(); it != symbol.end(); ++it)
            {
                auto c = *it;
                if (c == '\\' || c == '\'')
                {
                    quoted.push_back('\\');
                }
                quoted.push_back(c);
            }
            quoted.push_back('\'');
            return quoted;
        }
        auto make_valid_symbol(const File::StringType& symbol) -> File::StringType
        {
            File::StringType valid;
            valid.reserve(symbol.size());
            auto it = symbol.begin();
            if (it == symbol.end() || std::isdigit(*it))
            {
                valid.push_back('_');
            }
            for (; it != symbol.end(); ++it)
            {
                auto c = *it;
                valid.push_back((c == '_' || std::isalnum(c)) ? c : '_');
            }
            return valid;
        }

      public:
        auto get_file_extension() -> File::StringType
        {
            return STR(".lua");
        }
        auto generate_file_header(GeneratedFile& generated_file) -> void
        {
            generated_file.primary_file.write_string_to_file(STR("---@meta\n\n"));
        }
        auto generate_file_footer(GeneratedFile& generated_file) -> void
        {
        }
        auto generate_enum_declaration(File::StringType& content_buffer, UEnum* uenum) -> void
        {
            auto enum_name = uenum->GetName();
            content_buffer.append(fmt::format(STR("---@enum {}\n{} = {{\n"), enum_name, enum_name));
        }
        auto generate_enum_member(File::StringType& content_buffer, UEnum* uenum, const File::StringType& enum_value_name, const Unreal::FEnumNamePair& elem) -> void
        {
            content_buffer.append(fmt::format(STR("{}{} = {},\n"), generate_tab(), enum_value_name, elem.Value));
        }
        auto generate_enum_end(File::StringType& content_buffer, UEnum* uenum) -> void
        {
            content_buffer.append(STR("}"));
        }

        auto should_generate_class(UStruct* native_class)
        {
            // skip UObject to define externally
            return native_class != UObject::StaticClass();
        }
        auto generate_class(TypeGenerator<LuaTypesGenerator>* generator, ObjectInfo& object_info, GeneratedFile& generated_file, File::StringType& current_class_content)
        {
            UStruct* native_class = static_cast<UStruct*>(object_info.object);
            File::StringType content_buffer{};

            UStruct* inherits_from_class = native_class->GetSuperStruct();

            // Make sure that the base class is defined
            generator->generate_class_dependency(object_info, inherits_from_class, current_class_content);

            // If any properties have dependencies, make sure that they are defined
            // This makes sure that we don't have member variables with undefined types (if the types are local, otherwise we need to include the file that the struct exists in)
            std::vector<PropertyInfo> properties_to_generate{};
            for (XProperty* property : native_class->ForEachProperty())
            {
                properties_to_generate.emplace_back(
                        PropertyInfo{property, generator->generate_class_dependency_from_property(object_info, property, current_class_content)});
            }

            std::vector<FunctionInfo> functions_to_generate{};
            for (UFunction* function : native_class->ForEachFunction())
            {
                auto& function_info = functions_to_generate.emplace_back(FunctionInfo{function, object_info});

                for (XProperty* param : function->ForEachProperty())
                {
                    if (!param->HasAnyPropertyFlags(Unreal::CPF_Parm | Unreal::CPF_ReturnParm))
                    {
                        continue;
                    }

                    function_info.params.emplace_back(
                            PropertyInfo{param, generator->generate_class_dependency_from_property(object_info, param, current_class_content)});
                }
            }

            auto class_name = generate_class_name(native_class);

            generate_class_declaration(content_buffer, native_class, inherits_from_class);

            int32_t num_padding_elements{0};
            XProperty* last_property_in_this_class{nullptr};

            for (const auto& property_info : properties_to_generate)
            {
                XProperty* property = property_info.property;
                int32_t current_property_offset = property->GetOffset_Internal();
                int32_t current_property_size = property->GetSize();

                try
                {
                    const auto& property_name = property->GetName();
                    if (is_valid_lua_symbol(property_name))
                    {
                        content_buffer.append(fmt::format(STR("---@field {} {}\n"), property_name, generate_property_lua_name(property, true, native_class)));
                    }
                    else
                    {
                        content_buffer.append(
                                fmt::format(STR("---@field [{}] {}\n"), quote_lua_symbol(property_name), generate_property_lua_name(property, true, native_class)));
                    }
                }
                catch (std::exception& e)
                {
                    Output::send<LogLevel::Warning>(STR("Could not generate property '{}' because: {}\n"), property->GetFullName(), to_wstring(e.what()));
                    continue;
                }

                // TODO: Lua delegates

                last_property_in_this_class = property;
            }

            int32_t class_size = native_class->GetPropertiesSize();
            generate_class_struct_end(content_buffer, class_name, class_size, num_padding_elements, last_property_in_this_class);

            // Functions
            if (native_class->HasChildren())
            {
                content_buffer.append(STR("\n"));
                for (const auto& function_info : functions_to_generate)
                {
                    generator->generate_function_declaration(object_info, function_info, generated_file, content_buffer);
                }
            }

            generate_class_end(content_buffer, class_size);

            content_buffer.append(STR("\n\n"));

            current_class_content.append(content_buffer);
        }
        auto generate_class_declaration(File::StringType& content_buffer, UStruct* native_class, UStruct* inherits_from_class) -> void
        {
            auto class_name = generate_class_name(native_class);
            if (inherits_from_class)
            {
                content_buffer.append(fmt::format(STR("---@class {} : {}\n"), class_name, generate_class_name(inherits_from_class)));
            }
            else
            {
                content_buffer.append(fmt::format(STR("---@class {}\n"), class_name));
            }
        }
        auto generate_class_struct_end(File::StringType& content_buffer,
                                       const File::StringType& class_name,
                                       size_t class_size,
                                       int32_t num_padding_elements,
                                       XProperty* last_property_in_this_class) -> void
        {
            content_buffer.append(fmt::format(STR("{} = {{}}\n"), class_name));
        }
        auto generate_class_end(File::StringType& content_buffer, size_t class_size) -> void
        {
        }

        auto generate_function_declaration(TypeGenerator<LuaTypesGenerator>* generator,
                                           File::StringType& current_class_content,
                                           ObjectInfo& owner,
                                           const FunctionInfo& function_info,
                                           File::StringType function_name,
                                           XProperty* return_property,
                                           std::optional<PropertyInfo> return_property_info) -> void
        {
            for (size_t i = 0; i < function_info.params.size(); ++i)
            {
                const auto& param_info = function_info.params[i];
                if (!param_info.property->HasAnyPropertyFlags(Unreal::CPF_ReturnParm))
                {
                    try
                    {
                        auto param_name = param_info.property->GetName();
                        // TODO disambiguate param renames
                        current_class_content.append(fmt::format(STR("---@param {} {}\n"),
                                                                 make_valid_symbol(param_name),
                                                                 generate_property_lua_name(param_info.property, true, function_info.function)));
                    }
                    catch (std::exception& e)
                    {
                        Output::send<LogLevel::Warning>(STR("Could not generate function '{}' because: {}\n"),
                                                        function_info.function->GetFullName(),
                                                        to_wstring(e.what()));
                        return;
                    }
                }
            }

            if (return_property)
            {
                try
                {
                    current_class_content.append(fmt::format(STR("---@return {}\n"), generate_property_lua_name(return_property, true, function_info.function)));
                }
                catch (std::exception& e)
                {
                    Output::send<LogLevel::Warning>(STR("Could not generate function '{}' because: {}\n"),
                                                    function_info.function->GetFullName(),
                                                    to_wstring(e.what()));
                    return;
                }
            }

            auto class_name = generate_class_name(static_cast<UStruct*>(owner.object));

            if (is_valid_lua_symbol(function_name))
            {
                current_class_content.append(fmt::format(STR("function {}:{}("), class_name, function_name));
            }
            else
            {
                current_class_content.append(fmt::format(STR("{}[{}] = function("), class_name, quote_lua_symbol(function_name)));
            }

            for (size_t i = 0; i < function_info.params.size(); ++i)
            {
                const auto& param_info = function_info.params[i];
                if (!param_info.property->HasAnyPropertyFlags(Unreal::CPF_ReturnParm))
                {
                    auto param_name = param_info.property->GetName();
                    // TODO disambiguate param renames
                    current_class_content.append(fmt::format(STR("{}"), make_valid_symbol(param_name)));

                    if (i + 1 < function_info.params.size())
                    {
                        auto* next_param = function_info.params[i + 1].property;
                        if (next_param && (!next_param->HasAnyPropertyFlags(Unreal::CPF_ReturnParm) || i + 2 < function_info.params.size()))
                        {
                            current_class_content.append(STR(", "));
                        }
                    }
                }
            }
            current_class_content.append(STR(") end"));
        }
    };

    auto generate_cxx_headers(const std::filesystem::path directory_to_generate_in) -> void
    {
        TypeGenerator<CXXHeaderGenerator> generator{directory_to_generate_in};
        generator.generate();
    }

    auto generate_lua_types(const std::filesystem::path directory_to_generate_in) -> void
    {
        TypeGenerator<LuaTypesGenerator> generator{directory_to_generate_in};
        generator.generate();
    }
} // namespace RC::UEGenerator
