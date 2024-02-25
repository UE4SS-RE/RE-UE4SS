#pragma once

#include <string>
#include <set>
#include <tuple>
#include <cstdint>

namespace RC
{
    struct DLData
    {
        std::string libname;
        uint8_t* base_address;
        uint64_t map_start, map_end;
        size_t size;
        // sorted by start address
        std::set<std::tuple<uint8_t*, size_t, int>> phdrs;
        std::tuple<uint8_t*, size_t, int> find_phdr(uint8_t* addr)
        {
            for (auto& phdr : phdrs)
            {
                // use end address to find the phdr
                if (std::get<0>(phdr) + std::get<1>(phdr) > addr)
                {
                    return phdr;
                }
            }
            return std::make_tuple(nullptr, 0, 0);
        }
    };

    using ModuleInfo = DLData;
    using ModuleOS = DLData;
    using SystemInfo = DLData;
} // namespace RC
