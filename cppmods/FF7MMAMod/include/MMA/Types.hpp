#pragma once

#include <cstdint>
#include <Unreal/NameTypes.hpp>
#include <Unreal/Core/Serialization/MemoryLayout.hpp>

namespace RC::FF7MMA {

    using Unreal::FName;
    using Unreal::FTypeLayoutDesc;
    using Unreal::FFieldLayoutDesc;

    // FF7 property type ids as stored in the Members schema array
    enum class EFF7PropertyType : int32_t {
        None = 0,
        BoolProperty = 1,
        ByteProperty = 2,
        Int8Property = 3,
        UInt16Property = 4,
        Int16Property = 5,
        UIntProperty = 6,
        IntProperty = 7,
        Int64Property = 8,
        FloatProperty = 9,
        StrProperty = 10,
        NameProperty = 11,
    };

    // UAssetAPI-compatible type name for dumps
    inline const char* PropertyTypeName(EFF7PropertyType type) {
        switch (type) {
            case EFF7PropertyType::BoolProperty: return "BoolProperty";
            case EFF7PropertyType::ByteProperty: return "ByteProperty";
            case EFF7PropertyType::Int8Property: return "Int8Property";
            case EFF7PropertyType::UInt16Property: return "UInt16Property";
            case EFF7PropertyType::Int16Property: return "Int16Property";
            case EFF7PropertyType::UIntProperty: return "UIntProperty";
            case EFF7PropertyType::IntProperty: return "IntProperty";
            case EFF7PropertyType::Int64Property: return "Int64Property";
            case EFF7PropertyType::FloatProperty: return "FloatProperty";
            case EFF7PropertyType::StrProperty: return "StrProperty";
            case EFF7PropertyType::NameProperty: return "NameProperty";
            default: return "Unknown";
        }
    }

    // Schema entry: FName (8 bytes) + int32 type = 12 bytes in frozen memory
    struct FFF7Property {
        FName Name;
        EFF7PropertyType Type;
        bool bIsArray = false; // derived from the "_Array" name suffix
    };

    // Row key element of the KeyToIndex sparse array (20 bytes in frozen memory)
    struct FNameKey {
        FName Name;
        int32_t Index;
        int32_t NextIndex;
        int32_t Priority;
    };
    static_assert(sizeof(FNameKey) == 20, "FNameKey must be 20 bytes");

    // Frozen memory image pointer, FF7R uses the UE4 packing:
    //   bit 0: bIsFrozen, bits 1-63: signed offset from the pointer's own address (>> 1 arithmetic)
    // When bIsFrozen is clear the value is a real (unfrozen) pointer.
    struct FFrozenMemoryImagePtr {
        uint64_t Packed;

        bool IsFrozen() const { return (Packed & 1) != 0; }
        int64_t GetOffsetFromThis() const { return static_cast<int64_t>(Packed) >> 1; }
        void* GetUnfrozenPtr() const { return reinterpret_cast<void*>(Packed); }
    };
    static_assert(sizeof(FFrozenMemoryImagePtr) == 8, "FFrozenMemoryImagePtr must be 8 bytes");

    inline uint64_t AlignPosition(uint64_t pos, uint64_t alignment) {
        return (pos + alignment - 1) & ~(alignment - 1);
    }

} // namespace RC::FF7MMA
