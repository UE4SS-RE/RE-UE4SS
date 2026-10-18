#pragma once

#ifdef __linux__

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <type_traits>
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
            std::uint64_t device{};
            std::uint64_t inode{};
            std::int64_t modification_time_seconds{};
            std::int64_t modification_time_nanoseconds{};
            std::int64_t file_size{};
        };

        static_assert(std::has_unique_object_representations_v<IdentifyingProperties>);

      private:
        int m_file_descriptor{-1};
        uint8_t* m_memory_map{};
        size_t m_map_size{};
        OpenProperties m_open_properties{};
        std::filesystem::path m_file_path_and_name{};
        std::filesystem::path m_serialization_file_path_and_name{};
        std::vector<std::uint8_t> m_cache_bytes{};
        std::optional<IdentifyingProperties> m_cached_properties{};
        size_t m_next_record_index{};
        alignas(std::uint64_t) std::array<std::byte, sizeof(std::uint64_t)> m_return_storage{};
        bool m_is_file_open{};

      public:
        LinuxFile() = default;

        // Transfer the complete resource state without copying ownership
        // of the descriptor or performing potentially throwing allocations.
        // This also makes returning an opened LinuxFile safe without NRVO.
        LinuxFile(LinuxFile&& other) noexcept
        {
            swap(*this, other);
        }

        ~LinuxFile() override = default;

        // Exchange the complete file state, including descriptor and mapping
        // ownership. Swapping only the native resources would leave the paths,
        // open properties and serialization state attached to the wrong file.
        //
        // HandleTemplate uses this to transfer ownership without temporarily
        // duplicating native handles or performing potentially throwing copies.
        friend auto swap(LinuxFile& lhs, LinuxFile& rhs) noexcept -> void
        {
            using std::swap;

            swap(lhs.m_file_descriptor, rhs.m_file_descriptor);
            swap(lhs.m_memory_map, rhs.m_memory_map);
            swap(lhs.m_map_size, rhs.m_map_size);
            swap(lhs.m_open_properties, rhs.m_open_properties);
            lhs.m_file_path_and_name.swap(rhs.m_file_path_and_name);
            lhs.m_serialization_file_path_and_name.swap(rhs.m_serialization_file_path_and_name);
            swap(lhs.m_cache_bytes, rhs.m_cache_bytes);
            swap(lhs.m_cached_properties, rhs.m_cached_properties);
            swap(lhs.m_next_record_index, rhs.m_next_record_index);
            swap(lhs.m_return_storage, rhs.m_return_storage);
            swap(lhs.m_is_file_open, rhs.m_is_file_open);
        }

      private:
        auto static create_all_directories(const std::filesystem::path& file_name_and_path) -> void;

      private:
        auto reset_serialization_state() noexcept -> void;
        auto load_serialization_cache() -> bool;
        auto close_file() -> void;

      public:
        [[nodiscard]] auto is_file_open() const -> bool;

      public:
        RC_FILE_API auto set_file(int new_file_descriptor) -> void;
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
