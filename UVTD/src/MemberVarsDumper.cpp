#include <format>
#include <unordered_map>

#include <DynamicOutput/DynamicOutput.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/MemberVarsDumper.hpp>

namespace RC::UVTD
{
    static inline std::vector<File::StringType> s_types_to_not_dump{
            STR("FUnversionedStructSchema"),
            STR("ELifetimeCondition"),
            STR("UAISystemBase"),
            STR("FLevelCollection"),
            STR("FThreadSafeCounter"),
            STR("FWorldAsyncTraceState"),
            STR("FDelegateHandle"),
            STR("UAvoidanceManager"),
            STR("FOnBeginTearingDownEvent"),
            STR("UBlueprint"),
            STR("UCanvas"),
            STR("UActorComponent"),
            STR("AController"),
            STR("ULevel"),
            STR("FPhysScene_Chaos"),
            STR("APhysicsVolume"),
            STR("UDemoNetDriver"),
            STR("FEndPhysicsTickFunction"),
            STR("FFXSystemInterface"),
            STR("ERHIFeatureLevel"),
            STR("EFlushLevelStreamingType"),
            STR("ULineBatchComponent"),
            STR("AGameState"),
            STR("FOnGameStateSetEvent"),
            STR("AAudioVolume"),
            STR("FLatentActionManager"),
            STR("FOnLevelsChangedEvent"),
            STR("AParticleEventManager"),
            STR("UNavigationSystem"),
            STR("UNetDriver"),
            STR("AGameNetworkManager"),
            STR("ETravelType"),
            STR("FDefaultDelegateUserPolicy"),
            STR("TMulticastDelegate"),
            STR("FActorsInitializedParams"),
            STR("FOnBeginPostProcessSettings"),
            STR("FIntVector"),
            STR("UGameInstance"),
            STR("FWorldPSCPool"),
            STR("UMaterialParameterCollectionInstance"),
            STR("FParticlePerfStats"),
            STR("FWorldInGamePerformanceTrackers"),
            STR("UPhysicsCollisionHandler"),
            STR("UPhysicsFieldComponent"),
            STR("FPhysScene"),
            STR("APlayerController"),
            STR("IInterface_PostProcessVolume"),
            STR("FOnTickFlushEvent"),
            STR("FSceneInterface"),
            STR("FStartAsyncSimulationFunction"),
            STR("FStartPhysicsTickFunction"),
            STR("FOnNetTickEvent"),
            STR("ETickingGroup"),
            STR("FTickTaskLevel"),
            STR("FTimerManager"),
            STR("FURL"),
            STR("UWorldComposition"),
            STR("EWorldType"),
            STR("FSubsystemCollection"),
            STR("UWorldSubsystem"),
            STR("FStreamingLevelsToConsider"),
            STR("APawn"),
            STR("ACameraActor"),
            STR("EMapPropertyFlags"),
            STR("FScriptMapLayout"),
            STR("EArrayPropertyFlags"),
            STR("ICppClassTypeInfo"),
            STR("FNativeFunctionLookup"),
            STR("FGCReferenceTokenStream"),
            STR("FWindowsCriticalSection"),
            STR("FWindowsRWLock"),
            STR("FRepRecord"),
            STR("EClassCastFlags"),
            STR("FAudioDeviceHandle"),
            STR("TVector"),
            STR("FScriptSetLayout"),
            STR("FArchiveSerializedPropertyChain"),
            STR("FArchiveCookData"),
            STR("FFastPathLoadBuffer"),
            STR("FTokenStreamOwner"),
            STR("FUObjectCppClassStaticFunctions"),
            STR("FWorldCachedViewInfo"),
            STR("UContentBundleManager"),
            STR("FIrisSystemHolder"),
            STR("TSet"),
            STR("TOptional"),
            STR("FWorldPartitionInitializedEvent"),
            STR("FWorldPartitionUninitializedEvent"),
            STR("TIntVector3"),
            STR("AServerStreamingLevelsVisibility"),
            STR("FOnControllerIdChanged"),
            STR("FOnPlatformUserIdChanged"),
            STR("FObjectSubsystemCollection"),
            STR("FSceneViewStateReference"),
    };

