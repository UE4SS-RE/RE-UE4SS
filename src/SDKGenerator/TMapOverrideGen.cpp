#include <DynamicOutput/Output.hpp>
#include <SDKGenerator/TMapOverrideGen.hpp>
#include <Unreal/Common.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <File/Macros.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <File/File.hpp>
#include <JSON/JSON.hpp>
#include <unordered_map>

#pragma warning(disable: 4005)
#include <UE4SSProgram.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <USMapGenerator/writer.h>
#pragma warning(default: 4005)


namespace RC::UEGenerator
{
    using namespace ::RC::Unreal;
    
    std::unordered_set<FName> TMapOverrideGenerator::MapProperties{};

    auto TMapOverrideGenerator::generate_tmapoverride() -> void
    {
        Output::send(STR("Dumping TMap Property Overrides\n"));

        auto uaapifile = open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\UAssetAPITMapOverrides.json"), File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        auto fmodelfile = open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\FModelTMapOverrides.json"), File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        
        
        StringType uaapi_tmap_buffer;
        StringType fmodel_tmap_buffer;
        auto fm_object =  JSON::Object{};
        auto uaapi_object =  JSON::Object{};
        size_t num_objects_generated{};
        
        UObjectGlobals::ForEachUObject([&](void* untyped_object, [[maybe_unused]]int32_t chunk_index, [[maybe_unused]]int32_t object_index) {
            UObject* object = static_cast<UObject*>(untyped_object);
                        
            if (UStruct* casted_object = Cast<UStruct>(object))
            {
                auto as_class = Cast<UClass>(object);
                auto as_script_struct = as_class ? static_cast<UScriptStruct*>(nullptr) : Cast<UScriptStruct>(object);
                if (!as_class && !as_script_struct) { return LoopAction::Continue; }
                if (as_class && as_class->HasAnyClassFlags(CLASS_Native) || as_script_struct && as_script_struct->HasAnyStructFlags(STRUCT_Native))
                {
                    casted_object->ForEachProperty([&](FProperty* property) {
                        if (property->IsA<FMapProperty>() && !MapProperties.contains(property->GetFName()))
                        {
                            MapProperties.insert(property->GetFName());
                            auto propertyname = property->GetFName().ToString();
                            Output::send(STR("Found TMap Property: {} in Class: {}\n"), propertyname, object->GetName());
                            // Get TMap property Key and Value types and dump them
                            FProperty* key_property = static_cast<FMapProperty*>(property)->GetKeyProp();
                            FProperty* value_property = static_cast<FMapProperty*>(property)->GetValueProp();
                            
                            auto key_as_struct_property = static_cast<FStructProperty*>(key_property);
                            auto is_key_valid = key_property->IsA<FStructProperty>() && key_as_struct_property->GetStruct()->HasAnyStructFlags(STRUCT_SerializeNative);
                            
                            auto value_as_struct_property = static_cast<FStructProperty*>(value_property);
                            auto is_value_valid = value_property->IsA<FStructProperty>() && value_as_struct_property->GetStruct()->HasAnyStructFlags(STRUCT_SerializeNative);
                            
                            if (is_key_valid || is_value_valid)
                            {
                                // Generation.
                                auto& fm_json_object = fm_object.new_object(propertyname);
                                auto& uaapi_array = uaapi_object.new_array(propertyname);
                                                                
                                if (is_key_valid)
                                {
                                    auto keyname = key_as_struct_property->GetStruct()->GetName();
                                    fm_json_object.new_string(STR("Key"), keyname);
                                    uaapi_array.new_string(keyname);
                                }
                                else
                                {
                                    fm_json_object.new_string(STR("Key"), STR(""));
                                    uaapi_array.new_null();
                                }

                                if (is_value_valid)
                                {
                                    auto valuename = value_as_struct_property->GetStruct()->GetName();
                                    fm_json_object.new_string(STR("Value"), valuename);
                                    uaapi_array.new_string(valuename);
                                }
                                else
                                {
                                    fm_json_object.new_string(STR("Value"), STR(""));
                                    uaapi_array.new_null();
                                }
                                
                
                                ++num_objects_generated;
                            }
                        }
                        return LoopAction::Continue;
                    });
                }
            }
            else
            {
                return LoopAction::Continue;
            }
            return LoopAction::Continue;
        });

        // Retrieve JSON as a string.
        int32_t indent_level{};
        auto uaapi_string = uaapi_object.serialize(JSON::ShouldFormat::Yes, &indent_level);
        uaapi_tmap_buffer.append(uaapi_string);
        auto fm_string = fm_object.serialize(JSON::ShouldFormat::Yes, &indent_level);
        fmodel_tmap_buffer.append(fm_string);
        
        uaapifile.write_string_to_file(uaapi_tmap_buffer);
        fmodelfile.write_string_to_file(fmodel_tmap_buffer);
        Output::send(STR("Finished Dumping {} TMap Properties\n"), num_objects_generated);
        
    }
}