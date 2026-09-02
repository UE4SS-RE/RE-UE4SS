#include <MMA/Writer.hpp>
#include <Unreal/Core/Hash/CityHash.hpp>
#include <Unreal/Core/Serialization/FrozenMemoryImage.hpp>
#include <DynamicOutput/Output.hpp>

#include <cstring>

namespace RC::FF7MMA {

    namespace {
        uint32_t RoundUpPow2(uint32_t v) {
            if (v <= 1) return 1;
            --v;
            v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
            return v + 1;
        }

        // FChar::ToLower semantics: ASCII plus Latin-1 (0xC0-0xDE except 0xD7)
        CharType ToLowerChar(CharType c) {
            uint16_t u = static_cast<uint16_t>(c);
            if ((u >= 'A' && u <= 'Z') || (u >= 0xC0 && u <= 0xDE && u != 0xD7)) {
                return static_cast<CharType>(u + 0x20);
            }
            return c;
        }

        using Blob = Unreal::FFrozenImageWriter;

        struct StringFixup {
            uint64_t HeaderPos;
            StringType Value;
        };

        struct ArrayFixup {
            uint64_t HeaderPos;
            EFF7PropertyType Type;
            const PropertyArray* Array;
        };

        // Writes a 16-byte array/string header. Empty: packed=1, num=max=0.
        void WriteHeaderPlaceholder(Blob& blob, int32_t num) {
            blob.Write<uint64_t>(1); // patched later when num > 0
            blob.Write<int32_t>(num);
            blob.Write<int32_t>(num);
        }

        void WriteScalar(Blob& blob, EFF7PropertyType type, const PropertyValue& value,
                         std::vector<StringFixup>& stringFixups) {
            switch (type) {
                case EFF7PropertyType::BoolProperty:
                    blob.Write<uint8_t>(std::get<bool>(value) ? 1 : 0);
                    break;
                case EFF7PropertyType::ByteProperty:
                    blob.Write<uint8_t>(std::get<uint8_t>(value));
                    break;
                case EFF7PropertyType::Int8Property:
                    blob.Write<int8_t>(std::get<int8_t>(value));
                    break;
                case EFF7PropertyType::UInt16Property:
                    blob.Write<uint16_t>(std::get<uint16_t>(value));
                    break;
                case EFF7PropertyType::Int16Property:
                    blob.Write<int16_t>(std::get<int16_t>(value));
                    break;
                case EFF7PropertyType::UIntProperty:
                    blob.Align(4);
                    blob.Write<uint32_t>(std::get<uint32_t>(value));
                    break;
                case EFF7PropertyType::IntProperty:
                    blob.Align(4);
                    blob.Write<int32_t>(std::get<int32_t>(value));
                    break;
                case EFF7PropertyType::Int64Property:
                    blob.Write<int64_t>(std::get<int64_t>(value));
                    break;
                case EFF7PropertyType::FloatProperty:
                    blob.Align(4);
                    blob.Write<float>(std::get<float>(value));
                    break;
                case EFF7PropertyType::StrProperty: {
                    blob.Align(8);
                    const StringType& str = std::get<StringType>(value);
                    if (str.empty()) {
                        WriteHeaderPlaceholder(blob, 0);
                    } else {
                        stringFixups.push_back({blob.Pos(), str});
                        WriteHeaderPlaceholder(blob, static_cast<int32_t>(str.size()) + 1);
                    }
                    break;
                }
                case EFF7PropertyType::NameProperty: {
                    blob.Align(4);
                    FName name = std::get<FName>(value);
                    blob.Write<uint32_t>(name.GetComparisonIndex().ToUnstableInt());
                    blob.Write<uint32_t>(static_cast<uint32_t>(name.GetNumber()));
                    break;
                }
                default:
                    blob.Align(4);
                    blob.Write<int32_t>(0);
                    break;
            }
        }
    }

    uint64_t MMAImageBuilder::HashName(FName name) {
        // hash the base entry string (number stripped), lowercased;
        // pure 8-bit names hash as ANSI bytes, others as UTF-16LE
        FName baseName(name.GetComparisonIndex().ToUnstableInt(), 0);
        StringType base = baseName.ToString();

        bool wide = false;
        for (CharType c : base) {
            if (static_cast<uint16_t>(c) > 0xFF) { wide = true; break; }
        }

        uint64_t hash;
        if (!wide) {
            std::string bytes;
            bytes.reserve(base.size());
            for (CharType c : base) {
                bytes.push_back(static_cast<char>(static_cast<uint8_t>(ToLowerChar(c))));
            }
            hash = Unreal::CityHash64(bytes.data(), static_cast<uint32_t>(bytes.size()));
        } else {
            std::vector<char16_t> bytes;
            bytes.reserve(base.size());
            for (CharType c : base) {
                bytes.push_back(static_cast<char16_t>(ToLowerChar(c)));
            }
            hash = Unreal::CityHash64(reinterpret_cast<const char*>(bytes.data()),
                              static_cast<uint32_t>(bytes.size() * 2));
        }
        return hash + static_cast<uint32_t>(name.GetNumber());
    }

