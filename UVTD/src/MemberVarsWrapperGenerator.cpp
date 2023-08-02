#include <format>

#include <DynamicOutput/DynamicOutput.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/MemberVarsWrapperGenerator.hpp>

namespace RC::UVTD
{
    auto MemberVarsWrapperGenerator::generate_files() -> void
    {
        auto macro_setter_file = std::filesystem::path{STR("MacroSetter.hpp")};

        Output::send(STR("Generating file '{}'\n"), macro_setter_file.wstring());

        Output::Targets<Output::NewFileDevice> macro_setter_dumper;
        auto& macro_setter_file_device = macro_setter_dumper.get_device<Output::NewFileDevice>();
        macro_setter_file_device.set_file_name_and_path(macro_setter_file);
        macro_setter_file_device.set_formatter([](File::StringViewType string) {
            return File::StringType{string};
        });

        for (const auto& [class_name, class_entry] : type_container.get_class_entries())
        {
            if (class_entry.variables.empty())
            {
                continue;
            }

            auto wrapper_header_file = member_variable_layouts_gen_output_include_path /
                                       std::format(STR("MemberVariableLayout_HeaderWrapper_{}.hpp"), class_entry.class_name_clean);

            Output::send(STR("Generating file '{}'\n"), wrapper_header_file.wstring());

            Output::Targets<Output::NewFileDevice> header_wrapper_dumper;
            auto& wrapper_header_file_device = header_wrapper_dumper.get_device<Output::NewFileDevice>();
            wrapper_header_file_device.set_file_name_and_path(wrapper_header_file);
            wrapper_header_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            auto wrapper_src_file =
                    member_variable_layouts_gen_output_include_path / std::format(STR("MemberVariableLayout_SrcWrapper_{}.hpp"), class_entry.class_name_clean);

            Output::send(STR("Generating file '{}'\n"), wrapper_src_file.wstring());

            Output::Targets<Output::NewFileDevice> wrapper_src_dumper;
            auto& wrapper_src_file_device = wrapper_src_dumper.get_device<Output::NewFileDevice>();
            wrapper_src_file_device.set_file_name_and_path(wrapper_src_file);
            wrapper_src_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            header_wrapper_dumper.send(STR("static std::unordered_map<File::StringType, int32_t> MemberOffsets;\n\n"));
            wrapper_src_dumper.send(STR("std::unordered_map<File::StringType, int32_t> {}::MemberOffsets{{}};\n\n"), class_entry.class_name);

            auto private_variables_for_class = s_private_variables.find(class_entry.class_name);

            for (const auto& [variable_name, variable] : class_entry.variables)
            {
                if (variable.type.find(STR("TBaseDelegate")) != variable.type.npos)
                {
                    continue;
                }
                if (variable.type.find(STR("FUniqueNetIdRepl")) != variable.type.npos)
                {
                    continue;
                }
                if (variable.type.find(STR("FPlatformUserId")) != variable.type.npos)
                {
                    continue;
                }
                if (variable.type.find(STR("FVector2D")) != variable.type.npos)
                {
                    continue;
                }
                if (variable.type.find(STR("FReply")) != variable.type.npos)
                {
                    continue;
                }

                bool is_private{private_variables_for_class != s_private_variables.end() &&
                                private_variables_for_class->second.find(variable.name) != private_variables_for_class->second.end()};

                File::StringType final_variable_name = variable.name;
                File::StringType final_type_name = variable.type;

                if (variable.name == STR("EnumFlags"))
                {
                    final_variable_name = STR("EnumFlags_Internal");
                    is_private = true;
                }

                if (is_private)
                {
                    header_wrapper_dumper.send(STR("private:\n"));
                }
                else
                {
                    header_wrapper_dumper.send(STR("public:\n"));
                }

                header_wrapper_dumper.send(STR("    {}& Get{}();\n"), variable.type, final_variable_name);
                header_wrapper_dumper.send(STR("    const {}& Get{}() const;\n\n"), variable.type, final_variable_name);
                wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), variable.type, class_entry.class_name, final_variable_name);
                if (class_entry.class_name == STR("FArchive") || class_entry.class_name == STR("FArchiveState"))
                {
                    wrapper_src_dumper.send(STR("{\n"));
                    wrapper_src_dumper.send(
                            STR("    static auto& offsets = Version::IsBelow(4, 25) ? FArchive::MemberOffsets : FArchiveState::MemberOffsets;\n"));
                    wrapper_src_dumper.send(STR("    static auto offset = offsets.find(STR(\"{}\"));\n"), final_variable_name);
                    wrapper_src_dumper.send(STR("    if (offset == offsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that "
                                                "doesn't exist in this engine version.\"}}; }}\n"),
                                            class_entry.class_name,
                                            final_variable_name);
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset->second);\n"), variable.type);
                    wrapper_src_dumper.send(STR("}\n"));
                    wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), variable.type, class_entry.class_name, final_variable_name);
                    wrapper_src_dumper.send(STR("{\n"));
                    wrapper_src_dumper.send(
                            STR("    static auto& offsets = Version::IsBelow(4, 25) ? FArchive::MemberOffsets : FArchiveState::MemberOffsets;\n"));
                    wrapper_src_dumper.send(STR("    static auto offset = offsets.find(STR(\"{}\"));\n"), final_variable_name);
                    wrapper_src_dumper.send(STR("    if (offset == offsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that "
                                                "doesn't exist in this engine version.\"}}; }}\n"),
                                            class_entry.class_name,
                                            final_variable_name);
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset->second);\n"), variable.type);
                    wrapper_src_dumper.send(STR("}\n\n"));
                }
                else
                {
                    wrapper_src_dumper.send(STR("{\n"));
                    wrapper_src_dumper.send(STR("    static auto offset = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
                    wrapper_src_dumper.send(STR("    if (offset == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' "
                                                "that doesn't exist in this engine version.\"}}; }}\n"),
                                            class_entry.class_name,
                                            final_variable_name);
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset->second);\n"), variable.type);
                    wrapper_src_dumper.send(STR("}\n"));
                    wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), variable.type, class_entry.class_name, final_variable_name);
                    wrapper_src_dumper.send(STR("{\n"));
                    wrapper_src_dumper.send(STR("    static auto offset = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
                    wrapper_src_dumper.send(STR("    if (offset == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' "
                                                "that doesn't exist in this engine version.\"}}; }}\n"),
                                            class_entry.class_name,
                                            final_variable_name);
                    wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset->second);\n"), variable.type);
                    wrapper_src_dumper.send(STR("}\n\n"));
                }

                macro_setter_dumper.send(STR("if (auto val = parser.get_int64(STR(\"{}\"), STR(\"{}\"), -1); val != -1)\n"),
                                         class_entry.class_name,
                                         final_variable_name);
                macro_setter_dumper.send(STR("    Unreal::{}::MemberOffsets.emplace(STR(\"{}\"), static_cast<int32_t>(val));\n"),
                                         class_entry.class_name,
                                         final_variable_name);
            }
        }
    }

    auto MemberVarsWrapperGenerator::output_cleanup() -> void
    {
        if (std::filesystem::exists(member_variable_layouts_gen_output_include_path))
        {
            for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_output_include_path))
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
    }
} // namespace RC::UVTD