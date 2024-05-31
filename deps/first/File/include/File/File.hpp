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
} // namespace RC::File
