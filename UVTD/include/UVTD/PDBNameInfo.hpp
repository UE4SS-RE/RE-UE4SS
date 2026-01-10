#pragma once

#include <optional>
#include <vector>
#include <array>
#include <format>

#include <File/File.hpp>

#ifdef TEXT
#undef TEXT
#endif
#define STR(str) L##str

namespace RC::UVTD
{
    // PDB naming format: Major_Minor[-Suffix1][-Suffix2]
    // Minor version is always 2 digits (e.g., 5_01, 4_27)
    // Examples: 4_27, 5_01-CasePreserving, 4_27-MyGame-Debug
    struct PDBNameInfo
    {
        int32_t major_version{};
        int32_t minor_version{};
        std::array<File::StringType, 2> suffixes{};
        size_t suffix_count{};

        // The full original name (e.g., "4_27-CasePreserving")
        File::StringType full_name{};

        // Base version string without suffixes (e.g., "4_27")
        File::StringType base_version{};

        // Version string without underscores for class names (e.g., "427")
        File::StringType version_no_separator{};

        // Parse a PDB filename stem into components
        static auto parse(const File::StringType& pdb_name) -> std::optional<PDBNameInfo>
        {
            PDBNameInfo info;
            info.full_name = pdb_name;

            // Split by '-' to separate version from suffixes
            std::vector<File::StringType> parts;
            size_t start = 0;
            size_t pos = 0;

            while ((pos = pdb_name.find(STR('-'), start)) != File::StringType::npos)
            {
                parts.push_back(pdb_name.substr(start, pos - start));
                start = pos + 1;
            }
            parts.push_back(pdb_name.substr(start));

            if (parts.empty()) return std::nullopt;

            // First part should be Major_Minor
            const auto& version_part = parts[0];
            info.base_version = version_part;

            // Find the underscore separating major and minor
            auto underscore_pos = version_part.find(STR('_'));
            if (underscore_pos == File::StringType::npos) return std::nullopt;

            // Parse major version
            auto major_str = version_part.substr(0, underscore_pos);
            if (major_str.empty()) return std::nullopt;

            try
            {
                info.major_version = std::stoi(std::string(major_str.begin(), major_str.end()));
            }
            catch (...)
            {
                return std::nullopt;
            }

            // Parse minor version
            auto minor_str = version_part.substr(underscore_pos + 1);
            if (minor_str.empty()) return std::nullopt;

            try
            {
                info.minor_version = std::stoi(std::string(minor_str.begin(), minor_str.end()));
            }
            catch (...)
            {
                return std::nullopt;
            }

            // Generate version_no_separator (for class names like UnrealVirtual427)
            info.version_no_separator = std::format(STR("{}{}"),
                info.major_version,
                info.minor_version < 10 ? std::format(STR("0{}"), info.minor_version) : std::format(STR("{}"), info.minor_version));

            // Collect suffixes (max 2)
            info.suffix_count = 0;
            for (size_t i = 1; i < parts.size() && info.suffix_count < 2; ++i)
            {
                if (!parts[i].empty())
                {
                    info.suffixes[info.suffix_count++] = parts[i];
                }
            }

            return info;
        }

        // Check if this PDB has a specific suffix
        auto has_suffix(const File::StringType& suffix) const -> bool
        {
            for (size_t i = 0; i < suffix_count; ++i)
            {
                if (suffixes[i] == suffix) return true;
            }
            return false;
        }

        // Check if version is >= given major.minor
        auto is_at_least(int32_t maj, int32_t min) const -> bool
        {
            return (major_version > maj) || (major_version == maj && minor_version >= min);
        }

        // Get suffix string for filenames (e.g., "_CasePreserving" or "_CasePreserving_Debug")
        auto get_suffix_string() const -> File::StringType
        {
            File::StringType result;
            for (size_t i = 0; i < suffix_count; ++i)
            {
                result += STR("_") + suffixes[i];
            }
            return result;
        }

        // Generate preprocessor macro name from suffix (fallback when not in config)
        // e.g., "CasePreserving" -> "WITH_CASE_PRESERVING"
        static auto suffix_to_macro(const File::StringType& suffix) -> File::StringType
        {
            File::StringType result = STR("WITH_");
            bool need_underscore = false;

            for (size_t i = 0; i < suffix.size(); ++i)
            {
                auto c = suffix[i];

                // Insert underscore before uppercase letters (except at start)
                if (c >= 'A' && c <= 'Z')
                {
                    if (need_underscore && i > 0)
                    {
                        result += STR('_');
                    }
                    result += c;
                    need_underscore = false;
                }
                else if (c >= 'a' && c <= 'z')
                {
                    result += static_cast<wchar_t>(c - 'a' + 'A');
                    need_underscore = true;
                }
                else if (c >= '0' && c <= '9')
                {
                    result += c;
                    need_underscore = true;
                }
            }
            return result;
        }
    };
} // namespace RC::UVTD
