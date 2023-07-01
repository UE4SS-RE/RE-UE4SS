#ifndef RC_FILE_HANDLETEMPLATE_HPP
#define RC_FILE_HANDLETEMPLATE_HPP

#include <string>
#include <memory>
#include <filesystem>
#include <span>

#include <File/Common.hpp>
#include <File/FileType/FileBase.hpp>
#include <File/Enums.hpp>
#include <File/Exceptions.hpp>

namespace RC::File
{
    template<ImplementsFileInterface UnderlyingAbstraction>
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
        explicit HandleTemplate(FileType&& internal_handle)
                : m_internal_handle(std::move(internal_handle))
        {
        }

    public:
        HandleTemplate() = default;
        HandleTemplate(const HandleTemplate&) = delete;
        HandleTemplate& operator=(const HandleTemplate& original) = delete;
        HandleTemplate& operator=(HandleTemplate&& original)
        {
            m_internal_handle = std::move(original.m_internal_handle);
            original.m_internal_handle.invalidate_file();
            return *this;
        };
        HandleTemplate(HandleTemplate&& original) noexcept
        {
            m_internal_handle = std::move(original.m_internal_handle);
            original.m_internal_handle.invalidate_file();
        }
        ~HandleTemplate() { close(); }

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

        template<typename SerializedDataType>
        auto serialize_item(SerializedDataType data) -> void
        {
            throw std::runtime_error{"not reached"};
        }

        template<>
        auto serialize_item(unsigned long data) -> void
        {
            m_internal_handle.serialize_item({.data_type = GenericDataType::UnsignedLong, .data_ulong = data}, false);
        }

        template<>
        auto serialize_item(signed long data) -> void
        {
            m_internal_handle.serialize_item({.data_type = GenericDataType::SignedLong, .data_long = data}, false);
        }

        template<>
        auto serialize_item(unsigned long long data) -> void
        {
            m_internal_handle.serialize_item({.data_type = GenericDataType::UnsignedLongLong, .data_ulonglong = data}, false);
        }

        template<>
        auto serialize_item(signed long long data) -> void
        {
            m_internal_handle.serialize_item({.data_type = GenericDataType::SignedLongLong, .data_longlong = data}, false);
        }

        template<>
        auto serialize_item(unsigned int data) -> void
        {
            serialize_item<unsigned long>(data);
        }

        template<>
        auto serialize_item(signed int data) -> void
        {
            serialize_item<signed long>(data);
        }

        template<typename SerializedDataType>
        auto get_serialized_item() -> SerializedDataType
        {
            auto* data = static_cast<SerializedDataType*>(m_internal_handle.get_serialized_item(sizeof(SerializedDataType), false));
            if (!data)
            {
                THROW_INTERNAL_FILE_ERROR("Attempted to get serialized item but the data was nullptr")
            }
            return *data;
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
}

#endif //RC_FILE_HANDLETEMPLATE_HPP
