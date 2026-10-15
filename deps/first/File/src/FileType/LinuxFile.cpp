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
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <locale>
#include <stdexcept>
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
    static auto ErrnoString(int ErrorCode = errno) -> std::string
    {
        char Buf[256]{};
        // GNU strerror_r may return a pointer to a static string instead of filling Buf
        return std::string{strerror_r(ErrorCode, Buf, sizeof(Buf))};
    }

    // Reads exactly ByteCount bytes. Returns false if EOF is reached early.
    // Actual I/O failures are reported separately from truncated data.
    [[nodiscard]] static auto ReadCacheExact(int Fd, std::uint8_t* Destination, std::size_t ByteCount) -> bool
    {
        std::size_t TotalRead{};

        while (TotalRead < ByteCount)
        {
            const ssize_t Result = ::read(Fd, Destination + TotalRead, ByteCount - TotalRead);

            if (Result == 0)
            {
                return false;
            }

            if (Result < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::ReadCacheExact] Read failed: {}", ErrnoString()))
            }

            TotalRead += static_cast<std::size_t>(Result);
        }

        return true;
    }

    namespace
    {
        // Owns a cache descriptor independently of the public File handle.
        class CacheDescriptor
        {
            int FileDescriptor{-1};

          public:
            explicit CacheDescriptor(int Fd) noexcept : FileDescriptor(Fd)
            {
            }

            CacheDescriptor(const CacheDescriptor&) = delete;
            auto operator=(const CacheDescriptor&) -> CacheDescriptor& = delete;

            ~CacheDescriptor() noexcept
            {
                if (FileDescriptor >= 0)
                {
                    ::close(FileDescriptor);
                }
            }
        };
    } // namespace

    // Proper UTF-8 -> UTF-16 decode. Helpers' to_u16string(const std::string&) widens
    // byte-wise and therefore mangles non-ASCII; this mirrors to_string(std::u16string_view)
    // which encodes UTF-16 -> UTF-8 with the same facet.
    static auto Utf8ToU16String(const std::string& Input) -> std::u16string
    {
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> Converter{};
        std::u16string Decoded;

        try
        {
            Decoded = Converter.from_bytes(Input);
        }
        catch (const std::range_error& Error)
        {
            THROW_INTERNAL_FILE_ERROR(
                fmt::format("[LinuxFile::read_all] UTF-8 conversion failed: {}", Error.what()))
        }

        if (Converter.converted() != Input.size())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::read_all] Incomplete UTF-8 sequence")
        }

        return Decoded;
