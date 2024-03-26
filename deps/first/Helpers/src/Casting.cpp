#include <Helpers/Casting.hpp>

#ifdef _WIN32
#define WINDOWS
#define NOMINMAX
#include "Windows.h"
#endif

namespace RC::Helper::Casting
{

#ifdef WINDOWS
    auto read_process_memory(void* process_handle, void* base_address, void* buffer, size_t size, size_t* number_of_bytes_read) -> bool
    {
        return ReadProcessMemory(process_handle, base_address, buffer, size, number_of_bytes_read);
    }
#endif

} // namespace RC::Helper::Casting
