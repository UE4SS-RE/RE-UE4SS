#define NOMINMAX

#include <filesystem>
#include <format>

#include <UVTD/VTableDumper.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/Symbols.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>

#include <Windows.h>

#include <atlbase.h>
#include <dia2.h>

namespace RC::UVTD 
{
	auto VTableDumper::dump_vtable_for_symbol(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo& name_info, EnumEntry* enum_entries, Class* class_entry) -> void
	{
		// Symbol name => Offset in vtable
		static std::unordered_map<File::StringType, uint32_t> functions_already_dumped{};

		auto symbol_name = get_symbol_name(symbol);
		//Output::send(STR("symbol_name: {}\n"), symbol_name);

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

		bool is_overload{};
		if (auto it = functions_already_dumped.find(symbol_name); it != functions_already_dumped.end())
		{
			symbol_name.append(std::format(STR("_{}"), ++it->second));
			is_overload = true;
		}

		File::StringType symbol_name_clean{ symbol_name };
		std::replace(symbol_name_clean.begin(), symbol_name_clean.end(), ':', '_');
		std::replace(symbol_name_clean.begin(), symbol_name_clean.end(), '~', '$');

		DWORD sym_tag;
		symbol->get_symTag(&sym_tag);

		auto process_struct_or_class = [&]()
		{
			if (valid_udt_names.find(symbol_name) == valid_udt_names.end()) { return; }
			functions_already_dumped.clear();
			//Output::send(STR("Dumping vtable for symbol '{}', tag: '{}'\n"), symbol_name, sym_tag_to_string(sym_tag));

			auto& local_enum_entries = type_container.get_or_create_enum_entry(symbol_name, symbol_name_clean);
			auto& local_class_entry = type_container.get_or_create_class_entry(symbol_name, symbol_name_clean, name_info);

			HRESULT hr;
			CComPtr<IDiaEnumSymbols> sub_symbols;
			if (hr = symbol->findChildren(SymTagNull, nullptr, NULL, &sub_symbols.p); hr == S_OK)
			{
				CComPtr<IDiaSymbol> sub_symbol;
				ULONG num_symbols_fetched{};
				while (sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched) == S_OK && num_symbols_fetched == 1)
				{
					SymbolNameInfo new_name_info = name_info;
					dump_vtable_for_symbol(sub_symbol, new_name_info, &local_enum_entries, &local_class_entry);
				}
				sub_symbol = nullptr;
			}
			sub_symbols = nullptr;
		};

