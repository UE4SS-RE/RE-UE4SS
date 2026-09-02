#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "DataTable.hpp"

namespace RC::FF7MMA {

    /**
     * Rebuilds a complete frozen memory image from a parsed (and possibly edited)
     * table, matching the cooker's conventions: 20-byte key slots in slot order,
     * hash chains built by head-insertion, bucket count RoundUpPow2(rows/2),
     * bucket = (CityHash64(lowercase name entry bytes) + FName number) & mask.
     *
     * The returned buffer is heap-allocated and must stay alive for as long as the
     * game may read it (i.e. leak it or keep it registered).
     */
    class MMAImageBuilder {
    public:
        struct Result {
            uint8_t* Data = nullptr;
            size_t Size = 0;
            // Section offsets within Data, for redirecting a live blob's root
            // headers at these structures
            uint64_t KeysPos = 0;
            uint64_t BitsPos = 0;
            uint64_t IndexesPos = 0;
            uint64_t MembersPos = 0;
            uint64_t ValuesPos = 0;
            int32_t NumRows = 0;
            uint32_t BucketCount = 0;
        };

        // Hash used for the KeyToIndex buckets.
        static uint64_t HashName(FName name);

        static Result Build(const MMADataTable& table, std::vector<std::string>& warnings);
    };

} // namespace RC::FF7MMA
