#include "object_registry_validator.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <sstream>
#include <sys/uio.h>
#include <unistd.h>

namespace
{
    template <typename T>
    bool safe_read(std::uint64_t address, T& value) noexcept
    {
        if (address == 0u || address > std::numeric_limits<std::uintptr_t>::max() - sizeof(T))
        {
            return false;
        }
        iovec local{&value, sizeof(value)};
        iovec remote{reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), sizeof(value)};
        return process_vm_readv(getpid(), &local, 1, &remote, 1, 0) == static_cast<ssize_t>(sizeof(value));
    }

    struct ChunkedObjectArrayView
    {
        std::uint64_t objects{};
        std::uint64_t preallocated_objects{};
        std::int32_t max_elements{};
        std::int32_t num_elements{};
        std::int32_t max_chunks{};
        std::int32_t num_chunks{};
    };

    static_assert(sizeof(ChunkedObjectArrayView) == 0x20);
    static_assert(offsetof(ChunkedObjectArrayView, num_elements) == 0x14);

    struct ObjectItemView
    {
        std::uint64_t object{};
        std::int32_t flags{};
        std::int32_t cluster_root_index{};
        std::int32_t serial_number{};
        std::int32_t padding{};
    };

    struct UObjectBaseView
    {
        std::uint64_t vtable{};
        std::uint32_t object_flags{};
        std::int32_t internal_index{};
        std::uint64_t class_private{};
        std::uint64_t name_private{};
        std::uint64_t outer_private{};
    };

    static_assert(sizeof(ObjectItemView) == 0x18);
    static_assert(sizeof(UObjectBaseView) == 0x28);
    static_assert(offsetof(UObjectBaseView, class_private) == 0x10);
    static_assert(offsetof(UObjectBaseView, name_private) == 0x18);
    static_assert(offsetof(UObjectBaseView, outer_private) == 0x20);

    constexpr std::uint64_t k_obj_objects_offset = 0x10u;
    constexpr std::int32_t k_elements_per_chunk = 64 * 1024;
    constexpr std::int32_t k_reasonable_element_limit = 64 * 1024 * 1024;
    constexpr std::int32_t k_object_scan_limit = 8192;
    constexpr std::int32_t k_required_object_samples = 16;

    bool validate_uobject_base(std::uint64_t address,
                               std::int32_t expected_index,
                               std::int32_t num_elements,
                               UObjectBaseView& object) noexcept
    {
        if (!safe_read(address, object) || object.vtable == 0u || object.class_private == 0u ||
            object.internal_index != expected_index)
        {
            return false;
        }
        if (static_cast<std::uint32_t>(object.name_private >> 32u) > 1'000'000u)
        {
            return false;
        }

        std::uint64_t first_virtual{};
        std::byte first_instruction{};
        if (!safe_read(object.vtable, first_virtual) || first_virtual == 0u || !safe_read(first_virtual, first_instruction))
        {
            return false;
        }

        UObjectBaseView object_class{};
        if (!safe_read(object.class_private, object_class) || object_class.vtable == 0u || object_class.class_private == 0u ||
            object_class.internal_index < 0 || object_class.internal_index >= num_elements)
        {
            return false;
        }
        if (object.outer_private != 0u)
        {
            UObjectBaseView outer{};
            if (!safe_read(object.outer_private, outer) || outer.vtable == 0u || outer.class_private == 0u ||
                outer.internal_index < 0 || outer.internal_index >= num_elements)
            {
                return false;
            }
        }
        return true;
    }
}

