#pragma once

#include <File/Common.hpp>
#include <File/FileDef.hpp>
#include <File/HandleTemplate.hpp>
#include RC_OS_FILE_TYPE_INCLUDE_FILE

namespace RC::File
{
    RC_FILE_API auto open(const std::filesystem::path& file_path_and_name,
                          OpenFor = OpenFor::Reading,
                          OverwriteExistingFile = OverwriteExistingFile::No,
                          CreateIfNonExistent = CreateIfNonExistent::No) -> Handle;

    RC_FILE_API auto delete_file(const std::filesystem::path& file_path_and_name) -> void;

    // Search a path that can access `base / tail`, where the tail part is allowed to be case insensitive.
    // Returns the path found or std::nullopt if no path was found.
    RC_FILE_API auto get_path_if_exists(const std::filesystem::path& base, const std::filesystem::path& tail) -> std::optional<const std::filesystem::path>;
} // namespace RC::File
