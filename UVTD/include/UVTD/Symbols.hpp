#ifndef UNREALVTABLEDUMPER_SYMBOLS_HPP
#define UNREALVTABLEDUMPER_SYMBOLS_HPP

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
}

#endif