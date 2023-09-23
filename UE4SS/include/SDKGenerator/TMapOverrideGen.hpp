#pragma once

#include <filesystem>
#include <unordered_set>
#include <File/File.hpp>
#include <Unreal/NameTypes.hpp>

namespace RC::UEGenerator
{
    using RC::Unreal::FName;
    
    class TMapOverrideGenerator
    {
    public:
        static std::unordered_set<FName> MapProperties;
        static auto generate_tmapoverride() -> void;
    };
}



