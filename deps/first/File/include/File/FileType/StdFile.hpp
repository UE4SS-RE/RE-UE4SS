#pragma once

#include <filesystem>
#include <format>

#include <File/Common.hpp>
#include <File/FileType/FileBase.hpp>
#include <File/Macros.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace RC::File
{
    class StdFile : public FileInterface<StdFile>
    {
      private:
        using HANDLE = FILE*;

        struct IdentifyingProperties
        {
            struct stat file_stat{};
        };

      private:
        HANDLE m_file{};
        uint8_t* m_memory_map{};
        size_t m_memory_map_size{};
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
        ~StdFile() override = default;

      private:
        auto static create_all_directories(const std::filesystem::path& file_name_and_path) -> void;

      private:
        auto close_file() -> void;

      public:
        [[nodiscard]] auto is_file_open() const -> bool;

      public:
        RC_FILE_API auto set_file(HANDLE new_file) -> void;
        RC_FILE_API auto set_is_file_open(bool new_is_open) -> void;
        RC_FILE_API auto get_file() -> HANDLE;
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
        RC_FILE_API auto is_same_as(StdFile& other_file) -> bool override;
        [[nodiscard]] RC_FILE_API auto read_all() const -> StringType override;
        [[nodiscard]] RC_FILE_API auto memory_map() -> std::span<uint8_t> override;
        [[nodiscard]] RC_FILE_API auto static open_file(const std::filesystem::path& file_name_and_path, const OpenProperties& open_properties) -> StdFile;
        // File Interface -> END
    };

    // This file is automatically included ONLY if Windows is detected
    // Therefore, it's not necessary to do any checks here
    template <ImplementsFileInterface UnderlyingAbstraction>
    class HandleTemplate;
    using Handle = HandleTemplate<StdFile>;
} // namespace RC::File
