#include <fstream>

#include <File/File.hpp>
#include <File/FileType/WinFile.hpp>
#include <File/HandleTemplate.hpp>

#define NOMINMAX
#include <Windows.h>
#ifdef TEXT
#undef TEXT
#endif
#include <fmt/core.h>

namespace RC::File
{
    auto WinFile::is_valid() noexcept -> bool
    {
        return m_file != nullptr;
    }

    auto WinFile::invalidate_file() noexcept -> void
    {
        m_file = nullptr;
        m_map_handle = nullptr;
        m_memory_map = nullptr;
    }

    auto WinFile::delete_file(const std::filesystem::path& file_path_and_name) -> void
    {
        if constexpr (sizeof(CharType) > 1)
        {
            if (DeleteFileW(file_path_and_name.wstring().c_str()) == 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::delete_file] Was unable to delete file, error: {}", GetLastError()))
            }
        }
        else
        {
            if (DeleteFileA(file_path_and_name.string().c_str()) != 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::delete_file] Was unable to delete file, error: {}", GetLastError()))
            }
        }
    }

    auto WinFile::delete_file() -> void
    {
        if (m_is_file_open)
        {
            close_file();
        }

        delete_file(m_file_path_and_name);
    }

    auto WinFile::set_file(HANDLE new_file) -> void
    {
        m_file = new_file;
    }

    auto WinFile::get_file() -> HANDLE
    {
        return m_file;
    }

    auto WinFile::set_is_file_open(bool new_is_open) -> void
    {
        m_is_file_open = new_is_open;
    }

    auto WinFile::get_raw_handle() noexcept -> void*
    {
        return static_cast<void*>(m_file);
    }

    auto WinFile::get_file_path() const noexcept -> const std::filesystem::path&
    {
        return m_file_path_and_name;
    }

    auto WinFile::set_serialization_output_file(const std::filesystem::path& output_file) noexcept -> void
    {
        m_serialization_file_path_and_name = output_file;
    }

    auto WinFile::serialization_file_exists() -> bool
    {
        return std::filesystem::exists(m_serialization_file_path_and_name);
    }

    template <typename DataType>
    auto write_to_file(WinFile& file, DataType* data, DWORD num_bytes_to_write) -> void
    {
        if (!file.is_file_open())
        {
            THROW_INTERNAL_FILE_ERROR("[WinFile::write_to_file] Tried writing to file but the file is not open")
        }

        DWORD bytes_written{};
        if (!WriteFile(file.get_file(), data, num_bytes_to_write, &bytes_written, nullptr))
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::write_to_file] Tried writing to file but was unable to complete operation. Error: {}", GetLastError()))
        }
    }

    // Serialization Format (Windows):
    // volume_serial_number
    // file_index_low
    // file_index_high
    // creation_time_low
    // creation_time_high
    // last_write_time_low
    // last_write_time_high
    // file_size_low
    // file_size_high
    // user_data
    auto WinFile::serialize_identifying_properties() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[WinFile::serialize_identifying_properties]: Path & file name for serialization file is empty, please call "
                                      "'set_serialization_output_file'")
        }

        BY_HANDLE_FILE_INFORMATION file_info{};
        GetFileInformationByHandle(m_file, &file_info);

        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.dwVolumeSerialNumber}, true);

        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.nFileIndexLow}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.nFileIndexHigh}, true);

        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.ftCreationTime.dwLowDateTime}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.ftCreationTime.dwHighDateTime}, true);

        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.ftLastWriteTime.dwLowDateTime}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.ftLastWriteTime.dwHighDateTime}, true);

        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.nFileSizeLow}, true);
        serialize_item(GenericItemData{.data_type = GenericDataType::UnsignedLong, .data_ulong = file_info.nFileSizeHigh}, true);
    }

    auto WinFile::deserialize_identifying_properties() -> void
    {
        m_identifying_properties.volume_serial_number = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_index_low = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_index_high = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.creation_time_low = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.creation_time_high = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.last_write_time_low = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.last_write_time_high = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_size_low = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));
        m_identifying_properties.file_size_high = *static_cast<unsigned long*>(get_serialized_item(sizeof(unsigned long), true));

        // The cached identifying properties should be inaccessible by user-code,
        // so let's make sure that the next serialized item to be deserialized by the user-code
        // is the first item after the last identifying property item
        m_offset_to_next_serialized_item = sizeof(IdentifyingProperties);

        m_has_cached_identifying_properties = true;
    }

    auto WinFile::is_deserialized_and_live_equal() -> bool
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

        BY_HANDLE_FILE_INFORMATION live_info{};
        GetFileInformationByHandle(m_file, &live_info);

        if (live_info.dwVolumeSerialNumber != m_identifying_properties.volume_serial_number)
        {
            return false;
        }
        if (live_info.nFileIndexLow != m_identifying_properties.file_index_low)
        {
            return false;
        }
        if (live_info.nFileIndexHigh != m_identifying_properties.file_index_high)
        {
            return false;
        }
        if (live_info.ftLastWriteTime.dwLowDateTime != m_identifying_properties.last_write_time_low)
        {
            return false;
        }
        if (live_info.ftLastWriteTime.dwHighDateTime != m_identifying_properties.last_write_time_high)
        {
            return false;
        }
        if (live_info.ftCreationTime.dwLowDateTime != m_identifying_properties.creation_time_low)
        {
            return false;
        }
        if (live_info.ftCreationTime.dwHighDateTime != m_identifying_properties.creation_time_high)
        {
            return false;
        }
        if (live_info.nFileSizeLow != m_identifying_properties.file_size_low)
        {
            return false;
        }
        if (live_info.nFileSizeHigh != m_identifying_properties.file_size_high)
        {
            return false;
        }
        return true;
    }

    auto WinFile::invalidate_serialization() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[WinFile::invalidate_serialization] Could not invalidate serialization file because "
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

    auto WinFile::serialize_item(const GenericItemData& data, bool is_internal_item) -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR(
                    "[WinFile::serialize_item]: Path & file name for serialization file is empty, please call 'set_serialization_output_file'")
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

    auto WinFile::get_serialized_item(size_t data_size, bool is_internal_item) -> void*
    {
        if (!m_has_cache_in_memory)
        {
            if (m_serialization_file_path_and_name.empty())
            {
                THROW_INTERNAL_FILE_ERROR(
                        "[WinFile::get_serialized_item]: Path & file name for serialization file is empty, please call 'set_serialization_output_file'")
            }

            Handle cache_file = open(m_serialization_file_path_and_name);

            DWORD bytes_read{};
            auto res = ReadFile(cache_file.get_raw_handle(), &m_cache, cache_size, &bytes_read, nullptr);
            if (res == 0)
            {
                THROW_INTERNAL_FILE_ERROR(
                        fmt::format("[WinFile::get_serialized_item] Tried deserializing file but was unable to complete operation. Error: {}", GetLastError()))
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

    auto WinFile::close_current_file() -> void
    {
        close_file();
    }

    auto WinFile::create_all_directories(const std::filesystem::path& file_name_and_path) -> void
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
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::create_all_directories] Tried creating directories '{}' but encountered an error. Error: {}",
                                                  file_name_and_path.string(),
                                                  e.what()))
        }
    }

    auto WinFile::close_file() -> void
    {
        if (m_memory_map)
        {
            if (UnmapViewOfFile(m_memory_map) == 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::close_file] Was unable to unmap file, error: {}", GetLastError()))
            }
            else
            {
                m_memory_map = nullptr;
            }
        }

        if (m_map_handle)
        {
            if (CloseHandle(m_map_handle) == 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::close_file] Was unable to close map handle, error: {}", GetLastError()))
            }
            else
            {
                m_map_handle = nullptr;
            }
        }

        if (!is_valid() || !is_file_open())
        {
            return;
        }

        if (CloseHandle(m_file) == 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::close_file] Was unable to close file, error: {}", GetLastError()))
        }
        else
        {
            set_is_file_open(false);
        }
    }

    auto WinFile::is_file_open() const -> bool
    {
        return m_is_file_open;
    }

    auto WinFile::write_string_to_file(StringViewType string_to_write) -> void
    {
        int string_size = WideCharToMultiByte(CP_UTF8, 0, string_to_write.data(), static_cast<int>(string_to_write.size()), NULL, 0, NULL, NULL);
        if (string_size == 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::write_string_to_file] Tried writing string to file but string_size was 0. Error: {}", GetLastError()))
        }

        std::string string_converted_to_utf8(string_size, 0);
        if (WideCharToMultiByte(CP_UTF8, 0, string_to_write.data(), static_cast<int>(string_to_write.size()), &string_converted_to_utf8[0], string_size, NULL, NULL) ==
            0)
        {
            THROW_INTERNAL_FILE_ERROR(
                    fmt::format("[WinFile::write_string_to_file] Tried writing string to file but could not convert to utf-8. Error: {}", GetLastError()))
        }

        write_to_file(*this, string_converted_to_utf8.c_str(), string_size);
    }

    auto WinFile::is_same_as(WinFile& other_file) -> bool
    {
        BY_HANDLE_FILE_INFORMATION file_info{};
        if (GetFileInformationByHandle(m_file, &file_info) == 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::is_same_as] Tried retrieving file information by handle. Error: {}", GetLastError()))
        }

        BY_HANDLE_FILE_INFORMATION other_file_info{};
        if (GetFileInformationByHandle(other_file.get_file(), &other_file_info) == 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::is_same_as] Tried retrieving file information by handle. Error: {}", GetLastError()))
        }

        if (file_info.dwVolumeSerialNumber != other_file_info.dwVolumeSerialNumber)
        {
            return false;
        }
        if (file_info.nFileIndexLow != other_file_info.nFileIndexLow)
        {
            return false;
        }
        if (file_info.nFileIndexHigh != other_file_info.nFileIndexHigh)
        {
            return false;
        }
        if (file_info.ftLastWriteTime.dwLowDateTime != other_file_info.ftLastWriteTime.dwLowDateTime)
        {
            return false;
        }
        if (file_info.ftLastWriteTime.dwHighDateTime != other_file_info.ftLastWriteTime.dwHighDateTime)
        {
            return false;
        }
        if (file_info.ftCreationTime.dwLowDateTime != other_file_info.ftCreationTime.dwLowDateTime)
        {
            return false;
        }
        if (file_info.ftCreationTime.dwHighDateTime != other_file_info.ftCreationTime.dwHighDateTime)
        {
            return false;
        }
        if (file_info.nFileSizeLow != other_file_info.nFileSizeLow)
        {
            return false;
        }
        if (file_info.nFileSizeHigh != other_file_info.nFileSizeHigh)
        {
            return false;
        }
        return true;
    }

    auto WinFile::read_all() const -> StringType
    {
        StreamType stream{get_file_path(), std::ios::in | std::ios::binary};
        if (!stream)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::read_all] Tried to read entire file but returned error {}", errno))
        }
        else
        {
            // Strip the BOM if it exists
            File::StreamType::off_type start{};
            File::CharType bom[3]{};
            stream.read(bom, 3);
            if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
            {
                // BOM: UTF-8
                start = 3;
            }

            StringType file_contents;
            stream.seekg(0, std::ios::end);
            auto size = stream.tellg();
            if (size == -1)
            {
                return {};
            }
            file_contents.resize(size);
            stream.seekg(start, std::ios::beg);
            stream.read(&file_contents[0], file_contents.size());
            stream.close();
            return file_contents;
        }
    }

    auto WinFile::memory_map() -> std::span<uint8_t>
    {
        DWORD handle_desired_access{};
        DWORD mapping_desired_access{};
        switch (m_open_properties.open_for)
        {
        case OpenFor::Writing:
        case OpenFor::Appending:
        case OpenFor::ReadWrite:
            handle_desired_access = PAGE_READWRITE;
            mapping_desired_access = FILE_MAP_WRITE;
            break;
        case OpenFor::Reading:
            handle_desired_access = PAGE_READONLY;
            mapping_desired_access = FILE_MAP_READ;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[WinFile::memory_map] Tried to memory map file but 'm_open_properties' contains invalid data.")
        }

        m_map_handle = CreateFileMapping(get_raw_handle(), nullptr, handle_desired_access, 0, 0, nullptr);
        if (!m_map_handle)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::memory_map] Tried to memory map file but 'CreateFileMapping' returned error: {}", GetLastError()))
        }

        m_memory_map = static_cast<uint8_t*>(MapViewOfFile(m_map_handle, mapping_desired_access, 0, 0, 0));
        if (!m_memory_map)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::memory_map] Tried to memory map file but 'MapViewOfFile' returned error: {}", GetLastError()))
        }

        MEMORY_BASIC_INFORMATION buffer{};
        VirtualQuery(m_memory_map, &buffer, sizeof(decltype(buffer)));
        return std::span(m_memory_map, buffer.RegionSize);
    }

    auto WinFile::open_file(const std::filesystem::path& file_name_and_path, const OpenProperties& open_properties) -> WinFile
    {
        // Reminder: std::filesystem::canonical() will get the full path & file name on the drive
        // It only works if the directory already exists so check that first
        // Should CreateFile take the canonical path ?

        if (file_name_and_path.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[WinFile::open_file] Tried to open file but file_name_and_path was empty.")
        }

        DWORD desired_access;
        switch (open_properties.open_for)
        {
        case OpenFor::Writing:
            desired_access = GENERIC_WRITE;
            break;
        case OpenFor::Appending:
            desired_access = FILE_APPEND_DATA;
            break;
        case OpenFor::Reading:
            desired_access = GENERIC_READ;
            break;
        case OpenFor::ReadWrite:
            desired_access = GENERIC_READ | GENERIC_WRITE;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[WinFile::open_file] Tried to open file but received invalid data for the 'OpenFor' parameter.")
        }

        DWORD creation_disposition;
        if (open_properties.overwrite_existing_file == OverwriteExistingFile::Yes)
        {
            create_all_directories(file_name_and_path);
            creation_disposition = CREATE_ALWAYS;
        }
        else if (open_properties.create_if_non_existent == CreateIfNonExistent::Yes)
        {
            create_all_directories(file_name_and_path);
            creation_disposition = OPEN_ALWAYS;
        }
        else
        {
            creation_disposition = OPEN_EXISTING;
        }

        WinFile file{};

        // This very badly named API may create a new file or it may not but it will always open a file (unless there's an error)
        if constexpr (sizeof(CharType) > 1)
        {
            file.set_file(CreateFileW(file_name_and_path.wstring().c_str(),
                                      desired_access,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      nullptr,
                                      creation_disposition,
                                      FILE_ATTRIBUTE_NORMAL,
                                      nullptr));
        }
        else
        {
            file.set_file(CreateFileA(file_name_and_path.string().c_str(),
                                      desired_access,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      nullptr,
                                      creation_disposition,
                                      FILE_ATTRIBUTE_NORMAL,
                                      nullptr));
        }

        if (file.get_file() == INVALID_HANDLE_VALUE)
        {
            std::string_view open_type = open_properties.open_for == OpenFor::Writing || open_properties.open_for == OpenFor::Appending ? "writing" : "reading";

            DWORD error = GetLastError();
            if (error == 2)
            {
                throw FileNotFoundException{fmt::format("File not found: {}", file_name_and_path.filename().string())};
            }
            else if (error == 3)
            {
                throw FileNotFoundException{fmt::format("Path not found: {}", file_name_and_path.filename().string())};
            }
            else
            {
                THROW_INTERNAL_FILE_ERROR(
                        fmt::format("[WinFile::open_file] Tried opening file for {} but encountered an error. Path & File: {} | GetLastError() = {}\n",
                                    open_type,
                                    file_name_and_path.string(),
                                    error))
            }
        }

        file.m_file_path_and_name = file_name_and_path;
        file.set_is_file_open(true);
        file.m_open_properties = open_properties;

        return file;
    }
} // namespace RC::File
