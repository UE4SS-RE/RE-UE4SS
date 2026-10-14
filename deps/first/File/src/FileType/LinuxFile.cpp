#ifdef __linux__
#include <fstream>

#include <File/File.hpp>
#include <File/FileType/LinuxFile.hpp>
#include <File/HandleTemplate.hpp>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <array>
#include <bit>
#include <cerrno>
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <locale>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <Helpers/String.hpp>
#include <fmt/core.h>

namespace RC::File
{
    namespace
    {
        // Linux-only private serialization format.
        constexpr std::array<std::uint8_t, 8> CacheMagic{
            0xD3, 0x62, 0x13, 0xD0,
            0xFC, 0x29, 0xC6, 0x70
        };

        constexpr std::uint32_t CacheFormatVersion = 1;
        constexpr std::size_t CacheHeaderSize = 56;
        constexpr std::size_t CacheRecordSize = 9;

        // Explicit on-disk tags, independent of GenericDataType values.
        enum class CacheRecordTag : std::uint8_t
        {
            UnsignedLong = 0x01,
            SignedLong = 0x02,
            UnsignedLongLong = 0x03,
            SignedLongLong = 0x04,
        };

        template <typename Unsigned>
        constexpr auto EncodeLittleEndian(std::uint8_t* Destination, Unsigned Value) noexcept -> void
        {
            static_assert(std::is_unsigned_v<Unsigned>);
            static_assert(sizeof(Unsigned) == 4 || sizeof(Unsigned) == 8);

            for (std::size_t Index = 0; Index < sizeof(Unsigned); ++Index)
            {
                Destination[Index] = static_cast<std::uint8_t>(Value & 0xFFu);
                Value >>= 8;
            }
        }

        template <typename Unsigned>
        [[nodiscard]] constexpr auto DecodeLittleEndian(const std::uint8_t* Source) noexcept -> Unsigned
        {
            static_assert(std::is_unsigned_v<Unsigned>);
            static_assert(sizeof(Unsigned) == 4 || sizeof(Unsigned) == 8);

            Unsigned Value{};

            for (std::size_t Index = 0; Index < sizeof(Unsigned); ++Index)
            {
                Value |= static_cast<Unsigned>(Source[Index]) << (Index * 8);
            }

            return Value;
        }
    } // namespace

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
        constexpr std::size_t MaxChunkSize = 1024 * 1024;
        std::size_t TotalRead{};

        while (TotalRead < ByteCount)
        {
            const auto Remaining = ByteCount - TotalRead;
            const auto ChunkSize = Remaining < MaxChunkSize ? Remaining : MaxChunkSize;

            const ssize_t Result = ::read(Fd, Destination + TotalRead, ChunkSize);

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

                THROW_INTERNAL_FILE_ERROR(
                    fmt::format("[LinuxFile::read_cache_exact] Read failed: {}", ErrnoString()))
            }