    auto MemberVarsDumper::process_class(const PDB::TPIStream& tpi_stream,
                                         const PDB::CodeView::TPI::Record* class_record,
                                         const File::StringType& name,
                                         const SymbolNameInfo& name_info) -> void
    {
        auto changed = change_prefix(name, symbols.is_425_plus);
        if (!changed.has_value()) return;

        File::StringType class_name = *changed;
        File::StringType class_name_clean = Symbols::clean_name(class_name);
        auto& class_entry = type_container.get_or_create_class_entry(class_name, class_name_clean, name_info);

        auto fields = tpi_stream.GetTypeRecord(class_record->data.LF_CLASS.field);

        auto list_size = fields->header.size - sizeof(uint16_t);
        for (size_t i = 0; i < list_size; i++)
        {
            auto field_record = (PDB::CodeView::TPI::FieldList*)((uint8_t*)&fields->data.LF_FIELD.list + i);

            if (field_record->kind == PDB::CodeView::TPI::TypeRecordKind::LF_MEMBER)
            {
                process_member(tpi_stream, field_record, class_entry);
            }
        }
    }

    auto MemberVarsDumper::process_member(const PDB::TPIStream& tpi_stream, const PDB::CodeView::TPI::FieldList* field_record, Class& class_entry) -> void
    {
        File::StringType member_name = Symbols::get_leaf_name(field_record->data.LF_STMEMBER.name, field_record->data.LF_MEMBER.lfEasy.kind);
        auto changed = change_prefix(Symbols::get_type_name(tpi_stream, field_record->data.LF_MEMBER.index), symbols.is_425_plus);
        if (!changed.has_value()) return;

        File::StringType type_name = *changed;

        for (const auto& type_to_not_dump : s_types_to_not_dump)
        {
            if (type_name.find(type_to_not_dump) != type_name.npos)
            {
                return;
            }
        }

        auto& variable = class_entry.variables[member_name];
        variable.type = type_name;
        variable.name = member_name;
        variable.offset = *(uint16_t*)field_record->data.LF_MEMBER.offset;
    }

