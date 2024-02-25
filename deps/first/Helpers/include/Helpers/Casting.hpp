#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

#ifdef WIN32
#ifdef _WIN32
#define WINDOWS
#define NOMINMAX
#include "Windows.h"
#endif

#else

#include <sys/uio.h>

#endif

namespace RC::Helper::Casting
{
    template <typename To, typename From>
    auto ptr_cast(From ptr, int32_t offset = 0) -> To
    {
        static_assert(std::is_pointer_v<To> && std::is_pointer_v<From>, "ptr_cast can only be used on pointer types");

        return std::bit_cast<To>(std::bit_cast<char*>(ptr) + offset);
    }

    // Adds an offset to a pointer and then dereferences it
    template <typename To, typename From>
    auto ptr_cast_deref(From ptr, int32_t offset = 0) -> To
    {
        static_assert(std::is_pointer_v<From>, "ptr_cast_deref can only cast from pointer types");

        return *std::bit_cast<To*>(std::bit_cast<char*>(ptr) + offset);
    }

    // Compatibility with older code from before 'ptr_cast' existed
    template <typename To, typename From>
    auto offset_deref(From ptr, int32_t offset)
    {
        return ptr_cast_deref<To>(ptr, offset);
    }

#ifdef WINDOWS
    // Adds an offset to a "pointer", checks if it's pointing to valid memory and then dereferences it
    // Returns 0 if the "pointer" is not pointing to valid memory
    // TODO: Figure out some way to deal with the process_handle param... it's annoying to provide it for every call
    template <typename To, typename From>
    auto ptr_cast_deref_safe(From ptr, int32_t offset, HANDLE process_handle) -> To
    {
        static_assert(std::is_pointer_v<From>, "ptr_cast_deref_safe can only cast from pointer types");

        // For this example, To == UObject*
        // Therefore: To* == UObject**
        To* data_ptr = std::bit_cast<To*>(std::bit_cast<char*>(ptr) + offset);

        size_t bytes_read;
        uintptr_t is_valid_ptr_buffer;

        if (!ReadProcessMemory(process_handle, data_ptr, &is_valid_ptr_buffer, 0x8, &bytes_read))
        {
            return 0;
        }
        else
        {
            return *data_ptr;
        }
    }

    // Compatibility with older code from before 'ptr_cast' existed
    template <typename To, typename From>
    static auto offset_deref_safe(From* base_ptr, int32_t offset, HANDLE process_handle) -> To
    {
        return ptr_cast_deref_safe<To>(base_ptr, offset, process_handle);
    }
#else
    // use process_vm_readv
    template <typename To, typename From>
    auto ptr_cast_deref_safe(From ptr, int32_t offset, pid_t process_handle) -> To
    {
        static_assert(std::is_pointer_v<From>, "ptr_cast_deref_safe can only cast from pointer types");

        // For this example, To == UObject*
        // Therefore: To* == UObject**
        To* data_ptr = std::bit_cast<To*>(std::bit_cast<char*>(ptr) + offset);

        size_t bytes_read;
        uintptr_t is_valid_ptr_buffer;
        struct iovec local[1];
        struct iovec remote[1];
        local[0].iov_base = &is_valid_ptr_buffer;
        local[0].iov_len = sizeof(is_valid_ptr_buffer);
        remote[0].iov_base = data_ptr;
        remote[0].iov_len = sizeof(is_valid_ptr_buffer);
        if (process_vm_readv(process_handle, local, 1, remote, 1, 0) < 0)
        {
            return 0;
        }
        else
        {
            return *data_ptr;
        }
    }

    // Compatibility with older code from before 'ptr_cast' existed
    template <typename To, typename From>
    static auto offset_deref_safe(From* base_ptr, int32_t offset, pid_t process_handle) -> To
    {
        return ptr_cast_deref_safe<To>(base_ptr, offset, process_handle);
    }
#endif
} // namespace RC::Helper::Casting
