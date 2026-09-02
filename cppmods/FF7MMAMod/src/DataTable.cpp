#include <MMA/DataTable.hpp>
#include <DynamicOutput/Output.hpp>
#include <Helpers/String.hpp>

#include <cmath>
#include <cstdio>
#include <fstream>

using namespace RC;

namespace RC::FF7MMA {

    namespace {
        constexpr uint64_t SparseArrayHeaderSize = 40; // ptr+num+max + bitarray(16) + firstFree+numFree
        constexpr uint64_t KeySlotSize = 20;
        constexpr uint64_t SchemaEntrySize = 12;

        bool NameEndsWithArray(const StringType& name) {
            static const StringType suffix = STR("_Array");
            return name.size() > suffix.size() &&
                   name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
        }
    }

    bool MMADataTable::ParseFromLayout(const void* content, size_t contentSize, const FTypeLayoutDesc* layout,
                                       bool schemaOnly) {
        m_schema.clear();
        m_rows.clear();
        m_hashIndexes.clear();
        m_rowNameToIndex.clear();
        m_warnings.clear();
        m_error.clear();

        if (!content || contentSize == 0 || !layout) {
            m_error = "missing content or layout";
            return false;
        }

        m_contentPtr = content;
        m_contentSize = contentSize;
        m_layout = layout;

        // Locate the root fields from the runtime type layout descriptor
        bool foundKeyToIndex = false, foundMembers = false, foundValues = false;
        for (const FFieldLayoutDesc* field = layout->Fields; field; field = field->Next) {
            if (!field->Name) continue;
            std::wstring fieldName(field->Name);
            if (fieldName == L"KeyToIndex") { m_keyToIndexOffset = field->Offset; foundKeyToIndex = true; }
            else if (fieldName == L"Members") { m_membersOffset = field->Offset; foundMembers = true; }
            else if (fieldName == L"Values") { m_valuesOffset = field->Offset; foundValues = true; }
        }
        if (!foundKeyToIndex || !foundMembers || !foundValues) {
            // Not an EndDataObject table (animations and other MMA types share the
            // asset container but not this root layout); parsing them as tables
            // produces garbage FNames that crash on ToString
            m_error = "layout '" +
                      (layout->Name ? std::string(to_utf8_string(std::wstring(layout->Name))) : std::string("?")) +
                      "' is not an EndDataObject table";
            return false;
        }

        ImageReader reader(content, contentSize);
        reader.Warnings = &m_warnings;

        try {
            // 1. Keys: TSparseArray<FNameKey> with 20-byte slots
            std::vector<MMARow> keyOrder;
            if (!schemaOnly) {
                reader.Pos = m_keyToIndexOffset;
                uint64_t ptrPos = reader.Pos;
                FFrozenMemoryImagePtr dataPtr{reader.Read<uint64_t>()};
                int32_t dataNum = reader.Read<int32_t>();
                int32_t dataMax = reader.Read<int32_t>();
                (void)dataMax;
                std::vector<bool> allocationFlags = reader.ReadBitArray();
                reader.Read<int32_t>(); // FirstFreeIndex
                reader.Read<int32_t>(); // NumFreeIndices

                if (dataNum < 0 || dataNum > 5000000) {
                    throw ParseError("implausible key count " + std::to_string(dataNum));
                }
                if (dataNum > 0) {
                    if (!dataPtr.IsFrozen()) {
                        throw ParseError("keys sparse array has unfrozen data pointer");
                    }
                    int64_t keysData = static_cast<int64_t>(ptrPos) + dataPtr.GetOffsetFromThis();
                    reader.Require(static_cast<uint64_t>(keysData), static_cast<uint64_t>(dataNum) * KeySlotSize);

                    for (int32_t i = 0; i < dataNum; ++i) {
                        bool allocated = i < static_cast<int32_t>(allocationFlags.size()) && allocationFlags[i];
                        if (!allocated) continue;
                        uint64_t slot = static_cast<uint64_t>(keysData) + static_cast<uint64_t>(i) * KeySlotSize;
                        MMARow row;
                        row.RowName = FName(reader.ReadAt<uint32_t>(slot), reader.ReadAt<uint32_t>(slot + 4));
                        row.Index = reader.ReadAt<int32_t>(slot + 8);
                        row.NextIndex = reader.ReadAt<int32_t>(slot + 12);
                        row.Priority = reader.ReadAt<int32_t>(slot + 16);
                        row.SlotIndex = i;
                        keyOrder.push_back(std::move(row));
                    }
                }
            }

            // 2. Hash buckets: TArray<int32> directly after the sparse array
            if (!schemaOnly) {
                reader.Pos = m_keyToIndexOffset + SparseArrayHeaderSize;
                ArrayHeader header = reader.ReadArrayHeader(false, "Indexes");
                if (header.Num > 0 && header.Num < 50000000) {
                    reader.Require(header.DataOffset, static_cast<uint64_t>(header.Num) * 4);
                    m_hashIndexes.resize(header.Num);
                    std::memcpy(m_hashIndexes.data(), reader.Base() + header.DataOffset,
                                static_cast<size_t>(header.Num) * 4);
                }
            }

            // 3. Schema: TArray<FF7Property>, 12-byte entries
            {
                reader.Pos = m_membersOffset;
                ArrayHeader header = reader.ReadArrayHeader(true, "Members");
                if (header.Num > 100000) {
                    throw ParseError("implausible schema size " + std::to_string(header.Num));
                }
                // Validate all type ids before any FName is stringified; a bad type
                // means this is not really a table and the FNames are garbage
                for (int32_t i = 0; i < header.Num; ++i) {
                    reader.Pos = header.DataOffset + static_cast<uint64_t>(i) * SchemaEntrySize;
                    FFF7Property prop;
                    prop.Name = reader.ReadFName();
                    int32_t rawType = reader.Read<int32_t>();
                    if (rawType < 1 || rawType > 11) {
                        throw ParseError("schema[" + std::to_string(i) + "] has invalid type " +
                                         std::to_string(rawType) + ", not a data table");
                    }
                    prop.Type = static_cast<EFF7PropertyType>(rawType);
                    m_schema.push_back(prop);
                }
                for (auto& prop : m_schema) {
                    prop.bIsArray = NameEndsWithArray(prop.Name.ToString());
                }
            }

            if (m_schema.empty()) {
                m_error = "no schema properties found";
                return false;
            }

            // 4. Rows: TArray of schema-shaped structs, read sequentially with no
            // per-row stride, matching CUE4Parse/UAssetAPI exactly.
            if (!schemaOnly) {
                reader.Pos = m_valuesOffset;
                ArrayHeader header = reader.ReadArrayHeader(true, "Values");
                if (header.Num > 5000000) {
                    throw ParseError("implausible row count " + std::to_string(header.Num));
                }
                if (static_cast<size_t>(header.Num) != keyOrder.size()) {
                    m_warnings.push_back("row count " + std::to_string(header.Num) +
                                         " != allocated key count " + std::to_string(keyOrder.size()));
                }

                m_rows.reserve(header.Num);
                reader.Pos = header.DataOffset;
                for (int32_t i = 0; i < header.Num; ++i) {
                    MMARow row;
                    if (static_cast<size_t>(i) < keyOrder.size()) {
                        const MMARow& key = keyOrder[i];
                        row.RowName = key.RowName;
                        row.Index = key.Index;
                        row.NextIndex = key.NextIndex;
                        row.Priority = key.Priority;
                        row.SlotIndex = key.SlotIndex;
                        if (key.Index != i) {
                            m_warnings.push_back("key[" + std::to_string(i) + "] has Index " +
                                                 std::to_string(key.Index));
                        }
                    }
                        row.Values.reserve(m_schema.size());
                    row.ValueOffsets.reserve(m_schema.size());
                    for (const auto& prop : m_schema) {
                        uint64_t valueOffset = 0;
                        row.Values.push_back(ReadProperty(reader, prop, valueOffset));
                        row.ValueOffsets.push_back(valueOffset);
                    }
                    uint64_t rowKey = (static_cast<uint64_t>(row.RowName.GetComparisonIndex().ToUnstableInt()) << 32) |
                                      static_cast<uint32_t>(row.RowName.GetNumber());
                    m_rowNameToIndex[rowKey] = m_rows.size();
                    m_rows.push_back(std::move(row));
                }
            }
        }
        catch (const ParseError& e) {
            m_error = e.what();
            Output::send<LogLevel::Warning>(STR("[FF7MMAMod] Parse failed: {}\n"),
                                            to_generic_string(std::string(e.what())));
            return false;
        }

        return true;
    }

