#include <File/File.hpp>

namespace RC::File
{
    auto construct_handle(const std::filesystem::path& file_name, const OpenProperties& open_properties) -> Handle
    {
        auto internal_handle = Handle::FileType::open_file(file_name, open_properties);
        return Handle{std::move(internal_handle)};
    }

    auto open(const std::filesystem::path& file_path_and_name,
              OpenFor open_for,
              OverwriteExistingFile overwrite_existing_file,
              CreateIfNonExistent create_if_non_existent) -> Handle
    {
        OpenProperties open_properties{
                .open_for = open_for,
                .overwrite_existing_file = overwrite_existing_file,
                .create_if_non_existent = create_if_non_existent,
        };

        return construct_handle(file_path_and_name, open_properties);
    }

    auto delete_file(const std::filesystem::path& file_path_and_name) -> void
    {
        Handle::FileType::delete_file(file_path_and_name);
    }
} // namespace RC::File