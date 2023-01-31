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
    
    std::unordered_map<FName, int> TMapOverrideGenerator::MapProperties{};

    auto TMapOverrideGenerator::generate_tmapoverride() -> void
    {
        Output::send(STR("Dumping TMap Property Overrides\n"));

        auto uaapifile = open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\UAssetAPITMapOverrides.json"), File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        /*auto fmodelfile = File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\FModelTMapOverrides.json"), File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);*/
        
        
        StringType TMapBuffer;
        size_t num_objects_generated{};
        
        TMapBuffer.append(STR("{\n"));
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
                            MapProperties.insert_or_assign(property->GetFName(), 0);
                            std::wstring propertyname = property->GetFName().ToString();
                            Output::send(STR("Found TMap Property: {} in Class: {}\n"), propertyname, object->GetName());
                            // Get TMap property Key and Value types and dump them
                            FProperty* key_property = static_cast<FMapProperty*>(property)->GetKeyProp();
                            FProperty* value_property = static_cast<FMapProperty*>(property)->GetValueProp();
                            
                            if (key_property->IsA<FStructProperty>() || value_property->IsA<FStructProperty>())
                            {
                                if (num_objects_generated > 0)
                                {
                                    TMapBuffer.append(STR(",\n"));
                                }
                                TMapBuffer.append(std::format(STR("  \"{}\": [\n"), propertyname));
                    
                                if (key_property->IsA<FStructProperty>())
                                {
                                    TMapBuffer.append(std::format(STR("    \"{}\"\n"), static_cast<FStructProperty*>(key_property)->GetStruct()->GetName()));
                                }
                                else
                                {
                                    TMapBuffer.append(STR("    null,\n"));
                                }
                    
                                if (value_property->IsA<FStructProperty>())
                                {
                                    TMapBuffer.append(std::format(STR("    \"{}\"\n"), static_cast<FStructProperty*>(value_property)->GetStruct()->GetName()));
                                }
                                else
                                {
                                    TMapBuffer.append(STR("    null,\n"));
                                }
                                TMapBuffer.append(STR("  ]"));
                
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

        TMapBuffer.append(STR("\n}"));
        uaapifile.write_string_to_file(TMapBuffer);
        Output::send(STR("Finished Dumping {} TMap Properties\n"), num_objects_generated);
        
    }
}