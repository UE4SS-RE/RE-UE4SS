#pragma once

namespace RC::JMapGenerator
{
    // Dumps the full UObject reflection graph to a .jmap file (JSON format by trumank, https://github.com/trumank/jmap).
    // 'include_blueprint_types' additionally dumps Blueprint-generated classes/structs/enums (and their CDOs) instead of native (/Script/) objects only.
    // 'skip_property_values' omits class default object property values, which are the bulk of the file size and are not
    // needed by consumers that only care about layout (SDK generation, disassembler imports).
    auto generate_jmap(bool include_blueprint_types = true, bool skip_property_values = false) -> void;
} // namespace RC::JMapGenerator