    MMAImageBuilder::Result MMAImageBuilder::Build(const MMADataTable& table, std::vector<std::string>& warnings) {
        const auto& schema = table.GetSchema();
        const auto& rows = table.GetRows();
        const int32_t numRows = static_cast<int32_t>(rows.size());
        const int32_t numProps = static_cast<int32_t>(schema.size());

        const uint32_t bucketCount = RoundUpPow2(static_cast<uint32_t>(numRows > 1 ? numRows / 2 : 1));
        const uint32_t bucketMask = bucketCount - 1;

        // Hash chains via head-insertion over slots 0..N-1 (cooker convention)
        std::vector<int32_t> heads(bucketCount, -1);
        std::vector<int32_t> nextIndex(numRows, -1);
        for (int32_t i = 0; i < numRows; ++i) {
            uint32_t bucket = static_cast<uint32_t>(HashName(rows[i].RowName)) & bucketMask;
            nextIndex[i] = heads[bucket];
            heads[bucket] = i;
        }

        Blob blob;
        blob.Data.reserve(1 << 20);

        // Root struct: sparse array @0, Indexes @40, Members @56, Values @72
        constexpr uint64_t RootSize = 88;
        blob.Data.resize(RootSize, 0);

        // Keys: 20-byte slots, all allocated
        blob.Align(8);
        uint64_t keysPos = blob.Pos();
        for (int32_t i = 0; i < numRows; ++i) {
            const MMARow& row = rows[i];
            blob.Write<uint32_t>(row.RowName.GetComparisonIndex().ToUnstableInt());
            blob.Write<uint32_t>(static_cast<uint32_t>(row.RowName.GetNumber()));
            blob.Write<int32_t>(i);
            blob.Write<int32_t>(nextIndex[i]);
            blob.Write<int32_t>(row.Priority);
        }

        // Allocation bit array (all set)
        blob.Align(8);
        uint64_t bitsPos = blob.Pos();
        int32_t numWords = (numRows + 31) / 32;
        for (int32_t w = 0; w < numWords; ++w) {
            uint32_t word = 0;
            for (int32_t b = 0; b < 32; ++b) {
                if (w * 32 + b < numRows) word |= (1u << b);
            }
            blob.Write<uint32_t>(word);
        }

        // Hash buckets
        blob.Align(8);
        uint64_t indexesPos = blob.Pos();
        for (uint32_t b = 0; b < bucketCount; ++b) {
            blob.Write<int32_t>(heads[b]);
        }

        // Schema
        blob.Align(8);
        uint64_t membersPos = blob.Pos();
        for (const auto& prop : schema) {
            blob.Write<uint32_t>(prop.Name.GetComparisonIndex().ToUnstableInt());
            blob.Write<uint32_t>(static_cast<uint32_t>(prop.Name.GetNumber()));
            blob.Write<int32_t>(static_cast<int32_t>(prop.Type));
        }

        // Rows, sequential with reader-identical alignment
        std::vector<StringFixup> stringFixups;
        std::vector<ArrayFixup> arrayFixups;

        blob.Align(8);
        uint64_t valuesPos = blob.Pos();
        for (const MMARow& row : rows) {
            if (row.Values.size() != schema.size()) {
                warnings.push_back("row value count mismatch, skipping malformed row");
            }
            for (int32_t p = 0; p < numProps; ++p) {
                const auto& prop = schema[p];
                const PropertyValue& value = row.Values[p];
                if (prop.bIsArray) {
                    blob.Align(8);
                    auto arr = std::get<std::shared_ptr<PropertyArray>>(value);
                    int32_t num = arr ? static_cast<int32_t>(arr->Elements.size()) : 0;
                    if (num > 0) {
                        arrayFixups.push_back({blob.Pos(), prop.Type, arr.get()});
                    }
                    WriteHeaderPlaceholder(blob, num);
                } else {
                    WriteScalar(blob, prop.Type, value, stringFixups);
                }
            }
        }

        // Array element data
        for (const auto& fixup : arrayFixups) {
            uint64_t elemAlign;
            switch (fixup.Type) {
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
            blob.Align(8);
            blob.PatchFrozenPtr(fixup.HeaderPos, blob.Pos());
            for (const PropertyValue& elem : fixup.Array->Elements) {
                WriteScalar(blob, fixup.Type, elem, stringFixups);
                blob.Align(elemAlign);
            }
        }

        // String character data (including strings inside arrays, queued above)
        for (const auto& fixup : stringFixups) {
            blob.Align(2);
            blob.PatchFrozenPtr(fixup.HeaderPos, blob.Pos());
            blob.WriteBytes(fixup.Value.data(), fixup.Value.size() * sizeof(CharType));
            blob.Write<CharType>(0);
        }

        blob.Align(8);

        // Root struct headers (frozen pointers are relative to their own position)
        blob.PatchFrozenPtr(0, keysPos);
        blob.Patch<int32_t>(8, numRows);
        blob.Patch<int32_t>(12, numRows);
        blob.PatchFrozenPtr(16, bitsPos);
        blob.Patch<int32_t>(24, numRows);
        blob.Patch<int32_t>(28, numRows);
        blob.Patch<int32_t>(32, -1); // FirstFreeIndex
        blob.Patch<int32_t>(36, 0);  // NumFreeIndices
        blob.PatchFrozenPtr(40, indexesPos);
        blob.Patch<int32_t>(48, static_cast<int32_t>(bucketCount));
        blob.Patch<int32_t>(52, static_cast<int32_t>(bucketCount));
        blob.PatchFrozenPtr(56, membersPos);
        blob.Patch<int32_t>(64, numProps);
        blob.Patch<int32_t>(68, numProps);
        blob.PatchFrozenPtr(72, valuesPos);
        blob.Patch<int32_t>(80, numRows);
        blob.Patch<int32_t>(84, numRows);

        Result result;
        result.Size = blob.Data.size();
        result.Data = static_cast<uint8_t*>(::operator new(result.Size));
        std::memcpy(result.Data, blob.Data.data(), result.Size);
        result.KeysPos = keysPos;
        result.BitsPos = bitsPos;
        result.IndexesPos = indexesPos;
        result.MembersPos = membersPos;
        result.ValuesPos = valuesPos;
        result.NumRows = numRows;
        result.BucketCount = bucketCount;
        return result;
    }

} // namespace RC::FF7MMA
