#include <UVTD/UnrealVirtualGenerator.hpp>

#include <DynamicOutput/DynamicOutput.hpp>
#include <UVTD/Helpers.hpp>

namespace RC::UVTD
{
    auto UnrealVirtualGenerator::generate_files() -> void
    {
        auto pdb_name_no_underscore = pdb_name;
        pdb_name_no_underscore.replace(pdb_name_no_underscore.find(STR('_')), 1, STR(""));

        auto virtual_header_file = virtual_gen_output_include_path / std::format(SYSSTR("UnrealVirtual{}.hpp"), pdb_name_no_underscore);

        Output::send(SYSSTR("Generating file '{}'\n"), virtual_header_file.wstring());

        Output::Targets<Output::NewFileDevice> virtual_header_dumper;
        auto& virtual_header_file_device = virtual_header_dumper.get_device<Output::NewFileDevice>();
        virtual_header_file_device.set_file_name_and_path(virtual_header_file);
        virtual_header_file_device.set_formatter([](SystemStringViewType string) {
            return SystemStringType{string};
        });

        auto virtual_src_file = virtual_gen_function_bodies_path / std::format(SYSSTR("UnrealVirtual{}.cpp"), pdb_name_no_underscore);

        Output::send(SYSSTR("Generating file '{}'\n"), virtual_src_file);

        Output::Targets<Output::NewFileDevice> virtual_src_dumper;
        auto& virtual_src_file_device = virtual_src_dumper.get_device<Output::NewFileDevice>();
        virtual_src_file_device.set_file_name_and_path(virtual_src_file);
        virtual_src_file_device.set_formatter([](SystemStringViewType string) {
            return SystemStringType{string};
        });

        bool is_case_preserving_pdb = !(s_case_preserving_variants.find(pdb_name) == s_case_preserving_variants.end());
        bool is_non_case_preserving_pdb = !(s_non_case_preserving_variants.find(pdb_name) == s_non_case_preserving_variants.end());

        if (!is_case_preserving_pdb)
        {
            virtual_header_dumper.send(SYSSTR("#ifndef RC_UNREAL_UNREAL_VIRTUAL{}_HPP\n"), pdb_name_no_underscore);
            virtual_header_dumper.send(SYSSTR("#define RC_UNREAL_UNREAL_VIRTUAL{}_HPP\n"), pdb_name_no_underscore);
            virtual_header_dumper.send(SYSSTR("#include <Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtualBaseVC.hpp>\n\n"));
            virtual_header_dumper.send(SYSSTR("namespace RC::Unreal\n"));
            virtual_header_dumper.send(SYSSTR("{\n"));
            virtual_header_dumper.send(SYSSTR("    class UnrealVirtual{} : public UnrealVirtualBaseVC\n"), pdb_name_no_underscore);
            virtual_header_dumper.send(SYSSTR("    {\n"));
            virtual_header_dumper.send(SYSSTR("        auto set_virtual_offsets() -> void override;\n"));
            virtual_header_dumper.send(SYSSTR("    };\n"));
            virtual_header_dumper.send(SYSSTR("}\n\n\n"));
            virtual_header_dumper.send(SYSSTR("#endif //RC_UNREAL_UNREAL_VIRTUAL{}_HPP\n"), pdb_name_no_underscore);

            virtual_src_dumper.send(SYSSTR("#include <Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtual{}.hpp>\n\n"), pdb_name_no_underscore);
            virtual_src_dumper.send(SYSSTR("#include <functional>\n\n"));
            virtual_src_dumper.send(SYSSTR("// These are all the structs that have virtuals that need to have their offset set\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/UObject.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/UEngine.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/UScriptStruct.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/FOutputDevice.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/FField.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/FProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FNumericProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FObjectProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FMulticastDelegateProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FStructProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FArrayProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FMapProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FBoolProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/NumericPropertyTypes.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FSetProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FInterfaceProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FClassProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FSoftClassProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FEnumProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/Property/FFieldPathProperty.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/UFunction.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/UClass.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/World.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/UEnum.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/UEngine.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/FWorldContext.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/FArchive.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/AGameModeBase.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/AGameMode.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/UPlayer.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/ULocalPlayer.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("#include <Unreal/ITextData.hpp>\n"));
            // virtual_src_dumper.send(SYSSTR("#include <Unreal/UConsole.hpp>\n"));
            virtual_src_dumper.send(SYSSTR("\n"));
            virtual_src_dumper.send(SYSSTR("namespace RC::Unreal\n"));
            virtual_src_dumper.send(SYSSTR("{\n"));
            virtual_src_dumper.send(SYSSTR("    void UnrealVirtual{}::set_virtual_offsets()\n"), pdb_name_no_underscore);
            virtual_src_dumper.send(SYSSTR("    {\n"));
        }

        for (const auto& [class_name, class_entry] : type_container.get_class_entries())
        {
            if (!class_entry.functions.empty() && class_entry.valid_for_vtable == ValidForVTable::Yes && !is_case_preserving_pdb)
            {
                virtual_src_dumper.send(SYSSTR("#include <FunctionBodies/{}_VTableOffsets_{}_FunctionBody.cpp>\n"), pdb_name, class_name);
            }
        }

        if (!is_case_preserving_pdb)
        {
            virtual_src_dumper.send(SYSSTR("\n"));

            // Second & third passes just to separate VTable includes and MemberOffsets includes.
            if (is_non_case_preserving_pdb)
            {
                virtual_src_dumper.send(SYSSTR("#ifdef WITH_CASE_PRESERVING_NAME\n"));
                for (const auto& [class_name, class_entry] : type_container.get_class_entries())
                {
                    if (class_entry.variables.empty())
                    {
                        continue;
                    }

                    if (class_entry.valid_for_member_vars == ValidForMemberVars::Yes)
                    {
                        virtual_src_dumper.send(SYSSTR("#include <FunctionBodies/{}_CasePreserving_MemberVariableLayout_DefaultSetter_{}.cpp>\n"),
                                                pdb_name,
                                                class_name);
                    }
                }
                virtual_src_dumper.send(STR("#else\n"));
            }

            for (const auto& [class_name, class_entry] : type_container.get_class_entries())
            {
                if (class_entry.variables.empty())
                {
                    continue;
                }

                if (class_entry.valid_for_member_vars == ValidForMemberVars::Yes)
                {
                    virtual_src_dumper.send(STR("#include <FunctionBodies/{}_MemberVariableLayout_DefaultSetter_{}.cpp>\n"), pdb_name, class_name);
                }
            }

            if (is_non_case_preserving_pdb)
            {
                virtual_src_dumper.send(STR("#endif\n"));
            }

            virtual_src_dumper.send(STR("    }\n"));
            virtual_src_dumper.send(STR("}\n"));
        }
    }

    auto UnrealVirtualGenerator::output_cleanup() -> void
    {
        if (std::filesystem::exists(virtual_gen_output_include_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(virtual_gen_output_include_path))
            {
                if (item.is_directory())
                {
                    continue;
                }
                if (item.path().extension() != STR(".hpp"))
                {
                    continue;
                }

                File::delete_file(item.path());
            }
        }

        if (std::filesystem::exists(virtual_gen_function_bodies_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(virtual_gen_function_bodies_path))
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
