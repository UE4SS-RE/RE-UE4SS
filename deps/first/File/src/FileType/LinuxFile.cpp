#ifdef __linux__
#include <fstream>

#include <File/File.hpp>
#include <File/FileType/LinuxFile.hpp>
#include <File/HandleTemplate.hpp>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <Helpers/String.hpp>
#include <fmt/core.h>

namespace RC::File
{
    // Native cache bytes rely on the supported little-endian LP64 target.
    static_assert(sizeof(int) == 4 && sizeof(long) == 8 && sizeof(long long) == 8);
    static_assert(std::endian::native == std::endian::little);

    // Accept a saved errno so cleanup can continue through multiple system
    // calls without losing the error reported by an earlier failure.
    static auto errno_string(int error_code = errno) -> std::string
    {
        char buf[256]{};
        // GNU strerror_r may return a pointer to a static string instead of filling buf
        return std::string{strerror_r(error_code, buf, sizeof(buf))};
    }

    // Reads exactly byte_count bytes. Returns false if EOF is reached early.
    // Actual I/O failures are reported separately from truncated data.
    [[nodiscard]] static auto read_cache_exact(int fd, std::uint8_t* destination, std::size_t byte_count) -> bool
    {
        std::size_t total_read{};

        while (total_read < byte_count)
        {
            const ssize_t result = ::read(fd, destination + total_read, byte_count - total_read);

            if (result == 0)
            {
                return false;
            }

            if (result < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::read_cache_exact] Read failed: {}", errno_string()))
            }

            total_read += static_cast<std::size_t>(result);
        }

