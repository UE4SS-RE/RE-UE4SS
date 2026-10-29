#pragma once

namespace RC::OutTheShade
{
    // 'include_blueprint_types' additionally dumps Blueprint-generated classes/structs/enums
    // (UBlueprintGeneratedClass, UUserDefinedStruct, UUserDefinedEnum, ...) instead of native types only.
    // Lossy mode preserves numeric storage but cannot preserve unresolved enum names.
    auto generate_usmap(bool include_blueprint_types = true, bool allow_unresolved_enums = false) -> void;
}
