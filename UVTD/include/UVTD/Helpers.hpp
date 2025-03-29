#pragma once

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
    // Output paths for generated files
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

    auto to_string_type(const char* c_str) -> File::StringType;
    auto change_prefix(File::StringType input, bool is_425_plus) -> std::optional<File::StringType>;

    // Workaround that lets us have a unified 'TUObjectArray' struct regardless if the engine version uses a chunked or non-chunked variant of TUObjectArray.
    auto unify_uobject_array_if_needed(StringType& out_variable_type) -> bool;
} // namespace RC::UVTD