    PropertyValue MMADataTable::ReadProperty(ImageReader& reader, const FFF7Property& prop, uint64_t& outOffset) {
        if (!prop.bIsArray) {
            return ReadScalar(reader, prop.Type, &outOffset);
        }

        reader.Align(8);
        outOffset = reader.Pos; // array header position
        ArrayHeader header = reader.ReadArrayHeader(true, "array property");
        uint64_t continuePos = reader.Pos;

        auto arr = std::make_shared<PropertyArray>();
        arr->ElementType = prop.Type;

        if (header.Num > 0) {
            if (header.Num > 1000000) {
                throw ParseError("implausible array property size " + std::to_string(header.Num));
            }
            uint64_t elemAlign;
            switch (prop.Type) {
                case EFF7PropertyType::BoolProperty:
                case EFF7PropertyType::ByteProperty:
                case EFF7PropertyType::Int8Property:
                    elemAlign = 1; break;
                case EFF7PropertyType::UInt16Property:
                case EFF7PropertyType::Int16Property:
                    elemAlign = 2; break;
                default:
                    elemAlign = 4; break;
            }
            arr->Elements.reserve(header.Num);
            arr->DataOffset = header.DataOffset;
            reader.Pos = header.DataOffset;
            for (int32_t i = 0; i < header.Num; ++i) {
                arr->Elements.push_back(ReadScalar(reader, prop.Type));
                reader.Align(elemAlign);
            }
        }

        reader.Pos = continuePos;
        return arr;
    }

