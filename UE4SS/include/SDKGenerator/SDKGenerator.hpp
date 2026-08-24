#pragma once

#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <string>

#include <File/Macros.hpp>
#include <Constructs/Annotated.hpp>
#include <glaze/glaze.hpp>
#include <Helpers/String.hpp>
#include <SDKGenerator/BuiltinSDKBackend.hpp>

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

    // Widens a parsed backend description into the form the generator consumes.
    inline auto backend_settings_from_ascii(const SDKBackendSettings_ASCII& settings_ascii) -> SDKBackendSettings
    {
        SDKBackendSettings settings{};
        settings.IncludePrefix = to_wstring(settings_ascii.IncludePrefix.value);
        settings.HeaderFileExtension = to_wstring(settings_ascii.HeaderFileExtension.value);
        settings.UnrealImplementationNamespace = to_wstring(settings_ascii.UnrealImplementationNamespace.value);
        settings.SDKNamespace = to_wstring(settings_ascii.SDKNamespace.value);
        for (const auto& excluded_type : settings_ascii.ExcludedTypes.value)
        {
            settings.ExcludedTypes.emplace(to_wstring(excluded_type));
        }
        for (const auto& [type_name, header_path] : settings_ascii.UnreflectedTypes.value)
        {
            settings.UnreflectedTypes.emplace(to_wstring(type_name), to_wstring(header_path));
        }
        return settings;
    }

    // The UE4SS backend compiled in from assets/UE4SS_SDK_Backends/UE4SS.json. Lets callers without a
    // GUI -- Lua, keybinds, automated runs -- generate an SDK with no backend description on disk.
    inline auto get_builtin_backend_settings() -> SDKBackendSettings
    {
        SDKBackendSettings_ASCII settings_ascii{};
        std::string settings_buffer{g_builtin_ue4ss_backend_json};
        if (auto ec = glz::read_json(settings_ascii, settings_buffer); ec)
        {
            throw std::runtime_error{"Failed to parse the built-in SDK generator backend: " + glz::format_error(ec, settings_buffer)};
        }
        return backend_settings_from_ascii(settings_ascii);
    }

    auto generate_sdk(const std::filesystem::path& output_dir, SDKBackendSettings& backend_settings) -> void;
} // namespace RC::UEGenerator