namespace ue4ss::linux::core
{
    ObjectRegistryValidation validate_object_registry(std::uint64_t address,
                                                      std::uint16_t engine_major,
                                                      std::uint16_t engine_minor) noexcept
    {
        ObjectRegistryValidation result;
        if (engine_major != 5u || engine_minor > 6u)
        {
            result.state = ObjectRegistryValidationState::unsupported;
            result.detail = "no approved chunked GUObjectArray layout for Unreal " + std::to_string(engine_major) + "." +
                            std::to_string(engine_minor);
            return result;
        }

        if (address == 0u || address > std::numeric_limits<std::uint64_t>::max() - k_obj_objects_offset)
        {
            result.detail = "GUObjectArray resolver address is null or overflows";
            return result;
        }

        ChunkedObjectArrayView view{};
        if (!safe_read(address + k_obj_objects_offset, view))
        {
            result.detail = "GUObjectArray header is not fully readable";
            return result;
        }
        result.num_elements = view.num_elements;
        result.num_chunks = view.num_chunks;

        if (view.max_elements < 0 || view.num_elements < 0 || view.num_elements > view.max_elements ||
            view.max_elements > k_reasonable_element_limit || view.max_chunks < 0 || view.num_chunks < 0 ||
            view.num_chunks > view.max_chunks)
        {
            result.detail = "GUObjectArray counters violate ordering or safety limits";
            return result;
        }
        if (view.num_elements == 0 && view.num_chunks == 0 && view.objects == 0u)
        {
            result.state = ObjectRegistryValidationState::not_initialized;
            result.detail = "resolver is bound but Unreal has not initialized the object registry yet";
            return result;
        }
        if (view.num_elements == 0 || view.num_chunks == 0 || view.objects == 0u)
        {
            result.detail = "GUObjectArray has a partially initialized chunk table";
            return result;
        }

        const std::int32_t minimum_chunks = (view.num_elements + k_elements_per_chunk - 1) / k_elements_per_chunk;
        if (view.num_chunks < minimum_chunks || view.max_chunks < minimum_chunks)
        {
            result.detail = "GUObjectArray chunk counts cannot contain NumElements";
            return result;
        }

        std::uint64_t first_chunk{};
        if (!safe_read(view.objects, first_chunk) || first_chunk == 0u)
        {
            result.detail = "GUObjectArray first chunk pointer is unreadable or null";
            return result;
        }
        ObjectItemView first_item{};
        if (!safe_read(first_chunk, first_item))
        {
            result.detail = "GUObjectArray first object-item chunk is unreadable";
            return result;
        }

        const std::int32_t scan_limit = std::min(view.num_elements, k_object_scan_limit);
        for (std::int32_t index = 0; index < scan_limit && result.validated_objects < k_required_object_samples; ++index)
        {
            const std::int32_t chunk_index = index / k_elements_per_chunk;
            const std::int32_t within_chunk = index % k_elements_per_chunk;
            std::uint64_t chunk{};
            if (!safe_read(view.objects + static_cast<std::uint64_t>(chunk_index) * sizeof(std::uint64_t), chunk) || chunk == 0u)
            {
                result.detail = "GUObjectArray contains an unreadable chunk pointer while sampling objects";
                return result;
            }
            ObjectItemView item{};
            if (!safe_read(chunk + static_cast<std::uint64_t>(within_chunk) * sizeof(ObjectItemView), item))
            {
                result.detail = "FUObjectItem is unreadable while validating the UE 5.1 layout";
                return result;
            }
            if (item.object == 0u)
            {
                continue;
            }
            UObjectBaseView object{};
            if (!validate_uobject_base(item.object, index, view.num_elements, object))
            {
                result.detail = "FUObjectItem/UObjectBase invariants failed at object index " + std::to_string(index);
                return result;
            }
            ++result.validated_objects;
        }
        if (result.validated_objects < k_required_object_samples)
        {
            result.detail = "too few stable UObject samples were available for layout validation";
            return result;
        }

        result.state = ObjectRegistryValidationState::ready;
        std::ostringstream detail;
        detail << "UE 5.1 registry/UObject layout validated read-only: elements=" << view.num_elements << " chunks=" << view.num_chunks
               << " object_samples=" << result.validated_objects;
        result.detail = detail.str();
        return result;
    }
}