#if __clang__
#pragma clang diagnostic pop
#endif
    }

    auto LinuxFile::is_valid() noexcept -> bool
    {
        return FileDescriptor >= 0;
    }

    auto LinuxFile::invalidate_file() noexcept -> void
    {
        // Relinquish ownership without releasing resources.
        // The receiving handle becomes responsible for closing them.
        FileDescriptor = -1;
        m_memory_map = nullptr;
        MapSize = 0;
        m_is_file_open = false;
    }

    auto LinuxFile::delete_file(const std::filesystem::path& FilePathAndName) -> void
    {
        if (::unlink(FilePathAndName.c_str()) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::delete_file] Was unable to delete file, error: {}", ErrnoString()))
        }
    }

    auto LinuxFile::delete_file() -> void
    {
        if (m_is_file_open)
        {
            CloseFile();
        }

        delete_file(m_file_path_and_name);
    }

    auto LinuxFile::set_file(int NewFileDescriptor) -> void
    {
        FileDescriptor = NewFileDescriptor;
    }

    auto LinuxFile::get_file() -> int
    {
        return FileDescriptor;
    }

    auto LinuxFile::set_is_file_open(bool NewIsOpen) -> void
    {
        m_is_file_open = NewIsOpen;
    }

    auto LinuxFile::get_raw_handle() noexcept -> void*
    {
        return reinterpret_cast<void*>(static_cast<intptr_t>(FileDescriptor));
    }

    auto LinuxFile::get_file_path() const noexcept -> const std::filesystem::path&
    {
        return m_file_path_and_name;
    }

    auto LinuxFile::set_serialization_output_file(const std::filesystem::path& OutputFile) noexcept -> void
    {
        m_serialization_file_path_and_name = OutputFile;
        ResetSerializationState();
    }

    auto LinuxFile::serialization_file_exists() -> bool
    {
        return std::filesystem::exists(m_serialization_file_path_and_name);
    }

    auto LinuxFile::ResetSerializationState() noexcept -> void
    {
        CacheBytes.clear();
        CachedProperties.reset();
        NextRecordIndex = 0;
        ReturnStorage.fill(std::byte{0});
    }

    auto LinuxFile::LoadSerializationCache() -> bool
    {
        if (CachedProperties)
        {
            return true;
        }

        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::LoadSerializationCache] Serialization file path is empty")
        }

        const int RawFd = ::open(m_serialization_file_path_and_name.c_str(), O_RDONLY | O_CLOEXEC);
        if (RawFd < 0)
        {
            if (errno == ENOENT)
            {
                return false;
            }

            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::LoadSerializationCache] Open failed: {}", ErrnoString()))
        }
        CacheDescriptor Fd{RawFd};

        struct stat FileInfo{};
        if (::fstat(RawFd, &FileInfo) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::LoadSerializationCache] fstat failed: {}", ErrnoString()))
        }

        if (!S_ISREG(FileInfo.st_mode))
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::LoadSerializationCache] Cache path is not a regular file")
        }

        if (FileInfo.st_size < static_cast<off_t>(sizeof(IdentifyingProperties)) ||
            (FileInfo.st_size - sizeof(IdentifyingProperties)) % sizeof(std::uint64_t) != 0)
        {
            return false;
        }

        std::vector<std::uint8_t> Bytes(static_cast<size_t>(FileInfo.st_size));
        if (!ReadCacheExact(RawFd, Bytes.data(), Bytes.size()))
        {
            return false;
        }

        CacheBytes = std::move(Bytes);
        std::memcpy(&CachedProperties.emplace(), CacheBytes.data(), sizeof(IdentifyingProperties));
        return true;
    }

    template <typename DataType>
    auto WriteToFile(LinuxFile& File, const DataType* Data, size_t NumBytesToWrite) -> void
    {
        if (!File.is_file_open())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::write_to_file] Tried writing to file but the file is not open")
        }

        const auto* Bytes = reinterpret_cast<const uint8_t*>(Data);
        size_t TotalWritten{};
        while (TotalWritten < NumBytesToWrite)
        {
            ssize_t Written = ::write(File.get_file(), Bytes + TotalWritten, NumBytesToWrite - TotalWritten);
            if (Written < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                THROW_INTERNAL_FILE_ERROR(
                        fmt::format("[LinuxFile::write_to_file] Tried writing to file but was unable to complete operation. error: {}", ErrnoString()))
            }
            // A zero-byte write makes no progress. Without this check,
            // the loop would retry indefinitely.
            if (Written == 0)
            {
                THROW_INTERNAL_FILE_ERROR(
                    "[LinuxFile::write_to_file] Write made no progress")
            }

            TotalWritten += static_cast<size_t>(Written);
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

        struct stat FileInfo{};
        if (::fstat(FileDescriptor, &FileInfo) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::serialize_identifying_properties] fstat failed: {}", ErrnoString()))
        }
        const IdentifyingProperties Properties{static_cast<std::uint64_t>(FileInfo.st_dev),
                                              static_cast<std::uint64_t>(FileInfo.st_ino),
                                              static_cast<std::int64_t>(FileInfo.st_mtim.tv_sec),
                                              static_cast<std::int64_t>(FileInfo.st_mtim.tv_nsec),
                                              static_cast<std::int64_t>(FileInfo.st_size)};
        ResetSerializationState();
        auto Output = open(m_serialization_file_path_and_name, OpenFor::Writing, OverwriteExistingFile::Yes);
        WriteToFile(Output.get_underlying_type(), &Properties, sizeof(Properties));
        Output.close();
    }

    auto LinuxFile::deserialize_identifying_properties() -> void
    {
        if (!LoadSerializationCache())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::deserialize_identifying_properties] Serialization cache is missing or malformed")
        }
    }

    // Header reads and identity checks never consume or rewind user records.
    auto LinuxFile::is_deserialized_and_live_equal() -> bool
    {
        if (!LoadSerializationCache())
        {
            return false;
        }

        struct stat LiveInfo{};
        if (::fstat(FileDescriptor, &LiveInfo) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::is_deserialized_and_live_equal] fstat failed: {}", ErrnoString()))
        }

        const auto& Cached = *CachedProperties;
        return static_cast<std::uint64_t>(LiveInfo.st_dev) == Cached.Device && static_cast<std::uint64_t>(LiveInfo.st_ino) == Cached.Inode &&
               static_cast<std::int64_t>(LiveInfo.st_mtim.tv_sec) == Cached.ModificationTimeSeconds &&
               static_cast<std::int64_t>(LiveInfo.st_mtim.tv_nsec) == Cached.ModificationTimeNanoseconds &&
               static_cast<std::int64_t>(LiveInfo.st_size) == Cached.FileSize;
    }

    auto LinuxFile::invalidate_serialization() -> void
    {
        ResetSerializationState();
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::invalidate_serialization] Serialization file path is empty")
        }
        if (::unlink(m_serialization_file_path_and_name.c_str()) != 0 && errno != ENOENT)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::invalidate_serialization] Unable to delete cache: {}", ErrnoString()))
        }
    }

    auto LinuxFile::serialize_item(const GenericItemData& Data, bool IsInternalItem) -> void
    {
        if (IsInternalItem)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_item] Internal-item serialization is unsupported")
        }
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_item] Serialization file path is empty")
        }

        const void* Value{};
        switch (Data.data_type)
        {
        case GenericDataType::UnsignedLong:
            Value = &Data.data_ulong;
            break;
        case GenericDataType::SignedLong:
            Value = &Data.data_long;
            break;
        case GenericDataType::UnsignedLongLong:
            Value = &Data.data_ulonglong;
            break;
        case GenericDataType::SignedLongLong:
            Value = &Data.data_longlong;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::serialize_item] Unsupported integer type")
        }

        if (!serialization_file_exists())
        {
            serialize_identifying_properties();
        }
        const auto PreviousCursor = NextRecordIndex;
        // Discard loaded bytes before writing, including on a partial-write failure.
        ResetSerializationState();
        auto Output = open(m_serialization_file_path_and_name, OpenFor::Appending);
        WriteToFile(Output.get_underlying_type(), Value, sizeof(std::uint64_t));
        Output.close();
        NextRecordIndex = PreviousCursor;
    }

    auto LinuxFile::get_serialized_item(size_t DataSize, bool IsInternalItem) -> void*
    {
        if (IsInternalItem)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::get_serialized_item] Internal-item deserialization is unsupported")
        }
        if (DataSize != sizeof(std::uint32_t) && DataSize != sizeof(std::uint64_t))
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::get_serialized_item] Unsupported integer width")
        }
        if (!LoadSerializationCache())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::get_serialized_item] Serialization cache is missing or malformed")
        }
        if (NextRecordIndex >= (CacheBytes.size() - sizeof(IdentifyingProperties)) / sizeof(std::uint64_t))
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::get_serialized_item] No more serialized records")
        }

        // On little-endian LP64, the first four bytes also represent a matched int.
        // Return owned, aligned byte storage; callers copy it into their integer.
        std::memcpy(ReturnStorage.data(), CacheBytes.data() + sizeof(IdentifyingProperties) + NextRecordIndex * sizeof(std::uint64_t), ReturnStorage.size());
        ++NextRecordIndex;
        return ReturnStorage.data();
    }

    auto LinuxFile::close_current_file() -> void
    {
        CloseFile();
    }

    auto LinuxFile::CreateAllDirectories(const std::filesystem::path& FileNameAndPath) -> void
    {
        if (FileNameAndPath.parent_path().empty())
        {
            return;
        }

        try
        {
            std::filesystem::create_directories(FileNameAndPath.parent_path());
        }
        catch (const std::filesystem::filesystem_error& E)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::create_all_directories] Tried creating directories '{}' but encountered an error. error: {}",
                                                  FileNameAndPath.string(),
                                                  E.what()))
        }
    }

    auto LinuxFile::CloseFile() -> void
    {
        int UnmapError{};
        if (m_memory_map)
        {
            if (::munmap(m_memory_map, MapSize) != 0)
            {
                UnmapError = errno;
            }
            else
            {
                m_memory_map = nullptr;
                MapSize = 0;
            }
        }

        int CloseError{};
        if (is_valid() && is_file_open())
        {
            const int Fd = FileDescriptor;

            // Linux may release the descriptor even when close() reports an
            // error. Invalidate it before calling close() so a later cleanup
            // cannot accidentally close a descriptor reused elsewhere.
            FileDescriptor = -1;
            m_is_file_open = false;

            if (::close(Fd) != 0)
            {
                CloseError = errno;
            }
        }

        // Mapping and descriptor lifetimes are independent. Attempt both
        // cleanup operations before reporting an error to the caller.
        if (UnmapError)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::close_file] Was unable to unmap file, error: {}", ErrnoString(UnmapError)))
        }

        if (CloseError)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::close_file] Was unable to close file, {}", ErrnoString(CloseError)))
        }
    }

    auto LinuxFile::is_file_open() const -> bool
    {
        return m_is_file_open;
    }

    auto LinuxFile::write_string_to_file(StringViewType StringToWrite) -> void
    {
        // StringType is UTF-16 (char16_t) on Linux; files on disk are UTF-8.
        // Match Windows UTF-8 conversion by replacing isolated surrogate
        // code units with U+FFFD while preserving valid surrogate pairs.
        std::u16string NormalisedInput{StringToWrite};

        for (size_t Index = 0; Index < NormalisedInput.size(); ++Index)
        {
            const char16_t Unit = NormalisedInput[Index];

            if (Unit >= 0xD800 && Unit <= 0xDBFF)
            {
                if (Index + 1 < NormalisedInput.size() &&
                    NormalisedInput[Index + 1] >= 0xDC00 &&
                    NormalisedInput[Index + 1] <= 0xDFFF)
                {
                    ++Index;
                }
                else
                {
                    NormalisedInput[Index] = u'\uFFFD';
                }
            }
            else if (Unit >= 0xDC00 && Unit <= 0xDFFF)
            {
                NormalisedInput[Index] = u'\uFFFD';
            }
        }

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> Converter{};
        std::string StringConvertedToUtf8;

        try
        {
            StringConvertedToUtf8 = Converter.to_bytes(
                NormalisedInput.data(), NormalisedInput.data() + NormalisedInput.size());
        }
        catch (const std::range_error& Error)
        {
            THROW_INTERNAL_FILE_ERROR(
                fmt::format("[LinuxFile::write_string_to_file] UTF-16 conversion failed: {}", Error.what()))
        }
#if __clang__
#pragma clang diagnostic pop
#endif
        if (Converter.converted() != NormalisedInput.size())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::write_string_to_file] Incomplete UTF-16 conversion")
        }

        WriteToFile(*this, StringConvertedToUtf8.data(), StringConvertedToUtf8.size());
    }

    auto LinuxFile::is_same_as(LinuxFile& OtherFile) -> bool
    {
        struct stat FileInfo{};
        if (::fstat(FileDescriptor, &FileInfo) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::is_same_as] Tried retrieving file information by fd. {}", ErrnoString()))
        }

        struct stat OtherFileInfo{};
        if (::fstat(OtherFile.get_file(), &OtherFileInfo) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::is_same_as] Tried retrieving file information by fd. {}", ErrnoString()))
        }

        return FileInfo.st_dev == OtherFileInfo.st_dev && FileInfo.st_ino == OtherFileInfo.st_ino;
    }

    auto LinuxFile::read_all() const -> StringType
    {
        std::ifstream Stream{get_file_path(), std::ios::in | std::ios::binary};
        if (!Stream)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::read_all] Tried to read entire file but returned error {}", errno))
        }

        std::string FileContents;
        Stream.seekg(0, std::ios::end);
        auto Size = Stream.tellg();
        if (Size == -1)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::read_all] Unable to determine file size")
        }
        FileContents.resize(static_cast<size_t>(Size));
        Stream.seekg(0, std::ios::beg);
        if (!Stream)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::read_all] Unable to seek to beginning of file")
        }

        // The file may change between determining its size and reading it.
        // Never return an incomplete buffer as though the read succeeded.
        Stream.read(FileContents.data(), static_cast<std::streamsize>(FileContents.size()));
        if (!Stream || Stream.gcount() != static_cast<std::streamsize>(FileContents.size()))
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::read_all] Unable to read complete file")
        }

        Stream.close();

        // Strip the UTF-8 BOM if it exists
        if (FileContents.size() >= 3 && static_cast<uint8_t>(FileContents[0]) == 0xEF && static_cast<uint8_t>(FileContents[1]) == 0xBB &&
            static_cast<uint8_t>(FileContents[2]) == 0xBF)
        {
            FileContents.erase(0, 3);
        }

        // Files on disk are UTF-8; StringType is UTF-16 (char16_t) on Linux.
        return Utf8ToU16String(FileContents);
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
            return std::span(m_memory_map, MapSize);
        }

        int Prot{};
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
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::memory_map] Writable mapping requires OpenFor::ReadWrite")

        case OpenFor::ReadWrite:
            Prot = PROT_READ | PROT_WRITE;
            break;

        case OpenFor::Reading:
            Prot = PROT_READ;
            break;

        default:
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::memory_map] Invalid file access mode")
        }

        struct stat FileInfo{};
        if (::fstat(FileDescriptor, &FileInfo) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::memory_map] fstat failed. {}", ErrnoString()))
        }
        if (FileInfo.st_size == 0)
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::memory_map] Tried to memory map an empty file.")
        }

        void* Mapped = ::mmap(nullptr, static_cast<size_t>(FileInfo.st_size), Prot, MAP_SHARED, FileDescriptor, 0);
        if (Mapped == MAP_FAILED)
        {
            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::memory_map] mmap failed. {}", ErrnoString()))
        }

        m_memory_map = static_cast<uint8_t*>(Mapped);
        MapSize = static_cast<size_t>(FileInfo.st_size);
        return std::span(m_memory_map, MapSize);
    }

    auto LinuxFile::open_file(const std::filesystem::path& FileNameAndPath, const OpenProperties& OpenProperties) -> LinuxFile
    {
        if (FileNameAndPath.empty())
        {
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::open_file] Tried to open file but file_name_and_path was empty.")
        }

        int Flags{};
        switch (OpenProperties.open_for)
        {
        case OpenFor::Writing:
            Flags = O_WRONLY;
            break;
        case OpenFor::Appending:
            Flags = O_WRONLY | O_APPEND;
            break;
        case OpenFor::Reading:
            Flags = O_RDONLY;
            break;
        case OpenFor::ReadWrite:
            Flags = O_RDWR;
            break;
        default:
            THROW_INTERNAL_FILE_ERROR("[LinuxFile::open_file] Tried to open file but received invalid data for the 'OpenFor' parameter.")
        }

        // Do not allow ordinary File descriptors to survive exec().
        // Set this atomically at open time rather than using fcntl()
        // afterwards, which would introduce a descriptor-inheritance race.
        Flags |= O_CLOEXEC;

        if (OpenProperties.overwrite_existing_file == OverwriteExistingFile::Yes)
        {
            CreateAllDirectories(FileNameAndPath);
            Flags |= O_CREAT | O_TRUNC;
        }
        else if (OpenProperties.create_if_non_existent == CreateIfNonExistent::Yes)
        {
            CreateAllDirectories(FileNameAndPath);
            Flags |= O_CREAT;
        }

        LinuxFile File{};

        // Initialise potentially throwing C++ state before acquiring the
        // descriptor. Once open() succeeds, only non-throwing operations
        // and the non-throwing move constructor remain.
        File.m_file_path_and_name = FileNameAndPath;
        File.m_open_properties = OpenProperties;

        File.set_file(::open(FileNameAndPath.c_str(), Flags, 0644));

        if (File.get_file() < 0)
        {
            std::string_view OpenType = OpenProperties.open_for == OpenFor::Writing || OpenProperties.open_for == OpenFor::Appending ? "writing" : "reading";

            THROW_INTERNAL_FILE_ERROR(fmt::format("[LinuxFile::open_file] Tried opening file for {} but encountered an error. Path & File: {} | error: {}\n",
                                                  OpenType,
                                                  FileNameAndPath.string(),
                                                  ErrnoString()))
        }

        struct stat FileInfo{};
        if (::fstat(File.get_file(), &FileInfo) != 0)
        {
            const int StatError = errno;
            ::close(File.get_file());
            File.set_file(-1);
            THROW_INTERNAL_FILE_ERROR(
                fmt::format("[LinuxFile::open_file] fstat failed: {}", ErrnoString(StatError)))
        }

        // Linux allows opening directories as files (you can open them with a 
		// text editor even - but please don't!). Windows ordinarily does not allow
		// this and neither should we.
        if (S_ISDIR(FileInfo.st_mode))
        {
            ::close(File.get_file());
            File.set_file(-1);
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::open_file] Tried to open a directory as a file.")
        }

        File.set_is_file_open(true);

        return File;
    }
} // namespace RC::File

#endif // ifdef __linux__
