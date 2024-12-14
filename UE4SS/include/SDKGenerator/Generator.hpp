#pragma once

#include <File/File.hpp>

#include <Unreal/NameTypes.hpp>

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace RC::Unreal
{
    class UObject;
    class UFunction;
    class UStruct;
    class UClass;
    class UScriptStruct;
    class FProperty;
} // namespace RC::Unreal

namespace RC::UEGenerator
{
    enum class IsDelegateFunction
    {
        Yes,
        No,
    };

    auto generate_cxx_headers(const std::filesystem::path directory_to_generate_in) -> void;
    auto generate_lua_types(const std::filesystem::path directory_to_generate_in) -> void;
} // namespace RC::UEGenerator
