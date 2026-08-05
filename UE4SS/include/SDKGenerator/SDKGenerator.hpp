#pragma once

#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <string>

#include <File/Macros.hpp>
#include <Constructs/Annotated.hpp>
#include <glaze/glaze.hpp>

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

        // TODO(sdk-port): glaze >= 4 removed <glaze/core/macros.hpp> and GLZ_LOCAL_META. This aggregate is now
        //                 picked up by glaze's pure reflection, which produces the same key names/order.
        struct glaze
        {
            using T = SDKBackendSettings_ASCII;
            static constexpr auto value = glz::object("IncludePrefix",
                                                      &T::IncludePrefix,
                                                      "HeaderFileExtension",
                                                      &T::HeaderFileExtension,
                                                      "UnrealImplementationNamespace",
                                                      &T::UnrealImplementationNamespace,
                                                      "SDKNamespace",
                                                      &T::SDKNamespace,
                                                      "ExcludedTypes",
                                                      &T::ExcludedTypes,
                                                      "UnreflectedTypes",
                                                      &T::UnreflectedTypes);
        };
    };

    auto generate_sdk(const std::filesystem::path& output_dir, SDKBackendSettings& backend_settings) -> void;
} // namespace RC::UEGenerator
