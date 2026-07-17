#include "ue4ss/mod_catalog.hpp"

#include "ue4ss/case_path.hpp"

#include <algorithm>
#include <map>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
    [[nodiscard]] char ascii_lower(char character)
    {
        if (character >= 'A' && character <= 'Z')
        {
            return static_cast<char>(character + ('a' - 'A'));
        }
        return character;
    }

    [[nodiscard]] std::string ascii_lower(std::string_view value)
    {
        std::string normalized;
        normalized.reserve(value.size());
        for (const char character : value)
        {
            normalized.push_back(ascii_lower(character));
        }
        return normalized;
    }

    [[nodiscard]] bool is_plain_directory(const std::filesystem::path& path, std::error_code& error)
    {
        const auto status = std::filesystem::symlink_status(path, error);
        return !error && std::filesystem::is_directory(status) && !std::filesystem::is_symlink(status);
    }

    [[nodiscard]] bool is_plain_file(const std::filesystem::path& path, std::error_code& error)
    {
        const auto status = std::filesystem::symlink_status(path, error);
        return !error && std::filesystem::is_regular_file(status) && !std::filesystem::is_symlink(status);
    }

    [[nodiscard]] bool has_only_real_components(const std::filesystem::path& root,
                                                 const std::filesystem::path& resolved,
                                                 std::error_code& error)
    {
        const std::filesystem::path relative = resolved.lexically_relative(root);
        if (relative.empty() || relative.is_absolute())
        {
            return false;
        }

        std::filesystem::path current = root;
        for (const auto& component : relative)
        {
            if (component == "." || component == "..")
            {
                return false;
            }
            current /= component;
            const auto status = std::filesystem::symlink_status(current, error);
            if (error || std::filesystem::is_symlink(status))
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::string describe_resolution_error(std::string_view mod_name,
                                                         std::string_view item,
                                                         const ue4ss::linux::PathResolution& resolution)
    {
        return "mod '" + std::string{mod_name} + "' rejected: cannot resolve " + std::string{item} + ": " + resolution.message;
    }
} // namespace

namespace ue4ss::linux
{
    LuaModCatalog discover_lua_mods(const std::filesystem::path& ue4ss_root)
    {
        LuaModCatalog catalog;
        const std::filesystem::path mods_root = ue4ss_root / "Mods";

        std::error_code error;
        if (!std::filesystem::exists(mods_root, error))
        {
            if (error)
            {
                catalog.rejected_mods.push_back("cannot inspect Mods directory: " + error.message());
            }
            return catalog;
        }
        if (!is_plain_directory(mods_root, error))
        {
            catalog.rejected_mods.push_back(error ? "cannot inspect Mods directory: " + error.message()
                                                  : "Mods directory must be a real directory, not a symlink");
            return catalog;
        }

        std::map<std::string, std::vector<std::filesystem::path>> candidates;
        std::filesystem::directory_iterator iterator{mods_root, error};
        const std::filesystem::directory_iterator end;
        while (!error && iterator != end)
        {
            const auto path = iterator->path();
            std::error_code type_error;
            if (is_plain_directory(path, type_error))
            {
                candidates[ascii_lower(path.filename().string())].push_back(path);
            }
            else if (type_error)
            {
                catalog.rejected_mods.push_back("cannot inspect Mods entry '" + path.filename().string() + "': " + type_error.message());
            }
            iterator.increment(error);
        }
        if (error)
        {
            catalog.rejected_mods.push_back("cannot enumerate Mods directory: " + error.message());
        }

        for (auto& [normalized_name, paths] : candidates)
        {
            static_cast<void>(normalized_name);
            std::sort(paths.begin(), paths.end());
            if (paths.size() > 1u)
            {
                std::string detail{"case-colliding mod directories rejected:"};
                for (const auto& path : paths)
                {
                    detail += " " + path.filename().string();
                }
                catalog.rejected_mods.push_back(std::move(detail));
                continue;
            }

            const std::filesystem::path& mod_root = paths.front();
            const std::string mod_name = mod_root.filename().string();
            const PathResolution enabled = resolve_case_insensitive(mod_root.string(), "enabled.txt");
            if (!enabled)
            {
                if (enabled.error != PathResolutionError::not_found)
                {
                    catalog.rejected_mods.push_back(describe_resolution_error(mod_name, "enabled.txt", enabled));
                }
                continue;
            }

            error.clear();
            if (!has_only_real_components(mod_root, enabled.path, error) || !is_plain_file(enabled.path, error))
            {
                catalog.rejected_mods.push_back("mod '" + mod_name + "' rejected: enabled.txt must be a real regular file");
                continue;
            }

            const PathResolution script = resolve_case_insensitive(mod_root.string(), "Scripts/main.lua");
            if (!script)
            {
                catalog.rejected_mods.push_back(describe_resolution_error(mod_name, "Scripts/main.lua", script));
                continue;
            }
            error.clear();
            if (!has_only_real_components(mod_root, script.path, error) || !is_plain_file(script.path, error))
            {
                catalog.rejected_mods.push_back(
                        "mod '" + mod_name + "' rejected: Scripts/main.lua and its parent directories must not be symlinks");
                continue;
            }

            catalog.enabled_mods.push_back({mod_name, mod_root, script.path});
        }

        std::sort(catalog.enabled_mods.begin(), catalog.enabled_mods.end(), [](const LuaModDescriptor& left, const LuaModDescriptor& right) {
            const std::string left_normalized = ascii_lower(left.name);
            const std::string right_normalized = ascii_lower(right.name);
            return left_normalized == right_normalized ? left.name < right.name : left_normalized < right_normalized;
        });
        return catalog;
    }
} // namespace ue4ss::linux
