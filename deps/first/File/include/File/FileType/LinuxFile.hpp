#pragma once

#ifdef __linux__

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <utility>
#include <vector>

#include <File/Common.hpp>
#include <File/FileType/FileBase.hpp>
#include <File/Macros.hpp>

namespace RC::File
{
    class LinuxFile : public FileInterface<LinuxFile>
    {
      private:
        struct IdentifyingProperties
        {
            std::uint64_t Device{};
            std::uint64_t Inode{};
            std::int64_t ModificationTimeSeconds{};
            std::int64_t ModificationTimeNanoseconds{};
            std::int64_t FileSize{};
        };

      private:
        int FileDescriptor{-1};
        uint8_t* m_memory_map{};
        size_t MapSize{};
        OpenProperties m_open_properties{};
        std::filesystem::path m_file_path_and_name{};
        std::filesystem::path m_serialization_file_path_and_name{};
        std::vector<std::uint8_t> CacheBytes{};
        std::optional<IdentifyingProperties> CachedProperties{};
        size_t NextRecordIndex{};
        alignas(std::uint64_t) std::array<std::byte, sizeof(std::uint64_t)> ReturnStorage{};
        bool m_is_file_open{};

      public:
        LinuxFile() = default;

        // Transfer the complete resource state without copying ownership
        // of the descriptor or performing potentially throwing allocations.
        // This also makes returning an opened LinuxFile safe without NRVO.
        LinuxFile(LinuxFile&& Other) noexcept
        {
            swap(*this, Other);
        }

        ~LinuxFile() override = default;

        // Exchange the complete file state, including descriptor and mapping
        // ownership. Swapping only the native resources would leave the paths,
        // open properties and serialization state attached to the wrong file.
        //
        // HandleTemplate uses this to transfer ownership without temporarily
        // duplicating native handles or performing potentially throwing copies.
        friend auto swap(LinuxFile& Lhs, LinuxFile& Rhs) noexcept -> void
        {
            using std::swap;

            swap(Lhs.FileDescriptor, Rhs.FileDescriptor);
            swap(Lhs.m_memory_map, Rhs.m_memory_map);
            swap(Lhs.MapSize, Rhs.MapSize);
            swap(Lhs.m_open_properties, Rhs.m_open_properties);
            Lhs.m_file_path_and_name.swap(Rhs.m_file_path_and_name);
            Lhs.m_serialization_file_path_and_name.swap(Rhs.m_serialization_file_path_and_name);
            swap(Lhs.CacheBytes, Rhs.CacheBytes);
            swap(Lhs.CachedProperties, Rhs.CachedProperties);
            swap(Lhs.NextRecordIndex, Rhs.NextRecordIndex);
            swap(Lhs.ReturnStorage, Rhs.ReturnStorage);
            swap(Lhs.m_is_file_open, Rhs.m_is_file_open);
        }

      private:
        auto static CreateAllDirectories(const std::filesystem::path& file_name_and_path) -> void;

      private:
        auto ResetSerializationState() noexcept -> void;
        auto LoadSerializationCache() -> bool;
        auto CloseFile() -> void;

      public:
        [[nodiscard]] auto is_file_open() const -> bool;

      public:
        RC_FILE_API auto set_file(int NewFileDescriptor) -> void;
        RC_FILE_API auto set_is_file_open(bool new_is_open) -> void;
        RC_FILE_API auto get_file() -> int;
        RC_FILE_API auto serialization_file_exists() -> bool;

        // File Interface -> START
        RC_FILE_API auto is_valid() noexcept -> bool override;
        RC_FILE_API auto invalidate_file() noexcept -> void override;
        RC_FILE_API auto static delete_file(const std::filesystem::path&) -> void;
        RC_FILE_API auto delete_file() -> void override;
        RC_FILE_API auto get_raw_handle() noexcept -> void* override;
        [[nodiscard]] RC_FILE_API auto get_file_path() const noexcept -> const std::filesystem::path& override;
        RC_FILE_API auto set_serialization_output_file(const std::filesystem::path& output_file) noexcept -> void override;
        RC_FILE_API auto serialize_identifying_properties() -> void override;
        RC_FILE_API auto deserialize_identifying_properties() -> void override;
        RC_FILE_API auto is_deserialized_and_live_equal() -> bool override;
        RC_FILE_API auto invalidate_serialization() -> void override;
        RC_FILE_API auto serialize_item(const GenericItemData& data, bool is_internal_item = false) -> void override;
        RC_FILE_API auto get_serialized_item(size_t data_size, bool is_internal_item = false) -> void* override;
        RC_FILE_API auto close_current_file() -> void override;
        RC_FILE_API auto write_string_to_file(StringViewType string_to_write) -> void override;
        RC_FILE_API auto is_same_as(LinuxFile& other_file) -> bool override;
        [[nodiscard]] RC_FILE_API auto read_all() const -> StringType override;
        [[nodiscard]] RC_FILE_API auto memory_map() -> std::span<uint8_t> override;
        [[nodiscard]] RC_FILE_API auto static open_file(const std::filesystem::path& file_name_and_path, const OpenProperties& open_properties) -> LinuxFile;
        // File Interface -> END
    };

    // This file is automatically included ONLY if Linux is detected
    // Therefore, it's not necessary to do any checks here
    template <ImplementsFileInterface UnderlyingAbstraction>
    class HandleTemplate;
    using Handle = HandleTemplate<LinuxFile>;
} // namespace RC::File

#endif // ifdef __linux__
