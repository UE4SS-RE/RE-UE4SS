#ifndef RC_FILE_FILEBASE_HPP
#define RC_FILE_FILEBASE_HPP

#include <type_traits>
#include <span>

#include <File/InternalFile.hpp>
#include <File/Enums.hpp>
#include <File/Macros.hpp>

namespace RC::File
{
    template<typename InternalFileType>
    class RC_FILE_API FileInterface
    {
    public:
        virtual ~FileInterface() = default;

    public:
        // Returns whether the underlying file handle is valid
        virtual auto is_valid() noexcept -> bool = 0;

        // Invalidate the underlying file type.
        // This is used in move constructors to stop destruction of the underlying file.
        virtual auto invalidate_file() noexcept -> void = 0;

        // Closes & deletes the currently opened file
        // Throws std::runtime_error if an error occurred
        virtual auto delete_file() -> void = 0;

        /*
        // Deletes a file
        // Throws std::runtime_error if an error occurred
        auto static delete_file(const std::filesystem::path& file_path_and_name) -> void;
        */

        // Returns a pointer to the raw handle, do not use unless you know what you're doing
        virtual auto get_raw_handle() noexcept -> void* = 0;

        // Returns the full path to the file
        virtual auto get_file_path() const -> const std::filesystem::path& = 0;

        // Sets the full path and file name for the serialization file associated with this file
        virtual auto set_serialization_output_file(const std::filesystem::path& output_file) noexcept -> void = 0;

        // Serialize identifying properties to the serialization file
        // Throws std::runtime_error if an error occurred
        virtual auto serialize_identifying_properties() -> void = 0;

        // Deserialize the identifying properties from the serialization file
        // Access them via 'is_deserialized_and_live_equal'
        // Throws std::runtime_error if an error occurred
        virtual auto deserialize_identifying_properties() -> void = 0;

        // Returns whether the deserialized identifying data is equal to the currently opened file
        // Throws std::runtime_error if an error occurred
        virtual auto is_deserialized_and_live_equal() -> bool = 0;

        // Invalidate the serialization file, rendering any calls to 'is_deserialized_and_live_equal' to return false
        // Throws std::runtime_error if an error occurred
        virtual auto invalidate_serialization() -> void = 0;

        // Serialize a custom item to the serialization file
        // Must call 'set_serialization_output_file' before calling this function
        // Supported data types: un/signed int, un/signed long, un/signed longlong
        // Throws std::runtime_error if an error occurred
        virtual auto serialize_item(const GenericItemData& data, bool is_internal_item) -> void = 0;

        // Returns a serialized item from the serialization file
        // Throws std::runtime_error if an error occurred
        virtual auto get_serialized_item(size_t data_size, bool is_internal_item) -> void* = 0;

        // Throws std::runtime_error if an error occurred
        virtual auto close_current_file() -> void = 0;

        // Write a string to the currently opened file
        // Throws std::runtime_error if an error occurred
        virtual auto write_string_to_file(StringViewType) -> void = 0;

        // Returns whether the currently opened file is the same as another opened file
        // Throws std::runtime_error if an error occurred
        virtual auto is_same_as(InternalFileType& other_file) -> bool = 0;

        // Returns the entire contents of the currently opened file as a string
        // Throws std::runtime_error if an error occurred
        virtual auto read_all() const -> StringType = 0;

        virtual auto memory_map() -> std::span<uint8_t> = 0;

        /*
        // Opens a file
        // Throws std::runtime_error if an error occurred
        auto static open_file(const std::filesystem::path& file_name_and_path, const OpenProperties& open_properties) -> InternalFileType;
        */
    };

    template<typename FileAbstraction>
    concept ImplementsFileInterface = std::is_base_of_v<FileInterface<FileAbstraction>, FileAbstraction>;
}


#endif //RC_FILE_FILEBASE_HPP