    PropertyValue MMADataTable::ReadScalar(ImageReader& reader, EFF7PropertyType type, uint64_t* outOffset) {
        // record where the value starts, after its type-specific alignment
        switch (type) {
            case EFF7PropertyType::UIntProperty:
            case EFF7PropertyType::IntProperty:
            case EFF7PropertyType::FloatProperty:
            case EFF7PropertyType::NameProperty:
                reader.Align(4);
                break;
            case EFF7PropertyType::StrProperty:
                reader.Align(8);
                break;
            default:
                break;
        }
        if (outOffset) {
            *outOffset = reader.Pos;
        }
        switch (type) {
            case EFF7PropertyType::BoolProperty: {
                uint8_t b = reader.Read<uint8_t>();
                if (b > 1) {
                    reader.Warn("bool value " + std::to_string(b) + " at offset " +
                                std::to_string(reader.Pos - 1));
                }
                return b != 0;
            }
            case EFF7PropertyType::ByteProperty:
                return reader.Read<uint8_t>();
            case EFF7PropertyType::Int8Property:
                return reader.Read<int8_t>();
            case EFF7PropertyType::UInt16Property:
                return reader.Read<uint16_t>();
            case EFF7PropertyType::Int16Property:
                return reader.Read<int16_t>();
            case EFF7PropertyType::UIntProperty:
                return reader.Read<uint32_t>();
            case EFF7PropertyType::IntProperty:
                return reader.Read<int32_t>();
            case EFF7PropertyType::Int64Property:
                return reader.Read<int64_t>();
            case EFF7PropertyType::FloatProperty:
                return reader.Read<float>();
            case EFF7PropertyType::StrProperty:
                return reader.ReadString();
            case EFF7PropertyType::NameProperty:
                return reader.ReadFName();
            default:
                reader.Align(4);
                return reader.Read<int32_t>();  // unknown types read as int32
        }
    }

    int32_t MMADataTable::FindPropertyIndex(FName name) const {
        for (size_t i = 0; i < m_schema.size(); ++i) {
            if (m_schema[i].Name == name) {
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    MMARow* MMADataTable::FindRow(FName rowName) {
        uint64_t rowKey = (static_cast<uint64_t>(rowName.GetComparisonIndex().ToUnstableInt()) << 32) |
                          static_cast<uint32_t>(rowName.GetNumber());
        auto it = m_rowNameToIndex.find(rowKey);
        if (it != m_rowNameToIndex.end() && it->second < m_rows.size()) {
            return &m_rows[it->second];
        }
        return nullptr;
    }

    // ---------------- JSON export ----------------

    namespace {
        void AppendEscaped(std::string& out, const StringType& value) {
            out += '"';
            for (CharType c : value) {
                uint16_t u = static_cast<uint16_t>(c);
                switch (u) {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default:
                        if (u >= 0x20 && u < 0x7F) {
                            out += static_cast<char>(u);
                        } else {
                            char buf[8];
                            snprintf(buf, sizeof(buf), "\\u%04X", u);
                            out += buf;
                        }
                }
            }
            out += '"';
        }

        void AppendFloat(std::string& out, float value) {
            if (std::isnan(value)) { out += "NaN"; return; }
            if (std::isinf(value)) { out += value > 0 ? "Infinity" : "-Infinity"; return; }
            char buf[48];
            snprintf(buf, sizeof(buf), "%.9g", value);
            out += buf;
        }

        void AppendValue(std::string& out, const PropertyValue& value) {
            std::visit([&out](auto&& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, bool>) {
                    out += val ? "true" : "false";
                } else if constexpr (std::is_same_v<T, float>) {
                    AppendFloat(out, val);
                } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                                     std::is_same_v<T, uint32_t>) {
                    out += std::to_string(static_cast<uint64_t>(val));
                } else if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> ||
                                     std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t>) {
                    out += std::to_string(static_cast<int64_t>(val));
                } else if constexpr (std::is_same_v<T, StringType>) {
                    AppendEscaped(out, val);
                } else if constexpr (std::is_same_v<T, FName>) {
                    AppendEscaped(out, val.ToString());
                } else if constexpr (std::is_same_v<T, std::shared_ptr<PropertyArray>>) {
                    out += '[';
                    if (val) {
                        for (size_t i = 0; i < val->Elements.size(); ++i) {
                            if (i > 0) out += ',';
                            AppendValue(out, val->Elements[i]);
                        }
                    }
                    out += ']';
                } else {
                    out += "null";
                }
            }, value);
        }
    }

