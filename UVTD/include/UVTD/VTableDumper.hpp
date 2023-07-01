#ifndef UNREALVTABLEDUMPER_VTABLEDUMPER_HPP
#define UNREALVTABLEDUMPER_VTABLEDUMPER_HPP

#include <unordered_map>
#include <map>

#include <UVTD/Symbols.hpp>
#include <UVTD/TypeContainer.hpp>
#include <File/File.hpp>

#include <atlbase.h>
#include <dia2.h>

namespace RC::UVTD
{
	class VTableDumper
	{
	private:
		Symbols symbols;
		TypeContainer type_container;

		bool are_symbols_cached{};

	public:
		VTableDumper() = delete;
		explicit VTableDumper(Symbols symbols) : symbols(std::move(symbols)) {}

	public:
		auto generate_code() -> void;
		auto generate_files() -> void;

	public:
		auto get_type_container() const -> const TypeContainer& { return type_container; }

	private:
		auto dump_vtable_for_symbol(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo&, EnumEntry* = nullptr, struct Class* class_entry = nullptr) -> void;
		auto dump_vtable_for_symbol(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void;
	};
}

#endif