    auto MemberVarsDumper::dump_member_variable_layouts(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void
    {
        Output::send(STR("Dumping {} symbols for {}\n"), names.size(), symbols.pdb_file_path.filename().stem().wstring());

        const PDB::TPIStream tpi_stream = PDB::CreateTPIStream(symbols.pdb_file);

        for (const PDB::CodeView::TPI::Record* type_record : tpi_stream.GetTypeRecords())
        {
            if (type_record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_CLASS ||
                type_record->header.kind == PDB::CodeView::TPI::TypeRecordKind::LF_STRUCTURE)
            {
                if (type_record->data.LF_CLASS.property.fwdref) continue;

                const File::StringType class_name = Symbols::get_leaf_name(type_record->data.LF_CLASS.data, type_record->data.LF_CLASS.lfEasy.kind);
                if (!names.contains(class_name)) continue;

                const auto name_info = names.find(class_name);
                if (name_info == names.end()) continue;

                process_class(tpi_stream, type_record, class_name, name_info->second);
            }
        }

        return;
    }

    auto MemberVarsDumper::generate_code() -> void
    {
        std::unordered_map<File::StringType, SymbolNameInfo> member_vars_names;

        for (ObjectItem& item : s_object_items)
        {
            if (item.valid_for_member_vars != ValidForMemberVars::Yes) continue;
            member_vars_names.emplace(item.name, SymbolNameInfo{item.valid_for_vtable, item.valid_for_member_vars});
        }

        dump_member_variable_layouts(member_vars_names);
    }

    auto MemberVarsDumper::generate_files() -> void
    {
        File::StringType pdb_name = symbols.pdb_file_path.filename().stem();

        auto default_template_file = std::filesystem::path{STR("MemberVariableLayout.ini")};

        Output::send(STR("Generating file '{}'\n"), default_template_file.wstring());

        Output::Targets<Output::NewFileDevice> default_ini_dumper;
        auto& default_ini_file_device = default_ini_dumper.get_device<Output::NewFileDevice>();
        default_ini_file_device.set_file_name_and_path(member_variable_layouts_templates_output_path / default_template_file);
        default_ini_file_device.set_formatter([](File::StringViewType string) {
            return File::StringType{string};
        });

        auto template_file = std::format(STR("MemberVariableLayout_{}_Template.ini"), pdb_name);

        Output::send(STR("Generating file '{}'\n"), template_file);

        Output::Targets<Output::NewFileDevice> ini_dumper;
        auto& ini_file_device = ini_dumper.get_device<Output::NewFileDevice>();
        ini_file_device.set_file_name_and_path(member_variable_layouts_templates_output_path / template_file);
        ini_file_device.set_formatter([](File::StringViewType string) {
            return File::StringType{string};
        });

        auto pdb_name_no_underscore = pdb_name;
        pdb_name_no_underscore.replace(pdb_name_no_underscore.find(STR('_')), 1, STR(""));

        for (const auto& [class_name, class_entry] : type_container.get_class_entries())
        {
            if (class_entry.variables.empty())
            {
                continue;
            }

            auto default_setter_src_file = member_variable_layouts_gen_function_bodies_path /
                                           std::format(STR("{}_MemberVariableLayout_DefaultSetter_{}.cpp"), pdb_name, class_entry.class_name_clean);

            Output::send(STR("Generating file '{}'\n"), default_setter_src_file.wstring());

            Output::Targets<Output::NewFileDevice> default_setter_src_dumper;
            auto& default_setter_src_file_device = default_setter_src_dumper.get_device<Output::NewFileDevice>();
            default_setter_src_file_device.set_file_name_and_path(default_setter_src_file);
            default_setter_src_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            ini_dumper.send(STR("[{}]\n"), class_entry.class_name);
            default_ini_dumper.send(STR("[{}]\n"), class_entry.class_name);

            for (const auto& [variable_name, variable] : class_entry.variables)
            {
                ini_dumper.send(STR("{} = 0x{:X}\n"), variable.name, variable.offset);
                default_ini_dumper.send(STR("{} = -1\n"), variable.name);

                File::StringType final_variable_name = variable.name;

                if (variable.name == STR("EnumFlags"))
                {
                    final_variable_name = STR("EnumFlags_Internal");
                }

                default_setter_src_dumper.send(STR("if (auto it = {}::MemberOffsets.find(STR(\"{}\")); it == {}::MemberOffsets.end())\n"),
                                               class_entry.class_name,
                                               final_variable_name,
                                               class_entry.class_name);
                default_setter_src_dumper.send(STR("{\n"));
                default_setter_src_dumper.send(STR("    {}::MemberOffsets.emplace(STR(\"{}\"), 0x{:X});\n"),
                                               class_entry.class_name,
                                               final_variable_name,
                                               variable.offset);
                default_setter_src_dumper.send(STR("}\n\n"));
            }

            ini_dumper.send(STR("\n"));
            default_ini_dumper.send(STR("\n"));
        }
    }

    auto MemberVarsDumper::output_cleanup() -> void
    {
        if (std::filesystem::exists(member_variable_layouts_gen_function_bodies_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_function_bodies_path))
            {
                if (item.is_directory())
                {
                    continue;
                }
                if (item.path().extension() != STR(".hpp") && item.path().extension() != STR(".cpp"))
                {
                    continue;
                }

                File::delete_file(item.path());
            }
        }

        if (std::filesystem::exists(member_variable_layouts_gen_output_src_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_output_src_path))
            {
                if (item.is_directory())
                {
                    continue;
                }
                if (item.path().extension() != STR(".cpp"))
                {
                    continue;
                }

                File::delete_file(item.path());
            }
        }
    }
} // namespace RC::UVTD