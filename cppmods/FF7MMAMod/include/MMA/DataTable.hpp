#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <String/StringType.hpp>
#include "Types.hpp"
#include "FrozenReader.hpp"

namespace RC::FF7MMA {

    struct PropertyArray;

    using PropertyValue = std::variant<
        bool,           // BoolProperty
        uint8_t,        // ByteProperty
        int8_t,         // Int8Property
        uint16_t,       // UInt16Property
        int16_t,        // Int16Property
        uint32_t,       // UIntProperty
        int32_t,        // IntProperty
        int64_t,        // Int64Property
        float,          // FloatProperty
        StringType,     // StrProperty
        FName,          // NameProperty
        std::shared_ptr<PropertyArray>  // any *_Array member
    >;

    struct PropertyArray {
        EFF7PropertyType ElementType;
        std::vector<PropertyValue> Elements;
        uint64_t DataOffset = 0; // element data offset in the parsed image, 0 if added later
    };

    struct MMARow {
        FName RowName;
        int32_t Index = -1;
        int32_t NextIndex = -1;
        int32_t Priority = 0;
        int32_t SlotIndex = -1; // slot in the keys sparse array
        std::vector<PropertyValue> Values; // parallel to schema
        // Absolute image offset of each value (scalar storage, or the 16-byte
        // header for strings/arrays). Empty for rows added after parsing.
        std::vector<uint64_t> ValueOffsets;
    };

    /**
     * Parsed representation of a frozen EndDataObject table.
     * Parsing mirrors CUE4Parse/UAssetAPI semantics exactly so runtime dumps can be
     * compared byte-for-byte against UAssetAPI JSON exports.
     */
    class MMADataTable {
    public:
        MMADataTable() = default;

        // schemaOnly skips keys, hash buckets and rows: cheap schema cataloguing
        bool ParseFromLayout(const void* content, size_t contentSize, const FTypeLayoutDesc* layout,
                             bool schemaOnly = false);

        const std::vector<FFF7Property>& GetSchema() const { return m_schema; }
        const std::vector<MMARow>& GetRows() const { return m_rows; }
        std::vector<MMARow>& GetRowsMutable() { return m_rows; }
        const std::vector<int32_t>& GetHashIndexes() const { return m_hashIndexes; }
        const std::vector<std::string>& GetWarnings() const { return m_warnings; }
        const std::string& GetError() const { return m_error; }

        MMARow* FindRow(FName rowName);
        int32_t FindPropertyIndex(FName name) const;

        // Writes the parsed table as UTF-8 JSON. Returns false on IO failure.
        bool ExportToJsonFile(const std::filesystem::path& path, const std::string& tableName) const;

        // Original frozen content location, for later unfreeze/write-back work.
        const void* GetContentPtr() const { return m_contentPtr; }
        size_t GetContentSize() const { return m_contentSize; }
        const FTypeLayoutDesc* GetLayout() const { return m_layout; }
        uint32_t GetKeyToIndexOffset() const { return m_keyToIndexOffset; }
        uint32_t GetValuesOffset() const { return m_valuesOffset; }

    private:
        PropertyValue ReadScalar(ImageReader& reader, EFF7PropertyType type, uint64_t* outOffset = nullptr);
        PropertyValue ReadProperty(ImageReader& reader, const FFF7Property& prop, uint64_t& outOffset);

        std::vector<FFF7Property> m_schema;
        std::vector<MMARow> m_rows;
        std::vector<int32_t> m_hashIndexes;
        std::unordered_map<uint64_t, size_t> m_rowNameToIndex; // (comparison index << 32) | number
        std::vector<std::string> m_warnings;
        std::string m_error;

        const void* m_contentPtr = nullptr;
        size_t m_contentSize = 0;
        const FTypeLayoutDesc* m_layout = nullptr;
        uint32_t m_keyToIndexOffset = 0;
        uint32_t m_membersOffset = 0;
        uint32_t m_valuesOffset = 0;
    };

} // namespace RC::FF7MMA
