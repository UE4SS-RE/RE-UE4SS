#pragma once

namespace RC::OutTheShade
{
    // 'include_blueprint_types' additionally dumps Blueprint-generated classes/structs/enums
    // (UBlueprintGeneratedClass, UUserDefinedStruct, UUserDefinedEnum, ...) instead of native types only.
    // If FEnumProperty::Enum is null, numeric storage is preserved, but only if the enum name can be resolved.
    auto generate_usmap(bool include_blueprint_types = true) -> void;
}
