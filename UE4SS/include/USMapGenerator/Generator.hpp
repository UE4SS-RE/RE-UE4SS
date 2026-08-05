#pragma once

namespace RC::OutTheShade
{
    // 'include_blueprint_types' additionally dumps Blueprint-generated classes/structs/enums
    // (UBlueprintGeneratedClass, UUserDefinedStruct, UUserDefinedEnum, ...) instead of native types only.
    auto generate_usmap(bool include_blueprint_types = true) -> void;
}
