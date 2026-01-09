#include <DynamicOutput/DynamicOutput.hpp>
#include <DynamicOutput/Output.hpp>
#include <File/File.hpp>
#include <File/Macros.hpp>
#include <glaze/glaze.hpp>
#include <SDKGenerator/TMapOverrideGen.hpp>
#include <Unreal/Common.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <unordered_map>

#pragma warning(disable : 4005)
#include <SDKGenerator/UEHeaderGenerator.hpp>
#include <UE4SSProgram.hpp>
#include <USMapGenerator/writer.h>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#pragma warning(default : 4005)

namespace RC::UEGenerator
{
    using namespace ::RC::Unreal;

    std::unordered_set<FName> TMapOverrideGenerator::MapProperties{};

    auto TMapOverrideGenerator::generate_tmapoverride() -> void
    {
        Output::send(STR("Dumping TMap Property Overrides\n"));

        glz::generic fm_object{};
        glz::generic uaapi_object{};
        size_t num_objects_generated{};

        UObjectGlobals::ForEachUObject([&](void* untyped_object, [[maybe_unused]] int32_t chunk_index, [[maybe_unused]] int32_t object_index) {
            UObject* object = static_cast<UObject*>(untyped_object);

            if (UStruct* casted_object = Cast<UStruct>(object))
            {
                auto as_class = Cast<UClass>(object);
                auto as_script_struct = as_class ? static_cast<UScriptStruct*>(nullptr) : Cast<UScriptStruct>(object);
                if (!as_class && !as_script_struct)
                {
                    return LoopAction::Continue;
                }
                if ((as_class && as_class->HasAnyClassFlags(CLASS_Native)) || (as_script_struct && as_script_struct->HasAnyStructFlags(STRUCT_Native)))
                {
                    for (FProperty* property : TFieldRange<FProperty>(casted_object, EFieldIterationFlags::IncludeDeprecated))
                    {
                        if (!property->IsA<FMapProperty>() || MapProperties.contains(property->GetFName()))
                        {
                            continue;
                        }

                        MapProperties.insert(property->GetFName());

                        auto property_name = to_string(property->GetFName().ToString());

                        auto key_as_struct_property = CastField<FStructProperty>(static_cast<FMapProperty*>(property)->GetKeyProp());
                        auto key_struct_type = key_as_struct_property ? key_as_struct_property->GetStruct() : nullptr;
                        auto is_key_valid = key_struct_type && key_as_struct_property && key_struct_type->HasAnyStructFlags(STRUCT_SerializeNative);

                        auto value_as_struct_property = CastField<FStructProperty>(static_cast<FMapProperty*>(property)->GetValueProp());
                        auto value_struct_type = value_as_struct_property ? value_as_struct_property->GetStruct() : nullptr;
                        auto is_value_valid = value_struct_type && value_struct_type->HasAnyStructFlags(STRUCT_SerializeNative);

                        if (!is_key_valid && !is_value_valid)
                        {
                            continue;
                        }
                        Output::send(STR("Found Relevant TMap Property: {} in Class: {}\n"), to_wstring(property_name), object->GetName());

                        auto& fm_json_object = fm_object[property_name] = glz::generic::object_t{};
                        glz::generic::array_t uaapi_arr{};

                        if (is_key_valid)
                        {
                            auto key_name = to_string(key_as_struct_property->GetStruct()->GetName());
                            fm_json_object["Key"] = key_name;
                            uaapi_arr.emplace_back(key_name);
                        }
                        else
                        {
                            fm_json_object["Key"] = "";
                            uaapi_arr.emplace_back(nullptr);
                        }

                        if (is_value_valid)
                        {
                            auto value_name = to_string(value_as_struct_property->GetStruct()->GetName());
                            fm_json_object["Value"] = value_name;
                            uaapi_arr.emplace_back(value_name);
                        }
                        else
                        {
                            fm_json_object["Value"] = "";
                            uaapi_arr.emplace_back(nullptr);
                        }
                        uaapi_object[property_name] = std::move(uaapi_arr);

                        ++num_objects_generated;
                    }
                }
            }
            else
            {
                return LoopAction::Continue;
            }
            return LoopAction::Continue;
        });

        // Retrieve JSON as a string.
        auto uaapifile = open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\UAssetAPITMapOverrides.json"),
                              File::OpenFor::Writing,
                              File::OverwriteExistingFile::Yes,
                              File::CreateIfNonExistent::Yes);
        auto fmodelfile = open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\FModelTMapOverrides.json"),
                               File::OpenFor::Writing,
                               File::OverwriteExistingFile::Yes,
                               File::CreateIfNonExistent::Yes);

        if (auto uaapi_result = glz::write<glz::opts{.prettify = true}>(uaapi_object); uaapi_result.has_value())
        {
            uaapifile.write_string_to_file(to_wstring(uaapi_result.value()));
        }
        if (auto fm_result = glz::write<glz::opts{.prettify = true}>(fm_object); fm_result.has_value())
        {
            fmodelfile.write_string_to_file(to_wstring(fm_result.value()));
        }

        Output::send(STR("Finished Dumping {} TMap Properties\n"), num_objects_generated);
        MapProperties.clear();
    }
} // namespace RC::UEGenerator