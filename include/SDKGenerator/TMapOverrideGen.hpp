#ifndef UE4SS_REWRITTEN_TMAPOVERRIDE_GENERATOR_HPP
#define UE4SS_REWRITTEN_TMAPOVERRIDE_GENERATOR_HPP

#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <unordered_set>
#include <File/File.hpp>
#include <Unreal/NameTypes.hpp>

namespace RC::TMapOverrideGen
{
    using RC::Unreal::FName;
    
    class TMapOverrideGenerator
    {
    public:
        std::unordered_map<FName, int> MapProperties {};
        static auto generate_tmapoverride() -> void;
    };
}


#endif //UE4SS_REWRITTEN_TMAPOVERRIDE_GENERATOR_HPP
