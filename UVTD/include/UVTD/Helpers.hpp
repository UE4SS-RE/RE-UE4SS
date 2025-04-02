#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>

#include <File/File.hpp>
#include <UVTD/Symbols.hpp>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX

namespace RC::UVTD
{
    // Define the single root output directory
    static std::filesystem::path uvtd_output_root = "UVTD_Generated_Output";

    // Base paths relative to the output root directory (designed to match RE-UE4SS structure)
    static std::filesystem::path unreal_deps_base_path = uvtd_output_root / "deps" / "first" / "Unreal";
    static std::filesystem::path assets_base_path = uvtd_output_root / "assets";
    // Path for files belonging directly to the main UE4SS project structure
    static std::filesystem::path ue4ss_base_path = uvtd_output_root / "UE4SS";

    // --- VTable Generator Paths ---
    static std::filesystem::path vtable_gen_function_bodies_path = unreal_deps_base_path / "generated_include" / "FunctionBodies";
    static std::filesystem::path vtable_templates_output_path = assets_base_path / "VTableLayoutTemplates";

    // --- Virtual Implementation Generator Paths ---
    // Headers go into the main include structure for the Unreal module
    static std::filesystem::path virtual_gen_header_output_path = unreal_deps_base_path / "include" / "Unreal" / "VersionedContainer" / "UnrealVirtualImpl";
    static std::filesystem::path virtual_gen_source_output_path = unreal_deps_base_path / "src" / "VersionedContainer" / "UnrealVirtualImpl";
    static std::filesystem::path virtual_gen_included_function_bodies_path = unreal_deps_base_path / "generated_include" / "FunctionBodies";

    // --- Member Variable Layout Generator Paths ---
    static std::filesystem::path member_variable_layouts_gen_function_bodies_path = unreal_deps_base_path / "generated_include" / "FunctionBodies";
    static std::filesystem::path member_variable_layouts_wrappers_output_path = unreal_deps_base_path / "generated_include"; // Header & Src wrappers
    static std::filesystem::path member_variable_layouts_templates_output_path = assets_base_path / "MemberVarLayoutTemplates";

    // --- Standalone/Misc Paths (relative to output root) ---
    static std::filesystem::path macro_setter_output_path = ue4ss_base_path / "generated_include";
    // Keep SolBindings separate as they aren't directly part of RE-UE4SS build system by default
    static std::filesystem::path sol_bindings_output_path = uvtd_output_root / "GeneratedSolBindings";


    auto to_string_type(const char* c_str) -> File::StringType;
    auto change_prefix(File::StringType input, bool is_425_plus) -> std::optional<File::StringType>;

    // Workaround that lets us have a unified 'TUObjectArray' struct regardless if the engine version uses a chunked or non-chunked variant of TUObjectArray.
    auto unify_uobject_array_if_needed(StringType& out_variable_type) -> bool;
} // namespace RC::UVTD