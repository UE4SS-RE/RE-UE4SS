#include <Helpers/Casting.hpp>

#ifdef _WIN32
#define WINDOWS
#define NOMINMAX
#include "Windows.h"
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
#endif

} // namespace RC::Helper::Casting
