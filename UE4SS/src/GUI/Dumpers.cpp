#include <ctime>
#include <format>
#include <string>
#include <utility>

#include <Constructs/Views/EnumerateView.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <File/File.hpp>
#include <File/Macros.hpp>
#include <GUI/Dumpers.hpp>
#include <JSON/JSON.hpp>
#include <USMapGenerator/Generator.hpp>
#ifdef TEXT
#undef TEXT
#endif
#include <SDKGenerator/TMapOverrideGen.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/Searcher/ObjectSearcher.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <chrono>
#include <imgui.h>

namespace RC::GUI::Dumpers
{
    using namespace ::RC::Unreal;

    // TODO: Move these structs (and the enum) to a proper place in the Unreal module.
    //       They need to be controlled by the engine version select or before 2.0 releases.

    // In KH3, there's an extra float either at the end of FMeshUVChannelInfo or at the end of FStaticMaterial, I can't tell which one.
    enum
    {
        MAX_TEXCOORDS = 4,
        MAX_STATIC_TEXCOORDS = 8
    };
    struct FMeshUVChannelInfo
    {
        bool bInitialized;

        /** Whether this values was set manually or is auto generated. */
        bool bOverrideDensities;

        /**
         * The UV density in the mesh, before any transform scaling, in world unit per UV.
         * This value represents the length taken to cover a full UV unit.
         */
        float LocalUVDensities[MAX_TEXCOORDS];
    };

    struct FStaticMaterial_419AndBelow
    {
        using UMaterialInterface = ::RC::Unreal::UObject;
        UMaterialInterface* MaterialInterface;

        /*This name should be use by the gameplay to avoid error if the skeletal mesh Materials array topology change*/
        FName MaterialSlotName;

        /** Data used for texture streaming relative to each UV channels. */
        FMeshUVChannelInfo UVChannelData;
    };

    struct FStaticMaterial_420AndAbove
    {
        using UMaterialInterface = ::RC::Unreal::UObject;
        UMaterialInterface* MaterialInterface;

        /*This name should be use by the gameplay to avoid error if the skeletal mesh Materials array topology change*/
        FName MaterialSlotName;

        /*This name should be use when we re-import a skeletal mesh so we can order the Materials array like it should be*/
        FName ImportedMaterialSlotName;

        /** Data used for texture streaming relative to each UV channels. */
        FMeshUVChannelInfo UVChannelData;
    };

    auto generate_root_component_csv(UObject* root_component) -> StringType
    {
        StringType root_actor_buffer{};

        static auto location_property = root_component->GetPropertyByNameInChain(STR("RelativeLocation"));
        static auto rotation_property = root_component->GetPropertyByNameInChain(STR("RelativeRotation"));
        static auto scale_property = root_component->GetPropertyByNameInChain(STR("RelativeScale3D"));

        auto location = root_component->GetValuePtrByPropertyNameInChain<FVector>(STR("RelativeLocation"));
        FString location_string{};
        location_property->ExportTextItem(location_string, location, nullptr, nullptr, 0);
        root_actor_buffer.append(fmt::format(STR("\"{}\","), location_string.GetCharArray()));

        auto rotation = root_component->GetValuePtrByPropertyNameInChain<FRotator>(STR("RelativeRotation"));
        FString rotation_string{};
        rotation_property->ExportTextItem(rotation_string, rotation, nullptr, nullptr, 0);
        root_actor_buffer.append(fmt::format(STR("\"{}\","), rotation_string.GetCharArray()));

        auto scale = root_component->GetValuePtrByPropertyNameInChain<FVector>(STR("RelativeScale3D"));
        FString scale_string{};
        scale_property->ExportTextItem(scale_string, scale, nullptr, nullptr, 0);
        root_actor_buffer.append(fmt::format(STR("\"{}\","), scale_string.GetCharArray()));

        return root_actor_buffer;
    }

