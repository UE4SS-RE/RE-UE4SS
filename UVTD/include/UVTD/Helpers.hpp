#ifndef UNREALVTABLEDUMPER_HELPERS_HPP
#define UNREALVTABLEDUMPER_HELPERS_HPP

#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <UVTD/Symbols.hpp>
#include <File/File.hpp>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

#include <atlbase.h>
#include <dia2.h>

namespace RC::UVTD
{
	struct ObjectItem
	{
		File::StringType name{};
		ValidForVTable valid_for_vtable{};
		ValidForMemberVars valid_for_member_vars{};
	};
	// TODO: UConsole isn't found in all PDBs by this tool for some reason. Fix it.
	//       For now, to avoid problems, let's not generate for UConsole.
	static inline std::vector<ObjectItem> s_object_items{
			{STR("UObjectBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UObjectBaseUtility"), ValidForVTable::Yes, ValidForMemberVars::No},
			{STR("UObject"), ValidForVTable::Yes, ValidForMemberVars::No},
			{STR("UScriptStruct::ICppStructOps"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FOutputDevice"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UStruct"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UGameViewportClient"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			//{STR("UConsole"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FMalloc"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FArchive"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FArchiveState"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("AGameModeBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("AGameMode"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("AActor"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("AHUD"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UPlayer"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("ULocalPlayer"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FExec"), ValidForVTable::Yes, ValidForMemberVars::No},
			{STR("FField"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UField"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FNumericProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UNumericProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FMulticastDelegateProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UMulticastDelegateProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FObjectPropertyBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UObjectPropertyBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			/*{STR("FConsoleManager"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("UDataTable"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FConsoleVariableBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
			{STR("FConsoleCommandBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},*/

			{STR("UScriptStruct"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UWorld"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UFunction"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UClass"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UEnum"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FStructProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UStructProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FArrayProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UArrayProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FMapProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UMapProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FBoolProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UBoolProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FByteProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UByteProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FEnumProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UEnumProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FClassProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UClassProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FSoftClassProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("USoftClassProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FDelegateProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UDelegateProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FInterfaceProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("UInterfaceProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FFieldPathProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("FSetProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
			{STR("USetProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
	};

	static inline std::unordered_map<File::StringType, std::unordered_set<File::StringType>> s_private_variables{
			{
					STR("FField"),
					{
							STR("ClassPrivate"),
							STR("NamePrivate"),
							STR("Next"),
							STR("Owner"),
					}
			},
	};

	static std::unordered_set<File::StringType> NonCasePreservingVariants{
			{STR("4_27")},
	};

	static std::unordered_set<File::StringType> CasePreservingVariants{
			{STR("4_27_CasePreserving")},
	};

	static inline std::vector<File::StringType> UPrefixToFPrefix{
			STR("UProperty"),
			STR("UMulticastDelegateProperty"),
			STR("UObjectPropertyBase"),
			STR("UStructProperty"),
			STR("UArrayProperty"),
			STR("UMapProperty"),
			STR("UBoolProperty"),
			STR("UByteProperty"),
			STR("UNumericProperty"),
			STR("UEnumProperty"),
			STR("UClassProperty"),
			STR("USoftClassProperty"),
			STR("UDelegateProperty"),
			STR("UInterfaceProperty"),
			STR("USetProperty"),
	};

	static inline std::unordered_set<File::StringType> valid_udt_names{
			STR("UScriptStruct::ICppStructOps"),
			STR("UObjectBase"),
			STR("UObjectBaseUtility"),
			STR("UObject"),
			STR("UStruct"),
			STR("UGameViewportClient"),
			STR("UScriptStruct"),
			STR("FOutputDevice"),
			//STR("UConsole"),
			STR("FMalloc"),
			STR("FArchive"),
			STR("FArchiveState"),
			STR("AGameModeBase"),
			STR("AGameMode"),
			STR("AActor"),
			STR("AHUD"),
			STR("UPlayer"),
			STR("ULocalPlayer"),
			STR("FExec"),
			STR("UField"),
			STR("FField"),
			STR("FProperty"),
			STR("UProperty"),
			STR("FNumericProperty"),
			STR("UNumericProperty"),
			STR("FMulticastDelegateProperty"),
			STR("UMulticastDelegateProperty"),
			STR("FObjectPropertyBase"),
			STR("UObjectPropertyBase"),
			STR("UStructProperty"),
			STR("FStructProperty"),
			STR("UArrayProperty"),
			STR("FArrayProperty"),
			STR("UMapProperty"),
			STR("FMapProperty"),
			STR("UWorld"),
			STR("UFunction"),
			STR("FBoolProperty"),
			STR("UClass"),
			STR("UEnum"),
			STR("UBoolProperty"),
			STR("FByteProperty"),
			STR("UByteProperty"),
			STR("FEnumProperty"),
			STR("UEnumProperty"),
			STR("FClassProperty"),
			STR("UClassProperty"),
			STR("FSoftClassProperty"),
			STR("USoftClassProperty"),
			STR("FDelegateProperty"),
			STR("UDelegateProperty"),
			STR("FInterfaceProperty"),
			STR("UInterfaceProperty"),
			STR("FFieldPathProperty"),
			STR("FSetProperty"),
			STR("USetProperty"),
	};

	auto HRESULTToString(HRESULT result) -> std::string;

	auto is_valid_type_to_dump(File::StringType type_name) -> bool;

	auto sym_tag_to_string(DWORD sym_tag) -> File::StringViewType;

	auto base_type_to_string(CComPtr<IDiaSymbol> symbol) -> File::StringType;

	auto kind_to_string(DWORD kind) -> File::StringViewType;

	auto generate_pointer_type(CComPtr<IDiaSymbol> symbol) -> File::StringType;

	auto get_symbol_name(CComPtr<IDiaSymbol> symbol) -> File::StringType;

	auto get_type_name(CComPtr<IDiaSymbol> type) -> File::StringType;
}

#endif