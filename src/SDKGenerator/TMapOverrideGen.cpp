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


namespace RC::TMapOverrideGen
{
    using namespace ::RC::Unreal;
    using UObject = RC::Unreal::UObject;
    using XProperty = RC::Unreal::FProperty;
    using FField = RC::Unreal::FField;
    

    

    std::unordered_map<FName, int> MapProperties;

    auto dump_tmaps(UObject* object, XProperty* property, size_t NumProps) -> std::pair<StringType, size_t>
    {
        StringType TMapBuffer;
        
        if (property->IsA<FMapProperty>() && !MapProperties.contains(property->GetFName()))
        {
            MapProperties.insert_or_assign(property->GetFName(), 0);
            std::wstring propertyname = property->GetFName().ToString();
            Output::send(STR("Found TMap Property: {} in Class: {}\n"), propertyname, object->GetName());
            // Get TMap property Key and Value types and dump them
            XProperty* key_property = static_cast<FMapProperty*>(property)->GetKeyProp();
            XProperty* value_property = static_cast<FMapProperty*>(property)->GetValueProp();

                
            if (key_property->IsA<FStructProperty>() || value_property->IsA<FStructProperty>())
            {
                if (NumProps > 0)
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
                
                ++NumProps;
            }
        }

        return std::pair{TMapBuffer, NumProps};
    }

    auto TMapOverrideGenerator::generate_tmapoverride() -> void
    {
        Output::send(STR("Dumping TMap Property Overrides\n"));

        auto uaapifile = open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\UAssetAPITMapOverrides.json"), File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        /*auto fmodelfile = File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + STR("\\FModelTMapOverrides.json"), File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);*/
        
        
        StringType TMapJSONBuffer;
        size_t num_objects_generated{};
        
        TMapJSONBuffer.append(STR("{\n"));
        UObjectGlobals::ForEachUObject([&](void* untyped_object, [[maybe_unused]]int32_t chunk_index, [[maybe_unused]]int32_t object_index) {
            UObject* object = static_cast<UObject*>(untyped_object);

            if (UClass* casted_object = Cast<UClass>(object))
            {
                if ((casted_object->GetClassFlags() & CLASS_Native) != 0)
                {
                    casted_object->ForEachProperty([&](XProperty* property) {
                        auto [TMapBuffer, objects_generated] = dump_tmaps(object, property, num_objects_generated);
                        TMapJSONBuffer.append(TMapBuffer);
                        num_objects_generated = objects_generated;
                        return LoopAction::Continue;
                    });
                    
                }
            }
            else if (UScriptStruct* casted_struct = Cast<UScriptStruct>(object))
            {
                if ((casted_struct->GetStructFlags() & STRUCT_Native) != 0)
                {
                    casted_struct->ForEachProperty([&](XProperty* property) {
                        auto [TMapBuffer, objects_generated] = dump_tmaps(object, property, num_objects_generated);
                        TMapJSONBuffer.append(TMapBuffer);
                        num_objects_generated = objects_generated;
                        return LoopAction::Continue;
                    });
                }
            }
            return LoopAction::Continue;
        });

        TMapJSONBuffer.append(STR("\n}"));
        uaapifile.write_string_to_file(TMapJSONBuffer);
        Output::send(STR("Finished Dumping {} TMap Properties\n"), num_objects_generated);
        
    }
}