#include <File/File.hpp>
#include <algorithm>

#include <Helpers/String.hpp>

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

    auto get_path_if_exists(const std::filesystem::path& base, const std::filesystem::path& tail) -> std::optional<const std::filesystem::path> 
    {
        if (!std::filesystem::exists(base))
        {
            throw std::runtime_error("Base path does not exist: " + base.string());
        }
        if (tail.is_absolute())
        {
            throw std::runtime_error("Tail path must be relative: " + tail.string());
        }
        auto normalized_tail = tail.lexically_normal();
        auto full_path = base.lexically_normal();
        for (const auto& part : normalized_tail)
        {
            if (std::filesystem::exists(full_path / part)) 
            {
                full_path /= part;
            }
            else
            {
                bool found = false;
                for (const auto& entry : std::filesystem::directory_iterator(full_path))
                {
                    // case-insensitive comparison
                    auto entry_filename = entry.path().filename().string();
                    auto part_filename = part.string();
                    if (String::iequal(entry_filename, part_filename))
                    {
                        full_path /= entry.path().filename();
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    return std::nullopt;
                }
            }
        }
        return full_path;
    }
} // namespace RC::File