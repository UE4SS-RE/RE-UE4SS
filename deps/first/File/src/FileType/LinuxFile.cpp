#ifdef __linux__
#include <fstream>

#include <File/File.hpp>
#include <File/FileType/LinuxFile.hpp>
#include <File/HandleTemplate.hpp>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <codecvt>
#include <cstring>
#include <locale>

#include <Helpers/String.hpp>
#include <fmt/core.h>

namespace RC::File
{
    static auto errno_string() -> std::string
    {
        char buf[256]{};
        // GNU strerror_r may return a pointer to a static string instead of filling buf
        return std::string{strerror_r(errno, buf, sizeof(buf))};
    }

    // Proper UTF-8 -> UTF-16 decode. Helpers' to_u16string(const std::string&) widens
    // byte-wise and therefore mangles non-ASCII; this mirrors to_string(std::u16string_view)
    // which encodes UTF-16 -> UTF-8 with the same facet.
    static auto utf8_to_u16string(const std::string& input) -> std::u16string
    {
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter{};
        return converter.from_bytes(input);
#if __clang__
#pragma clang diagnostic pop
#endif
    }

    auto LinuxFile::is_valid() noexcept -> bool
    {
        return m_fd >= 0;
    }

    auto LinuxFile::invalidate_file() noexcept -> void
    {
        m_fd = -1;
        m_memory_map = nullptr;
        m_map_size = 0;
    }

