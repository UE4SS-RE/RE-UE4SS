#include <fstream>
#include <codecvt>

#include <File/File.hpp>
#include <File/FileType/StdFile.hpp>
#include <File/HandleTemplate.hpp>

#include <sys/mman.h>
#include <fcntl.h>

namespace RC::File
{
    auto StdFile::create_all_directories(const std::filesystem::path& file_name_and_path) -> void
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
            THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::create_all_directories] Tried creating directories '{}' but encountered an error. Error: {}",
                                                  file_name_and_path.string(),
                                                  e.what()))
        }
    }

    auto StdFile::calc_open_flags(const std::filesystem::path& file_name_and_path, const OpenProperties& open_properties) -> int
    {

        int flags{};
        int rwflags{};
        switch (open_properties.open_for)
        {
        case OpenFor::Writing:
            rwflags = O_WRONLY;
            break;
        case OpenFor::Appending:
            rwflags = O_WRONLY;
            flags = O_APPEND;
            break;
        case OpenFor::Reading:
            rwflags = O_RDONLY;
            break;
        default:
            printf("open_for: %d\n", open_properties.open_for);
            THROW_INTERNAL_FILE_ERROR("[StdFile::open_file] Tried to open file but received invalid data for the 'OpenFor' parameter.")
        }

        if (open_properties.overwrite_existing_file == OverwriteExistingFile::Yes)
        {
            flags |= O_CREAT | O_TRUNC;
            if (rwflags == O_RDONLY)
            {
                rwflags = O_RDWR;
            }
            create_all_directories(file_name_and_path);
        }
        else if (open_properties.create_if_non_existent == CreateIfNonExistent::Yes)
        {
            flags |= O_CREAT;
            if (rwflags == O_RDONLY)
            {
                rwflags = O_RDWR;
            }
            create_all_directories(file_name_and_path);
        }
        else
        {
            if (!std::filesystem::exists(file_name_and_path))
            {
                printf("file_name_and_path: %s not exists\n", file_name_and_path.string().c_str());
                THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::open_file] Tried opening file but file does not exist: {}", file_name_and_path.string()))
            }
        }

        return rwflags | flags;
    }

    auto StdFile::is_valid() noexcept -> bool
    {
        return m_file > 0;
    }

    auto StdFile::invalidate_file() noexcept -> void
    {
        m_file = 0;
    }

    auto StdFile::delete_file(const std::filesystem::path& file_path_and_name) -> void
    {
        std::filesystem::remove(file_path_and_name);
    }

    auto StdFile::delete_file() -> void
    {
        if (m_is_file_open)
        {
            close_file();
        }

        delete_file(m_file_path_and_name);
    }

    auto StdFile::set_file(HANDLE new_file) -> void
    {
        m_file = new_file;
    }

    auto StdFile::get_file() -> HANDLE
    {
        return m_file;
    }

    auto StdFile::set_is_file_open(bool new_is_open) -> void
    {
        m_is_file_open = new_is_open;
    }

    auto StdFile::get_raw_handle() noexcept -> void*
    {
        long a = m_file;
        return (void*)(a);
    }

    auto StdFile::get_file_path() const noexcept -> const std::filesystem::path&
    {
        return m_file_path_and_name;
    }

    auto StdFile::set_serialization_output_file(const std::filesystem::path& output_file) noexcept -> void
    {
        m_serialization_file_path_and_name = output_file;
    }

    auto StdFile::serialization_file_exists() -> bool
    {
        return std::filesystem::exists(m_serialization_file_path_and_name);
    }

    template <typename DataType>
    auto write_to_file(StdFile& file, DataType* data, size_t num_bytes_to_write) -> void
    {
        if (!file.is_file_open())
        {
            THROW_INTERNAL_FILE_ERROR("[StdFile::write_to_file] Tried writing to file but the file is not open")
        }

        size_t bytes_written = write(file.get_file(), data, sizeof(DataType) * num_bytes_to_write);
        if (bytes_written < 0)
        {
            THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::write_to_file] Tried writing to file but was unable to complete operation. Error: {}", bytes_written));
        }
    }

    auto StdFile::serialize_identifying_properties() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[StdFile::serialize_identifying_properties]: Path & file name for serialization file is empty, please call "
                                      "'set_serialization_output_file'")
        }

        struct stat file_info
        {
        };
        fstat(m_file, &file_info);

        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.st_dev}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.st_ino}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.st_mode}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.st_nlink}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.st_uid}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.st_gid}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.st_rdev}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::SignedLong, .data_long = file_info.st_size}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::SignedLong, .data_long = file_info.st_atime}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::SignedLong, .data_long = file_info.st_mtime}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::SignedLong, .data_long = file_info.st_ctime}, true);
    }

    auto StdFile::deserialize_identifying_properties() -> void
    {
        m_identifying_properties.file_stat.st_dev = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_stat.st_ino = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_stat.st_mode = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_stat.st_nlink = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_stat.st_uid = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_stat.st_gid = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_stat.st_rdev = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_stat.st_size = *static_cast<signed long*>(get_serialized_item(sizeof(signed long), true));
        m_identifying_properties.file_stat.st_atime = *static_cast<signed long*>(get_serialized_item(sizeof(signed long), true));
        m_identifying_properties.file_stat.st_mtime = *static_cast<signed long*>(get_serialized_item(sizeof(signed long), true));
        m_identifying_properties.file_stat.st_ctime = *static_cast<signed long*>(get_serialized_item(sizeof(signed long), true));
        m_offset_to_next_serialized_item = 11 * 8;

        m_has_cached_identifying_properties = true;
    }

    auto StdFile::is_deserialized_and_live_equal() -> bool
    {
        if (!m_has_cached_identifying_properties)
        {
            if (!std::filesystem::exists(m_serialization_file_path_and_name))
            {
                return false;
            }
            else
            {
                deserialize_identifying_properties();
            }
        }

        return true;
    }

    auto StdFile::invalidate_serialization() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[StdFile::invalidate_serialization] Could not invalidate serialization file because "
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

    auto StdFile::serialize_item(const GenericItemData& data, bool is_internal_item) -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR(
                    "[StdFile::serialize_item]: Path & file name for serialization file is empty, please call 'set_serialization_output_file'")
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

    auto StdFile::get_serialized_item(size_t data_size, bool is_internal_item) -> void*
    {
        if (!m_has_cache_in_memory)
        {
            if (m_serialization_file_path_and_name.empty())
            {
                THROW_INTERNAL_FILE_ERROR(
                        "[StdFile::get_serialized_item]: Path & file name for serialization file is empty, please call 'set_serialization_output_file'")
            }

            Handle cache_file = open(m_serialization_file_path_and_name);
            auto bytes_read = read((int)cache_file.get_raw_handle(), &m_cache, cache_size);
            if (bytes_read < 0)
            {
                THROW_INTERNAL_FILE_ERROR(
                        std::format("[StdFile::get_serialized_item] Tried deserializing file but was unable to complete operation. Error: {}", bytes_read))
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

    auto StdFile::close_current_file() -> void
    {
        close_file();
    }

    auto StdFile::close_file() -> void
    {
        if (m_memory_map)
        {
            size_t aligned_size = m_memory_map_size & ~(sysconf(_SC_PAGE_SIZE) - 1);
            if (munmap(m_memory_map, aligned_size) != 0)
            {
                THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::close_file] Was unable to unmap file, error: {}", errno))
            }
            else
            {
                m_memory_map = nullptr;
                m_memory_map_size = 0;
            }
        }

        if (!is_valid() || !is_file_open())
        {
            return;
        }

        if (close(m_file) == -1)
        {
            THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::close_file] Was unable to close file, error: {}", errno))
        }
        else
        {
            m_file = 0;
            set_is_file_open(false);
        }
    }

    auto StdFile::is_file_open() const -> bool
    {
        return m_is_file_open;
    }

    auto StdFile::write_string_to_file(UEStringViewType string_to_write) -> void
    {
        try
        {
            static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter{};
            auto utf8_string = converter.to_bytes(string_to_write.begin(), string_to_write.begin() + string_to_write.size());
            write_file_string_to_file(utf8_string);
        }
        catch (const std::exception& e)
        {
            THROW_INTERNAL_FILE_ERROR(
                    std::format("[StdFile::write_string_to_file] Tried writing string to file but could not convert to utf-8. Error: {}", e.what()))
        }
    }

    auto StdFile::write_file_string_to_file(StringViewType string_to_write) -> void
    {
        try
        {
            write_to_file(*this, string_to_write.data(), string_to_write.size());
        }
        catch (const std::exception& e)
        {
            THROW_INTERNAL_FILE_ERROR(
                    std::format("[StdFile::write_string_to_file] Tried writing string to file but could not convert to utf-8. Error: {}", e.what()))
        }
    }

    auto StdFile::is_same_as(StdFile& other_file) -> bool
    {
        struct stat file_info
        {
        };
        if (fstat(m_file, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::is_same_as] Tried retrieving file information by handle. Error: {}", errno))
        }

        struct stat other_file_info
        {
        };
        if (fstat(other_file.get_file(), &other_file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::is_same_as] Tried retrieving file information by handle. Error: {}", errno))
        }

        if (file_info.st_dev != other_file_info.st_dev)
        {
            return false;
        }

        if (file_info.st_ino != other_file_info.st_ino)
        {
            return false;
        }

        if (file_info.st_mode != other_file_info.st_mode)
        {
            return false;
        }

        if (file_info.st_nlink != other_file_info.st_nlink)
        {
            return false;
        }

        if (file_info.st_uid != other_file_info.st_uid)
        {
            return false;
        }

        if (file_info.st_gid != other_file_info.st_gid)
        {
            return false;
        }

        if (file_info.st_rdev != other_file_info.st_rdev)
        {
            return false;
        }

        if (file_info.st_size != other_file_info.st_size)
        {
            return false;
        }

        if (file_info.st_mtime != other_file_info.st_mtime)
        {
            return false;
        }

        if (file_info.st_ctime != other_file_info.st_ctime)
        {
            return false;
        }

        return true;
    }

    auto StdFile::read_file_all() const -> StringType
    {
        auto file_name_and_path = get_file_path();
        int flags = O_RDONLY;
        int readfd = ::open(file_name_and_path.string().c_str(), flags);
        if (readfd < 0)
        {
            THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::read_all] Tried to read entire file but returned error {}", errno))
        }
        else
        {
            // Strip the BOM if it exists
            File::CharType bom[3]{};
            read(readfd, bom, 3);
            off_t start = 0;
            if ((unsigned char)bom[0] == 0xEF && (unsigned char)bom[1] == 0xBB && (unsigned char)bom[2] == 0xBF)
            {
                // BOM: UTF-8
                start = 3;
            }

            File::StringType file_contents;
            ssize_t size = lseek(readfd, 0, SEEK_END);
            lseek(readfd, start, SEEK_SET);
            if (size == -1)
            {
                return {};
            }
            file_contents.resize(size - start);
            read(readfd, &file_contents[0], size - start);
            close(readfd);
            return file_contents;
        }
    }

    auto StdFile::read_all() const -> UEStringType
    {
        auto utf8_string = read_file_all();
        return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(utf8_string);
    }

    auto StdFile::memory_map() -> std::span<uint8_t>
    {
        int flags = PROT_NONE;
        switch (m_open_properties.open_for)
        {
        case OpenFor::Writing:
        case OpenFor::Appending:
            flags |= PROT_WRITE;
        case OpenFor::Reading:
            flags |= PROT_READ;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[StdFile::memory_map] Tried to memory map file but 'm_open_properties' contains invalid data.")
        }

        // seek to get size to map while keep original file position
        auto original_position = lseek(m_file, 0, SEEK_CUR);
        auto file_size = lseek(m_file, 0, SEEK_END);
        lseek(m_file, original_position, SEEK_SET);

        auto res = mmap(nullptr, file_size, flags, MAP_SHARED, m_file, 0);
        if (res == (void*)-1)
        {
            THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::memory_map] Tried to memory map file but 'mmap' returned error: {}", errno))
        }

        m_memory_map = (uint8_t*)res;
        m_memory_map_size = file_size;

        return std::span(m_memory_map, file_size);
    }

    auto StdFile::open_file(const std::filesystem::path& file_name_and_path, const OpenProperties& open_properties) -> StdFile
    {
        // Reminder: std::filesystem::canonical() will get the full path & file name on the drive
        // It only works if the directory already exists so check that first
        // Should CreateFile take the canonical path ?

        if (file_name_and_path.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[StdFile::open_file] Tried to open file but file_name_and_path was empty.")
        }

        int flags = calc_open_flags(file_name_and_path, open_properties);

        StdFile file{};

        // This very badly named API may create a new file or it may not but it will always open a file (unless there's an error)
        int fd = ::open(file_name_and_path.string().c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

        if (fd > 0)
        {
            file.set_file(fd);
        }
        else
        {
            std::string_view open_type = open_properties.open_for == OpenFor::Writing || open_properties.open_for == OpenFor::Appending ? "writing" : "reading";

            fprintf(stderr, "open file %s failed, error: %d\n", file_name_and_path.string().c_str(), errno);
            THROW_INTERNAL_FILE_ERROR(std::format("[StdFile::open_file] Tried opening file for {} but encountered an error. Path & File: {} | errno = {}\n",
                                                  open_type,
                                                  file_name_and_path.string(),
                                                  errno))
        }

        file.m_file_path_and_name = file_name_and_path;
        file.set_is_file_open(true);
        file.m_open_properties = open_properties;

        return file;
    }
} // namespace RC::File