    static auto generate_actors_csv_file(UClass* dump_actor_class) -> StringType
    {
        StringType file_buffer{};

        file_buffer.append(STR("---,Actor,Location,Rotation,Scale,Meshes\n"));

        size_t actor_count{};
        FindObjectSearcher(dump_actor_class, AnySuperStruct::StaticClass()).ForEach([&](UObject* object) {
            if (object->HasAnyFlags(RF_ClassDefaultObject))
            {
                return LoopAction::Continue;
            }

            auto actor = static_cast<AActor*>(object);

            auto root_component = actor->GetValuePtrByPropertyNameInChain<UObject*>(STR("RootComponent"));
            if (!root_component || !*root_component)
            {
                return LoopAction::Continue;
            }

            StringType actor_buffer{};

            actor_buffer.append(fmt::format(STR("Row_{},"), actor_count));

            static auto game_mode_base = UObjectGlobals::FindFirstOf(STR("GameModeBase"));
            static auto class_property = game_mode_base->GetPropertyByNameInChain(STR("GameStateClass"));
            FString actor_class_string{};
            class_property->ExportTextItem(actor_class_string, &actor->GetClassPrivate(), nullptr, nullptr, 0);
            actor_buffer.append(fmt::format(STR("{},"), actor_class_string.GetCharArray()));

            // TODO: build system to handle other types of components - possibly including a way to specify which components to dump and which properties are important via a config file
            actor_buffer.append(generate_root_component_csv(*root_component));
            actor_buffer.append(STR("\""));
            static auto static_mesh_component_class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.StaticMeshComponent"));
            const auto& static_mesh_components = actor->K2_GetComponentsByClass(static_mesh_component_class);
            if (static_mesh_components.Num() > 0)
            {
                actor_buffer.append(STR("("));
                for (auto [static_mesh_component_ptr, static_mesh_component_index] : static_mesh_components | views::enumerate)
                {
                    const auto mesh = *static_mesh_component_ptr->GetValuePtrByPropertyNameInChain<UObject*>(STR("StaticMesh"));
                    if (!mesh)
                    {
                        Output::send<LogLevel::Warning>(STR("SKIPPING COMPONENT! StaticMeshComponent '{}' has no mesh.\n"),
                                                        static_mesh_component_ptr->GetOuterPrivate()->GetName());
                        continue;
                    }

                    static auto mesh_property = static_mesh_component_ptr->GetPropertyByNameInChain(STR("StaticMesh"));
                    FString mesh_string{};
                    mesh_property->ExportTextItem(mesh_string, &mesh, nullptr, nullptr, 0);
                    actor_buffer.append(fmt::format(STR("(StaticMesh={}',"), mesh_string.GetCharArray()));

                    auto materials_for_each_body = [&](const UObject* material_interface) {
                        if (material_interface)
                        {
                            auto material_full_name = material_interface->GetOuterPrivate()->GetFullName();
                            const auto material_type_space_location = material_full_name.find(STR(" "));
                            if (material_type_space_location == material_full_name.npos)
                            {
                                Output::send<LogLevel::Warning>(STR("SKIPPING MATERIAL! Was unable to find space in full material name in component: '{}'.\n"),
                                                                material_full_name);
                                return;
                            }

                            if (material_type_space_location > static_cast<unsigned long long>(std::numeric_limits<long long>::max()))
                            {
                                throw std::runtime_error{"integer overflow when converting material_type_space_location signed\n"};
                            }
                            auto material_typeless_name = StringViewType{material_full_name.begin() + static_cast<long long>(material_type_space_location) + 1,
                                                                         material_full_name.end()};

                            actor_buffer.append(fmt::format(STR("{}'"), material_interface->GetClassPrivate()->GetName()));
                            actor_buffer.append(fmt::format(STR("\"\"{}"), material_typeless_name));
                            actor_buffer.append(STR("\"\"'"));
                        }
                    };

                    if (Version::IsAtMost(4, 19))
                    {
                        const auto materials = *mesh->GetValuePtrByPropertyName<TArray<FStaticMaterial_419AndBelow>>(STR("StaticMaterials"));
                        if (materials.GetData())
                        {
                            actor_buffer.append(STR("Materials=("));
                        }
                        for (auto [material, material_index] : materials | views::enumerate)
                        {
                            materials_for_each_body(material.MaterialInterface);
                            if (material_index + 1 < materials.Num())
                            {
                                actor_buffer.append(STR(","));
                            }
                            else
                            {
                                actor_buffer.append(STR(")"));
                            }
                        }
                    }
                    else
                    {
                        const auto& materials = *mesh->GetValuePtrByPropertyName<TArray<FStaticMaterial_420AndAbove>>(STR("StaticMaterials"));
                        if (materials.GetData())
                        {
                            actor_buffer.append(STR("Materials=("));
                        }
                        for (auto [material, material_index] : materials | views::enumerate)
                        {
                            materials_for_each_body(material.MaterialInterface);
                            if (material_index + 1 < materials.Num())
                            {
                                actor_buffer.append(STR(","));
                            }
                            else
                            {
                                actor_buffer.append(STR(")"));
                            }
                        }
                    }
                    actor_buffer.append(STR(")"));

                    if (static_mesh_component_index + 1 < static_mesh_components.Num())
                    {
                        actor_buffer.append(STR(","));
                    }
                }
                actor_buffer.append(STR(")"));
            }
            actor_buffer.append(STR("\"\n"));
            file_buffer.append(actor_buffer);

            ++actor_count;
            return LoopAction::Continue;
        });

        return file_buffer;
    }

