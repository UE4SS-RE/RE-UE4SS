#pragma once

namespace RC::GUI::Dumpers
{
    struct MapDumpFileName
    {
        int8_t currentSMdumpnum{0};
        int8_t currentAAdumpnum{0};
    };

    auto render() -> void;

    void call_generate_static_mesh_file();
    void call_generate_all_actor_file();

} // namespace RC::GUI::Dumpers