            TotalRead += static_cast<std::size_t>(Result);
        }

        return true;
    }

    // Writes exactly ByteCount bytes or throws.
    // A zero-progress write is an error, not an invitation to loop forever.
    static auto WriteCacheExact(int Fd, const std::uint8_t* Source, std::size_t ByteCount) -> void
    {
        constexpr std::size_t MaxChunkSize = 1024 * 1024;
        std::size_t TotalWritten{};

        while (TotalWritten < ByteCount)
        {
            const auto Remaining = ByteCount - TotalWritten;
            const auto ChunkSize = Remaining < MaxChunkSize ? Remaining : MaxChunkSize;

            const ssize_t Result = ::write(Fd, Source + TotalWritten, ChunkSize);

            if (Result < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                THROW_INTERNAL_FILE_ERROR(
                    fmt::format("[LinuxFile::write_cache_exact] Write failed: {}", ErrnoString()))
            }

            if (Result == 0)
            {
                THROW_INTERNAL_FILE_ERROR(
                    "[LinuxFile::write_cache_exact] Write made no progress")
            }

            TotalWritten += static_cast<std::size_t>(Result);
        }
    }

    namespace
    {
        // Owns a cache descriptor independently of the public File handle.
        class CacheDescriptor
        {
            int FileDescriptor{-1};

          public:
            explicit CacheDescriptor(int Fd) noexcept : FileDescriptor(Fd) {}

            CacheDescriptor(const CacheDescriptor&) = delete;
            auto operator=(const CacheDescriptor&) -> CacheDescriptor& = delete;

            ~CacheDescriptor() noexcept
            {
                if (FileDescriptor >= 0)
                {
                    ::close(FileDescriptor);
                }
            }

            [[nodiscard]] auto Get() const noexcept -> int
            {
                return FileDescriptor;
            }

            auto CloseChecked() -> void
            {
                if (FileDescriptor < 0)
                {
                    return;
                }

                const int Fd = FileDescriptor;
                FileDescriptor = -1;

                // Do not retry close(): on Linux the descriptor may
                // already have been released, even when close fails.
                if (::close(Fd) != 0)
                {
                    THROW_INTERNAL_FILE_ERROR(
                        fmt::format("[LinuxFile::CacheDescriptor] Close failed: {}", ErrnoString()))
                }
            }
        };

        enum class CacheLoadStatus
        {
            Missing,
            Malformed,
            Loaded,
        };

        struct CacheFileBytes
        {
            CacheLoadStatus Status{};
            std::vector<std::uint8_t> Bytes{};
        };

        [[nodiscard]] auto LoadCacheBytes(const std::filesystem::path& Path) -> CacheFileBytes
        {
            const int RawFd = ::open(Path.c_str(), O_RDONLY | O_CLOEXEC);

            if (RawFd < 0)
            {
                if (errno == ENOENT)
                {
                    return {CacheLoadStatus::Missing, {}};
                }

                THROW_INTERNAL_FILE_ERROR(
                    fmt::format("[LinuxFile::load_cache_bytes] Open failed: {}", ErrnoString()))
            }

            CacheDescriptor Fd{RawFd};

            struct stat FileInfo{};
            if (::fstat(Fd.Get(), &FileInfo) != 0)
            {
                THROW_INTERNAL_FILE_ERROR(
                    fmt::format("[LinuxFile::load_cache_bytes] fstat failed: {}", ErrnoString()))
            }

            if (!S_ISREG(FileInfo.st_mode))
            {
                THROW_INTERNAL_FILE_ERROR(
                    "[LinuxFile::load_cache_bytes] Cache path is not a regular file")
            }

            // The file must contain one complete header followed by
            // zero or more complete fixed-size records.
            if (FileInfo.st_size < 0 ||
                static_cast<std::uintmax_t>(FileInfo.st_size) < CacheHeaderSize)
            {
                Fd.CloseChecked();
                return {CacheLoadStatus::Malformed, {}};
            }

            const auto FileSize = static_cast<std::uintmax_t>(FileInfo.st_size);
            const auto PayloadSize = FileSize - CacheHeaderSize;

            if (PayloadSize % CacheRecordSize != 0)
            {
                Fd.CloseChecked();
                return {CacheLoadStatus::Malformed, {}};
            }

            // Validate the fixed header before allocating memory based on
            // the file's reported size. A corrupt or unsupported cache might
            // be arbitrarily large, and there is no reason to allocate it
            // before establishing that it belongs to this format.
            std::array<std::uint8_t, CacheHeaderSize> Header{};

            if (!ReadCacheExact(Fd.Get(), Header.data(), Header.size()))
            {
                Fd.CloseChecked();
                return {CacheLoadStatus::Malformed, {}};
            }

            if (std::memcmp(Header.data(), CacheMagic.data(), CacheMagic.size()) != 0 ||
                DecodeLittleEndian<std::uint32_t>(Header.data() + 8) != CacheFormatVersion ||
                DecodeLittleEndian<std::uint32_t>(Header.data() + 12) != 0)
            {
                Fd.CloseChecked();
                return {CacheLoadStatus::Malformed, {}};
            }

            CacheFileBytes Result{CacheLoadStatus::Loaded, {}};

            if (FileSize > Result.Bytes.max_size())
            {
                Fd.CloseChecked();
                return {CacheLoadStatus::Malformed, {}};
            }

            Result.Bytes.resize(static_cast<std::size_t>(FileSize));

            // The descriptor is now positioned immediately after the header.
            // Preserve those validated bytes and read only the remaining
            // records. A short read indicates a truncated or changed cache.
            std::memcpy(Result.Bytes.data(), Header.data(), Header.size());

            const std::size_t Remaining = Result.Bytes.size() - Header.size();

            if (!ReadCacheExact(
                    Fd.Get(),
                    Result.Bytes.data() + Header.size(),
                    Remaining))
            {
                Fd.CloseChecked();
                return {CacheLoadStatus::Malformed, {}};
            }

            Fd.CloseChecked();
            return Result;
        }
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
        CachedSnapshot.reset();
        NextRecordIndex = 0;
        ReturnStorage.fill(std::byte{0});
    }

    auto LinuxFile::serialization_file_exists() -> bool
    {
        return std::filesystem::exists(m_serialization_file_path_and_name);
    }

    // Load and validate the complete serialization cache before exposing
    // any of its contents. A failed validation must never publish a
    // partially decoded snapshot.
    //
    // Returns false for missing, malformed or unsupported cache files.
    // Filesystem I/O errors are reported as exceptions.
    //
    // A validated snapshot is reused until explicitly discarded. External
    // modification of the cache while a snapshot is loaded is outside
    // this implementation's consistency guarantees.
    //
    // Loading does not advance or reset the logical user-record cursor.
    auto LinuxFile::LoadSerializationSnapshot() -> bool
    {
        if (CachedSnapshot)
        {
            return true;
        }

        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::load_serialization_snapshot] Serialization file path is empty")
        }

        auto CacheFile = LoadCacheBytes(m_serialization_file_path_and_name);

        if (CacheFile.Status != CacheLoadStatus::Loaded)
        {
            return false;
        }

        const auto* Bytes = CacheFile.Bytes.data();

        if (std::memcmp(Bytes, CacheMagic.data(), CacheMagic.size()) != 0 ||
            DecodeLittleEndian<std::uint32_t>(Bytes + 8) != CacheFormatVersion ||
            DecodeLittleEndian<std::uint32_t>(Bytes + 12) != 0)
        {
            return false;
        }

        CacheSnapshot Decoded{};

        Decoded.Properties.Device =
            DecodeLittleEndian<std::uint64_t>(Bytes + 16);

        Decoded.Properties.Inode =
            DecodeLittleEndian<std::uint64_t>(Bytes + 24);

        Decoded.Properties.ModificationTimeSeconds =
            std::bit_cast<std::int64_t>(
                DecodeLittleEndian<std::uint64_t>(Bytes + 32));

        Decoded.Properties.ModificationTimeNanoseconds =
            std::bit_cast<std::int64_t>(
                DecodeLittleEndian<std::uint64_t>(Bytes + 40));

        Decoded.Properties.FileSize =
            DecodeLittleEndian<std::uint64_t>(Bytes + 48);

        const std::size_t RecordCount =
            (CacheFile.Bytes.size() - CacheHeaderSize) / CacheRecordSize;

        if (RecordCount > Decoded.Records.max_size())
        {
            return false;
        }

        Decoded.Records.reserve(RecordCount);

        for (std::size_t Index = 0; Index < RecordCount; ++Index)
        {
            const std::size_t Offset =
                CacheHeaderSize + Index * CacheRecordSize;

            GenericDataType Type{};

            switch (static_cast<CacheRecordTag>(Bytes[Offset]))
            {
            case CacheRecordTag::UnsignedLong:
                Type = GenericDataType::UnsignedLong;
                break;

            case CacheRecordTag::SignedLong:
                Type = GenericDataType::SignedLong;
                break;

            case CacheRecordTag::UnsignedLongLong:
                Type = GenericDataType::UnsignedLongLong;
                break;

            case CacheRecordTag::SignedLongLong:
                Type = GenericDataType::SignedLongLong;
                break;

            default:
                return false;
            }

            Decoded.Records.push_back(SerializedRecord{
                Type,
                DecodeLittleEndian<std::uint64_t>(Bytes + Offset + 1)
            });
        }

        // Publish only after the complete file passes validation.
        CachedSnapshot.emplace(std::move(Decoded));

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

    // Linux serialization cache format, version 1.
    // All multi-byte integers are encoded little-endian.
    //
    // Header (56 bytes):
    // Offset  Size  Field
    //   0       8   Magic: D3 62 13 D0 FC 29 C6 70
    //   8       4   Format version (1)
    //  12       4   Reserved (must be zero)
    //  16       8   Device ID (uint64)
    //  24       8   Inode (uint64)
    //  32       8   Modification time, seconds (int64)
    //  40       8   Modification time, nanoseconds (int64)
    //  48       8   File size (uint64)
    //
    // User records (9 bytes each, immediately following the header):
    // Offset  Size  Field
    //   0       1   Type tag:
    //                 0x01 = UnsignedLong
    //                 0x02 = SignedLong
    //                 0x03 = UnsignedLongLong
    //                 0x04 = SignedLongLong
    //   1       8   Integer value (64-bit representation)
    //
    // Signed values use two's-complement encoding.
    // Total file size = 56 + (9 * number_of_user_records).
    // The header is replaced, not appended, by explicit metadata serialization.
    auto LinuxFile::serialize_identifying_properties() -> void
    {
        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::serialize_identifying_properties] Serialization file path is empty")
        }

        // Identify the original file, not the serialization cache.
        struct stat FileInfo{};
        if (::fstat(FileDescriptor, &FileInfo) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(
                fmt::format("[LinuxFile::serialize_identifying_properties] fstat failed: {}", ErrnoString()))
        }

        // Build the entire versioned header before opening the cache.
        // Reserved bytes remain zero.
        std::array<std::uint8_t, CacheHeaderSize> Header{};

        std::memcpy(Header.data(), CacheMagic.data(), CacheMagic.size());

        EncodeLittleEndian<std::uint32_t>(Header.data() + 8, CacheFormatVersion);

        EncodeLittleEndian<std::uint64_t>(
            Header.data() + 16,
            static_cast<std::uint64_t>(FileInfo.st_dev));

        EncodeLittleEndian<std::uint64_t>(
            Header.data() + 24,
            static_cast<std::uint64_t>(FileInfo.st_ino));

        EncodeLittleEndian<std::uint64_t>(
            Header.data() + 32,
            std::bit_cast<std::uint64_t>(
                static_cast<std::int64_t>(FileInfo.st_mtim.tv_sec)));

        EncodeLittleEndian<std::uint64_t>(
            Header.data() + 40,
            std::bit_cast<std::uint64_t>(
                static_cast<std::int64_t>(FileInfo.st_mtim.tv_nsec)));

        EncodeLittleEndian<std::uint64_t>(
            Header.data() + 48,
            static_cast<std::uint64_t>(FileInfo.st_size));

        CreateAllDirectories(m_serialization_file_path_and_name);

        // From this point onwards, an attempted write must not leave
        // the previous in-memory snapshot trusted.
        CachedSnapshot.reset();
        NextRecordIndex = 0;
        ReturnStorage.fill(std::byte{0});

        const int RawFd = ::open(
            m_serialization_file_path_and_name.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
            0644);

        if (RawFd < 0)
        {
            THROW_INTERNAL_FILE_ERROR(
                fmt::format(
                    "[LinuxFile::serialize_identifying_properties] Open failed: {}",
                    ErrnoString()))
        }

        CacheDescriptor Fd{RawFd};

        WriteCacheExact(Fd.Get(), Header.data(), Header.size());

        Fd.CloseChecked();
    }

    auto LinuxFile::deserialize_identifying_properties() -> void
    {
        if (!LoadSerializationSnapshot())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::deserialize_identifying_properties] Serialization cache is missing or malformed")
        }

        // Identifying properties are decoded into the validated snapshot.
        // They are not user records and do not affect the read cursor.
    }

    // Determine whether the original file still matches the identifying
    // properties recorded in the serialization cache.
    //
    // Device, inode, modification timestamp and file size are compared.
    // This detects changes reflected in those properties, but does not
    // establish content equality or calculate a content hash.
    //
    // Missing or malformed caches return false; filesystem errors throw.
    // Repeated checks must not consume or rewind serialized user records.
    auto LinuxFile::is_deserialized_and_live_equal() -> bool
    {
        if (!LoadSerializationSnapshot())
        {
            return false;
        }

        struct stat LiveInfo{};
        if (::fstat(FileDescriptor, &LiveInfo) != 0)
        {
            THROW_INTERNAL_FILE_ERROR(
                fmt::format("[LinuxFile::is_deserialized_and_live_equal] fstat failed: {}", ErrnoString()))
        }

        const auto& Cached = CachedSnapshot->Properties;

        return static_cast<std::uint64_t>(LiveInfo.st_dev) == Cached.Device &&
               static_cast<std::uint64_t>(LiveInfo.st_ino) == Cached.Inode &&
               static_cast<std::int64_t>(LiveInfo.st_mtim.tv_sec) == Cached.ModificationTimeSeconds &&
               static_cast<std::int64_t>(LiveInfo.st_mtim.tv_nsec) == Cached.ModificationTimeNanoseconds &&
               static_cast<std::uint64_t>(LiveInfo.st_size) == Cached.FileSize;
    }

    auto LinuxFile::invalidate_serialization() -> void
    {
        // Discard all trusted serialization state before attempting deletion.
        // Even if unlink() fails, subsequent reads and validity checks must
        // not use metadata or records loaded from the old cache.
        CachedSnapshot.reset();
        NextRecordIndex = 0;
        ReturnStorage.fill(std::byte{0});

        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::invalidate_serialization] Serialization file path is empty")
        }

        // Unlink directly rather than checking existence first. This avoids
        // a race between exists() and deletion. An already-missing cache is
        // successfully invalidated; all other filesystem errors are reported.
        if (::unlink(m_serialization_file_path_and_name.c_str()) != 0)
        {
            if (errno != ENOENT)
            {
                THROW_INTERNAL_FILE_ERROR(
                    fmt::format(
                        "[LinuxFile::invalidate_serialization] Unable to delete cache: {}",
                        ErrnoString()))
            }
        }
    }

    auto LinuxFile::serialize_item(const GenericItemData& Data, bool IsInternalItem) -> void
    {
        // The legacy internal-item flag was used to write identifying
        // properties through the ordinary integer serialization machinery.
        // Version 1 stores those properties in a dedicated header instead.
        //
        // This Linux implementation deliberately rejects the legacy mode.
        // Rejection must have no side effects, including cursor movement.
        if (IsInternalItem)
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::serialize_item] Internal-item serialization is unsupported")
        }

        if (m_serialization_file_path_and_name.empty())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::serialize_item] Serialization file path is empty")
        }

        // Convert the supported public integer types into explicit private
        // format tags and 64-bit value representations. Validate the type
        // before accessing or modifying the serialization cache.
        CacheRecordTag Tag{};
        std::uint64_t ValueBits{};

        switch (Data.data_type)
        {
        case GenericDataType::UnsignedLong:
            Tag = CacheRecordTag::UnsignedLong;
            ValueBits = static_cast<std::uint64_t>(Data.data_ulong);
            break;

        case GenericDataType::SignedLong:
            Tag = CacheRecordTag::SignedLong;
            ValueBits = std::bit_cast<std::uint64_t>(
                static_cast<std::int64_t>(Data.data_long));
            break;

        case GenericDataType::UnsignedLongLong:
            Tag = CacheRecordTag::UnsignedLongLong;
            ValueBits = static_cast<std::uint64_t>(Data.data_ulonglong);
            break;

        case GenericDataType::SignedLongLong:
            Tag = CacheRecordTag::SignedLongLong;
            ValueBits = std::bit_cast<std::uint64_t>(
                static_cast<std::int64_t>(Data.data_longlong));
            break;

        default:
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::serialize_item] Unsupported integer type")
        }

        // Every user record is exactly nine bytes: one explicit type tag
        // followed by the complete 64-bit value in little-endian order.
        // Never write a native union or compiler-dependent structure.
        std::array<std::uint8_t, CacheRecordSize> Record{};
        Record[0] = static_cast<std::uint8_t>(Tag);
        EncodeLittleEndian<std::uint64_t>(Record.data() + 1, ValueBits);

        // Only an absent cache permits automatic header creation.
        // Existing malformed, unsupported or stale caches must never be
        // silently overwritten.
        struct stat CacheInfo{};

        if (::stat(m_serialization_file_path_and_name.c_str(), &CacheInfo) != 0)
        {
            if (errno != ENOENT)
            {
                THROW_INTERNAL_FILE_ERROR(
                    fmt::format(
                        "[LinuxFile::serialize_item] Cache stat failed: {}",
                        ErrnoString()))
            }

            // Starting a fresh cache also resets the logical read cursor.
            serialize_identifying_properties();
        }

        // Reuse an already validated snapshot rather than rereading and
        // decoding the entire cache for every appended record.
        //
        // A missing snapshot requires complete on-disk validation.
        // Concurrent or external modification of an already loaded cache
        // is outside this implementation's consistency guarantees.
        if (!LoadSerializationSnapshot())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::serialize_item] Existing cache is malformed or unsupported")
        }

        if (!is_deserialized_and_live_equal())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::serialize_item] Existing cache metadata does not match the live file")
        }

        // Prepare the updated snapshot before modifying the file.
        // Any allocation failure here leaves the existing cache untouched.
        CachedSnapshot->Records.push_back(SerializedRecord{
            Data.data_type,
            ValueBits
        });

        const std::size_t PreviousCursor = NextRecordIndex;

        // Detach the updated snapshot before attempting filesystem changes.
        // If opening, writing or closing fails, this local copy is discarded
        // and no uncommitted record remains trusted by the LinuxFile object.
        //
        // Moving the snapshot avoids copying all previously stored records.
        auto CompletedSnapshot = std::move(*CachedSnapshot);

        CachedSnapshot.reset();
        NextRecordIndex = 0;
        ReturnStorage.fill(std::byte{0});

        // The header must already exist. Do not use O_CREAT here: if it
        // disappears unexpectedly, fail rather than creating a headerless
        // cache containing only a user record.
        const int RawFd = ::open(
            m_serialization_file_path_and_name.c_str(),
            O_WRONLY | O_APPEND | O_CLOEXEC);

        if (RawFd < 0)
        {
            THROW_INTERNAL_FILE_ERROR(
                fmt::format(
                    "[LinuxFile::serialize_item] Open for append failed: {}",
                    ErrnoString()))
        }

        CacheDescriptor Fd{RawFd};

        // Partial writes and EINTR are handled by WriteCacheExact().
        // If writing or closing fails, the cursor remains reset and no
        // validated snapshot survives. A truncated record will be rejected
        // when the cache is next loaded.
        WriteCacheExact(Fd.Get(), Record.data(), Record.size());
        Fd.CloseChecked();

        // The record was fully written and the descriptor closed
        // successfully. The updated snapshot now describes the cache
        // on disk and can be reused without another complete read.
        //
        // Restore the logical cursor only after successful completion.
        CachedSnapshot.emplace(std::move(CompletedSnapshot));
        NextRecordIndex = PreviousCursor;
    }

    auto LinuxFile::get_serialized_item(size_t DataSize, bool IsInternalItem) -> void*
    {
        // Identifying properties are stored in the dedicated cache header.
        // The legacy internal-item flag is deliberately unsupported on Linux.
        //
        // Reject it before touching any state, including the read cursor
        // and the scratch storage holding the previous returned value.
        if (IsInternalItem)
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::get_serialized_item] Internal-item deserialization is unsupported")
        }

        // The public interface specifies only the requested byte count,
        // not an exact C++ type. On our LP64 target the supported integer
        // widths are 32 and 64 bits.
        if (DataSize != sizeof(std::uint32_t) &&
            DataSize != sizeof(std::uint64_t))
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::get_serialized_item] Unsupported integer width")
        }

        if (!LoadSerializationSnapshot())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::get_serialized_item] Serialization cache is missing or malformed")
        }

        if (NextRecordIndex >= CachedSnapshot->Records.size())
        {
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::get_serialized_item] No more serialized records")
        }

        const auto& Record =
            CachedSnapshot->Records[NextRecordIndex];

        // The file decoder has already validated the record's type tag
        // and decoded its complete 64-bit little-endian value.
        //
        // Narrowing must use the stored signedness. We cannot establish
        // the caller's requested signedness because this API only
        // receives a size. Correctly matched calls are supported.
        if (Record.Type == GenericDataType::SignedLong ||
            Record.Type == GenericDataType::SignedLongLong)
        {
            const auto Value =
                std::bit_cast<std::int64_t>(Record.ValueBits);

            if (DataSize == sizeof(std::int32_t))
            {
                if (Value < std::numeric_limits<std::int32_t>::min() ||
                    Value > std::numeric_limits<std::int32_t>::max())
                {
                    THROW_INTERNAL_FILE_ERROR(
                        "[LinuxFile::get_serialized_item] Signed value cannot fit in 32 bits")
                }

                const auto Narrowed = static_cast<std::int32_t>(Value);
                std::memcpy(
                    ReturnStorage.data(),
                    &Narrowed,
                    sizeof(Narrowed));
            }
            else
            {
                std::memcpy(
                    ReturnStorage.data(),
                    &Value,
                    sizeof(Value));
            }
        }
        else if (Record.Type == GenericDataType::UnsignedLong ||
                 Record.Type == GenericDataType::UnsignedLongLong)
        {
            const std::uint64_t Value = Record.ValueBits;

            if (DataSize == sizeof(std::uint32_t))
            {
                if (Value > std::numeric_limits<std::uint32_t>::max())
                {
                    THROW_INTERNAL_FILE_ERROR(
                        "[LinuxFile::get_serialized_item] Unsigned value cannot fit in 32 bits")
                }

                const auto Narrowed = static_cast<std::uint32_t>(Value);
                std::memcpy(
                    ReturnStorage.data(),
                    &Narrowed,
                    sizeof(Narrowed));
            }
            else
            {
                std::memcpy(
                    ReturnStorage.data(),
                    &Value,
                    sizeof(Value));
            }
        }
        else
        {
            // Defensive check: a validated snapshot should never
            // contain an unrecognised integer category.
            THROW_INTERNAL_FILE_ERROR(
                "[LinuxFile::get_serialized_item] Invalid cached integer type")
        }

        // Only successfully decoded and range-checked records are consumed.
        // Any exception above leaves this record available for retry.
        ++NextRecordIndex;

        // This points to owned, aligned BYTE storage, not a live object
        // of the requested integer type. Callers must copy the bytes into
        // a properly typed object, never dereference a casted pointer.
        //
        // The bytes remain valid until the next retrieval, a serialization
        // state mutation, or destruction of this LinuxFile instance.
        return static_cast<void*>(ReturnStorage.data());
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