    static auto generate_actors_json_file(UClass* class_to_dump) -> StringType
    {
        auto global_json_array = JSON::Array{};

        size_t actor_count{};
        FindObjectSearcher(class_to_dump, AnySuperStruct::StaticClass()).ForEach([&](UObject* object) {
            if (object->HasAnyFlags(RF_ClassDefaultObject))
            {
                return LoopAction::Continue;
            }

            auto actor = static_cast<AActor*>(object);

            auto root_component = actor->GetValuePtrByPropertyNameInChain<UObject*>(STR("RootComponent"));
            if (!root_component || !*root_component)
            {
                return LoopAction::Continue;
            }

            auto& actor_json_object = global_json_array.new_object();

            actor_json_object.new_string(STR("Name"), fmt::format(STR("Row_{}"), actor_count));

            static auto game_mode_base = UObjectGlobals::FindFirstOf(STR("GameModeBase"));
            static auto class_property = game_mode_base->GetPropertyByNameInChain(STR("GameStateClass"));
            FString actor_class_string{};
            class_property->ExportTextItem(actor_class_string, &actor->GetClassPrivate(), nullptr, nullptr, 0);
            actor_json_object.new_string(STR("Actor"), fmt::format(STR("{}"), StringViewType{actor_class_string.GetCharArray()}));

            auto& root_component_json_object = actor_json_object.new_object(STR("RootComponent"));

            FString root_component_class_string{};
            class_property->ExportTextItem(root_component_class_string, &(*root_component)->GetClassPrivate(), nullptr, nullptr, 0);
            root_component_json_object.new_string(STR("SceneComponentClass"), fmt::format(STR("{}"), StringViewType{root_component_class_string.GetCharArray()}));

            auto& location_json_object = root_component_json_object.new_object(STR("Location"));
            auto location = (*root_component)->GetValuePtrByPropertyNameInChain<FVector>(STR("RelativeLocation"));
            location_json_object.new_number(STR("X"), location->X());
            location_json_object.new_number(STR("Y"), location->Y());
            location_json_object.new_number(STR("Z"), location->Z());

            auto& rotation_json_object = root_component_json_object.new_object(STR("Rotation"));
            auto rotation = (*root_component)->GetValuePtrByPropertyNameInChain<FRotator>(STR("RelativeRotation"));
            rotation_json_object.new_number(STR("Pitch"), rotation->GetPitch());
            rotation_json_object.new_number(STR("Yaw"), rotation->GetYaw());
            rotation_json_object.new_number(STR("Roll"), rotation->GetRoll());

            auto& scale_json_object = root_component_json_object.new_object(STR("Scale"));
            auto scale = (*root_component)->GetValuePtrByPropertyNameInChain<FVector>(STR("RelativeScale3D"));
            scale_json_object.new_number(STR("X"), scale->X());
            scale_json_object.new_number(STR("Y"), scale->Y());
            scale_json_object.new_number(STR("Z"), scale->Z());

            ++actor_count;
            return LoopAction::Continue;
        });

        int32_t indent_level{};
        return global_json_array.serialize(JSON::ShouldFormat::Yes, &indent_level);
    }

    auto CurrentDumpNums = MapDumpFileName{};