    bool MMADataTable::ExportToJsonFile(const std::filesystem::path& path, const std::string& tableName) const {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return false;
        }

        std::string buf;
        buf.reserve(1 << 20);

        buf += "{\n\"table\": ";
        AppendEscaped(buf, StringType(tableName.begin(), tableName.end()));
        buf += ",\n\"contentSize\": " + std::to_string(m_contentSize);
        buf += ",\n\"fieldOffsets\": {\"KeyToIndex\": " + std::to_string(m_keyToIndexOffset) +
               ", \"Members\": " + std::to_string(m_membersOffset) +
               ", \"Values\": " + std::to_string(m_valuesOffset) + "}";

        buf += ",\n\"warnings\": [";
        for (size_t i = 0; i < m_warnings.size() && i < 200; ++i) {
            if (i > 0) buf += ',';
            AppendEscaped(buf, StringType(m_warnings[i].begin(), m_warnings[i].end()));
        }
        buf += "]";

        buf += ",\n\"schema\": [";
        for (size_t i = 0; i < m_schema.size(); ++i) {
            if (i > 0) buf += ',';
            buf += "\n{\"name\": ";
            AppendEscaped(buf, m_schema[i].Name.ToString());
            buf += ", \"type\": \"";
            buf += PropertyTypeName(m_schema[i].Type);
            buf += "\", \"isArray\": ";
            buf += m_schema[i].bIsArray ? "true" : "false";
            buf += '}';
        }
        buf += "\n]";

        buf += ",\n\"keys\": [";
        out << buf;
        buf.clear();
        for (size_t i = 0; i < m_rows.size(); ++i) {
            const MMARow& row = m_rows[i];
            if (i > 0) buf += ',';
            buf += "\n{\"name\": ";
            AppendEscaped(buf, row.RowName.ToString());
            buf += ", \"index\": " + std::to_string(row.Index) +
                   ", \"nextIndex\": " + std::to_string(row.NextIndex) +
                   ", \"priority\": " + std::to_string(row.Priority) +
                   ", \"slot\": " + std::to_string(row.SlotIndex) + '}';
            if (buf.size() > (1 << 20)) { out << buf; buf.clear(); }
        }
        buf += "\n]";

        buf += ",\n\"indexes\": [";
        for (size_t i = 0; i < m_hashIndexes.size(); ++i) {
            if (i > 0) buf += ',';
            buf += std::to_string(m_hashIndexes[i]);
            if (buf.size() > (1 << 20)) { out << buf; buf.clear(); }
        }
        buf += "]";

        // Rows hold values positionally, matching schema order
        buf += ",\n\"rows\": [";
        for (size_t i = 0; i < m_rows.size(); ++i) {
            const MMARow& row = m_rows[i];
            if (i > 0) buf += ',';
            buf += "\n{\"$name\": ";
            AppendEscaped(buf, row.RowName.ToString());
            buf += ", \"v\": [";
            for (size_t vi = 0; vi < row.Values.size(); ++vi) {
                if (vi > 0) buf += ',';
                AppendValue(buf, row.Values[vi]);
            }
            buf += "]}";
            if (buf.size() > (1 << 20)) { out << buf; buf.clear(); }
        }
        buf += "\n]\n}\n";
        out << buf;

        return out.good();
    }

} // namespace RC::FF7MMA
