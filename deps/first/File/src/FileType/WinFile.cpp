#ifdef _WIN32
#include <fstream>
#include <limits>
#include <memory>

#include <File/File.hpp>
#include <File/FileType/WinFile.hpp>
#include <File/HandleTemplate.hpp>

#define NOMINMAX
#include <Windows.h>
#include <Helpers/String.hpp>
#include <Helpers/SysError.hpp>
#ifdef TEXT
#undef TEXT
#endif
#include <fmt/core.h>

namespace RC::File
{
    namespace
    {
        // Keep mapping metadata outside WinFile to preserve its original ABI.
        struct WinFileMappingState
        {
            uint8_t* View{};
            size_t Size{};
        };
    }

    auto WinFile::is_valid() noexcept -> bool
    {
        return m_file != nullptr;
    }

    auto WinFile::invalidate_file() noexcept -> void
    {
        // Relinquish ownership without releasing resources.
        // The receiving handle becomes responsible for closing them.
        m_file = nullptr;
        m_map_handle = nullptr;
        m_memory_map = nullptr;
        m_is_file_open = false;
    }

    auto WinFile::delete_file(const std::filesystem::path& file_path_and_name) -> void
    {
        if constexpr (sizeof(CharType) > 1)
        {
            if (DeleteFileW(file_path_and_name.wstring().c_str()) == 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::delete_file] Was unable to delete file, error: {}",
                                                      to_string(SysError(GetLastError())).c_str()))
            }
        }
        else
        {
            if (DeleteFileA(file_path_and_name.string().c_str()) == 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::delete_file] Was unable to delete file, error: {}",
                                                      to_string(SysError(GetLastError())).c_str()))
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
        return m_file;
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

        const auto* Bytes = reinterpret_cast<const unsigned char*>(data);
        DWORD TotalWritten{};

        // WriteFile() can succeed without transferring the entire buffer.
        // Continue from the last completed byte rather than silently
        // accepting a partial write as successful.
        while (TotalWritten < num_bytes_to_write)
        {
            DWORD BytesWritten{};
            if (!WriteFile(file.get_file(), Bytes + TotalWritten, num_bytes_to_write - TotalWritten, &BytesWritten, nullptr))
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::write_to_file] Tried writing to file but was unable to complete operation. error: {}",
                                                      to_string(SysError(GetLastError())).c_str()))
            }

            // A successful call reporting no progress would otherwise
            // leave us retrying the same operation indefinitely.
            if (BytesWritten == 0)
            {
                THROW_INTERNAL_FILE_ERROR("[WinFile::write_to_file] Write made no progress")
            }

            TotalWritten += BytesWritten;
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

            deserialize_identifying_properties();
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
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::get_serialized_item] Tried deserializing file but was unable to complete operation. error: {}",
                                                      to_string(SysError(GetLastError())).c_str()))
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
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::create_all_directories] Tried creating directories '{}' but encountered an error. error: {}",
                                                  file_name_and_path.string(),
                                                  e.what()))
        }
    }

    auto WinFile::close_file() -> void
    {
        DWORD UnmapError{};
        auto* MappingState = reinterpret_cast<WinFileMappingState*>(m_memory_map);
        if (MappingState && MappingState->View)
        {
            if (UnmapViewOfFile(MappingState->View) == 0)
            {
                UnmapError = GetLastError();
            }
            else
            {
                MappingState->View = nullptr;
            }
        }

        DWORD MappingCloseError{};
        if (m_map_handle)
        {
            if (CloseHandle(m_map_handle) == 0)
            {
                MappingCloseError = GetLastError();
            }
            else
            {
                m_map_handle = nullptr;
            }
        }

        // Retain metadata while either mapping resource remains owned.
        // This preserves the information needed to retry failed cleanup.
        if (MappingState && !MappingState->View && !m_map_handle)
        {
            delete MappingState;
            m_memory_map = nullptr;
        }

        DWORD FileCloseError{};
        if (is_valid() && is_file_open())
        {
            if (CloseHandle(m_file) == 0)
            {
                FileCloseError = GetLastError();
            }
            else
            {
                // The handle is no longer owned after a successful close.
                // Clear both representations of its state so is_valid() and
                // get_raw_handle() cannot report a handle that has been released.
                m_file = nullptr;
                set_is_file_open(false);
            }
        }

        // Views, mapping handles and file handles have independent lifetimes.
        // Attempt all cleanup operations before reporting failures, preserving
        // each error because subsequent Windows API calls may change it.
        if (UnmapError)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::close_file] Was unable to unmap file, error: {}", to_string(SysError(UnmapError)).c_str()))
        }

        if (MappingCloseError)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::close_file] Was unable to close map handle, {}", to_string(SysError(MappingCloseError)).c_str()))
        }

        if (FileCloseError)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::close_file] Was unable to close file, {}", to_string(SysError(FileCloseError)).c_str()))
        }
    }

    auto WinFile::is_file_open() const -> bool
    {
        return m_is_file_open;
    }

    auto WinFile::write_string_to_file(StringViewType string_to_write) -> void
    {
        int string_size =
                WideCharToMultiByte(CP_UTF8, 0, FromCharTypePtr<wchar_t>(string_to_write.data()), static_cast<int>(string_to_write.size()), NULL, 0, NULL, NULL);
        if (string_size == 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::write_string_to_file] Tried writing string to file but string_size was 0. {}",
                                                  to_string(SysError(GetLastError())).c_str()))
        }

        std::string string_converted_to_utf8(string_size, 0);
        if (WideCharToMultiByte(CP_UTF8,
                                0,
                                FromCharTypePtr<wchar_t>(string_to_write.data()),
                                static_cast<int>(string_to_write.size()),
                                &string_converted_to_utf8[0],
                                string_size,
                                NULL,
                                NULL) == 0)
        {
            THROW_INTERNAL_FILE_ERROR(
                    fmt::format("[WinFile::write_string_to_file] Tried writing string to file but could not convert to utf-8. {}",
                                to_string(SysError(GetLastError())).c_str()))
        }

        write_to_file(*this, string_converted_to_utf8.c_str(), string_size);
    }

    auto WinFile::is_same_as(WinFile& other_file) -> bool
    {
        BY_HANDLE_FILE_INFORMATION file_info{};
        if (GetFileInformationByHandle(m_file, &file_info) == 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::is_same_as] Tried retrieving file information by handle. {}", to_string(SysError(GetLastError())).c_str()))
        }

        BY_HANDLE_FILE_INFORMATION other_file_info{};
        if (GetFileInformationByHandle(other_file.get_file(), &other_file_info) == 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::is_same_as] Tried retrieving file information by handle. {}", to_string(SysError(GetLastError())).c_str()))
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
        std::ifstream Stream{get_file_path(), std::ios::in | std::ios::binary};
        if (!Stream)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::read_all] Tried to read entire file but returned error {}", errno))
        }
        else
        {
            std::string FileContents;
            constexpr std::streamsize BufferSize = 4096;
            char Buffer[BufferSize];

            // Read raw bytes rather than wide characters. Files are UTF-8,
            // while StringType uses UTF-16 on Windows.
            while (Stream.read(Buffer, BufferSize))
            {
                FileContents.append(Buffer, static_cast<size_t>(BufferSize));
            }

            // Preserve the final partial chunk and reject genuine I/O failures.
            FileContents.append(Buffer, static_cast<size_t>(Stream.gcount()));

            if (Stream.bad() || !Stream.eof())
            {
                THROW_INTERNAL_FILE_ERROR("[WinFile::read_all] Unable to read complete file")
            }

            Stream.close();

            // Strip the BOM if it exists
            if (FileContents.size() >= 3 &&
                static_cast<unsigned char>(FileContents[0]) == 0xEF &&
                static_cast<unsigned char>(FileContents[1]) == 0xBB &&
                static_cast<unsigned char>(FileContents[2]) == 0xBF)
            {
                // BOM: UTF-8
                FileContents.erase(0, 3);
            }

            if (FileContents.empty())
            {
                return {};
            }

            // MultiByteToWideChar accepts an int-sized input length.
            if (FileContents.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
            {
                THROW_INTERNAL_FILE_ERROR("[WinFile::read_all] File exceeds UTF-8 conversion input limit")
            }

            const int InputSize = static_cast<int>(FileContents.size());

            const int WideSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                                      FileContents.data(), InputSize, nullptr, 0);
            if (WideSize == 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::read_all] Invalid UTF-8 input: {}",
                                                      to_string(SysError(GetLastError())).c_str()))
            }

            StringType Decoded(static_cast<size_t>(WideSize), CharType{});

            const int Converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                                      FileContents.data(), InputSize,
                                                      FromCharTypePtr<wchar_t>(Decoded.data()), WideSize);
            if (Converted == 0)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::read_all] UTF-8 decoding failed: {}",
                                                      to_string(SysError(GetLastError())).c_str()))
            }

            if (Converted != WideSize)
            {
                THROW_INTERNAL_FILE_ERROR("[WinFile::read_all] UTF-8 conversion returned an unexpected length")
            }

            return Decoded;
        }
    }

    auto WinFile::memory_map() -> std::span<uint8_t>
    {
        // The file owns at most one active mapped view. Repeated calls
        // must reuse it rather than losing the address needed by
        // UnmapViewOfFile() during cleanup.
        //
        // The mapping remains valid until the file is closed and does
        // not automatically resize if the underlying file changes.
        if (m_memory_map)
        {
            auto* MappingState = reinterpret_cast<WinFileMappingState*>(m_memory_map);
            if (MappingState->View)
            {
                return std::span(MappingState->View, MappingState->Size);
            }
        }

        DWORD handle_desired_access, mapping_desired_access;

        switch (m_open_properties.open_for)
        {
        case OpenFor::Writing:
        case OpenFor::Appending:
            // PAGE_READWRITE requires both GENERIC_READ and GENERIC_WRITE.
            // These modes deliberately open without read access, so they
            // cannot support a writable file mapping.
            //
            // Preserve ordinary file-opening permissions and require
            // OpenFor::ReadWrite when a writable mapping is needed.
            THROW_INTERNAL_FILE_ERROR("[WinFile::memory_map] Writable mapping requires OpenFor::ReadWrite.")

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

        // MapViewOfFile() may have failed on an earlier attempt after
        // CreateFileMapping() succeeded. Reuse that mapping object
        // rather than overwriting and leaking its handle.
        if (!m_map_handle)
        {
            LARGE_INTEGER FileSize{};
            if (!GetFileSizeEx(get_file(), &FileSize))
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::memory_map] GetFileSizeEx failed: {}",
                                          to_string(SysError(GetLastError())).c_str()))
            }

            // Allocate metadata before acquiring the Windows mapping handle.
            // Allocation failure must not leave native resources stranded.
            auto MappingState = std::make_unique<WinFileMappingState>();
            MappingState->Size = static_cast<size_t>(FileSize.QuadPart);

            m_map_handle = CreateFileMapping(get_raw_handle(), nullptr, handle_desired_access, 0, 0, nullptr);
            if (!m_map_handle)
            {
                THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::memory_map] Tried to memory map file but 'CreateFileMapping' returned {}",
                                          to_string(SysError(GetLastError())).c_str()))
            }

            m_memory_map = reinterpret_cast<uint8_t*>(MappingState.release());
        }

        auto* MappingState = reinterpret_cast<WinFileMappingState*>(m_memory_map);
        MappingState->View = static_cast<uint8_t*>(MapViewOfFile(m_map_handle, mapping_desired_access, 0, 0, 0));
        if (!MappingState->View)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::memory_map] Tried to memory map file but 'MapViewOfFile' returned {}",
                                      to_string(SysError(GetLastError())).c_str()))
        }

        return std::span(MappingState->View, MappingState->Size);
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

            THROW_INTERNAL_FILE_ERROR(fmt::format("[WinFile::open_file] Tried opening file for {} but encountered an error. Path & File: {} | error: {}\n",
                                                  open_type, file_name_and_path.string(), to_string(SysError(GetLastError())).c_str()))
        }

        file.m_file_path_and_name = file_name_and_path;
        file.set_is_file_open(true);
        file.m_open_properties = open_properties;

        return file;
    }
} // namespace RC::File

#endif // ifdef _WIN32