    void call_generate_static_mesh_file()
    {
        Output::send(STR("Dumping CSV of all loaded static mesh actors, positions and mesh properties\n"));
        static auto dump_actor_class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.StaticMeshActor"));
        std::wstring file_buffer{};
        file_buffer.append(generate_actors_csv_file(dump_actor_class));
        auto file =
                File::open(fmt::format(STR("{}\\{}-ue4ss_static_mesh_data.csv"), UE4SSProgram::get_program().get_working_directory(), long(std::time(nullptr))),
                           File::OpenFor::Writing,
                           File::OverwriteExistingFile::Yes,
                           File::CreateIfNonExistent::Yes);
        file.write_string_to_file(file_buffer);
        Output::send(STR("Finished dumping CSV of all loaded static mesh actors, positions and mesh properties\n"));
    }

    void call_generate_all_actor_file()
    {
        Output::send(STR("Dumping CSV of all loaded actor types, positions and mesh properties\n"));
        std::wstring file_buffer{};
        file_buffer.append(generate_actors_csv_file(AActor::StaticClass()));
        auto file = File::open(fmt::format(STR("{}\\{}-ue4ss_actor_data.csv"), UE4SSProgram::get_program().get_working_directory(), long(std::time(nullptr))),
                               File::OpenFor::Writing,
                               File::OverwriteExistingFile::Yes,
                               File::CreateIfNonExistent::Yes);
        file.write_string_to_file(file_buffer);
        Output::send(STR("Finished dumping CSV of all loaded actor types, positions and mesh properties\n"));
    }

    auto render() -> void
    {
        if (!UnrealInitializer::StaticStorage::bIsInitialized)
        {
            return;
        }

        if (ImGui::Button("Dump all static actor meshes to file"))
        {

            call_generate_static_mesh_file();

            /*auto file = File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\ue4ss_static_mesh_data.json"), File::OpenFor::Writing,
            File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes); file.write_string_to_file(generate_actors_json_file(dump_actor_class));*/
        }

        if (ImGui::Button("Dump all actors to file"))
        {

            call_generate_all_actor_file();

            /*auto file = File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\ue4ss_actor_data.json"), File::OpenFor::Writing,
            File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes); file.write_string_to_file(generate_actors_json_file(AActor::StaticClass()));*/
        }

        /*ImGui::SameLine();*/

        /*if (ImGui::Button("Test #1"))
        {
            auto func = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Engine.KismetSystemLibrary:LoadAsset"));
            func->ForEachProperty([](FProperty* param) {
                Output::send<LogLevel::Verbose>(STR("{}\n"), param->GetName());
                if (auto as_struct_prop = CastField<FStructProperty>(param); as_struct_prop)
                {
                    Output::send<LogLevel::Verbose>(STR("Param is StructProperty: {}\n"), as_struct_prop->GetStruct()->GetFullName());
                    if (as_struct_prop->GetStruct()->IsChildOf(UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.LatentActionInfo"))))
                    {
                        Output::send<LogLevel::Verbose>(STR("StructProperty is LatentActionInfo\n"));
                    }
                }
                return LoopAction::Continue;
            });
        }*/

        if (ImGui::Button("Generate .usmap file\nUnrealMappingsDumper by OutTheShade"))
        {
            OutTheShade::generate_usmap();
        }

        if (ImGui::Button("Generate TMapOverride file\n"))
        {
            UEGenerator::TMapOverrideGenerator::generate_tmapoverride();
        }

        if (ImGui::Button("Generate UHT Compatible Headers\n"))
        {
            UE4SSProgram::get_program().generate_uht_compatible_headers();
        }

        if (ImGui::Button("Dump CXX Headers\n"))
        {
            File::StringType working_dir{UE4SSProgram::get_program().get_working_directory()};
            UE4SSProgram::get_program().generate_cxx_headers(working_dir + STR("\\CXXHeaderDump"));
        }

        if (ImGui::Button("Generate Lua Types\n"))
        {
            File::StringType working_dir{UE4SSProgram::get_program().get_working_directory()};
            UE4SSProgram::get_program().generate_lua_types(working_dir + STR("\\Mods\\shared\\types"));
        }
    }
} // namespace RC::GUI::Dumpers
