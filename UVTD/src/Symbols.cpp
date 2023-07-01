#include <format>

#include <UVTD/Symbols.hpp>
#include <UVTD/Helpers.hpp>

namespace RC::UVTD
{
	auto Symbols::generate_const_qualifier(CComPtr<IDiaSymbol>& symbol) -> bool
	{
		HRESULT hr;
		CComPtr<IDiaSymbol> real_symbol = symbol;
		DWORD sym_tag;
		real_symbol->get_symTag(&sym_tag);

		// TODO: Fix this. It's currently broken for two reasons.
		//       1. If sym_tag == SymTagFunctionType, we assume that we're looking for the return type but we might be looking for the const qualifier on the function itself.
		//       2. Even if we change this to fix the above problem, for some reason, calling 'get_constType' on the SymTagFunctionType sets the BOOL to FALSE, and calling it on SymTagFunction returns S_FALSE.
		if (sym_tag == SymTagFunctionType)
		{
			if (hr = real_symbol->get_type(&real_symbol.p); hr != S_OK)
			{
				throw std::runtime_error{ std::format("Call to 'get_type()' failed with error '{}'", HRESULTToString(hr)) };
			}

			real_symbol->get_symTag(&sym_tag);
			if (sym_tag == SymTagPointerType)
			{
				if (hr = real_symbol->get_type(&real_symbol.p); hr != S_OK)
				{
					throw std::runtime_error{ std::format("Call to 'get_type()' failed with error '{}'", HRESULTToString(hr)) };
				}
			}
		}

		BOOL is_const = FALSE;
		real_symbol->get_constType(&is_const);

		return is_const;
	}

	auto Symbols::generate_type(CComPtr<IDiaSymbol>& symbol) -> File::StringType
	{
		HRESULT hr;
		CComPtr<IDiaSymbol> function_type_symbol;
		if (hr = symbol->get_type(&function_type_symbol); hr != S_OK)
		{
			function_type_symbol = symbol;
		}

		// TODO: Const for pointers
		//       We only support const for non-pointers and the data the pointer is pointing to
		auto return_type_name = get_symbol_name(function_type_symbol);
		auto pointer_type = generate_pointer_type(function_type_symbol);
		File::StringType const_qualifier = generate_const_qualifier(function_type_symbol) ? STR("const") : STR("");

		return std::format(STR("{}{}{}"), const_qualifier.empty() ? STR("") : std::format(STR("{} "), const_qualifier), return_type_name, pointer_type);
	}

	auto Symbols::generate_function_params(CComPtr<IDiaSymbol>& symbol) -> std::vector<FunctionParam>
	{
		HRESULT hr;

		CComPtr<IDiaSymbol> function_type_symbol;
		if (hr = symbol->get_type(&function_type_symbol); hr != S_OK)
		{
			throw std::runtime_error{ std::format("Call to 'get_type(&function_type_symbol)' failed with error: {}", HRESULTToString(hr)) };
		}

		std::vector<FunctionParam> params{};

		DWORD tag;
		symbol->get_symTag(&tag);

		CComPtr<IDiaEnumSymbols> sub_symbols;
		if (hr = symbol->findChildren(SymTagNull, nullptr, NULL, &sub_symbols.p); hr == S_OK)
		{
			CComPtr<IDiaSymbol> sub_symbol;
			ULONG num_symbols_fetched{};
			LONG count;
			hr = sub_symbols->get_Count(&count);
			while (sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched) == S_OK && num_symbols_fetched == 1)
			{
				DWORD data_kind;
				sub_symbol->get_dataKind(&data_kind);

				if (data_kind != DataKind::DataIsParam)
				{
					sub_symbol = nullptr;
					continue;
				}

				params.push_back(FunctionParam{
					.type = generate_type(sub_symbol),
					.name = get_symbol_name(sub_symbol)
				});
			}
			sub_symbol = nullptr;
		}
		sub_symbols = nullptr;

		return params;
	}

	auto Symbols::generate_function_signature(CComPtr<IDiaSymbol>& symbol) -> FunctionSignature
	{
		DWORD tag;
		symbol->get_symTag(&tag);
		auto params = generate_function_params(symbol);
		auto return_type = generate_type(symbol);
		auto const_qualifier = generate_const_qualifier(symbol);

		return FunctionSignature{
			.return_type = return_type,
			.name = get_symbol_name(symbol),
			.params = params,
			.const_qualifier = const_qualifier
		};
	}

	auto Symbols::setup_symbol_loader() -> void
	{
		if (!std::filesystem::exists(pdb_file))
		{
			throw std::runtime_error{ std::format("PDB '{}' not found", pdb_file.string()) };
		}

		auto hr = CoCreateInstance(CLSID_DiaSource, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), reinterpret_cast<void**>(&dia_source));
		if (FAILED(hr))
		{
			throw std::runtime_error{ std::format("CoCreateInstance failed. Register msdia140.dll. Error: {}", HRESULTToString(hr)) };
		}

		if (hr = dia_source->loadDataFromPdb(pdb_file.c_str()); FAILED(hr))
		{
			throw std::runtime_error{ std::format("Failed to load symbol data with error: {}", HRESULTToString(hr)) };
		}

		if (hr = dia_source->openSession(&dia_session); FAILED(hr))
		{
			throw std::runtime_error{ std::format("Call to 'openSession' failed with error: {}", HRESULTToString(hr)) };
		}

		if (hr = dia_session->get_globalScope(&dia_global_symbol); FAILED(hr))
		{
			throw std::runtime_error{ std::format("Call to 'get_globalScope' failed with error: {}", HRESULTToString(hr)) };
		}
	}
}