        return true;
    }

    namespace
    {
        // Owns a cache descriptor independently of the public File handle.
        class CacheDescriptor
        {
            int m_file_descriptor{-1};

          public:
            explicit CacheDescriptor(int fd) noexcept : m_file_descriptor(fd)
            {
            }

            CacheDescriptor(const CacheDescriptor&) = delete;
            auto operator=(const CacheDescriptor&) -> CacheDescriptor& = delete;

            ~CacheDescriptor() noexcept
            {
                if (m_file_descriptor >= 0)
                {
                    ::close(m_file_descriptor);
                }
            }
        };
    } // namespace

    // Helpers::to_u16string widens bytes without decoding UTF-8.
    // Decode leniently, replacing malformed sequences with U+FFFD to match Windows.
    static auto utf8_to_u16_string(const std::string& input) -> std::u16string
    {
        std::u16string decoded;
        decoded.reserve(input.size());

        for (size_t index = 0; index < input.size();)
        {
            std::uint32_t code_point = static_cast<std::uint8_t>(input[index++]);
            if (code_point < 0x80)
            {
                decoded.push_back(static_cast<char16_t>(code_point));
                continue;
            }

            size_t remaining{};
            std::uint32_t minimum{};
            if (code_point >= 0xC2 && code_point <= 0xDF)
            {
                code_point &= 0x1F;
                remaining = 1;
                minimum = 0x80;
            }
            else if (code_point >= 0xE0 && code_point <= 0xEF)
            {
                code_point &= 0x0F;
                remaining = 2;
                minimum = 0x800;
            }
            else if (code_point >= 0xF0 && code_point <= 0xF4)
            {
                code_point &= 0x07;
                remaining = 3;
                minimum = 0x10000;
            }
            else
            {
                decoded.push_back(u'\uFFFD');
                continue;
            }

            while (remaining > 0 && index < input.size() && (static_cast<std::uint8_t>(input[index]) & 0xC0) == 0x80)
            {
                code_point = (code_point << 6) | (static_cast<std::uint8_t>(input[index++]) & 0x3F);
                --remaining;
            }

            // Require shortest-form encodings of Unicode scalar values, excluding surrogates.
            if (remaining != 0 || code_point < minimum || code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF))
            {
                decoded.push_back(u'\uFFFD');
            }
            else if (code_point <= 0xFFFF)
            {
                decoded.push_back(static_cast<char16_t>(code_point));
            }
            else
            {
                code_point -= 0x10000;
                decoded.push_back(static_cast<char16_t>(0xD800 + (code_point >> 10)));
                decoded.push_back(static_cast<char16_t>(0xDC00 + (code_point & 0x3FF)));
            }
        }

        return decoded;
    }

    auto LinuxFile::is_valid() noexcept -> bool
    {
        return m_file_descriptor >= 0;
    }

    auto LinuxFile::invalidate_file() noexcept -> void
    {
        // Relinquish ownership without releasing resources.
        // The receiving handle becomes responsible for closing them.
        m_file_descriptor = -1;
        m_memory_map = nullptr;
        m_map_size = 0;
        m_is_file_open = false;
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

    auto LinuxFile::set_file(int new_file_descriptor) -> void
    {
        m_file_descriptor = new_file_descriptor;
    }

    auto LinuxFile::get_file() -> int
    {
        return m_file_descriptor;
    }

    auto LinuxFile::set_is_file_open(bool new_is_open) -> void
    {
        m_is_file_open = new_is_open;
    }

    auto LinuxFile::get_raw_handle() noexcept -> void*
    {
        return reinterpret_cast<void*>(static_cast<intptr_t>(m_file_descriptor));
    }

    auto LinuxFile::get_file_path() const noexcept -> const std::filesystem::path&
    {
        return m_file_path_and_name;
    }

    auto LinuxFile::set_serialization_output_file(const std::filesystem::path& output_file) noexcept -> void
    {
        m_serialization_file_path_and_name = output_file;
        reset_serialization_state();
    }

    auto LinuxFile::serialization_file_exists() -> bool
    {
        return std::filesystem::exists(m_serialization_file_path_and_name);
    }

    auto LinuxFile::reset_serialization_state() noexcept -> void
    {
        m_cache_bytes.clear();
        m_cached_properties.reset();
        m_next_record_index = 0;
    }

    auto LinuxFile::load_serialization_cache() -> bool
    {
        if (m_cached_properties)
        {
            return true;
        }

        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::load_serialization_cache] Serialization file path is empty")
        }

        const int raw_fd = ::open(m_serialization_file_path_and_name.c_str(), O_RDONLY | O_CLOEXEC);
        if (raw_fd < 0)
        {
            if (errno == ENOENT)
            {
                return false;
            }

            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::load_serialization_cache] Open failed: {}", errno_string()))
        }
        CacheDescriptor fd{raw_fd};

        struct stat file_info{};
        if (::fstat(raw_fd, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::load_serialization_cache] fstat failed: {}", errno_string()))
        }

        if (!S_ISREG(file_info.st_mode))
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::load_serialization_cache] Cache path is not a regular file")
        }

        if (file_info.st_size < static_cast<off_t>(sizeof(IdentifyingProperties)) ||
            (file_info.st_size - sizeof(IdentifyingProperties)) % sizeof(std::uint64_t) != 0)
        {
            return false;
        }

        std::vector<std::uint8_t> bytes(static_cast<size_t>(file_info.st_size));
        if (!read_cache_exact(raw_fd, bytes.data(), bytes.size()))
        {
            return false;
        }

        m_cache_bytes = std::move(bytes);
        std::memcpy(&m_cached_properties.emplace(), m_cache_bytes.data(), sizeof(IdentifyingProperties));
        return true;
    }

    static auto write_to_file(LinuxFile& file, const void* data, size_t num_bytes_to_write) -> void
    {
        if (!file.is_file_open())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::write_to_file] Tried writing to file but the file is not open")
        }

        const auto* bytes = static_cast<const uint8_t*>(data);
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
            // A zero-byte write makes no progress. Without this check,
            // the loop would retry indefinitely.
            if (written == 0)
            {
                THROW_INTERNAL_FILE_ERROR("[LinuxFile::write_to_file] Write made no progress")
            }

            total_written += static_cast<size_t>(written);
        }
    }

    // Machine-local native header followed by eight-byte native integer slots.
    // Explicit header serialization truncates the cache, including user records.
    auto LinuxFile::serialize_identifying_properties() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_identifying_properties] Serialization file path is empty")
        }

        struct stat file_info{};
        if (::fstat(m_file_descriptor, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::serialize_identifying_properties] fstat failed: {}", errno_string()))
        }
        const IdentifyingProperties properties{static_cast<std::uint64_t>(file_info.st_dev),
                                               static_cast<std::uint64_t>(file_info.st_ino),
                                               static_cast<std::int64_t>(file_info.st_mtim.tv_sec),
                                               static_cast<std::int64_t>(file_info.st_mtim.tv_nsec),
                                               static_cast<std::int64_t>(file_info.st_size)};
        reset_serialization_state();
        auto output = open(m_serialization_file_path_and_name, OpenFor::Writing, OverwriteExistingFile::Yes);
        write_to_file(output.get_underlying_type(), &properties, sizeof(properties));
        output.close();
    }

    auto LinuxFile::deserialize_identifying_properties() -> void
    {
        if (!load_serialization_cache())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::deserialize_identifying_properties] Serialization cache is missing or malformed")
        }
    }

    // Header reads and identity checks never consume or rewind user records.
    auto LinuxFile::is_deserialized_and_live_equal() -> bool
    {
        if (!load_serialization_cache())
        {
            return false;
        }

        struct stat live_info{};
        if (::fstat(m_file_descriptor, &live_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::is_deserialized_and_live_equal] fstat failed: {}", errno_string()))
        }

        const auto& cached = *m_cached_properties;
        return static_cast<std::uint64_t>(live_info.st_dev) == cached.device && static_cast<std::uint64_t>(live_info.st_ino) == cached.inode &&
               static_cast<std::int64_t>(live_info.st_mtim.tv_sec) == cached.modification_time_seconds &&
               static_cast<std::int64_t>(live_info.st_mtim.tv_nsec) == cached.modification_time_nanoseconds &&
               static_cast<std::int64_t>(live_info.st_size) == cached.file_size;
    }

    auto LinuxFile::invalidate_serialization() -> void
    {
        reset_serialization_state();
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::invalidate_serialization] Serialization file path is empty")
        }
        if (::unlink(m_serialization_file_path_and_name.c_str()) != 0 && errno != ENOENT)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::invalidate_serialization] Unable to delete cache: {}", errno_string()))
        }
    }

    auto LinuxFile::serialize_item(const GenericItemData& data, bool is_internal_item) -> void
    {
        if (is_internal_item)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_item] Internal-item serialization is unsupported")
        }
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_item] Serialization file path is empty")
        }

        const void* value{};
        switch (data.data_type)
        {
        case GenericDataType::UnsignedLong:
            value = &data.data_ulong;
            break;
        case GenericDataType::SignedLong:
            value = &data.data_long;
            break;
        case GenericDataType::UnsignedLongLong:
            value = &data.data_ulonglong;
            break;
        case GenericDataType::SignedLongLong:
            value = &data.data_longlong;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_item] Unsupported integer type")
        }

        if (!serialization_file_exists())
        {
            serialize_identifying_properties();
        }
        // Set the loaded cache aside, so a failure anywhere below leaves it unloaded
        // and the next read reloads it. The cursor is kept either way.
        auto cache_bytes = std::exchange(m_cache_bytes, {});
        const auto cached_properties = std::exchange(m_cached_properties, std::nullopt);
        auto output = open(m_serialization_file_path_and_name, OpenFor::Appending);
        write_to_file(output.get_underlying_type(), value, sizeof(std::uint64_t));
        struct stat file_info{};
        const bool size_known = ::fstat(output.get_underlying_type().get_file(), &file_info) == 0;
        output.close();

        // Extend the loaded cache rather than rereading the whole file on the next read.
        // If the file isn't exactly the loaded bytes plus this record, something else
        // has written to it, so leave the cache unloaded.
        if (cached_properties && size_known && static_cast<size_t>(file_info.st_size) == cache_bytes.size() + sizeof(std::uint64_t))
        {
            const auto* bytes = static_cast<const std::uint8_t*>(value);
            cache_bytes.insert(cache_bytes.end(), bytes, bytes + sizeof(std::uint64_t));
            m_cache_bytes = std::move(cache_bytes);
            m_cached_properties = cached_properties;
        }
    }

    auto LinuxFile::get_serialized_item(size_t data_size, bool is_internal_item) -> void*
    {
        if (is_internal_item)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::get_serialized_item] Internal-item deserialization is unsupported")
        }
        if (data_size != sizeof(std::uint32_t) && data_size != sizeof(std::uint64_t))
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::get_serialized_item] Unsupported integer width")
        }
        if (!load_serialization_cache())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::get_serialized_item] Serialization cache is missing or malformed")
        }
        if (m_next_record_index >= (m_cache_bytes.size() - sizeof(IdentifyingProperties)) / sizeof(std::uint64_t))
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::get_serialized_item] No more serialized records")
        }

        // On little-endian LP64, the first four bytes also represent a matched int.
        // Return owned, aligned byte storage; callers copy it into their integer.
        std::memcpy(m_return_storage.data(),
                    m_cache_bytes.data() + sizeof(IdentifyingProperties) + m_next_record_index * sizeof(std::uint64_t),
                    m_return_storage.size());
        ++m_next_record_index;
        return m_return_storage.data();
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
        int unmap_error{};
        if (m_memory_map)
        {
            if (::munmap(m_memory_map, m_map_size) != 0)
            {
                unmap_error = errno;
            }
            else
            {
                m_memory_map = nullptr;
                m_map_size = 0;
            }
        }

        int close_error{};
        if (is_valid() && is_file_open())
        {
            const int fd = m_file_descriptor;

            // Linux may release the descriptor even when close() reports an
            // error. Invalidate it before calling close() so a later cleanup
            // cannot accidentally close a descriptor reused elsewhere.
            m_file_descriptor = -1;
            m_is_file_open = false;

            if (::close(fd) != 0)
            {
                close_error = errno;
            }
        }

        // Mapping and descriptor lifetimes are independent. Attempt both
        // cleanup operations before reporting an error to the caller.
        if (unmap_error)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::close_file] Was unable to unmap file, error: {}", errno_string(unmap_error)))
        }

        if (close_error)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::close_file] Was unable to close file, {}", errno_string(close_error)))
        }
    }

    auto LinuxFile::is_file_open() const -> bool
    {
        return m_is_file_open;
    }

    // Write UTF-8 from UTF-16 input, replacing unpaired surrogates with U+FFFD to match Windows.
    auto LinuxFile::write_string_to_file(StringViewType string_to_write) -> void
    {
        std::string string_converted_to_utf8;
        // Three bytes per UTF-16 unit also covers a four-byte surrogate pair.
        string_converted_to_utf8.reserve(string_to_write.size() * 3);

        for (size_t index = 0; index < string_to_write.size(); ++index)
        {
            std::uint32_t code_point = string_to_write[index];
            if (code_point >= 0xD800 && code_point <= 0xDBFF)
            {
                if (index + 1 < string_to_write.size() && string_to_write[index + 1] >= 0xDC00 && string_to_write[index + 1] <= 0xDFFF)
                {
                    code_point = 0x10000 + ((code_point - 0xD800) << 10) + (string_to_write[++index] - 0xDC00);
                }
                else
                {
                    code_point = 0xFFFD;
                }
            }
            else if (code_point >= 0xDC00 && code_point <= 0xDFFF)
            {
                code_point = 0xFFFD;
            }

            if (code_point < 0x80)
            {
                string_converted_to_utf8.push_back(static_cast<char>(code_point));
            }
            else if (code_point < 0x800)
            {
                string_converted_to_utf8.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
                string_converted_to_utf8.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            }
            else if (code_point < 0x10000)
            {
                string_converted_to_utf8.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
                string_converted_to_utf8.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
                string_converted_to_utf8.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            }
            else
            {
                string_converted_to_utf8.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
                string_converted_to_utf8.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
                string_converted_to_utf8.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
                string_converted_to_utf8.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
            }
        }

        write_to_file(*this, string_converted_to_utf8.data(), string_converted_to_utf8.size());
    }

    auto LinuxFile::is_same_as(LinuxFile& other_file) -> bool
    {
        struct stat file_info{};
        if (::fstat(m_file_descriptor, &file_info) != 0)
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
        constexpr std::streamsize buffer_size = 4096;
        char buffer[buffer_size];

        // Read raw bytes rather than wide characters. Files are UTF-8,
        // while StringType uses UTF-16 on Linux.
        while (stream.read(buffer, buffer_size))
        {
            file_contents.append(buffer, static_cast<size_t>(buffer_size));
        }

        // Preserve the final partial chunk and reject genuine I/O failures.
        file_contents.append(buffer, static_cast<size_t>(stream.gcount()));

        if (stream.bad() || !stream.eof())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::read_all] Unable to read complete file")
        }

        // Strip the UTF-8 BOM if it exists
        if (file_contents.size() >= 3 && static_cast<uint8_t>(file_contents[0]) == 0xEF && static_cast<uint8_t>(file_contents[1]) == 0xBB &&
            static_cast<uint8_t>(file_contents[2]) == 0xBF)
        {
            file_contents.erase(0, 3);
        }

        // Files on disk are UTF-8; StringType is UTF-16 (char16_t) on Linux.
        return utf8_to_u16_string(file_contents);
    }

    auto LinuxFile::memory_map() -> std::span<uint8_t>
    {
        // The file owns at most one active mapping. Repeated calls return
        // the existing view rather than creating another mapping and
        // losing the address required by munmap() during close.
        //
        // The mapping remains valid until the file is closed. It does not
        // automatically resize if the underlying file changes.
        if (m_memory_map)
        {
            return std::span(m_memory_map, m_map_size);
        }

        int prot{};
        switch (m_open_properties.open_for)
        {
        case OpenFor::Writing:
        case OpenFor::Appending:
            // A shared writable mapping requires a descriptor opened
            // for both reading and writing. These modes deliberately
            // open with O_WRONLY, which mmap() cannot use here.
            //
            // Do not change the permissions of ordinary file opens
            // merely to accommodate memory mapping.
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::memory_map] Writable mapping requires OpenFor::ReadWrite")

        case OpenFor::ReadWrite:
            prot = PROT_READ | PROT_WRITE;
            break;

        case OpenFor::Reading:
            prot = PROT_READ;
            break;

        default:
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::memory_map] Invalid file access mode")
        }

        struct stat file_info{};
        if (::fstat(m_file_descriptor, &file_info) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::memory_map] fstat failed. {}", errno_string()))
        }
        if (file_info.st_size == 0)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::memory_map] Tried to memory map an empty file.")
        }

        void* mapped = ::mmap(nullptr, static_cast<size_t>(file_info.st_size), prot, MAP_SHARED, m_file_descriptor, 0);
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

        // Do not allow ordinary File descriptors to survive exec().
        // Set this atomically at open time rather than using fcntl()
        // afterwards, which would introduce a descriptor-inheritance race.
        flags |= O_CLOEXEC;

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

        // Initialise potentially throwing C++ state before acquiring the
        // descriptor. Once open() succeeds, only non-throwing operations
        // and the non-throwing move constructor remain.
        file.m_file_path_and_name = file_name_and_path;
        file.m_open_properties = open_properties;

        file.set_file(::open(file_name_and_path.c_str(), flags, 0644));

        if (file.get_file() < 0)
        {
            std::string_view open_type = open_properties.open_for == OpenFor::Writing || open_properties.open_for == OpenFor::Appending ? "writing" : "reading";

            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::open_file] Tried opening file for {} but encountered an error. Path & File: {} | error: {}\n",
                                                  open_type,
                                                  file_name_and_path.string(),
                                                  errno_string()))
        }

        struct stat file_info{};
        if (::fstat(file.get_file(), &file_info) != 0)
        {
            const int stat_error = errno;
            ::close(file.get_file());
            file.set_file(-1);
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::open_file] fstat failed: {}", errno_string(stat_error)))
        }

        // Linux allows opening directories as files (you can open them with a
        // text editor even - but please don't!). Windows ordinarily does not allow
        // this and neither should we.
        if (S_ISDIR(file_info.st_mode))
        {
            ::close(file.get_file());
            file.set_file(-1);
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::open_file] Tried to open a directory as a file.")
        }

        file.set_is_file_open(true);

        return file;
    }
} // namespace RC::File

#endif // ifdef __linux__
