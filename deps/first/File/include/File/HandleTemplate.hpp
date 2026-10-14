#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <memory>
#include <span>
#include <type_traits>
#include <string>

#include <File/Common.hpp>
#include <File/Enums.hpp>
#include <File/Exceptions.hpp>
#include <File/FileType/FileBase.hpp>

namespace RC::File
{
    template <ImplementsFileInterface UnderlyingAbstraction>
    class HandleTemplate
    {
      public:
        using FileType = UnderlyingAbstraction;
        using ThisType = HandleTemplate<UnderlyingAbstraction>;

      private:
        friend auto construct_handle(const std::filesystem::path& file_name, const OpenProperties& open_properties) -> ThisType;

        friend auto operator==(ThisType& a, ThisType& b) -> bool
        {
            return a.m_internal_handle.is_same_as(b.m_internal_handle);
        }

      private:
        FileType m_internal_handle{};

      protected:
        explicit HandleTemplate(FileType&& InternalHandle)
        {
            // Transfer the complete platform-specific state rather than
            // copying native resource handles and invalidating the source.
            using std::swap;
            swap(m_internal_handle, InternalHandle);
        }

      public:
        HandleTemplate() = default;
        HandleTemplate(const HandleTemplate&) = delete;
        HandleTemplate& operator=(const HandleTemplate& original) = delete;
        HandleTemplate& operator=(HandleTemplate&& Original)
        {
            if (this == &Original)
            {
                return *this;
            }

            // Release resources already owned by the destination before
            // accepting those of the source. If closure fails, leave the
            // source's ownership unchanged so cleanup can be retried.
            close();

            using std::swap;
            swap(m_internal_handle, Original.m_internal_handle);
            return *this;
        };
        HandleTemplate(HandleTemplate&& Original) noexcept(std::is_nothrow_default_constructible_v<FileType> && std::is_nothrow_swappable_v<FileType>)
        {
            // A moved-from handle receives the default, unowned state.
            // No native resource is copied or independently duplicated.
            using std::swap;
            swap(m_internal_handle, Original.m_internal_handle);
        }
        ~HandleTemplate() noexcept
        {
            // Explicit close() reports resource-release failures to the caller.
            // Destruction cannot safely propagate those exceptions, particularly
            // when another exception is already unwinding the stack.
            //
            // The platform implementations attempt all resource releases
            // before reporting errors, so suppressing the exception here
            // does not prevent the remaining cleanup from being attempted.
            try
            {
                close();
            }
            catch (...)
            {
                // Cleanup failures must not escape the destructor.
            }
        }

      public:
        [[nodiscard]] auto is_valid() -> bool
        {
            return m_internal_handle.is_valid();
        }

        auto delete_file() -> void
        {
            m_internal_handle.delete_file();
        }

        [[nodiscard]] auto get_raw_handle() -> void*
        {
            return m_internal_handle.get_raw_handle();
        }

        [[nodiscard]] auto get_underlying_type() -> FileType&
        {
            return m_internal_handle;
        }

        [[nodiscard]] auto get_file_path() -> const std::filesystem::path&
        {
            return m_internal_handle.get_file_path();
        }

        auto set_serialization_output_file(const std::filesystem::path& output_file) -> void
        {
            m_internal_handle.set_serialization_output_file(output_file);
        }

        auto serialize_identifying_properties() -> void
        {
            m_internal_handle.serialize_identifying_properties();
        }

        auto deserialize_identifying_properties() -> void
        {
            m_internal_handle.deserialize_identifying_properties();
        }

        auto is_deserialized_and_live_equal() -> bool
        {
            return m_internal_handle.is_deserialized_and_live_equal();
        }

        auto invalidate_serialization() -> void
        {
            m_internal_handle.invalidate_serialization();
        }

        template <typename SerializedDataType>
        auto serialize_item(SerializedDataType data) -> void
        {
            throw std::runtime_error{"not reached"};
        }

        template <>
        auto serialize_item(unsigned long data) -> void
        {
            m_internal_handle.serialize_item({.data_type = GenericDataType::UnsignedLong, .data_ulong = data}, false);
        }

        template <>
        auto serialize_item(signed long data) -> void
        {
            m_internal_handle.serialize_item({.data_type = GenericDataType::SignedLong, .data_long = data}, false);
        }

        template <>
        auto serialize_item(unsigned long long data) -> void
        {
            m_internal_handle.serialize_item({.data_type = GenericDataType::UnsignedLongLong, .data_ulonglong = data}, false);
        }

        template <>
        auto serialize_item(signed long long data) -> void
        {
            m_internal_handle.serialize_item({.data_type = GenericDataType::SignedLongLong, .data_longlong = data}, false);
        }

        template <>
        auto serialize_item(unsigned int data) -> void
        {
            serialize_item<unsigned long>(data);
        }

        template <>
        auto serialize_item(signed int data) -> void
        {
            serialize_item<signed long>(data);
        }

        template <typename SerializedDataType>
        auto get_serialized_item() -> SerializedDataType
        {
            void* Data = m_internal_handle.get_serialized_item(sizeof(SerializedDataType), false);
            if (!Data)
            {
                THROW_INTERNAL_FILE_ERROR("Attempted to get serialized item but the data was nullptr")
            }

            if constexpr (std::is_integral_v<SerializedDataType> &&
                          (sizeof(SerializedDataType) == 4 || sizeof(SerializedDataType) == 8))
            {
                // The supported integer types are returned as native bytes.
                // Copy into byte storage before constructing the value:
                // the source need not contain a live, aligned integer object.
                // This also avoids requiring a default constructor.
                std::array<std::byte, sizeof(SerializedDataType)> Bytes{};
                std::memcpy(Bytes.data(), Data, Bytes.size());
                return std::bit_cast<SerializedDataType>(Bytes);
            }
            else
            {
                // Preserve the previous template behaviour for types outside
                // the documented integer serialisation contract. In
                // particular, do not add new construction requirements.
                return *static_cast<SerializedDataType*>(Data);
            }
        }

        auto write_string_to_file(StringViewType string_to_write) -> void
        {
            m_internal_handle.write_string_to_file(string_to_write);
        }

        [[nodiscard]] auto read_all() const -> StringType
        {
            return m_internal_handle.read_all();
        }

        [[nodiscard]] auto memory_map() -> std::span<uint8_t>
        {
            return m_internal_handle.memory_map();
        }

        auto close() -> void
        {
            m_internal_handle.close_current_file();
        }
    };
} // namespace RC::File
