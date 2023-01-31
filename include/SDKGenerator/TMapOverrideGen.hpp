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
    using ::RC::Unreal
    using UObject = RC::Unreal::UObject;
    using UStruct = RC::Unreal::UStruct;
    using UClass = RC::Unreal::UClass;
    using FProperty = RC::Unreal::FProperty;
    using FField = RC::Unreal::FField;

    class TMapOverrideGenerator
    {
    public:

        std::unordered_map<FName, int> MapProperties;
        
        
        
        static auto generate_tmapoverride() -> void;
        static auto dump_tmaps(UObject* object, FProperty* property, size_t NumProps) -> std::pair<StringType, size_t>;

        
    };
}


#endif //UE4SS_REWRITTEN_TMAPOVERRIDE_GENERATOR_HPP