    auto LinuxFile::delete_file(const std::filesystem::path& file_path_and_name) -> void
    {
        if (::unlink(file_path_and_name.c_str()) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::delete_file] Was unable to delete file, error: {}", errno_string()))
        }
    }

    auto LinuxFile::delete_file() -> void
    {
        if (m_is_file_open)
        {
            close_file();
        }

        delete_file(m_file_path_and_name);
    }

    auto LinuxFile::set_file(int new_fd) -> void
    {
        m_fd = new_fd;
    }

    auto LinuxFile::get_file() -> int
    {
        return m_fd;
    }

    auto LinuxFile::set_is_file_open(bool new_is_open) -> void
    {
        m_is_file_open = new_is_open;
    }

    auto LinuxFile::get_raw_handle() noexcept -> void*
    {
        return reinterpret_cast<void*>(static_cast<intptr_t>(m_fd));
    }

    auto LinuxFile::get_file_path() const noexcept -> const std::filesystem::path&
    {
        return m_file_path_and_name;
    }

    auto LinuxFile::set_serialization_output_file(const std::filesystem::path& output_file) noexcept -> void
    {
        m_serialization_file_path_and_name = output_file;
    }

    auto LinuxFile::serialization_file_exists() -> bool
    {
        return std::filesystem::exists(m_serialization_file_path_and_name);
    }

    template <typename DataType>
    auto write_to_file(LinuxFile& file, const DataType* data, size_t num_bytes_to_write) -> void
    {
        if (!file.is_file_open())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::write_to_file] Tried writing to file but the file is not open")
        }

        const auto* bytes = reinterpret_cast<const uint8_t*>(data);
        size_t total_written{};
        while (total_written < num_bytes_to_write)
        {
            ssize_t written = ::write(file.get_file(), bytes + total_written, num_bytes_to_write - total_written);
            if (written < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                THROW_INTERNAL_FILE_ERROR(
                        fmt::format("[LinuxFile::write_to_file] Tried writing to file but was unable to complete operation. error: {}", errno_string()))
            }
            total_written += static_cast<size_t>(written);
        }
    }

    // Serialization Format (Linux):
    // device
    // inode
    // mtime_sec
    // mtime_nsec
    // file_size
    // user_data
    auto LinuxFile::serialize_identifying_properties() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_identifying_properties]: Path & file name for serialization file is empty, please call "
                                      "'set_serialization_output_file'")
        }

        struct stat file_info{};
        if (::fstat(m_fd, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::serialize_identifying_properties] fstat failed, error: {}", errno_string()))
        }

        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLongLong, .data_ulonglong = static_cast<unsigned long long>(file_info.st_dev)},
                       true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLongLong, .data_ulonglong = static_cast<unsigned long long>(file_info.st_ino)},
                       true);
        serialize_item(GenericItemData{.data_type = GenericDataType::SignedLongLong, .data_longlong = static_cast<signed long long>(file_info.st_mtim.tv_sec)},
                       true);
        serialize_item(GenericItemData{.data_type = GenericDataType::SignedLongLong, .data_longlong = static_cast<signed long long>(file_info.st_mtim.tv_nsec)},
                       true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLongLong, .data_ulonglong = static_cast<unsigned long long>(file_info.st_size)},
                       true);
    }

    auto LinuxFile::deserialize_identifying_properties() -> void
    {
        m_identifying_properties.device = *static_cast<uint64_t*>(get_serialized_item(sizeof(uint64_t), true));
        m_identifying_properties.inode = *static_cast<uint64_t*>(get_serialized_item(sizeof(uint64_t), true));
        m_identifying_properties.mtime_sec = *static_cast<int64_t*>(get_serialized_item(sizeof(int64_t), true));
        m_identifying_properties.mtime_nsec = *static_cast<int64_t*>(get_serialized_item(sizeof(int64_t), true));
        m_identifying_properties.file_size = *static_cast<uint64_t*>(get_serialized_item(sizeof(uint64_t), true));

        // The cached identifying properties should be inaccessible by user-code,
        // so let's make sure that the next serialized item to be deserialized by the user-code
        // is the first item after the last identifying property item
        m_offset_to_next_serialized_item = sizeof(IdentifyingProperties);

        m_has_cached_identifying_properties = true;
    }

    auto LinuxFile::is_deserialized_and_live_equal() -> bool
    {
        if (!m_has_cached_identifying_properties)
        {
            if (!std::filesystem::exists(m_serialization_file_path_and_name))
            {
                return false;
            }

            deserialize_identifying_properties();
        }

        struct stat live_info{};
        if (::fstat(m_fd, &live_info) != 0)
        {
            return false;
        }

        return static_cast<uint64_t>(live_info.st_dev) == m_identifying_properties.device &&
               static_cast<uint64_t>(live_info.st_ino) == m_identifying_properties.inode &&
               static_cast<int64_t>(live_info.st_mtim.tv_sec) == m_identifying_properties.mtime_sec &&
               static_cast<int64_t>(live_info.st_mtim.tv_nsec) == m_identifying_properties.mtime_nsec &&
               static_cast<uint64_t>(live_info.st_size) == m_identifying_properties.file_size;
    }

    auto LinuxFile::invalidate_serialization() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::invalidate_serialization] Could not invalidate serialization file because "
                                      "'m_serialization_file_path_and_name' was empty, please call 'set_serialization_output_file'")
        }

        if (std::filesystem::exists(m_serialization_file_path_and_name))
        {
            delete_file(m_serialization_file_path_and_name);
        }
    }

    template <typename DataType>
    auto serialize_typed_item(DataType data, Handle& output_file) -> void
    {
        write_to_file(output_file.get_underlying_type(), &data, sizeof(DataType));
    }

    auto LinuxFile::serialize_item(const GenericItemData& data, bool is_internal_item) -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR(
                    "[LinuxFile::serialize_item]: Path & file name for serialization file is empty, please call 'set_serialization_output_file'")
        }

        if (!serialization_file_exists() && !is_internal_item)
        {
            // If the serialization cache file doesn't exist & this is not an identifying property item,
            // then we need to serialize the identifying properties before continuing
            serialize_identifying_properties();
        }

        Handle serialization_file = open(m_serialization_file_path_and_name, OpenFor::Appending, OverwriteExistingFile::No, CreateIfNonExistent::Yes);

        switch (data.data_type)
        {
        case GenericDataType::UnsignedLong:
            serialize_typed_item<unsigned long>(data.data_ulong, serialization_file);
            serialization_file.get_underlying_type().m_offset_to_next_serialized_item += sizeof(unsigned long);
            break;
        case GenericDataType::SignedLong:
            serialize_typed_item<signed long>(data.data_long, serialization_file);
            serialization_file.get_underlying_type().m_offset_to_next_serialized_item += sizeof(signed long);
            break;
        case GenericDataType::UnsignedLongLong:
            serialize_typed_item<unsigned long long>(data.data_ulonglong, serialization_file);
            serialization_file.get_underlying_type().m_offset_to_next_serialized_item += sizeof(unsigned long long);
            break;
        case GenericDataType::SignedLongLong:
            serialize_typed_item<signed long long>(data.data_longlong, serialization_file);
            serialization_file.get_underlying_type().m_offset_to_next_serialized_item += sizeof(signed long long);
            break;
        }

        serialization_file.close();
    }

    auto LinuxFile::get_serialized_item(size_t data_size, bool is_internal_item) -> void*
    {
        if (!m_has_cache_in_memory)
        {
            if (m_serialization_file_path_and_name.empty())
            {
                THROW_INTERNAL_FILE_ERROR(
                        "[LinuxFile::get_serialized_item]: Path & file name for serialization file is empty, please call 'set_serialization_output_file'")
            }

            Handle cache_file = open(m_serialization_file_path_and_name);

            ssize_t bytes_read = ::read(cache_file.get_underlying_type().get_file(), &m_cache, cache_size);
            if (bytes_read < 0)
            {
                THROW_INTERNAL_FILE_ERROR(
                        fmt::format("[LinuxFile::get_serialized_item] Tried deserializing file but was unable to complete operation. error: {}",
                                    errno_string()))
            }

            cache_file.close();

            m_has_cache_in_memory = true;
        }

        if (!m_has_cached_identifying_properties && !is_internal_item)
        {
            deserialize_identifying_properties();
        }

        void* data_ptr = &m_cache[m_offset_to_next_serialized_item];
        m_offset_to_next_serialized_item += data_size;
        return data_ptr;
    }

    auto LinuxFile::close_current_file() -> void
    {
        close_file();
    }

    auto LinuxFile::create_all_directories(const std::filesystem::path& file_name_and_path) -> void
    {
        if (file_name_and_path.parent_path().empty())
        {
            return;
        }

        try
        {
            std::filesystem::create_directories(file_name_and_path.parent_path());
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::create_all_directories] Tried creating directories '{}' but encountered an error. error: {}",
                                                  file_name_and_path.string(),
                                                  e.what()))
        }
    }

    auto LinuxFile::close_file() -> void
    {
        if (m_memory_map)
        {
            if (::munmap(m_memory_map, m_map_size) != 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::close_file] Was unable to unmap file, error: {}", errno_string()))
            }
            m_memory_map = nullptr;
            m_map_size = 0;
        }

        if (!is_valid() || !is_file_open())
        {
            return;
        }

        if (::close(m_fd) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::close_file] Was unable to close file, {}", errno_string()))
        }

        set_is_file_open(false);
        m_fd = -1;
    }

    auto LinuxFile::is_file_open() const -> bool
    {
        return m_is_file_open;
    }

    auto LinuxFile::write_string_to_file(StringViewType string_to_write) -> void
    {
        // StringType is UTF-16 (char16_t) on Linux; files on disk are UTF-8.
        auto string_converted_to_utf8 = to_string(string_to_write);
        write_to_file(*this, string_converted_to_utf8.data(), string_converted_to_utf8.size());
    }

    auto LinuxFile::is_same_as(LinuxFile& other_file) -> bool
    {
        struct stat file_info{};
        if (::fstat(m_fd, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::is_same_as] Tried retrieving file information by fd. {}", errno_string()))
        }

        struct stat other_file_info{};
        if (::fstat(other_file.get_file(), &other_file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::is_same_as] Tried retrieving file information by fd. {}", errno_string()))
        }

        return file_info.st_dev == other_file_info.st_dev && file_info.st_ino == other_file_info.st_ino;
    }

    auto LinuxFile::read_all() const -> StringType
    {
        std::ifstream stream{get_file_path(), std::ios::in | std::ios::binary};
        if (!stream)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::read_all] Tried to read entire file but returned error {}", errno))
        }

        std::string file_contents;
        stream.seekg(0, std::ios::end);
        auto size = stream.tellg();
        if (size == -1)
        {
            return {};
        }
        file_contents.resize(static_cast<size_t>(size));
        stream.seekg(0, std::ios::beg);
        stream.read(file_contents.data(), static_cast<std::streamsize>(file_contents.size()));
        stream.close();

        // Strip the UTF-8 BOM if it exists
        if (file_contents.size() >= 3 && static_cast<uint8_t>(file_contents[0]) == 0xEF && static_cast<uint8_t>(file_contents[1]) == 0xBB &&
            static_cast<uint8_t>(file_contents[2]) == 0xBF)
        {
            file_contents.erase(0, 3);
        }

        // Files on disk are UTF-8; StringType is UTF-16 (char16_t) on Linux.
        return utf8_to_u16string(file_contents);
    }

    auto LinuxFile::memory_map() -> std::span<uint8_t>
    {
        int prot{};
        switch (m_open_properties.open_for)
        {
        case OpenFor::Writing:
        case OpenFor::Appending:
        case OpenFor::ReadWrite:
            prot = PROT_READ | PROT_WRITE;
            break;
        case OpenFor::Reading:
            prot = PROT_READ;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::memory_map] Tried to memory map file but 'm_open_properties' contains invalid data.")
        }

        struct stat file_info{};
        if (::fstat(m_fd, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::memory_map] fstat failed. {}", errno_string()))
        }
        if (file_info.st_size == 0)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::memory_map] Tried to memory map an empty file.")
        }

        void* mapped = ::mmap(nullptr, static_cast<size_t>(file_info.st_size), prot, MAP_SHARED, m_fd, 0);
        if (mapped == MAP_FAILED)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::memory_map] mmap failed. {}", errno_string()))
        }

        m_memory_map = static_cast<uint8_t*>(mapped);
        m_map_size = static_cast<size_t>(file_info.st_size);
        return std::span(m_memory_map, m_map_size);
    }

    auto LinuxFile::open_file(const std::filesystem::path& file_name_and_path, const OpenProperties& open_properties) -> LinuxFile
    {
        if (file_name_and_path.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::open_file] Tried to open file but file_name_and_path was empty.")
        }

        int flags{};
        switch (open_properties.open_for)
        {
        case OpenFor::Writing:
            flags = O_WRONLY;
            break;
        case OpenFor::Appending:
            flags = O_WRONLY | O_APPEND;
            break;
        case OpenFor::Reading:
            flags = O_RDONLY;
            break;
        case OpenFor::ReadWrite:
            flags = O_RDWR;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::open_file] Tried to open file but received invalid data for the 'OpenFor' parameter.")
        }

        if (open_properties.overwrite_existing_file == OverwriteExistingFile::Yes)
        {
            create_all_directories(file_name_and_path);
            flags |= O_CREAT | O_TRUNC;
        }
        else if (open_properties.create_if_non_existent == CreateIfNonExistent::Yes)
        {
            create_all_directories(file_name_and_path);
            flags |= O_CREAT;
        }

        LinuxFile file{};
        file.set_file(::open(file_name_and_path.c_str(), flags, 0644));

        if (file.get_file() < 0)
        {
            std::string_view open_type = open_properties.open_for == OpenFor::Writing || open_properties.open_for == OpenFor::Appending ? "writing" : "reading";

            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::open_file] Tried opening file for {} but encountered an error. Path & File: {} | error: {}\n",
                                                  open_type,
                                                  file_name_and_path.string(),
                                                  errno_string()))
        }

        file.m_file_path_and_name = file_name_and_path;
        file.set_is_file_open(true);
        file.m_open_properties = open_properties;

        return file;
    }
} // namespace RC::File

#endif // ifdef __linux__
