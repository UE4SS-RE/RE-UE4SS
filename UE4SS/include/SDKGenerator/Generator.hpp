#pragma once

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
} // namespace RC::Unreal

namespace RC::UEGenerator
{
    enum class IsDelegateFunction
    {
        Yes,
        No,
    };

    inline auto resolve_output_directory(const std::filesystem::path& directory) -> std::filesystem::path
    {
#ifdef _WIN32
        // Extended-length paths avoid the legacy MAX_PATH limit on Windows.
        std::filesystem::path resolved_directory{"\\\\?\\"};
        resolved_directory += directory;
        return resolved_directory;
#else
        return directory;
#endif
    }

    auto generate_cxx_headers(const std::filesystem::path directory_to_generate_in) -> void;
    auto generate_lua_types(const std::filesystem::path directory_to_generate_in) -> void;
} // namespace RC::UEGenerator
