#pragma once

#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <string>

#include <File/Macros.hpp>
#include <Constructs/Annotated.hpp>
#include <glaze/glaze.hpp>
#include <glaze/core/macros.hpp>

namespace RC::UEGenerator
{
    struct SDKBackendSettings
    {
        StringType IncludePrefix{};
        StringType HeaderFileExtension{};
        StringType UnrealImplementationNamespace{};
        StringType SDKNamespace{};
        std::unordered_set<StringType> ExcludedTypes{};
        std::unordered_map<StringType, StringType> UnreflectedTypes{};
    };

    struct SDKBackendSettings_ASCII
    {
        Annotated<Strings<"">, std::string> IncludePrefix{"Unreal"};
        Annotated<Strings<"">, std::string> HeaderFileExtension{"hpp"};
        Annotated<Strings<"">, std::string> UnrealImplementationNamespace{"RC::Unreal"};
        Annotated<Strings<"">, std::string> SDKNamespace{"UE4SS_Default"};

        Annotated<Strings<"">, std::unordered_set<std::string>> ExcludedTypes{std::unordered_set<std::string>{}};
        Annotated<Strings<"">, std::unordered_map<std::string, std::string>> UnreflectedTypes{std::unordered_map<std::string, std::string>{}};

        GLZ_LOCAL_META(SDKBackendSettings_ASCII, IncludePrefix, HeaderFileExtension, UnrealImplementationNamespace, SDKNamespace, ExcludedTypes, UnreflectedTypes);
    };

    auto generate_sdk(const std::filesystem::path& output_dir, SDKBackendSettings& backend_settings) -> void;
} // namespace RC::UEGenerator
