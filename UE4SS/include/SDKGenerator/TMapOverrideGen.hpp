#pragma once

#include <File/File.hpp>
#include <Unreal/NameTypes.hpp>
#include <filesystem>
#include <unordered_set>

namespace RC::UEGenerator
{
    using ::RC::Unreal::FName;

    class TMapOverrideGenerator
    {
      public:
        static ::std::unordered_set<FName> MapProperties;
        static auto generate_tmapoverride() -> void;
    };
} // namespace RC::UEGenerator
