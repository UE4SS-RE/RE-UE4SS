#include <Helpers/Casting.hpp>

#ifdef _WIN32
#define WINDOWS
#define NOMINMAX
#include "Windows.h"
#elif defined(__linux__)
#include <array>
#include <sys/uio.h>
#include <unistd.h>
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
#elif defined(__linux__)
    auto check_readable(void*, void* src_ptr) -> bool
    {
        if (src_ptr == nullptr)
        {
            return false;
        }

        // Unlike mincore(), process_vm_readv() also observes read protection
        // and reports EFAULT instead of dereferencing an invalid game pointer.
        std::array<unsigned char, sizeof(uintptr_t)> probe{};
        iovec local{probe.data(), probe.size()};
        iovec remote{src_ptr, probe.size()};
        return process_vm_readv(getpid(), &local, 1, &remote, 1, 0) == static_cast<ssize_t>(probe.size());
    }
#endif

} // namespace RC::Helper::Casting
