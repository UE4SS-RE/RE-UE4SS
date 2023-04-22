#ifndef UE4SS_REWRITTEN_SDKGENERATOR_GENERATOR_HPP
#define UE4SS_REWRITTEN_SDKGENERATOR_GENERATOR_HPP

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

#include <File/File.hpp>
#include <Unreal/NameTypes.hpp>

namespace RC::Unreal
{
    class UObject;
    class UFunction;
    class UStruct;
    class UClass;
    class UScriptStruct;
    class FProperty;
}

namespace RC::UEGenerator
{
    enum class IsDelegateFunction
    {
        Yes,
        No,
    };

    auto generate_cxx_headers(const std::filesystem::path directory_to_generate_in) -> void;
    auto generate_lua_types(const std::filesystem::path directory_to_generate_in) -> void;
}


#endif //UE4SS_REWRITTEN_SDKGENERATOR_GENERATOR_HPP
