#pragma once

#include <String/StringType.hpp>
#include <Unreal/Core/Serialization/FrozenMemoryImage.hpp>
#include "Types.hpp"

namespace RC::FF7MMA {

    // Generic frozen-image machinery lives in the Unreal submodule; this adds the
    // FF7-specific pieces (runtime 8-byte FMinimalName reads, StringType return).
    using ParseError = Unreal::FFrozenImageParseError;
    using ArrayHeader = Unreal::FFrozenArrayHeader;

    class ImageReader : public Unreal::FFrozenImageReader {
    public:
        ImageReader(const void* data, size_t size)
            : Unreal::FFrozenImageReader(data, size, Unreal::EFrozenPointerFormat::UE4) {}

        // Runtime FName in the loaded image: int32 comparison index + int32 number
        FName ReadFName() {
            uint32_t index = Read<uint32_t>();
            uint32_t number = Read<uint32_t>();
            return FName(index, number);
        }

        StringType ReadString() {
            std::wstring wide = Unreal::FFrozenImageReader::ReadString();
            return StringType(wide.begin(), wide.end());
        }
    };

} // namespace RC::FF7MMA
