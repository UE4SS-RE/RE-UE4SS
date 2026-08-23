#include "unreal_runtime.hpp"
#include "object_registry_validator.hpp"

#include "ue4ss/patternsleuth_abi.h"

#include <Unreal/FMemory.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UnrealVersion.hpp>

#include <bit>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace
{
    void mock_linux_fname_constructor(RC::Unreal::FName* output,
                                      const RC::CharType*,
                                      RC::Unreal::EFindName)
    {
        *output = RC::Unreal::FName{42u, 7u};
    }
}

namespace
{
    constexpr std::uint64_t k_guobject_array = 0x1000u;
    constexpr std::uint64_t k_fname_to_string = 0x2000u;
    constexpr std::uint64_t k_fname_ctor = 0x3000u;
    constexpr std::uint64_t k_gmalloc = 0x4000u;
    constexpr std::uint64_t k_static_construct_object = 0x5000u;
    constexpr std::uint64_t k_game_engine_tick = 0x6000u;

    std::uint64_t pointer_bits(const void* pointer)
    {
        return static_cast<std::uint64_t>(std::bit_cast<std::uintptr_t>(pointer));
    }
}

int main()
{
    using ue4ss::linux::core::ResolvedUnrealState;

    constexpr std::uint64_t required = UE4SS_PS_GUOBJECT_ARRAY | UE4SS_PS_FNAME_TO_STRING | UE4SS_PS_FNAME_CTOR |
                                       UE4SS_PS_GMALLOC | UE4SS_PS_STATIC_CONSTRUCT_OBJECT | UE4SS_PS_GAME_ENGINE_TICK |
                                       UE4SS_PS_ENGINE_VERSION;
    ResolvedUnrealState resolved{
            .available_mask = required,
            .engine_major = 5,
            .engine_minor = 5,
            .guobject_array = k_guobject_array,
            .fname_to_string = k_fname_to_string,
            .fname_ctor = k_fname_ctor,
            .gmalloc = k_gmalloc,
            .static_construct_object = k_static_construct_object,
            .game_engine_tick = k_game_engine_tick,
    };

    std::string error;
    ResolvedUnrealState incomplete = resolved;
    incomplete.available_mask &= ~UE4SS_PS_GUOBJECT_ARRAY;
    assert(!ue4ss::linux::core::apply_unreal_resolver_state(incomplete, error));
    assert(!error.empty());
    assert(RC::Unreal::GUObjectArray == nullptr);

    assert(ue4ss::linux::core::apply_unreal_resolver_state(resolved, error));
    assert(error.empty());
    assert(RC::Unreal::Version::Major == 5);
    assert(RC::Unreal::Version::Minor == 5);
    assert(pointer_bits(RC::Unreal::GUObjectArray) == k_guobject_array);
    assert(pointer_bits(RC::Unreal::GMalloc) == k_gmalloc);
    assert(pointer_bits(RC::Unreal::FName::ToStringInternal.get_function_address()) == k_fname_to_string);
    assert(pointer_bits(RC::Unreal::FName::ConstructorInternal.get_function_address()) == k_fname_ctor);
    assert(pointer_bits(RC::Unreal::UObjectGlobals::GlobalState::StaticConstructObjectInternal.get_function_address()) ==
           k_static_construct_object);
    assert(pointer_bits(RC::Unreal::UObjectGlobals::GlobalState::StaticConstructObjectInternalDeprecated.get_function_address()) ==
           k_static_construct_object);
    assert(pointer_bits(RC::Unreal::UEngine::TickInternal.get_function_address()) == k_game_engine_tick);

    RC::Unreal::FName::ConstructorInternal.assign_address(&mock_linux_fname_constructor);
    const RC::Unreal::FName constructed{STR("PalServer"), RC::Unreal::FNAME_Find};
    assert(constructed.GetComparisonIndex().ToUnstableInt() == 42u);
    assert(constructed.GetNumber() == 7);

    resolved.engine_major = 6;
    assert(!ue4ss::linux::core::apply_unreal_resolver_state(resolved, error));
    assert(!error.empty());

    struct ObjectArrayView
    {
        std::uint64_t objects;
        std::uint64_t preallocated_objects;
        std::int32_t max_elements;
        std::int32_t num_elements;
        std::int32_t max_chunks;
        std::int32_t num_chunks;
    };
    static_assert(sizeof(ObjectArrayView) == 0x20);

    struct ObjectItemView
    {
        std::uint64_t object;
        std::int32_t flags;
        std::int32_t cluster_root_index;
        std::int32_t serial_number;
        std::int32_t padding;
    };
    struct UObjectBaseView
    {
        std::uint64_t vtable;
        std::uint32_t object_flags;
        std::int32_t internal_index;
        std::uint64_t class_private;
        std::uint64_t name_private;
        std::uint64_t outer_private;
    };
    static_assert(sizeof(ObjectItemView) == 0x18);
    static_assert(sizeof(UObjectBaseView) == 0x28);

    alignas(8) std::array<std::byte, 0x30> registry{};
    std::byte readable_code{};
    std::array<std::uint64_t, 1> vtable{pointer_bits(&readable_code)};
    std::array<UObjectBaseView, 16> objects{};
    std::array<ObjectItemView, 16> first_chunk{};
    for (std::size_t index = 0; index < objects.size(); ++index)
    {
        objects[index] = {
                .vtable = pointer_bits(vtable.data()),
                .object_flags = 0,
                .internal_index = static_cast<std::int32_t>(index),
                .class_private = pointer_bits(&objects[0]),
                .name_private = static_cast<std::uint64_t>(index),
                .outer_private = index == 0 ? 0u : pointer_bits(&objects[0]),
        };
        first_chunk[index] = {
                .object = pointer_bits(&objects[index]),
                .flags = 0,
                .cluster_root_index = -1,
                .serial_number = static_cast<std::int32_t>(index + 1),
                .padding = 0,
        };
    }
    std::array<std::uint64_t, 1> chunks{pointer_bits(first_chunk.data())};
    const ObjectArrayView view{
            .objects = pointer_bits(chunks.data()),
            .preallocated_objects = 0,
            .max_elements = 65536,
            .num_elements = static_cast<std::int32_t>(objects.size()),
            .max_chunks = 1,
            .num_chunks = 1,
    };
    std::memcpy(registry.data() + 0x10, &view, sizeof(view));

    auto validation = ue4ss::linux::core::validate_object_registry(pointer_bits(registry.data()), 5, 5);
    assert(validation.state == ue4ss::linux::core::ObjectRegistryValidationState::ready);
    assert(validation.num_elements == static_cast<std::int32_t>(objects.size()));
    assert(validation.num_chunks == 1);
    assert(validation.validated_objects == static_cast<std::int32_t>(objects.size()));

    registry.fill(std::byte{});
    validation = ue4ss::linux::core::validate_object_registry(pointer_bits(registry.data()), 5, 5);
    assert(validation.state == ue4ss::linux::core::ObjectRegistryValidationState::not_initialized);
    validation = ue4ss::linux::core::validate_object_registry(1, 5, 5);
    assert(validation.state == ue4ss::linux::core::ObjectRegistryValidationState::invalid);
    validation = ue4ss::linux::core::validate_object_registry(pointer_bits(registry.data()), 5, 7);
    assert(validation.state == ue4ss::linux::core::ObjectRegistryValidationState::unsupported);
    return 0;
}
