#pragma once

#ifdef __linux__

#include <filesystem>
#include <format>

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
            uint64_t device{};
            uint64_t inode{};
            int64_t mtime_sec{};
            int64_t mtime_nsec{};
            uint64_t file_size{};
        };

      private:
        int m_fd{-1};
        uint8_t* m_memory_map{};
        size_t m_map_size{};
        OpenProperties m_open_properties{};
        std::filesystem::path m_file_path_and_name{};
        std::filesystem::path m_serialization_file_path_and_name{};
        IdentifyingProperties m_identifying_properties{};
        constexpr static inline size_t cache_size = 0x500;
        unsigned char m_cache[cache_size]{};
        size_t m_offset_to_next_serialized_item{};
        bool m_has_cache_in_memory{};
        bool m_has_cached_identifying_properties{};
        bool m_is_file_open{};

      public:
        ~LinuxFile() override = default;

      private:
        auto static create_all_directories(const std::filesystem::path& file_name_and_path) -> void;

      private:
        auto close_file() -> void;

      public:
        [[nodiscard]] auto is_file_open() const -> bool;

      public:
        RC_FILE_API auto set_file(int new_fd) -> void;
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
