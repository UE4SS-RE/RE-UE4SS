#ifndef UNREALVTABLEDUMPER_SYMBOLS_HPP
#define UNREALVTABLEDUMPER_SYMBOLS_HPP

#include <unordered_map>
#include <map>
#include <vector>
#include <format>

#include <File/File.hpp>

#include <atlbase.h>
#include <dia2.h>

namespace RC::UVTD
{
	enum class DumpMode { VTable, MemberVars, SolBindings };

	enum class ValidForVTable { Yes, No };
	enum class ValidForMemberVars { Yes, No };

	struct SymbolNameInfo
	{
		ValidForVTable valid_for_vtable{};
		ValidForMemberVars valid_for_member_vars{};

		explicit SymbolNameInfo(ValidForVTable valid_for_vtable, ValidForMemberVars valid_for_member_vars) :
			valid_for_vtable(valid_for_vtable),
			valid_for_member_vars(valid_for_member_vars)
		{
		}
	};

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

	struct FunctionParam
	{
		File::StringType type;
		File::StringType name;

		auto to_string() const -> File::StringType
		{
			return std::format(STR("{} {}"), type, name);
		}
	};

	struct FunctionSignature
	{
		File::StringType return_type;
		File::StringType name;
		std::vector<FunctionParam> params;
		bool const_qualifier;

		auto to_string() const -> File::StringType
		{
			File::StringType params_string{};

			for (size_t i = 0; i < params.size(); i++)
			{
				bool should_add_comma = i < params.size() - 1;
				params_string.append(std::format(STR("{}{}"), params[i].to_string(), should_add_comma ? STR(", ") : STR("")));
			}

			return std::format(STR("{} {}({}){};"), return_type, name, params_string, const_qualifier ? STR("const") : STR(""));
		}
	};

	struct FunctionBody
	{
		File::StringType name;
		FunctionSignature signature;
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

	class Symbols {
	public:
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

	public:
		std::filesystem::path pdb_file;
		bool is_425_plus;

		std::unordered_map<File::StringType, EnumEntry> enum_entries;
		std::unordered_map<File::StringType, Class> class_entries;

		CComPtr<IDiaDataSource> dia_source;
		CComPtr<IDiaSession> dia_session;
		CComPtr<IDiaSymbol> dia_global_symbol;

	public:
		Symbols() = delete;

		explicit Symbols(std::filesystem::path pdb_file) : pdb_file(std::move(pdb_file))
		{
			auto version_string = this->pdb_file.filename().stem().string();
			auto major_version = std::atoi(version_string.substr(0, 1).c_str());
			auto minor_version = std::atoi(version_string.substr(2).c_str());
			is_425_plus = (major_version > 4) || (major_version == 4 && minor_version >= 25);

			setup_symbol_loader();
		}

	public:
		auto get_or_create_enum_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean) -> EnumEntry&;
		auto get_or_create_class_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean, const SymbolNameInfo& name_info) -> Class&;

		auto generate_const_qualifier(CComPtr<IDiaSymbol>& symbol) -> bool;
		auto generate_type(CComPtr<IDiaSymbol>& symbol) -> File::StringType;

		auto generate_function_params(CComPtr<IDiaSymbol>& symbol) -> std::vector<FunctionParam>;
		auto generate_function_signature(CComPtr<IDiaSymbol>& symbol) -> FunctionSignature;

	private:
		auto setup_symbol_loader() -> void;
	};
}

#endif