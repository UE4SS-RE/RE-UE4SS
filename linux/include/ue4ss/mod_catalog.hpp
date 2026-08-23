#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ue4ss::linux
{
    struct LuaModDescriptor
    {
        std::string name;
        std::filesystem::path root;
        std::filesystem::path main_script;
    };

    struct LuaModCatalog
    {
        std::vector<LuaModDescriptor> enabled_mods;
        std::vector<std::string> rejected_mods;
    };

    // Discovers enabled Lua mods without executing any code. All path
    // components are resolved with UE4SS' ASCII case-insensitive semantics.
    // Ambiguous case variants and symlinked mod content are rejected.
    [[nodiscard]] LuaModCatalog discover_lua_mods(const std::filesystem::path& ue4ss_root);
} // namespace ue4ss::linux
