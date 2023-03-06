#ifndef UE4SS_GUI_DUMPERS_HPP
#define UE4SS_GUI_DUMPERS_HPP

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
    
    
}

#endif //UE4SS_GUI_DUMPERS_HPP
