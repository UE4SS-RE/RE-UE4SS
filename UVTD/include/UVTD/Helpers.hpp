#ifndef UNREALVTABLEDUMPER_HELPERS_HPP
#define UNREALVTABLEDUMPER_HELPERS_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <File/File.hpp>
#include <UVTD/Symbols.hpp>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

namespace RC::UVTD
{
    static std::filesystem::path vtable_gen_output_path = "GeneratedVTables";
    static std::filesystem::path vtable_gen_output_include_path = vtable_gen_output_path / "generated_include";
    static std::filesystem::path vtable_gen_output_src_path = vtable_gen_output_path / "generated_src";
    static std::filesystem::path vtable_gen_output_function_bodies_path = vtable_gen_output_include_path / "FunctionBodies";
    static std::filesystem::path vtable_templates_output_path = "VTableLayoutTemplates";
    static std::filesystem::path virtual_gen_output_path = "GeneratedVirtualImplementations";
    static std::filesystem::path virtual_gen_output_include_path = virtual_gen_output_path / "generated_include";
    static std::filesystem::path virtual_gen_function_bodies_path = virtual_gen_output_include_path / "FunctionBodies";

    static std::filesystem::path member_variable_layouts_gen_output_path = "GeneratedMemberVariableLayouts";
    static std::filesystem::path member_variable_layouts_gen_output_include_path = member_variable_layouts_gen_output_path / "generated_include";
    static std::filesystem::path member_variable_layouts_gen_output_src_path = member_variable_layouts_gen_output_path / "generated_src";
    static std::filesystem::path member_variable_layouts_gen_function_bodies_path = member_variable_layouts_gen_output_include_path / "FunctionBodies";
    static std::filesystem::path member_variable_layouts_templates_output_path = "MemberVarLayoutTemplates";

    struct ObjectItem
    {
        File::StringType name{};
        ValidForVTable valid_for_vtable{};
        ValidForMemberVars valid_for_member_vars{};
    };
    // TODO: UConsole isn't found in all PDBs by this tool for some reason. Fix it.
    //       For now, to avoid problems, let's not generate for UConsole.
    static inline std::vector<ObjectItem> s_object_items{{STR("UObjectBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
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
                                                         {STR("UPlayer"), ValidForVTable::Yes, ValidForMemberVars::Yes},
                                                         {STR("ULocalPlayer"), ValidForVTable::Yes, ValidForMemberVars::Yes},
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
                                                         {STR("ITextData"), ValidForVTable::Yes, ValidForMemberVars::No},
                                                         {STR("FUObjectArray"), ValidForVTable::No, ValidForMemberVars::Yes},
                                                         {STR("FChunkedFixedUObjectArray"), ValidForVTable::No, ValidForMemberVars::Yes},
                                                         {STR("FFixedUObjectArray"), ValidForVTable::No, ValidForMemberVars::Yes},
                                                         {STR("FUObjectItem"), ValidForVTable::No, ValidForMemberVars::Yes}};

    static inline std::unordered_map<File::StringType, std::unordered_set<File::StringType>> s_private_variables{
            {STR("FField"),
             {
                     STR("ClassPrivate"),
                     STR("NamePrivate"),
                     STR("Next"),
                     STR("Owner"),
             }},
    };

    static std::unordered_set<File::StringType> s_non_case_preserving_variants{
            {STR("4_27")},
    };

    static std::unordered_set<File::StringType> s_case_preserving_variants{
            {STR("4_27_CasePreserving")},
    };

    static inline std::unordered_set<File::StringType> s_valid_udt_names{STR("UScriptStruct::ICppStructOps"),
                                                                       STR("UObjectBase"),
                                                                       STR("UObjectBaseUtility"),
                                                                       STR("UObject"),
                                                                       STR("UStruct"),
                                                                       STR("UGameViewportClient"),
                                                                       STR("UScriptStruct"),
                                                                       STR("FOutputDevice"),
                                                                       // STR("UConsole"),
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
                                                                       STR("FFrame")};

    static inline std::vector<File::StringType> s_uprefix_to_fprefix{
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

    // Any members by the name 'LHS' will instead use 'RHS' in generated code.
    static inline std::unordered_map<StringType, StringType> s_member_rename_map{
            {STR("Class"), STR("ClassPrivate")},
            {STR("Name"), STR("NamePrivate")},
            {STR("Outer"), STR("OuterPrivate")},
    };

    auto to_string_type(const char* c_str) -> File::StringType;
    auto change_prefix(File::StringType input, bool is_425_plus) -> std::optional<File::StringType>;

    // Workaround that lets us have a unified 'TUObjectArray' struct regardless if the engine version uses a chunked or non-chunked variant of TUObjectArray.
    auto unify_uobject_array_if_needed(StringType& out_variable_type) -> bool;
} // namespace RC::UVTD

#endif