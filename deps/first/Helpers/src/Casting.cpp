#include <Helpers/Casting.hpp>

#ifdef _WIN32
#define WINDOWS
#define NOMINMAX
#include "Windows.h"
#else
#include <cstdio>
#include <fstream>
#include <string>
#endif

namespace RC::Helper::Casting
{

#ifdef WINDOWS
    auto check_readable(void* handle, void* src_ptr) -> bool
    {
        uintptr_t is_valid_ptr_buffer;
        size_t bytes_read;

        return ReadProcessMemory(*reinterpret_cast<HANDLE*>(handle), src_ptr, &is_valid_ptr_buffer, 0x8, &bytes_read) != 0;
    }
#else
    auto check_readable(void*, void* src_ptr) -> bool
    {
        const auto address = reinterpret_cast<uintptr_t>(src_ptr);
        std::ifstream maps{"/proc/self/maps"};
        std::string line;
        while (std::getline(maps, line))
        {
            uintptr_t region_start{};
            uintptr_t region_end{};
            char permissions[5]{};
            if (std::sscanf(line.c_str(), "%lx-%lx %4s", &region_start, &region_end, permissions) != 3)
            {
                continue;
            }
            if (permissions[0] == 'r' && address >= region_start && address + sizeof(uintptr_t) <= region_end)
            {
                return true;
            }
        }
        return false;
    }
#endif

} // namespace RC::Helper::Casting
