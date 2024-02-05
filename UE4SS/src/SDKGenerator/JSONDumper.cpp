#include <DynamicOutput/DynamicOutput.hpp>
#include <JSON/JSON.hpp>
#include <SDKGenerator/Common.hpp>
#include <SDKGenerator/JSONDumper.hpp>
#include <Timer/ScopedTimer.hpp>
#include <UE4SSProgram.hpp>
#pragma warning(disable : 4005)
#include <Unreal/FProperty.hpp>
#include <Unreal/UAssetRegistry.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#pragma warning(default : 4005)

namespace RC::UEGenerator::JSONDumper
{
    using namespace ::RC::Unreal;

    auto static is_valid_class_to_dump(File::StringViewType class_name, UObject* object) -> bool
    {
        static bool is_below_425 = Unreal::Version::IsBelow(4, 25);
        if (is_below_425 && Unreal::TypeChecker::is_property(object))
        {
            // Ignore properties in GUObjectArray
            return false;
        }

        UClass* object_class = object->GetClassPrivate();
        if (!object_class->IsChildOf<Unreal::UBlueprintGeneratedClass>() || object_class->IsChildOf<Unreal::UAnimBlueprintGeneratedClass>())
        {
            return false;
        }

        UClass* object_as_class = static_cast<UClass*>(object);
        if (object_as_class->HasAnyClassFlags(EClassFlags::CLASS_Native))
        {
            return false;
        }

        if (object_as_class->HasAnyFlags(static_cast<EObjectFlags>(EObjectFlags::RF_DefaultSubObject | EObjectFlags::RF_ArchetypeObject)))
        {
            return false;
        }

        if (class_name.find(SYSSTR("BP_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(SYSSTR("ENE_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(SYSSTR("BPL_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(SYSSTR("OBJ_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(SYSSTR("LIB_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(SYSSTR("PRJ_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(SYSSTR("WPN_")) != class_name.npos)
        {
            return true;
        }

        return false;
    }

    auto static should_skip_property(FProperty* property) -> bool
    {
        static FName uber_graph_frame_name = FName(STR("UberGraphFrame"));
        static FName default_scene_root_name = FName(STR("DefaultSceneRoot"));

        if (property->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_ReturnParm))
        {
            return false;
        }

        FName property_fname = property->GetFName();
        if (property_fname.Equals(uber_graph_frame_name))
        {
            return true;
        }
        if (property_fname.Equals(default_scene_root_name))
        {
            return true;
        }

        auto property_name = property_fname.ToString();
        if (property_name.find(STR("CallFunc")) != property_name.npos)
        {
            return true;
        }

        return false;
    }

    auto static should_skip_general_function(UFunction* function) -> bool
    {
        static FName receive_name = FName(STR("Receive"));
        static FName receive_typo_name = FName(STR("Recieve"));
        static FName receive_begin_play_name = FName(STR("ReceiveBeginPlay"));
        static FName receive_destroyed_name = FName(STR("ReceiveDestroyedPlay"));
        static FName receive_tick_name = FName(STR("ReceiveTick"));
        static FName user_construction_script_name = FName(STR("UserConstructionScript"));

        FName function_fname = function->GetNamePrivate();
        if (function_fname.Equals(receive_name))
        {
            return true;
        }
        if (function_fname.Equals(receive_typo_name))
        {
            return true;
        }
        if (function_fname.Equals(receive_begin_play_name))
        {
            return true;
        }
        if (function_fname.Equals(receive_destroyed_name))
        {
            return true;
        }
        if (function_fname.Equals(receive_tick_name))
        {
            return true;
        }
        if (function_fname.Equals(user_construction_script_name))
        {
            return true;
        }

        auto function_name = function_fname.ToString();
        if (function_name.find(STR("Ubergraph")) != function_name.npos)
        {
            return true;
        }
        if (function_name.find(STR("OnTick_")) != function_name.npos)
        {
            return true;
        }
        if (function_name.find(STR("OnLoaded")) != function_name.npos)
        {
            return true;
        }

        return false;
    }

    auto static is_event(UFunction* function) -> bool
    {
        auto function_name = function->GetName();
        return function_name.starts_with(STR("On")) && !function_name.starts_with(STR("OnRep_"));
    }

    auto static should_skip_function(UFunction* function) -> bool
    {
        if ((function->GetFunctionFlags() & EFunctionFlags::FUNC_Delegate) != 0)
        {
            return true;
        }
        // Can't use FUNC_Event or FUNC_BlueprintEvent because it seems that everything has these flag
        // if ((function->get_function_flags() & EFunctionFlags::FUNC_Event) != 0) { return true; }
        // if ((function->get_function_flags() & EFunctionFlags::FUNC_BlueprintEvent) != 0) { return true; }
        if (is_event(function))
        {
            return true;
        }

        return should_skip_general_function(function);
    }

    auto static should_skip_event(File::StringViewType event_name) -> bool
    {
        if (event_name.find(SYSSTR("BndEvt")) != event_name.npos)
        {
            return true;
        }
        return false;
    }

    auto dump_to_json(File::StringViewType file_name) -> void
    {
        Output::send(SYSSTR("Loading all assets...\n"));
        UAssetRegistry::LoadAllAssets();

        Output::send(SYSSTR("Dumping to JSON file\n"));
        auto json = JSON::Array{};

        UObjectGlobals::ForEachUObject([&](void* raw_object, int32_t chunk_index, int32_t object_index) {
            if (!raw_object)
            {
                return LoopAction::Continue;
            }
            UObject* object = static_cast<UObject*>(raw_object);

            auto object_name = UEStringToSystemString(object->GetName());
            if (!is_valid_class_to_dump(object_name, object))
            {
                return LoopAction::Continue;
            }
            UClass* object_as_class = static_cast<UClass*>(object);

            object_name.erase(object_name.size() - 2, 2);

            auto& bp_class = json.new_object();
            bp_class.new_string(SYSSTR("bp_class"), object_name);
            if (auto* super_struct = object_as_class->GetSuperStruct(); super_struct)
            {
                bp_class.new_string(SYSSTR("inherits"), UEStringToSystemString(super_struct->GetName()));
            }
            else
            {
                bp_class.new_null(SYSSTR("inherits"));
            }

            auto& events = bp_class.new_array(SYSSTR("events"));
            for (UFunction* event_function : object_as_class->ForEachFunction())
            {
                if (should_skip_general_function(event_function))
                {
                    continue;
                }
                if (!is_event(event_function))
                {
                    continue;
                }

                auto event_name = UEStringToSystemString(event_function->GetName());
                if (should_skip_event(event_name))
                {
                    continue;
                }

                auto& bp_events = events.new_object();
                bp_events.new_string(SYSSTR("name"), event_name);

                auto& bp_event_args = bp_events.new_array(SYSSTR("args"));
                for (FProperty* param : event_function->ForEachProperty())
                {
                    if (should_skip_property(param))
                    {
                        continue;
                    }

                    auto& bp_event_arg = bp_event_args.new_object();
                    bp_event_arg.new_string(SYSSTR("name"), UEStringToSystemString(param->GetName()));
                    bp_event_arg.new_string(SYSSTR("type"), generate_property_cxx_name(param, true, event_function));
                    bool is_out = param->HasAnyPropertyFlags(EPropertyFlags::CPF_OutParm) && !param->HasAnyPropertyFlags(EPropertyFlags::CPF_ConstParm);
                    bp_event_arg.new_bool(SYSSTR("is_out"), is_out);
                    bp_event_arg.new_bool(SYSSTR("is_return"), param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_ReturnParm));
                }
            }

            auto& functions = bp_class.new_array(SYSSTR("functions"));
            for (UFunction* function : object_as_class->ForEachFunction())
            {
                if (should_skip_function(function))
                {
                    continue;
                }

                auto& bp_function = functions.new_object();
                bp_function.new_string(SYSSTR("name"), UEStringToSystemString(function->GetName()));

                auto& bp_function_args = bp_function.new_array(SYSSTR("args"));
                for (FProperty* param : function->ForEachProperty())
                {
                    if (should_skip_property(param))
                    {
                        continue;
                    }

                    auto& bp_function_arg = bp_function_args.new_object();
                    bp_function_arg.new_string(SYSSTR("name"), UEStringToSystemString(param->GetName()));
                    bp_function_arg.new_string(SYSSTR("type"), generate_property_cxx_name(param, true, function));
                    bool is_out = param->HasAnyPropertyFlags(EPropertyFlags::CPF_OutParm) && !param->HasAnyPropertyFlags(EPropertyFlags::CPF_ConstParm);
                    bp_function_arg.new_bool(SYSSTR("is_out"), is_out);
                    bp_function_arg.new_bool(SYSSTR("is_return"), param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_ReturnParm));
                }
            }

            auto& properties = bp_class.new_array(SYSSTR("properties"));
            for (FProperty* property : object_as_class->ForEachProperty())
            {
                if (should_skip_property(property))
                {
                    continue;
                }

                auto& bp_property = properties.new_object();
                bp_property.new_string(SYSSTR("name"),UEStringToSystemString(property->GetName()));
                bp_property.new_string(SYSSTR("type"), generate_property_cxx_name(property, true, object_as_class));
            }

            auto& delegates = bp_class.new_array(SYSSTR("delegates"));
            for (UFunction* delegate_function : object_as_class->ForEachFunction())
            {
                if (should_skip_general_function(delegate_function))
                {
                    continue;
                }
                if ((delegate_function->GetFunctionFlags() & EFunctionFlags::FUNC_Delegate) == 0)
                {
                    continue;
                }

                auto& bp_delegate = delegates.new_object();
                bp_delegate.new_string(SYSSTR("name"), UEStringToSystemString(delegate_function->GetName()));

                auto& bp_delegate_args = bp_delegate.new_array(SYSSTR("args"));
                for (FProperty* param : delegate_function->ForEachProperty())
                {
                    if (should_skip_property(param))
                    {
                        continue;
                    }

                    auto& bp_delegate_arg = bp_delegate_args.new_object();
                    bp_delegate_arg.new_string(SYSSTR("name"), UEStringToSystemString(param->GetName()));
                    bp_delegate_arg.new_string(SYSSTR("type"), generate_property_cxx_name(param, true, delegate_function));
                    bool is_out = param->HasAnyPropertyFlags(EPropertyFlags::CPF_OutParm) && !param->HasAnyPropertyFlags(EPropertyFlags::CPF_ConstParm);
                    bp_delegate_arg.new_bool(SYSSTR("is_out"), is_out);
                    bp_delegate_arg.new_bool(SYSSTR("is_return"), param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_ReturnParm));
                }
            }

            return LoopAction::Continue;
        });

        auto json_file = File::open(file_name, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        int32_t indent_level{};
        json_file.write_string_to_file(json.serialize(JSON::ShouldFormat::Yes, &indent_level));
        json_file.close();

        Output::send(SYSSTR("Unloading all forcefully loaded assets\n"));
        UAssetRegistry::FreeAllForcefullyLoadedAssets();
    }
} // namespace RC::UEGenerator::JSONDumper
