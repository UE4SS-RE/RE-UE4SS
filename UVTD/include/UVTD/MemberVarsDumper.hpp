#ifndef UNREALVTABLEDUMPER_MEMBERVARS_HPP
#define UNREALVTABLEDUMPER_MEMBERVARS_HPP

#include <unordered_map>

#include <UVTD/Symbols.hpp>
#include <UVTD/TypeContainer.hpp>
#include <File/File.hpp>

#include <atlbase.h>
#include <dia2.h>

namespace RC::UVTD
{
	class MemberVarsDumper
	{
	private:
		Symbols symbols;
		TypeContainer type_container;

	public:
		MemberVarsDumper() = delete;
		explicit MemberVarsDumper(Symbols symbols) : symbols(std::move(symbols))
		{


		}

	public:
		auto generate_code() -> void;
		auto generate_files() -> void;

	public:
		auto get_type_container() const -> const TypeContainer& { return type_container; }

	private:
		using EnumEntriesTypeAlias = struct EnumEntry*;
		auto dump_member_variable_layout(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo&, EnumEntriesTypeAlias = nullptr, struct Class* class_entry = nullptr) -> void;
		auto dump_member_variable_layouts(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void;
	};
}

#endif