		if (sym_tag == SymTagUDT)
		{
			process_struct_or_class();	
		}
		else if (sym_tag == SymTagBaseClass)
		{
			process_struct_or_class();
		}
		else if (sym_tag == SymTagFunction)
		{
			if (!enum_entries) { throw std::runtime_error{ "enum_entries is nullptr" }; }
			if (!class_entry) { throw std::runtime_error{ "class_entry is nullptr" }; }

			BOOL is_virtual = FALSE;
			symbol->get_virtual(&is_virtual);
			if (is_virtual == FALSE) { return; }


			HRESULT hr;
			DWORD offset_in_vtable = 0;
			if (hr = symbol->get_virtualBaseOffset(&offset_in_vtable); hr != S_OK)
			{
				throw std::runtime_error{ std::format("Call to 'get_virtualBaseOffset' failed with error '{}'", HRESULTToString(hr)) };
			}

			//Output::send(STR("Dumping virtual function for symbol '{}', tag: '{}', offset: '{}'\n"), symbol_name, sym_tag_to_string(sym_tag), offset_in_vtable / 8);

			auto& function = class_entry->functions[offset_in_vtable];
			function.name = symbol_name_clean;
			function.signature = symbols.generate_function_signature(symbol);
			function.offset = offset_in_vtable;
			function.is_overload = is_overload;
			functions_already_dumped.emplace(symbol_name, 1);
		}
	}

	auto VTableDumper::dump_vtable_for_symbol(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void
	{
		Output::send(STR("Dumping {} struct symbols for {}\n"), names.size(), symbols.pdb_file.filename().stem().wstring());

		CComPtr<IDiaEnumSymbols> dia_enum_symbols;
		for (const auto& [name, name_info] : names)
		{
			//Output::send(STR("Looking for {}\n"), name);
			HRESULT hr{};

			if (hr = symbols.dia_session->findChildren(nullptr, SymTagNull, nullptr, nsNone, &dia_enum_symbols.p); hr != S_OK)
			{
				throw std::runtime_error{ std::format("Call to 'findChildren' failed with error '{}'", HRESULTToString(hr)) };
			}

			CComPtr<IDiaSymbol> symbol;
			ULONG celt_fetched;
			if (hr = dia_enum_symbols->Next(1, &symbol.p, &celt_fetched); hr != S_OK)
			{
				throw std::runtime_error{ std::format("Ran into an error with a symbol while calling 'Next', error: {}\n", HRESULTToString(hr)) };
			}

			CComPtr<IDiaEnumSymbols> sub_symbols;
			hr = symbol->findChildren(SymTagNull, name.c_str(), NULL, &sub_symbols.p);
			if (hr != S_OK)
			{
				throw std::runtime_error{ std::format("Ran into a problem while calling 'findChildren', error: {}\n", HRESULTToString(hr)) };
			}

			CComPtr<IDiaSymbol> sub_symbol;
			ULONG num_symbols_fetched;

			sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched);
			if (name == STR("UConsole"))
			{
				LONG count;
				sub_symbols->get_Count(&count);
				//Output::send(STR("  Symbols count: {}\n"), count);
				//Output::send(STR("  sub_symbol: {}\n"), (void*)sub_symbol);
			}
			if (!sub_symbol)
			{
				//Output::send(STR("Missed symbol '{}'\n"), name);
				symbol = nullptr;
				sub_symbols = nullptr;
				dia_enum_symbols = nullptr;
				continue;
			}
			dump_vtable_for_symbol(sub_symbol, name_info);
		}
	}

	auto VTableDumper::generate_code() -> void
	{
		std::unordered_map<File::StringType, SymbolNameInfo> vtable_names;
		for (const auto& object_item : s_object_items)
		{
			if (object_item.valid_for_vtable != ValidForVTable::Yes) continue;
			
			vtable_names.emplace(object_item.name, SymbolNameInfo{ object_item.valid_for_vtable, object_item.valid_for_member_vars });
		}

		dump_vtable_for_symbol(vtable_names);
	}

	auto VTableDumper::generate_files() -> void
	{
		static std::filesystem::path vtable_gen_output_path = "GeneratedVTables";
		static std::filesystem::path vtable_gen_output_include_path = vtable_gen_output_path / "generated_include";
		static std::filesystem::path vtable_gen_output_src_path = vtable_gen_output_path / "generated_src";
		static std::filesystem::path vtable_gen_output_function_bodies_path = vtable_gen_output_include_path / "FunctionBodies";
		static std::filesystem::path vtable_templates_output_path = "VTableLayoutTemplates";
		static std::filesystem::path member_variable_layouts_gen_output_path = "GeneratedMemberVariableLayouts";
		static std::filesystem::path member_variable_layouts_gen_output_include_path = member_variable_layouts_gen_output_path / "generated_include";
		static std::filesystem::path member_variable_layouts_gen_output_src_path = member_variable_layouts_gen_output_path / "generated_src";
		static std::filesystem::path member_variable_layouts_gen_function_bodies_path = member_variable_layouts_gen_output_include_path / "FunctionBodies";
		static std::filesystem::path member_variable_layouts_templates_output_path = "MemberVarLayoutTemplates";
		static std::filesystem::path virtual_gen_output_path = "GeneratedVirtualImplementations";
		static std::filesystem::path virtual_gen_output_include_path = virtual_gen_output_path / "generated_include";
		static std::filesystem::path virtual_gen_function_bodies_path = virtual_gen_output_include_path / "FunctionBodies";

		File::StringType pdb_name = symbols.pdb_file.filename().stem();

		if (std::filesystem::exists(vtable_gen_output_include_path))
		{
			for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_include_path))
			{
				if (item.is_directory()) { continue; }
				if (item.path().extension() != STR(".hpp") && item.path().extension() != STR(".cpp")) { continue; }

				File::delete_file(item.path());
			}
		}

		if (std::filesystem::exists(vtable_gen_output_function_bodies_path))
		{
			for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_function_bodies_path))
			{
				if (item.is_directory()) { continue; }
				if (item.path().extension() != STR(".cpp")) { continue; }

				File::delete_file(item.path());
			}
		}

		if (std::filesystem::exists(vtable_gen_output_src_path))
		{
			for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_src_path))
			{
				if (item.is_directory()) { continue; }
				if (item.path().extension() != STR(".cpp")) { continue; }

				File::delete_file(item.path());
			}
		}

		for (const auto& [class_name, class_entry] : type_container.get_class_entries())
		{
			Output::send(STR("Generating file '{}_VTableOffsets_{}_FunctionBody.cpp'\n"), pdb_name, class_entry.class_name_clean);
			Output::Targets<Output::NewFileDevice> function_body_dumper;
			auto& function_body_file_device = function_body_dumper.get_device<Output::NewFileDevice>();
			function_body_file_device.set_file_name_and_path(vtable_gen_output_function_bodies_path / std::format(STR("{}_VTableOffsets_{}_FunctionBody.cpp"), pdb_name, class_name));
			function_body_file_device.set_formatter([](File::StringViewType string) {
				return File::StringType{ string };
				});

			for (const auto& [function_index, function_entry] : class_entry.functions)
			{
				auto local_class_name = class_entry.class_name;
				if (auto pos = local_class_name.find(STR("Property")); pos != local_class_name.npos)
				{
					local_class_name.replace(0, 1, STR("F"));
				}

				function_body_dumper.send(STR("if (auto it = {}::VTableLayoutMap.find(STR(\"{}\")); it == {}::VTableLayoutMap.end())\n"), local_class_name, function_entry.name, local_class_name);
				function_body_dumper.send(STR("{\n"));
				function_body_dumper.send(STR("    {}::VTableLayoutMap.emplace(STR(\"{}\"), 0x{:X});\n"), local_class_name, function_entry.name, function_entry.offset);
				function_body_dumper.send(STR("}\n\n"));
			}
		}

		auto template_file = std::format(STR("VTableLayout_{}_Template.ini"), pdb_name);
		Output::send(STR("Generating file '{}'\n"), template_file);
		Output::Targets<Output::NewFileDevice> ini_dumper;
		auto& ini_file_device = ini_dumper.get_device<Output::NewFileDevice>();
		ini_file_device.set_file_name_and_path(vtable_templates_output_path / template_file);
		ini_file_device.set_formatter([](File::StringViewType string) {
			return File::StringType{ string };
			});

		for (const auto& [class_name, class_entry] : type_container.get_class_entries())
		{
			ini_dumper.send(STR("[{}]\n"), class_entry.class_name);

			for (const auto& [function_index, function_entry] : class_entry.functions)
			{
				if (function_entry.is_overload)
				{
					ini_dumper.send(STR("; {}\n"), function_entry.signature.to_string());
				}
				ini_dumper.send(STR("{}\n"), function_entry.name);
			}

			ini_dumper.send(STR("\n"));
		}
	}
}