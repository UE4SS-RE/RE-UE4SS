#pragma once

namespace RC::Unreal
{
    class UObject;
}

namespace RC::GUI::Dumpers
{
    using namespace Unreal;

    struct MapDumpFileName
    {
        int8_t currentSMdumpnum{0};
        int8_t currentAAdumpnum{0};
    };

    auto render() -> void;

    auto call_generate_static_mesh_file() -> void;
    auto call_generate_all_actor_file() -> void;
    auto call_generate_object_as_json(UObject*) -> void;

} // namespace RC::GUI::Dumpers
