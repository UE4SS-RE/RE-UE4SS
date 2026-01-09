#include <DynamicOutput/DynamicOutput.hpp>
#include <glaze/glaze.hpp>
#include <SDKGenerator/Common.hpp>
#include <SDKGenerator/JSONDumper.hpp>
#include <Timer/ScopedTimer.hpp>
#include <UE4SSProgram.hpp>
#pragma warning(disable : 4005)
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/UAssetRegistry.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
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

        if (class_name.find(STR("BP_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(STR("ENE_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(STR("BPL_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(STR("OBJ_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(STR("LIB_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(STR("PRJ_")) != class_name.npos)
        {
            return true;
        }
        if (class_name.find(STR("WPN_")) != class_name.npos)
        {
            return true;
        }

        return false;
    }

    auto static should_skip_property(FProperty* property) -> bool
    {
        static FName uber_graph_frame_name = FName(STR("UberGraphFrame"), FNAME_Find);
        static FName default_scene_root_name = FName(STR("DefaultSceneRoot"), FNAME_Find);

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
        static FName receive_name = FName(STR("Receive"), FNAME_Find);
        static FName receive_typo_name = FName(STR("Recieve"), FNAME_Find);
        static FName receive_begin_play_name = FName(STR("ReceiveBeginPlay"), FNAME_Find);
        static FName receive_destroyed_name = FName(STR("ReceiveDestroyedPlay"), FNAME_Find);
        static FName receive_tick_name = FName(STR("ReceiveTick"), FNAME_Find);
        static FName user_construction_script_name = FName(STR("UserConstructionScript"), FNAME_Find);

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
        if (event_name.find(STR("BndEvt")) != event_name.npos)
        {
            return true;
        }
        return false;
    }

    auto dump_to_json(File::StringViewType file_name) -> void
    {
        Output::send(STR("Loading all assets...\n"));
        UAssetRegistry::LoadAllAssets();

        Output::send(STR("Dumping to JSON file\n"));
        glz::generic::array_t json_array{};

        UObjectGlobals::ForEachUObject([&](void* raw_object, int32_t chunk_index, int32_t object_index) {
            if (!raw_object)
            {
                return LoopAction::Continue;
            }
            UObject* object = static_cast<UObject*>(raw_object);

            auto object_name = object->GetName();
            if (!is_valid_class_to_dump(object_name, object))
            {
                return LoopAction::Continue;
            }
            UClass* object_as_class = static_cast<UClass*>(object);

            object_name.erase(object_name.size() - 2, 2);

            glz::generic bp_class = glz::generic::object_t{};
            bp_class["bp_class"] = to_string(object_name);
            if (auto* super_struct = object_as_class->GetSuperStruct(); super_struct)
            {
                bp_class["inherits"] = to_string(super_struct->GetName());
            }
            else
            {
                bp_class["inherits"] = nullptr;
            }

            glz::generic::array_t events_array{};
            for (UFunction* event_function : TFieldRange<UFunction>(object_as_class, EFieldIterationFlags::None))
            {
                if (should_skip_general_function(event_function))
                {
                    continue;
                }
                if (!is_event(event_function))
                {
                    continue;
                }

                auto event_name = event_function->GetName();
                if (should_skip_event(event_name))
                {
                    continue;
                }

                glz::generic bp_event = glz::generic::object_t{};
                bp_event["name"] = to_string(event_name);

                glz::generic::array_t bp_event_args_array{};
                for (FProperty* param : TFieldRange<FProperty>(event_function, EFieldIterationFlags::IncludeDeprecated))
                {
                    if (should_skip_property(param))
                    {
                        continue;
                    }

                    glz::generic bp_event_arg = glz::generic::object_t{};
                    bp_event_arg["name"] = to_string(param->GetName());
                    bp_event_arg["type"] = to_string(generate_property_cxx_name(param, true, event_function));
                    bool is_out = param->HasAnyPropertyFlags(EPropertyFlags::CPF_OutParm) && !param->HasAnyPropertyFlags(EPropertyFlags::CPF_ConstParm);
                    bp_event_arg["is_out"] = is_out;
                    bp_event_arg["is_return"] = param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_ReturnParm);
                    bp_event_args_array.emplace_back(std::move(bp_event_arg));
                }
                bp_event["args"] = std::move(bp_event_args_array);
                events_array.emplace_back(std::move(bp_event));
            }
            bp_class["events"] = std::move(events_array);

            glz::generic::array_t functions_array{};
            for (UFunction* function : TFieldRange<UFunction>(object_as_class, EFieldIterationFlags::None))
            {
                if (should_skip_function(function))
                {
                    continue;
                }

                glz::generic bp_function = glz::generic::object_t{};
                bp_function["name"] = to_string(function->GetName());

                glz::generic::array_t bp_function_args_array{};
                for (FProperty* param : TFieldRange<FProperty>(function, EFieldIterationFlags::IncludeDeprecated))
                {
                    if (should_skip_property(param))
                    {
                        continue;
                    }

                    glz::generic bp_function_arg = glz::generic::object_t{};
                    bp_function_arg["name"] = to_string(param->GetName());
                    bp_function_arg["type"] = to_string(generate_property_cxx_name(param, true, function));
                    bool is_out = param->HasAnyPropertyFlags(EPropertyFlags::CPF_OutParm) && !param->HasAnyPropertyFlags(EPropertyFlags::CPF_ConstParm);
                    bp_function_arg["is_out"] = is_out;
                    bp_function_arg["is_return"] = param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_ReturnParm);
                    bp_function_args_array.emplace_back(std::move(bp_function_arg));
                }
                bp_function["args"] = std::move(bp_function_args_array);
                functions_array.emplace_back(std::move(bp_function));
            }
            bp_class["functions"] = std::move(functions_array);

            glz::generic::array_t properties_array{};
            for (FProperty* property : TFieldRange<FProperty>(object_as_class, EFieldIterationFlags::IncludeDeprecated))
            {
                if (should_skip_property(property))
                {
                    continue;
                }

                glz::generic bp_property = glz::generic::object_t{};
                bp_property["name"] = to_string(property->GetName());
                bp_property["type"] = to_string(generate_property_cxx_name(property, true, object_as_class));
                properties_array.emplace_back(std::move(bp_property));
            }
            bp_class["properties"] = std::move(properties_array);

            glz::generic::array_t delegates_array{};
            for (UFunction* delegate_function : TFieldRange<UFunction>(object_as_class, EFieldIterationFlags::None))
            {
                if (should_skip_general_function(delegate_function))
                {
                    continue;
                }
                if ((delegate_function->GetFunctionFlags() & EFunctionFlags::FUNC_Delegate) == 0)
                {
                    continue;
                }

                glz::generic bp_delegate = glz::generic::object_t{};
                bp_delegate["name"] = to_string(delegate_function->GetName());

                glz::generic::array_t bp_delegate_args_array{};
                for (FProperty* param : TFieldRange<FProperty>(delegate_function, EFieldIterationFlags::IncludeDeprecated))
                {
                    if (should_skip_property(param))
                    {
                        continue;
                    }

                    glz::generic bp_delegate_arg = glz::generic::object_t{};
                    bp_delegate_arg["name"] = to_string(param->GetName());
                    bp_delegate_arg["type"] = to_string(generate_property_cxx_name(param, true, delegate_function));
                    bool is_out = param->HasAnyPropertyFlags(EPropertyFlags::CPF_OutParm) && !param->HasAnyPropertyFlags(EPropertyFlags::CPF_ConstParm);
                    bp_delegate_arg["is_out"] = is_out;
                    bp_delegate_arg["is_return"] = param->HasAnyPropertyFlags(Unreal::EPropertyFlags::CPF_ReturnParm);
                    bp_delegate_args_array.emplace_back(std::move(bp_delegate_arg));
                }
                bp_delegate["args"] = std::move(bp_delegate_args_array);
                delegates_array.emplace_back(std::move(bp_delegate));
            }
            bp_class["delegates"] = std::move(delegates_array);

            json_array.emplace_back(std::move(bp_class));
            return LoopAction::Continue;
        });

        auto json_file = File::open(file_name, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        glz::generic json_root{std::move(json_array)};
        if (auto result = glz::write<glz::opts{.prettify = true}>(json_root); result.has_value())
        {
            json_file.write_string_to_file(to_wstring(result.value()));
        }
        json_file.close();

        Output::send(STR("Unloading all forcefully loaded assets\n"));
        UAssetRegistry::FreeAllForcefullyLoadedAssets();
    }
} // namespace RC::UEGenerator::JSONDumper
