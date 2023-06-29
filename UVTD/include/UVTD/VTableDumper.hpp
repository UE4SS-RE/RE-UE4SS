#ifndef UNREALVTABLEDUMPER_VTABLEDUMPER_HPP
#define UNREALVTABLEDUMPER_VTABLEDUMPER_HPP

#include <unordered_map>
#include <map>

#include <UVTD/Symbols.hpp>
#include <File/File.hpp>

#include <atlbase.h>
#include <dia2.h>

namespace RC::UVTD
{
	class VTableDumper
	{
	private:
		struct MemberVariable
		{
			File::StringType type;
			File::StringType name;
			int32_t offset;
		};

		struct EnumEntry
		{
			File::StringType name;
			File::StringType name_clean;
			std::map<File::StringType, MemberVariable> variables;
		};

		struct FunctionBody
		{
			File::StringType name;
			File::StringType signature;
			uint32_t offset;
			bool is_overload;
		};

		struct Class
		{
			File::StringType class_name;
			File::StringType class_name_clean;
			std::map<uint32_t, FunctionBody> functions;
			// Key: Variable name
			std::map<File::StringType, MemberVariable> variables;
			uint32_t last_virtual_offset;
			ValidForVTable valid_for_vtable{ ValidForVTable::No };
			ValidForMemberVars valid_for_member_vars{ ValidForMemberVars::No };
		};

	private:
		std::filesystem::path pdb_file;
		CComPtr<IDiaDataSource> dia_source;
		CComPtr<IDiaSession> dia_session;
		CComPtr<IDiaSymbol> dia_global_symbol;
		CComPtr<IDiaEnumSymbols> dia_global_symbols_enum;
		bool is_425_plus;

		std::unordered_map<File::StringType, EnumEntry> enum_entries;
		std::unordered_map<File::StringType, Class> class_entries;

		bool are_symbols_cached{};

	public:
		VTableDumper() = delete;
		explicit VTableDumper(std::filesystem::path pdb_file) : pdb_file(std::move(pdb_file))
		{
			auto version_string = this->pdb_file.filename().stem().string();
			auto major_version = std::atoi(version_string.substr(0, 1).c_str());
			auto minor_version = std::atoi(version_string.substr(2).c_str());
			is_425_plus = (major_version > 4) || (major_version == 4 && minor_version >= 25);
		}

	public:
		auto setup_symbol_loader() -> void;
		using EnumEntriesTypeAlias = struct EnumEntry*;
		auto dump_vtable_for_symbol(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo&, EnumEntriesTypeAlias = nullptr, struct Class* class_entry = nullptr) -> void;
		auto dump_vtable_for_symbol(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void;
		auto dump_member_variable_layouts(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo&, EnumEntriesTypeAlias = nullptr, struct Class* class_entry = nullptr) -> void;
		auto dump_member_variable_layouts(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void;
		auto generate_code(DumpMode) -> void;
		auto generate_files(DumpMode) -> void;
		auto experimental_generate_members() -> void;

	private:
		auto get_or_create_enum_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean) -> EnumEntry&;
		auto get_or_create_class_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean, const SymbolNameInfo& name_info) -> Class&;

	private:
		auto static generate_const_qualifier(CComPtr<IDiaSymbol>& symbol) -> File::StringType;
		auto static generate_type(CComPtr<IDiaSymbol>& symbol) -> File::StringType;
		auto static generate_function_params(CComPtr<IDiaSymbol>& symbol) -> File::StringType;
		auto static generate_function_signature(CComPtr<IDiaSymbol>& symbol) -> File::StringType;
	};
}

#endif