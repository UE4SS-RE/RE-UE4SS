#include <unordered_map>
#include <format>
#include <UVTD/MemberVarsDumper.hpp>
#include <UVTD/Helpers.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

namespace RC::UVTD
{
	auto MemberVarsDumper::generate_code() -> void
	{
		std::unordered_map<File::StringType, SymbolNameInfo> member_vars_names;

		for (ObjectItem& item : s_object_items)
		{
			if (item.valid_for_member_vars != ValidForMemberVars::Yes) continue;
			member_vars_names.emplace(item.name, SymbolNameInfo{ item.valid_for_vtable, item.valid_for_member_vars });
		}

		dump_member_variable_layouts(member_vars_names);
	}

	auto MemberVarsDumper::generate_files() -> void
	{
		static std::filesystem::path member_variable_layouts_gen_output_path = "GeneratedMemberVariableLayouts";
		static std::filesystem::path member_variable_layouts_gen_output_include_path = member_variable_layouts_gen_output_path / "generated_include";
		static std::filesystem::path member_variable_layouts_gen_output_src_path = member_variable_layouts_gen_output_path / "generated_src";
		static std::filesystem::path member_variable_layouts_gen_function_bodies_path = member_variable_layouts_gen_output_include_path / "FunctionBodies";
		static std::filesystem::path member_variable_layouts_templates_output_path = "MemberVarLayoutTemplates";

		static std::filesystem::path virtual_gen_output_path = "GeneratedVirtualImplementations";
		static std::filesystem::path virtual_gen_output_include_path = virtual_gen_output_path / "generated_include";
		static std::filesystem::path virtual_gen_function_bodies_path = virtual_gen_output_include_path / "FunctionBodies";

		File::StringType pdb_name = symbols.pdb_file_path.filename().stem();

		if (std::filesystem::exists(member_variable_layouts_gen_output_include_path))
		{
			for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_output_include_path))
			{
				if (item.is_directory()) { continue; }
				if (item.path().extension() != STR(".hpp")) { continue; }

				File::delete_file(item.path());
			}
		}

		if (std::filesystem::exists(member_variable_layouts_gen_function_bodies_path))
		{
			for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_function_bodies_path))
			{
				if (item.is_directory()) { continue; }
				if (item.path().extension() != STR(".hpp") && item.path().extension() != STR(".cpp")) { continue; }

				File::delete_file(item.path());
			}
		}

		if (std::filesystem::exists(member_variable_layouts_gen_output_src_path))
		{
			for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_output_src_path))
			{
				if (item.is_directory()) { continue; }
				if (item.path().extension() != STR(".cpp")) { continue; }

				File::delete_file(item.path());
			}
		}

		if (std::filesystem::exists(virtual_gen_output_include_path))
		{
			for (const auto& item : std::filesystem::directory_iterator(virtual_gen_output_include_path))
			{
				if (item.is_directory()) { continue; }
				if (item.path().extension() != STR(".hpp")) { continue; }

				File::delete_file(item.path());
			}
		}

		if (std::filesystem::exists(virtual_gen_function_bodies_path))
		{
			for (const auto& item : std::filesystem::directory_iterator(virtual_gen_function_bodies_path))
			{
				if (item.is_directory()) { continue; }
				if (item.path().extension() != STR(".cpp")) { continue; }

				File::delete_file(item.path());
			}
		}

		auto default_template_file = std::filesystem::path{ STR("MemberVariableLayout.ini") };
		Output::send(STR("Generating file '{}'\n"), default_template_file.wstring());
		Output::Targets<Output::NewFileDevice> default_ini_dumper;
		auto& default_ini_file_device = default_ini_dumper.get_device<Output::NewFileDevice>();
		default_ini_file_device.set_file_name_and_path(member_variable_layouts_templates_output_path / default_template_file);
		default_ini_file_device.set_formatter([](File::StringViewType string) {
			return File::StringType{ string };
			});

		auto template_file = std::format(STR("MemberVariableLayout_{}_Template.ini"), pdb_name);
		Output::send(STR("Generating file '{}'\n"), template_file);
		Output::Targets<Output::NewFileDevice> ini_dumper;
		auto& ini_file_device = ini_dumper.get_device<Output::NewFileDevice>();
		ini_file_device.set_file_name_and_path(member_variable_layouts_templates_output_path / template_file);
		ini_file_device.set_formatter([](File::StringViewType string) {
			return File::StringType{ string };
			});

		auto pdb_name_no_underscore = pdb_name;
		pdb_name_no_underscore.replace(pdb_name_no_underscore.find(STR('_')), 1, STR(""));

		auto virtual_header_file = virtual_gen_output_include_path / std::format(STR("UnrealVirtual{}.hpp"), pdb_name_no_underscore);
		Output::send(STR("Generating file '{}'\n"), virtual_header_file.wstring());
		Output::Targets<Output::NewFileDevice> virtual_header_dumper;
		auto& virtual_header_file_device = virtual_header_dumper.get_device<Output::NewFileDevice>();
		virtual_header_file_device.set_file_name_and_path(virtual_header_file);
		virtual_header_file_device.set_formatter([](File::StringViewType string) {
			return File::StringType{ string };
			});

		auto virtual_src_file = virtual_gen_function_bodies_path / std::format(STR("UnrealVirtual{}.cpp"), pdb_name_no_underscore);
		Output::send(STR("Generating file '{}'\n"), virtual_src_file.wstring());
		Output::Targets<Output::NewFileDevice> virtual_src_dumper;
		auto& virtual_src_file_device = virtual_src_dumper.get_device<Output::NewFileDevice>();
		virtual_src_file_device.set_file_name_and_path(virtual_src_file);
		virtual_src_file_device.set_formatter([](File::StringViewType string) {
			return File::StringType{ string };
			});

		bool is_case_preserving_pdb = !(CasePreservingVariants.find(pdb_name) == CasePreservingVariants.end());
		bool is_non_case_preserving_pdb = !(NonCasePreservingVariants.find(pdb_name) == NonCasePreservingVariants.end());

		if (!is_case_preserving_pdb)
		{
			virtual_header_dumper.send(STR("#ifndef RC_UNREAL_UNREAL_VIRTUAL_{}\n#define RC_UNREAL_UNREAL_VIRTUAL{}_HPP\n\n"), pdb_name_no_underscore, pdb_name_no_underscore);
			virtual_header_dumper.send(STR("#include <Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtualBaseVC.hpp>\n\n"));
			virtual_header_dumper.send(STR("namespace RC::Unreal\n"));
			virtual_header_dumper.send(STR("{\n"));
			virtual_header_dumper.send(STR("    class UnrealVirtual{} : public UnrealVirtualBaseVC\n"), pdb_name_no_underscore);
			virtual_header_dumper.send(STR("    {\n"));
			virtual_header_dumper.send(STR("        void set_virtual_offsets() override;\n"));
			virtual_header_dumper.send(STR("    }\n"));
			virtual_header_dumper.send(STR("}\n\n\n"));
			virtual_header_dumper.send(STR("#endif // RC_UNREAL_UNREAL_VIRTUAL{}_HPP\n"), pdb_name_no_underscore);

			virtual_src_dumper.send(STR("#include <Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtual{}.hpp>\n\n"), pdb_name_no_underscore);
			virtual_src_dumper.send(STR("// These are all the structs that have virtuals that need to have their offset set\n"));
			virtual_src_dumper.send(STR("#include <Unreal/UObject.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/UScriptStruct.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/FOutputDevice.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/FField.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/FProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FNumericProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FObjectProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FMulticastDelegateProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FStructProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FArrayProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FMapProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FBoolProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/NumericPropertyTypes.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FSetProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FInterfaceProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FClassProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FSoftClassProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FEnumProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/Property/FFieldPathProperty.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/UFunction.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/UClass.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/World.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/UEnum.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/FArchive.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/AGameModeBase.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/AGameMode.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/UPlayer.hpp>\n"));
			virtual_src_dumper.send(STR("#include <Unreal/ULocalPlayer.hpp>\n"));
			//virtual_src_dumper.send(STR("#include <Unreal/UConsole.hpp>\n"));
			virtual_src_dumper.send(STR("\n"));
			virtual_src_dumper.send(STR("namespace RC::Unreal\n"));
			virtual_src_dumper.send(STR("{\n"));
			virtual_src_dumper.send(STR("    void UnrealVirtual{}::set_virtual_offsets()\n"), pdb_name_no_underscore);
			virtual_src_dumper.send(STR("    {\n"));
		}

		for (const auto& [class_name, class_entry] : type_container.get_class_entries())
		{
			if (!class_entry.functions.empty() && class_entry.valid_for_vtable == ValidForVTable::Yes && !is_case_preserving_pdb)
			{
				virtual_src_dumper.send(STR("#include <FunctionBodies/{}_VTableOffsets_{}_FunctionBody.cpp>\n"), pdb_name, class_name);
			}

			if (class_entry.variables.empty()) { continue; }

			auto default_setter_src_file = member_variable_layouts_gen_function_bodies_path / std::format(STR("{}_MemberVariableLayout_DefaultSetter_{}.cpp"), pdb_name, class_entry.class_name_clean);
			Output::send(STR("Generating file '{}'\n"), default_setter_src_file.wstring());
			Output::Targets<Output::NewFileDevice> default_setter_src_dumper;
			auto& default_setter_src_file_device = default_setter_src_dumper.get_device<Output::NewFileDevice>();
			default_setter_src_file_device.set_file_name_and_path(default_setter_src_file);
			default_setter_src_file_device.set_formatter([](File::StringViewType string) {
				return File::StringType{ string };
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

				default_setter_src_dumper.send(STR("if (auto it = {}::MemberOffsets.find(STR(\"{}\")); it == {}::MemberOffsets.end())\n"), class_entry.class_name, final_variable_name, class_entry.class_name);
				default_setter_src_dumper.send(STR("{\n"));
				default_setter_src_dumper.send(STR("    {}::MemberOffsets.emplace(STR(\"{}\"), 0x{:X});\n"), class_entry.class_name, final_variable_name, variable.offset);
				default_setter_src_dumper.send(STR("}\n\n"));
			}

			ini_dumper.send(STR("\n"));
			default_ini_dumper.send(STR("\n"));
		}

		if (!is_case_preserving_pdb)
		{
			virtual_src_dumper.send(STR("\n"));

			// Second & third passes just to separate VTable includes and MemberOffsets includes.
			if (is_non_case_preserving_pdb)
			{
				virtual_src_dumper.send(STR("#ifdef WITH_CASE_PRESERVING_NAME\n"));
				for (const auto& [class_name, class_entry] : type_container.get_class_entries())
				{
					if (class_entry.variables.empty()) { continue; }

					if (class_entry.valid_for_member_vars == ValidForMemberVars::Yes)
					{
						virtual_src_dumper.send(STR("#include <FunctionBodies/{}_CasePreserving_MemberVariableLayout_DefaultSetter_{}.cpp>\n"), pdb_name, class_name);
					}
				}
				virtual_src_dumper.send(STR("#else\n"));
			}

			for (const auto& [class_name, class_entry] : type_container.get_class_entries())
			{
				if (class_entry.variables.empty()) { continue; }

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

		auto macro_setter_file = std::filesystem::path{ STR("MacroSetter.hpp") };
		Output::send(STR("Generating file '{}'\n"), macro_setter_file.wstring());
		Output::Targets<Output::NewFileDevice> macro_setter_dumper;
		auto& macro_setter_file_device = macro_setter_dumper.get_device<Output::NewFileDevice>();
		macro_setter_file_device.set_file_name_and_path(macro_setter_file);
		macro_setter_file_device.set_formatter([](File::StringViewType string) {
			return File::StringType{ string };
			});

		for (const auto& [class_name, enum_entry] : type_container.get_enum_entries())
		{
			if (enum_entry.variables.empty()) { continue; }

			auto wrapper_header_file = member_variable_layouts_gen_output_include_path / std::format(STR("MemberVariableLayout_HeaderWrapper_{}.hpp"), enum_entry.name_clean);
			Output::send(STR("Generating file '{}'\n"), wrapper_header_file.wstring());
			Output::Targets<Output::NewFileDevice> header_wrapper_dumper;
			auto& wrapper_header_file_device = header_wrapper_dumper.get_device<Output::NewFileDevice>();
			wrapper_header_file_device.set_file_name_and_path(wrapper_header_file);
			wrapper_header_file_device.set_formatter([](File::StringViewType string) {
				return File::StringType{ string };
				});

			auto wrapper_src_file = member_variable_layouts_gen_output_include_path / std::format(STR("MemberVariableLayout_SrcWrapper_{}.hpp"), enum_entry.name_clean);
			Output::send(STR("Generating file '{}'\n"), wrapper_src_file.wstring());
			Output::Targets<Output::NewFileDevice> wrapper_src_dumper;
			auto& wrapper_src_file_device = wrapper_src_dumper.get_device<Output::NewFileDevice>();
			wrapper_src_file_device.set_file_name_and_path(wrapper_src_file);
			wrapper_src_file_device.set_formatter([](File::StringViewType string) {
				return File::StringType{ string };
				});

			header_wrapper_dumper.send(STR("static std::unordered_map<File::StringType, int32_t> MemberOffsets;\n\n"));
			wrapper_src_dumper.send(STR("std::unordered_map<File::StringType, int32_t> {}::MemberOffsets{{}};\n\n"), enum_entry.name);

			auto private_variables_for_class = s_private_variables.find(enum_entry.name);

			for (const auto& [variable_name, variable] : enum_entry.variables)
			{
				if (variable.type.find(STR("TBaseDelegate")) != variable.type.npos) { continue; }
				if (variable.type.find(STR("FUniqueNetIdRepl")) != variable.type.npos) { continue; }
				if (variable.type.find(STR("FPlatformUserId")) != variable.type.npos) { continue; }
				if (variable.type.find(STR("FVector2D")) != variable.type.npos) { continue; }
				if (variable.type.find(STR("FReply")) != variable.type.npos) { continue; }

				bool is_private{ private_variables_for_class != s_private_variables.end() && private_variables_for_class->second.find(variable.name) != private_variables_for_class->second.end() };

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
				wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), variable.type, enum_entry.name, final_variable_name);
				if (enum_entry.name == STR("FArchive") || enum_entry.name == STR("FArchiveState"))
				{
					wrapper_src_dumper.send(STR("{\n"));
					wrapper_src_dumper.send(STR("    static auto& offsets = Version::IsBelow(4, 25) ? FArchive::MemberOffsets : FArchiveState::MemberOffsets;\n"));
					wrapper_src_dumper.send(STR("    static auto offset = offsets.find(STR(\"{}\"));\n"), final_variable_name);
					wrapper_src_dumper.send(STR("    if (offset == offsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}}; }}\n"), enum_entry.name, final_variable_name);
					wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset->second);\n"), variable.type);
					wrapper_src_dumper.send(STR("}\n"));
					wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), variable.type, enum_entry.name, final_variable_name);
					wrapper_src_dumper.send(STR("{\n"));
					wrapper_src_dumper.send(STR("    static auto& offsets = Version::IsBelow(4, 25) ? FArchive::MemberOffsets : FArchiveState::MemberOffsets;\n"));
					wrapper_src_dumper.send(STR("    static auto offset = offsets.find(STR(\"{}\"));\n"), final_variable_name);
					wrapper_src_dumper.send(STR("    if (offset == offsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}}; }}\n"), enum_entry.name, final_variable_name);
					wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset->second);\n"), variable.type);
					wrapper_src_dumper.send(STR("}\n\n"));
				}
				else
				{
					wrapper_src_dumper.send(STR("{\n"));
					wrapper_src_dumper.send(STR("    static auto offset = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
					wrapper_src_dumper.send(STR("    if (offset == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}}; }}\n"), enum_entry.name, final_variable_name);
					wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset->second);\n"), variable.type);
					wrapper_src_dumper.send(STR("}\n"));
					wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), variable.type, enum_entry.name, final_variable_name);
					wrapper_src_dumper.send(STR("{\n"));
					wrapper_src_dumper.send(STR("    static auto offset = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
					wrapper_src_dumper.send(STR("    if (offset == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}}; }}\n"), enum_entry.name, final_variable_name);
					wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset->second);\n"), variable.type);
					wrapper_src_dumper.send(STR("}\n\n"));
				}

				macro_setter_dumper.send(STR("if (auto val = parser.get_int64(STR(\"{}\"), STR(\"{}\"), -1); val != -1)\n"), enum_entry.name, final_variable_name);
				macro_setter_dumper.send(STR("    Unreal::{}::MemberOffsets.emplace(STR(\"{}\"), static_cast<int32_t>(val));\n"), enum_entry.name, final_variable_name);
			}
		}
	}

	auto MemberVarsDumper::dump_member_variable_layout(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo& name_info, EnumEntriesTypeAlias enum_entry, Class* class_entry) -> void
	{
		auto symbol_name = get_symbol_name(symbol);

		bool could_not_replace_prefix{};
		for (const auto& UPrefixed : UPrefixToFPrefix)
		{
			for (size_t i = symbol_name.find(UPrefixed); i != symbol_name.npos; i = symbol_name.find(UPrefixed))
			{
				if (symbols.is_425_plus)
				{
					could_not_replace_prefix = true;
					break;
				}
				symbol_name.replace(i, 1, STR("F"));
				++i;
			}
		}

		if (could_not_replace_prefix)
		{
			return;
		}

		File::StringType symbol_name_clean{ symbol_name };
		std::replace(symbol_name_clean.begin(), symbol_name_clean.end(), ':', '_');
		std::replace(symbol_name_clean.begin(), symbol_name_clean.end(), '~', '$');

		DWORD sym_tag;
		symbol->get_symTag(&sym_tag);

		if (symbol_name == STR("Names"))
		{
			Output::send(STR("tag for Names: {}\n"), sym_tag_to_string(sym_tag));
		}

		HRESULT hr;
		if (sym_tag == SymTagUDT)
		{
			if (valid_udt_names.find(symbol_name) == valid_udt_names.end()) { return; }
			auto& local_class_entry = type_container.get_or_create_class_entry(symbol_name, symbol_name_clean, name_info);
			auto& local_enum_entry = type_container.get_or_create_enum_entry(symbol_name, symbol_name_clean);

			Output::send(STR("UDT symbol name: {}, enum_entry name: {}, enum_entry name clean: {}\n"), symbol_name, local_enum_entry.name, local_enum_entry.name_clean);

			CComPtr<IDiaEnumSymbols> sub_symbols;
			if (hr = symbol->findChildren(SymTagNull, nullptr, NULL, &sub_symbols.p); hr == S_OK)
			{
				CComPtr<IDiaSymbol> sub_symbol;
				ULONG num_symbols_fetched{};
				while (sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched) == S_OK && num_symbols_fetched == 1)
				{
					SymbolNameInfo new_name_info = name_info;
					dump_member_variable_layout(sub_symbol, new_name_info, &local_enum_entry, &local_class_entry);
				}
				sub_symbol = nullptr;
			}
			sub_symbols = nullptr;
		}
		else if (sym_tag == SymTagData)
		{
			if (!enum_entry) { throw std::runtime_error{ "enum_entries is nullptr" }; }
			if (!class_entry) { throw std::runtime_error{ "class_entry is nullptr" }; }

			DWORD kind;
			symbol->get_dataKind(&kind);
			if (kind != DataKind::DataIsMember) { return; }

			CComPtr<IDiaSymbol> type;
			if (hr = symbol->get_type(&type); hr != S_OK) { throw std::runtime_error{ "Could not get type\n" }; }

			DWORD type_tag;
			type->get_symTag(&type_tag);

			auto type_name = get_type_name(type);
			if (!is_valid_type_to_dump(type_name)) { return; }

			for (const auto& UPrefixed : UPrefixToFPrefix)
			{
				for (size_t i = type_name.find(UPrefixed); i != type_name.npos; i = type_name.find(UPrefixed))
				{
					if (symbols.is_425_plus)
					{
						could_not_replace_prefix = true;
						break;
					}
					type_name.replace(i, 1, STR("F"));
					++i;
				}
			}

			if (could_not_replace_prefix)
			{
				return;
			}

			LONG offset;
			symbol->get_offset(&offset);

			auto& enum_entry_variable = enum_entry->variables[symbol_name];
			enum_entry_variable.type = type_name;
			enum_entry_variable.name = symbol_name;
			enum_entry_variable.offset = offset;

			Output::send(STR("{} {} ({}, {}); 0x{:X}\n"), type_name, symbol_name, sym_tag_to_string(type_tag), kind_to_string(kind), offset);
			auto& class_entry_variable = class_entry->variables[symbol_name];
			class_entry_variable.type = type_name;
			class_entry_variable.name = symbol_name;
			class_entry_variable.offset = offset;
		}
	}

	auto MemberVarsDumper::dump_member_variable_layouts(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void
	{
		Output::send(STR("Dumping {} symbols for {}\n"), names.size(), symbols.pdb_file_path.filename().stem().wstring());

		CComPtr<IDiaEnumSymbols> dia_enum_symbols;
		for (const auto& [name, name_info] : names)
		{
// 			Output::send(STR("{}...\n"), name);
// 			HRESULT hr{};
// 			if (hr = symbols.dia_session->findChildren(nullptr, SymTagNull, nullptr, nsNone, &dia_enum_symbols.p); hr != S_OK)
// 			{
// 				throw std::runtime_error{ std::format("Call to 'findChildren' failed with error '{}'", HRESULTToString(hr)) };
// 			}
// 
// 			CComPtr<IDiaSymbol> symbol;
// 			ULONG celt_fetched;
// 			if (hr = dia_enum_symbols->Next(1, &symbol.p, &celt_fetched); hr != S_OK)
// 			{
// 				throw std::runtime_error{ std::format("Ran into an error with a symbol while calling 'Next', error: {}\n", HRESULTToString(hr)) };
// 			}
// 
// 			CComPtr<IDiaEnumSymbols> sub_symbols;
// 			hr = symbol->findChildren(SymTagNull, name.c_str(), NULL, &sub_symbols.p);
// 			if (hr != S_OK)
// 			{
// 				throw std::runtime_error{ std::format("Ran into a problem while calling 'findChildren', error: {}\n", HRESULTToString(hr)) };
// 			}
// 
// 			CComPtr<IDiaSymbol> sub_symbol;
// 			ULONG num_symbols_fetched;
// 
// 			sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched);
// 			if (!sub_symbol)
// 			{
// 				//Output::send(STR("Missed symbol '{}'\n"), name);
// 				symbol = nullptr;
// 				sub_symbols = nullptr;
// 				dia_enum_symbols = nullptr;
// 				continue;
// 			}
// 
// 			dump_member_variable_layout(sub_symbol, name_info);
		}
	}
}