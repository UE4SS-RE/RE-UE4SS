#include "unreal_readonly.hpp"

#include "game_thread_scheduler.hpp"
#include "unreal_container_bridge.hpp"
#include "ue4ss/patternsleuth_abi.h"
#include "ue4ss/utf.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <initializer_list>
#include <memory>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <string_view>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

namespace
{
    // UE's generated Windows table has one destructor slot and places Free at
    // 0x30. The Itanium ABI used by this Clang Linux build has one additional
    // destructor entry, so every FMalloc method after the destructor is +8.
    // PalServer 1.0.1 call sites independently prove Realloc=0x28,
    // Free=0x38, and QuantizeSize=0x40.
    constexpr std::uint64_t k_fmalloc_realloc_vtable_offset = 0x28u;
    constexpr std::uint64_t k_fmalloc_free_vtable_offset = 0x38u;
    constexpr std::int32_t k_max_fname_code_units = 4096;
    constexpr std::int32_t k_max_unreal_string_code_units = 16 * 1024 * 1024;
    constexpr std::uint64_t k_obj_objects_offset = 0x10u;
    constexpr std::int32_t k_elements_per_chunk = 64 * 1024;
    constexpr std::int32_t k_reasonable_element_limit = 64 * 1024 * 1024;
    constexpr std::uint64_t k_super_struct_offset = 0x40u;
    constexpr std::uint64_t k_children_offset = 0x48u;
    constexpr std::uint64_t k_child_properties_offset = 0x50u;
    constexpr std::uint64_t k_properties_size_offset = 0x58u;
    // UEPseudo's generated UE 5.1 UFunction layout and the independently
    // resolved ProcessLocalScriptFunction call-site both place FunctionFlags
    // at +0xb0. Access remains gated by a live UFunction handle below.
    constexpr std::uint64_t k_ue51_function_flags_offset = 0xb0u;
    // UEPseudo's generated UE 5.1 layout places UClass::ClassDefaultObject at
    // 0x110 on both target ABIs. The runtime accepts this member only after
    // proving that the pointee is the live, flagged instance of that UClass.
    constexpr std::uint64_t k_ue51_class_default_object_offset = 0x110u;
    // UEPseudo's generated UE 5.1 layouts place FInterfaceProperty::InterfaceClass
    // at +0x78 and UClass::Interfaces at +0x1d0. The latter is a
    // TArray<FImplementedInterface>; it is used to reproduce
    // UObjectBaseUtility::GetInterfaceAddress without calling an unresolved
    // engine helper.
    constexpr std::uint64_t k_ue51_interface_class_offset = 0x78u;
    constexpr std::uint64_t k_ue51_uclass_interfaces_offset = 0x1d0u;
    // UEPseudo's UE 5.1 UDataTable layout. These are accepted only after the
    // RowStruct UObject and the complete FScriptMap header/rows validate.
    constexpr std::uint64_t k_ue51_data_table_row_struct_offset = 0x28u;
    constexpr std::uint64_t k_ue51_data_table_row_map_offset = 0x30u;
    constexpr std::int32_t k_max_implemented_interfaces = 4096;
    // The generated table is MSVC-specific. Linux uses the Itanium ABI and
    // PalGameEngine may add or override entries, so an offset is not trusted.
    // Search a bounded UEngine table and accept only one entry which exactly
    // equals the independently resolved UGameEngine::Tick address.
    constexpr std::uint64_t k_ue51_engine_vtable_scan_limit = 0x800u;
    constexpr std::uint64_t k_ue51_msvc_process_event_vtable_offset = 0x260u;
    constexpr std::uint64_t k_ue51_linux_process_event_vtable_offset = 0x268u;
    constexpr std::size_t k_max_super_depth = 256u;
    constexpr std::size_t k_max_outer_depth = 256u;
    constexpr std::size_t k_max_property_fields = 8192u;
    constexpr std::int32_t k_max_properties_size = 16 * 1024 * 1024;
    constexpr std::int32_t k_max_property_element_size = 4096;
    constexpr std::int32_t k_max_script_array_elements = 16 * 1024 * 1024;
    constexpr std::int32_t k_max_enum_entries = 64 * 1024;
    constexpr std::uint64_t k_ue51_uenum_names_scan_begin = 0x30u;
    constexpr std::uint64_t k_ue51_uenum_names_scan_end = 0x90u;
    constexpr std::uint32_t k_find_excluded_object_flags = 0x10u | 0x20u; // CDO | Archetype
    constexpr std::uint64_t k_property_flag_const_parameter = 0x2u;
    constexpr std::uint64_t k_property_flag_parameter = 0x80u;
    constexpr std::uint64_t k_property_flag_out_parameter = 0x100u;
    constexpr std::uint64_t k_property_flag_return_parameter = 0x400u;
    constexpr std::uint64_t k_property_flag_zero_constructor = 0x200u;
    constexpr std::uint64_t k_property_flag_no_destructor = 0x1000000000u;

    // UE 5.1's generated offsets describe the MSVC vtable. The Itanium ABI
    // used by the Linux server has both complete and deleting destructor
    // entries, moving every following FProperty virtual by one pointer. These
    // slots are never called until initialize_fproperty_abi has validated
    // their executable targets and observable behavior on a real IntProperty.
    constexpr std::uint64_t k_fproperty_identical_slot = 0xb8u;
    constexpr std::uint64_t k_fproperty_hash_slot = 0x100u;
    constexpr std::uint64_t k_fproperty_copy_single_to_script_vm_slot = 0x108u;
    constexpr std::uint64_t k_fproperty_destroy_value_slot = 0x130u;
    constexpr std::uint64_t k_fproperty_initialize_value_slot = 0x138u;
    constexpr std::uint64_t k_fproperty_min_alignment_slot = 0x150u;
    // UE 5.1 MSVC places ImportText_Internal at +0xe8. The second Itanium
    // destructor entry shifts it to +0xf0 on Linux.
    constexpr std::uint64_t k_fproperty_import_text_slot = 0xf0u;
    // UE 5.1 MSVC generated slots start at +0x168. The second Itanium
    // destructor entry shifts the Linux FMulticastDelegateProperty methods by
    // one pointer. PalServer's exported sparse-property vtable independently
    // resolves these entries to Get/Add/Remove/Clear implementations.
    constexpr std::uint64_t k_multicast_get_delegate_slot = 0x170u;
    constexpr std::uint64_t k_multicast_add_delegate_slot = 0x180u;
    constexpr std::uint64_t k_multicast_remove_delegate_slot = 0x188u;
    constexpr std::uint64_t k_multicast_clear_delegate_slot = 0x190u;
    // UE 5.1's MSVC ITextData table has a single destructor entry followed by
    // GetSourceString (+0x8) and GetDisplayString (+0x10). The Itanium ABI has
    // complete and deleting destructor entries, so the Linux display-string
    // slot is shifted by one pointer.
    constexpr std::uint64_t k_ue51_linux_itextdata_display_string_slot = 0x18u;

    struct ChunkedObjectArrayView
    {
        std::uint64_t objects{};
        std::uint64_t preallocated_objects{};
        std::int32_t max_elements{};
        std::int32_t num_elements{};
        std::int32_t max_chunks{};
        std::int32_t num_chunks{};
    };

    struct ObjectItemView
    {
        std::uint64_t object{};
        std::int32_t flags{};
        std::int32_t cluster_root_index{};
        std::int32_t serial_number{};
        std::int32_t padding{};
    };

    // Native UE 5.1 Linux layout proven from PalServer's resolved
    // StaticConstructObject implementation. The unbound TFunction occupies
    // [0x40, 0x80); keeping it as zeroed bytes avoids constructing a C++
    // runtime object across the Unreal boundary.
    struct alignas(16) StaticConstructObjectParametersView
    {
        std::uint64_t unreal_class{};
        std::uint64_t outer{};
        ue4ss::linux::core::RawFName name;
        std::uint32_t object_flags{};
        std::uint32_t internal_object_flags{};
        std::uint8_t copy_transients_from_class_defaults{};
        std::uint8_t assume_template_is_archetype{};
        std::array<std::byte, 6> boolean_padding{};
        std::uint64_t object_template{};
        std::uint64_t instance_graph{};
        std::uint64_t external_package{};
        std::array<std::byte, 0x40> property_init_callback{};
        std::uint64_t subobject_overrides{};
    };

    struct UObjectBaseView
    {
        std::uint64_t vtable{};
        std::uint32_t object_flags{};
        std::int32_t internal_index{};
        std::uint64_t class_private{};
        ue4ss::linux::core::RawFName name_private{};
        std::uint64_t outer_private{};
    };

    struct FPropertyPrefixView
    {
        std::uint64_t vtable{};
        std::uint64_t class_private{};
        std::array<std::byte, 0x10> owner{};
        std::uint64_t next{};
        ue4ss::linux::core::RawFName name_private{};
        std::uint32_t flags_private{};
        std::int32_t array_dim{};
        std::int32_t element_size{};
        std::uint32_t padding{};
        std::uint64_t property_flags{};
        std::array<std::byte, 4> replication{};
        std::int32_t offset_internal{};
    };

    struct FBoolPropertyTailView
    {
        std::uint8_t field_size{};
        std::uint8_t byte_offset{};
        std::uint8_t byte_mask{};
        std::uint8_t field_mask{};
    };

    struct FScriptArrayView
    {
        std::uint64_t data{};
        std::int32_t num{};
        std::int32_t max{};
    };

#pragma pack(push, 1)
    struct NativeFURLView
    {
        ue4ss::linux::core::UnrealStringHeader protocol;
        ue4ss::linux::core::UnrealStringHeader host;
        std::int32_t port{};
        std::int32_t valid{};
        ue4ss::linux::core::UnrealStringHeader map;
        ue4ss::linux::core::UnrealStringHeader redirect_url;
        FScriptArrayView options;
        ue4ss::linux::core::UnrealStringHeader portal;
    };
#pragma pack(pop)

    struct FEnumNamePairView
    {
        ue4ss::linux::core::RawFName name;
        std::int64_t value{};
    };

    struct FWeakObjectPtrView
    {
        std::int32_t object_index{};
        std::int32_t object_serial_number{};
    };

    struct FScriptDelegateView
    {
        FWeakObjectPtrView object;
        ue4ss::linux::core::RawFName function_name;
    };

    struct FScriptInterfaceView
    {
        std::uint64_t object{};
        std::uint64_t interface_pointer{};
    };

    struct FImplementedInterfaceView
    {
        std::uint64_t interface_class{};
        std::int32_t pointer_offset{};
        std::uint8_t implemented_by_k2{};
        std::array<std::byte, 3> padding{};
    };

    struct FTopLevelAssetPathView
    {
        ue4ss::linux::core::RawFName package_name;
        ue4ss::linux::core::RawFName asset_name;
    };

    struct FSoftObjectPtrView
    {
        FWeakObjectPtrView weak_ptr;
        std::int32_t tag_at_last_test{};
        std::int32_t padding{};
        FTopLevelAssetPathView asset_path;
        ue4ss::linux::core::UnrealStringHeader sub_path_string;
    };

    struct FTextView
    {
        std::uint64_t data{};
        std::uint64_t shared_ref_collector{};
        std::uint32_t flags{};
        std::uint32_t unknown{};
    };

    // FScriptSet is FScriptSparseArray + hash allocation on UE 5.1.  The
    // inline bit-array words are used while the allocation fits four words;
    // otherwise secondary_data points at engine-owned words.  UEPseudo's
    // compiled sizeof gate below prevents this mirror from silently drifting.
    struct FScriptSetView
    {
        FScriptArrayView elements;
        std::array<std::uint32_t, 4> allocation_flags_inline{};
        std::uint64_t allocation_flags_secondary{};
        std::int32_t allocation_flags_num_bits{};
        std::int32_t allocation_flags_max_bits{};
        std::int32_t first_free_index{};
        std::int32_t num_free_indices{};
        std::int32_t hash_inline{};
        std::int32_t hash_inline_padding{};
        std::uint64_t hash_secondary{};
        std::int32_t hash_size{};
        std::int32_t padding{};
    };

    struct ReflectedProperty
    {
        std::uint64_t address{};
        std::uint64_t next{};
        std::string name;
        std::string type_name;
        std::int32_t element_size{};
        std::int32_t offset{};
        std::uint64_t flags{};
        FBoolPropertyTailView boolean{};
        std::uint64_t struct_address{};
        std::uint64_t inner_property_address{};
        std::uint64_t key_property_address{};
        std::uint64_t value_property_address{};
        std::uint64_t enum_address{};
        std::uint64_t enum_underlying_property_address{};
        std::uint64_t interface_class_address{};
        bool enum_underlying_signed{};
    };

    static_assert(sizeof(ChunkedObjectArrayView) == 0x20);
    static_assert(sizeof(ObjectItemView) == 0x18);
    static_assert(offsetof(StaticConstructObjectParametersView, name) == 0x10);
    static_assert(offsetof(StaticConstructObjectParametersView, object_template) == 0x28);
    static_assert(offsetof(StaticConstructObjectParametersView, property_init_callback) == 0x40);
    static_assert(offsetof(StaticConstructObjectParametersView, subobject_overrides) == 0x80);
    static_assert(alignof(StaticConstructObjectParametersView) == 0x10);
    static_assert(sizeof(StaticConstructObjectParametersView) == 0x90);
    static_assert(sizeof(UObjectBaseView) == 0x28);
    static_assert(offsetof(FPropertyPrefixView, next) == 0x20);
    static_assert(offsetof(FPropertyPrefixView, name_private) == 0x28);
    static_assert(offsetof(FPropertyPrefixView, array_dim) == 0x34);
    static_assert(offsetof(FPropertyPrefixView, element_size) == 0x38);
    static_assert(offsetof(FPropertyPrefixView, offset_internal) == 0x4c);
    static_assert(sizeof(FPropertyPrefixView) == 0x50);
    static_assert(sizeof(FScriptArrayView) == 0x10);
    static_assert(sizeof(NativeFURLView) == 0x68);
    static_assert(offsetof(NativeFURLView, host) == 0x10);
    static_assert(offsetof(NativeFURLView, port) == 0x20);
    static_assert(offsetof(NativeFURLView, valid) == 0x24);
    static_assert(offsetof(NativeFURLView, map) == 0x28);
    static_assert(offsetof(NativeFURLView, redirect_url) == 0x38);
    static_assert(offsetof(NativeFURLView, options) == 0x48);
    static_assert(offsetof(NativeFURLView, portal) == 0x58);
    static_assert(sizeof(FEnumNamePairView) == 0x10);
    static_assert(sizeof(FWeakObjectPtrView) == 0x8);
    static_assert(sizeof(FScriptDelegateView) == 0x10);
    static_assert(sizeof(FScriptInterfaceView) == 0x10);
    static_assert(sizeof(FImplementedInterfaceView) == 0x10);
    static_assert(offsetof(FImplementedInterfaceView, pointer_offset) == 0x8);
    static_assert(offsetof(FImplementedInterfaceView, implemented_by_k2) == 0xc);
    static_assert(sizeof(FTopLevelAssetPathView) == 0x10);
    static_assert(offsetof(FSoftObjectPtrView, asset_path) == 0x10);
    static_assert(offsetof(FSoftObjectPtrView, sub_path_string) == 0x20);
    static_assert(sizeof(FSoftObjectPtrView) == 0x30);
    static_assert(sizeof(FTextView) == 0x18);
    static_assert(sizeof(FScriptSetView) == 0x50);
    static_assert(offsetof(FScriptSetView, allocation_flags_inline) == 0x10);
    static_assert(offsetof(FScriptSetView, allocation_flags_secondary) == 0x20);
    static_assert(offsetof(FScriptSetView, allocation_flags_num_bits) == 0x28);
    static_assert(offsetof(FScriptSetView, first_free_index) == 0x30);
    static_assert(offsetof(FScriptSetView, hash_inline) == 0x38);
    static_assert(offsetof(FScriptSetView, hash_secondary) == 0x40);
    static_assert(offsetof(FScriptSetView, hash_size) == 0x48);

    template <typename T>
    [[nodiscard]] bool safe_read(std::uint64_t address, T& value) noexcept
    {
        if (address == 0u || address > std::numeric_limits<std::uintptr_t>::max() - sizeof(T))
        {
            return false;
        }
        iovec local{&value, sizeof(value)};
        iovec remote{reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), sizeof(value)};
        return process_vm_readv(getpid(), &local, 1, &remote, 1, 0) == static_cast<ssize_t>(sizeof(value));
    }

    [[nodiscard]] bool safe_read_bytes(std::uint64_t address, void* output, std::size_t size) noexcept
    {
        if (size == 0u)
        {
            return true;
        }
        if (address == 0u || output == nullptr || address > std::numeric_limits<std::uintptr_t>::max() - size)
        {
            return false;
        }
        iovec local{output, size};
        iovec remote{reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), size};
        return process_vm_readv(getpid(), &local, 1, &remote, 1, 0) == static_cast<ssize_t>(size);
    }

    // Candidate UEnum layouts must not invoke FName::ToString: a random
    // comparison index can make the engine dereference outside its name pool.
    // This probe is deliberately read-only and uses only bounded TArray/TPair
    // invariants. FNames are decoded only after one offset wins across several
    // independent live UEnum objects.
    [[nodiscard]] bool probe_enum_names_header(std::uint64_t enum_address,
                                               std::uint64_t names_offset,
                                               std::int32_t& entry_count) noexcept
    {
        entry_count = 0;
        FScriptArrayView names{};
        if (enum_address == 0u || names_offset < k_ue51_uenum_names_scan_begin ||
            names_offset > k_ue51_uenum_names_scan_end || names_offset % alignof(void*) != 0u ||
            !safe_read(enum_address + names_offset, names) || names.num <= 0 || names.max < names.num ||
            names.max > k_max_enum_entries || names.data == 0u || names.data % alignof(FEnumNamePairView) != 0u ||
            names.data > std::numeric_limits<std::uint64_t>::max() -
                                 static_cast<std::uint64_t>(names.num) * sizeof(FEnumNamePairView))
        {
            return false;
        }
        const std::int32_t sample_count = std::min<std::int32_t>(names.num, 512);
        std::unordered_set<std::uint64_t> sampled_names;
        std::size_t plausible_values{};
        bool has_canonical_small_value{};
        for (std::int32_t index = 0; index < sample_count; ++index)
        {
            FEnumNamePairView pair{};
            if (!safe_read(names.data + static_cast<std::uint64_t>(index) * sizeof(pair), pair) ||
                pair.name.comparison_index == 0u || pair.name.number > 1'000'000u)
            {
                return false;
            }
            const std::uint64_t key = (static_cast<std::uint64_t>(pair.name.number) << 32u) |
                                      pair.name.comparison_index;
            if (!sampled_names.emplace(key).second) return false;
            if (pair.value >= std::numeric_limits<std::int32_t>::min() &&
                pair.value <= static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max()))
            {
                ++plausible_values;
            }
            has_canonical_small_value = has_canonical_small_value ||
                                        (pair.value >= -1 && pair.value <= 1);
        }
        if (plausible_values * 4u < static_cast<std::size_t>(sample_count) * 3u ||
            !has_canonical_small_value) return false;
        entry_count = names.num;
        return true;
    }

    [[nodiscard]] bool safe_write_bytes(std::uint64_t address, const void* input, std::size_t size) noexcept
    {
        if (size == 0u)
        {
            return true;
        }
        if (address == 0u || input == nullptr || address > std::numeric_limits<std::uintptr_t>::max() - size)
        {
            return false;
        }
        iovec local{const_cast<void*>(input), size};
        iovec remote{reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)), size};
        return process_vm_writev(getpid(), &local, 1, &remote, 1, 0) == static_cast<ssize_t>(size);
    }

    [[nodiscard]] bool is_executable_mapping(std::uint64_t address) noexcept;

    template <typename Function>
    [[nodiscard]] bool load_fproperty_virtual(std::uint64_t property_address,
                                              std::uint64_t slot_offset,
                                              Function& output,
                                              std::string& error) noexcept
    {
        output = nullptr;
        FPropertyPrefixView property{};
        std::uint64_t candidate{};
        if (!safe_read(property_address, property) || property.vtable == 0u ||
            property.vtable > std::numeric_limits<std::uint64_t>::max() - slot_offset ||
            ((property.vtable + slot_offset) % alignof(void*)) != 0u ||
            !safe_read(property.vtable + slot_offset, candidate) || candidate == 0u ||
            !is_executable_mapping(candidate))
        {
            error = "FProperty vtable slot +0x";
            std::array<char, 24> offset{};
            std::snprintf(offset.data(), offset.size(), "%llx", static_cast<unsigned long long>(slot_offset));
            error += offset.data();
            error += " is unreadable or not executable";
            return false;
        }
        output = std::bit_cast<Function>(static_cast<std::uintptr_t>(candidate));
        return true;
    }

    template <typename DecodeName>
    [[nodiscard]] bool decode_enum_property_metadata(std::uint64_t field_address,
                                                     DecodeName&& decode_name,
                                                     ReflectedProperty& output,
                                                     std::string& error) noexcept
    {
        if (output.type_name == "ByteProperty")
        {
            if (!safe_read(field_address + 0x78u, output.enum_address))
            {
                error = "FByteProperty has unreadable optional UEnum metadata";
                return false;
            }
            return true;
        }
        if (output.type_name != "EnumProperty") return true;

        if (!safe_read(field_address + 0x78u, output.enum_underlying_property_address) ||
            output.enum_underlying_property_address == 0u ||
            !safe_read(field_address + 0x80u, output.enum_address) || output.enum_address == 0u)
        {
            error = "FEnumProperty has an unreadable or null UnderlyingProp/UEnum";
            return false;
        }
        FPropertyPrefixView underlying{};
        ue4ss::linux::core::RawFName underlying_class_name{};
        std::string underlying_type;
        if (!safe_read(output.enum_underlying_property_address, underlying) ||
            underlying.vtable == 0u || underlying.class_private == 0u || underlying.array_dim != 1 ||
            underlying.offset_internal != 0 || underlying.element_size != output.element_size ||
            !safe_read(underlying.class_private, underlying_class_name) ||
            !decode_name(underlying_class_name, underlying_type, error))
        {
            error = error.empty() ? "FEnumProperty UnderlyingProp metadata is invalid" : error;
            return false;
        }
        const bool signed_underlying = underlying_type == "Int8Property" ||
                                       underlying_type == "Int16Property" ||
                                       underlying_type == "IntProperty" ||
                                       underlying_type == "Int64Property";
        const bool unsigned_underlying = underlying_type == "ByteProperty" ||
                                         underlying_type == "UInt16Property" ||
                                         underlying_type == "UInt32Property" ||
                                         underlying_type == "UInt64Property";
        if ((!signed_underlying && !unsigned_underlying) ||
            (underlying.element_size != 1 && underlying.element_size != 2 &&
             underlying.element_size != 4 && underlying.element_size != 8))
        {
            error = "FEnumProperty UnderlyingProp is not a supported integral FProperty";
            return false;
        }
        output.enum_underlying_signed = signed_underlying;
        return true;
    }

    template <typename DecodeName>
    [[nodiscard]] bool find_reflected_property(std::uint64_t class_address,
                                                std::string_view requested_name,
                                                DecodeName&& decode_name,
                                                ReflectedProperty& output,
                                                std::string& error) noexcept
    {
        output = {};
        if (class_address == 0u || requested_name.empty())
        {
            error = "a live class and non-empty property name are required";
            return false;
        }

        std::int32_t properties_size{};
        if (!safe_read(class_address + k_properties_size_offset, properties_size) || properties_size <= 0 ||
            properties_size > k_max_properties_size)
        {
            error = "the class has an invalid UE 5.1 UStruct::PropertiesSize";
            return false;
        }

        std::unordered_set<std::uint64_t> visited;
        std::uint64_t current_class = class_address;
        std::size_t field_count{};
        for (std::size_t depth = 0; current_class != 0u && depth < k_max_super_depth; ++depth)
        {
            std::uint64_t field_address{};
            if (!safe_read(current_class + k_child_properties_offset, field_address))
            {
                error = "UStruct::ChildProperties became unreadable";
                return false;
            }
            while (field_address != 0u)
            {
                if (++field_count > k_max_property_fields || !visited.emplace(field_address).second)
                {
                    error = "FField property chain is cyclic or exceeds the safety limit";
                    return false;
                }
                FPropertyPrefixView property{};
                if (!safe_read(field_address, property) || property.vtable == 0u || property.class_private == 0u)
                {
                    error = "FProperty metadata became unreadable";
                    return false;
                }
                ue4ss::linux::core::RawFName field_class_name{};
                std::string type_name;
                std::string property_name;
                if (!safe_read(property.class_private, field_class_name) ||
                    !decode_name(field_class_name, type_name, error) ||
                    !decode_name(property.name_private, property_name, error))
                {
                    error = error.empty() ? "FFieldClass or property name could not be decoded" : error;
                    return false;
                }
                if (!type_name.ends_with("Property"))
                {
                    error = "UStruct::ChildProperties contains a non-property FFieldClass: " + type_name;
                    return false;
                }
                if (property_name == requested_name)
                {
                    if (property.array_dim != 1)
                    {
                        error = "static property arrays are not supported yet: " + property_name;
                        return false;
                    }
                    FBoolPropertyTailView boolean{};
                    std::int32_t effective_element_size = property.element_size;
                    if (type_name == "BoolProperty")
                    {
                        if (!safe_read(field_address + 0x78u, boolean) || boolean.field_size == 0u ||
                            boolean.field_mask == 0u || boolean.byte_mask == 0u || boolean.byte_offset >= boolean.field_size)
                        {
                            error = "FBoolProperty bitfield metadata is invalid (element_size=" +
                                    std::to_string(property.element_size) + ")";
                            return false;
                        }
                        // FieldSize is the authoritative byte span for an
                        // FBoolProperty bitfield.
                        effective_element_size = boolean.field_size;
                    }
                    if (effective_element_size <= 0 || effective_element_size > k_max_property_element_size ||
                        property.offset_internal < 0 || property.offset_internal > properties_size - effective_element_size)
                    {
                        error = "property '" + property_name + "' (" + type_name + ") has offset=" +
                                std::to_string(property.offset_internal) + " element_size=" +
                                std::to_string(property.element_size) + " effective_size=" +
                                std::to_string(effective_element_size) + " outside class PropertiesSize=" +
                                std::to_string(properties_size);
                        return false;
                    }
                    output = {
                            .address = field_address,
                            .next = property.next,
                            .name = std::move(property_name),
                            .type_name = std::move(type_name),
                            .element_size = effective_element_size,
                            .offset = property.offset_internal,
                            .flags = property.property_flags,
                    };
                    output.boolean = boolean;
                    if (output.type_name == "StructProperty" &&
                        (!safe_read(field_address + 0x78u, output.struct_address) || output.struct_address == 0u))
                    {
                        error = "FStructProperty has an unreadable or null UScriptStruct";
                        output = {};
                        return false;
                    }
                    if (output.type_name == "ArrayProperty" &&
                        (!safe_read(field_address + 0x78u, output.inner_property_address) ||
                         output.inner_property_address == 0u))
                    {
                        error = "FArrayProperty has an unreadable or null Inner FProperty";
                        output = {};
                        return false;
                    }
                    if (output.type_name == "SetProperty" &&
                        (!safe_read(field_address + 0x78u, output.inner_property_address) ||
                         output.inner_property_address == 0u))
                    {
                        error = "FSetProperty has an unreadable or null Element FProperty";
                        output = {};
                        return false;
                    }
                    if (output.type_name == "MapProperty" &&
                        (!safe_read(field_address + 0x78u, output.key_property_address) ||
                         output.key_property_address == 0u ||
                         !safe_read(field_address + 0x80u, output.value_property_address) ||
                         output.value_property_address == 0u))
                    {
                        error = "FMapProperty has an unreadable or null key/value FProperty";
                        output = {};
                        return false;
                    }
                    if (output.type_name == "InterfaceProperty" &&
                        (!safe_read(field_address + k_ue51_interface_class_offset,
                                    output.interface_class_address) ||
                         output.interface_class_address == 0u))
                    {
                        error = "FInterfaceProperty has an unreadable or null InterfaceClass";
                        output = {};
                        return false;
                    }
                    if (!decode_enum_property_metadata(field_address, decode_name, output, error))
                    {
                        output = {};
                        return false;
                    }
                    return true;
                }
                field_address = property.next;
            }
            if (!safe_read(current_class + k_super_struct_offset, current_class))
            {
                error = "UStruct::SuperStruct became unreadable while finding a property";
                return false;
            }
        }
        if (current_class != 0u)
        {
            error = "class hierarchy exceeded the safety depth limit";
            return false;
        }
        error = "property '" + std::string{requested_name} + "' was not found in the class hierarchy";
        return false;
    }

    [[nodiscard]] bool is_signed_integer_property(std::string_view name) noexcept
    {
        return name == "Int8Property" || name == "Int16Property" || name == "IntProperty" || name == "Int64Property";
    }

    [[nodiscard]] bool is_unsigned_integer_property(std::string_view name) noexcept
    {
        return name == "ByteProperty" || name == "UInt16Property" || name == "UInt32Property" ||
               name == "UInt64Property";
    }

    [[nodiscard]] bool is_direct_object_property(std::string_view name) noexcept
    {
        return name == "ObjectProperty" || name == "ObjectPtrProperty" ||
               name == "ClassProperty";
    }

    [[nodiscard]] std::optional<std::int32_t> custom_property_storage_size(
            std::string_view name) noexcept
    {
        if (name == "BoolProperty" || name == "Int8Property" ||
            name == "ByteProperty")
        {
            return 1;
        }
        if (name == "Int16Property" || name == "UInt16Property")
        {
            return 2;
        }
        if (name == "IntProperty" || name == "UInt32Property" ||
            name == "FloatProperty")
        {
            return 4;
        }
        if (name == "Int64Property" || name == "UInt64Property" ||
            name == "DoubleProperty" || name == "NameProperty" ||
            name == "ObjectProperty" || name == "ObjectPtrProperty" ||
            name == "ClassProperty" || name == "WeakObjectProperty")
        {
            return 8;
        }
        if (name == "StrProperty" || name == "ArrayProperty" ||
            name == "DelegateProperty")
        {
            return 16;
        }
        if (name == "TextProperty")
        {
            return 24;
        }
        if (name == "SoftObjectProperty" || name == "SoftClassProperty")
        {
            return 48;
        }
        return std::nullopt;
    }

    struct SyntheticCustomInnerProperty
    {
        std::array<std::byte, 0x80> storage{};
        ue4ss::linux::core::RawFName class_name{};
    };

    [[nodiscard]] bool make_custom_reflected_property(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::CustomPropertyDescription& description,
            ReflectedProperty& output,
            SyntheticCustomInnerProperty& inner_storage,
            std::string& error) noexcept
    {
        output = {
                .name = description.name,
                .type_name = description.type.type_name,
                .element_size = description.type.element_size,
                .offset = description.offset,
        };
        if (description.type.type_name == "BoolProperty")
        {
            output.boolean = {
                    .field_size = 1u,
                    .byte_offset = 0u,
                    .byte_mask = 1u,
                    .field_mask = 1u,
            };
        }
        if (description.type.type_name != "ArrayProperty")
        {
            return true;
        }
        if (!description.array_inner.has_value())
        {
            error = "ArrayProperty custom descriptor has no inner property";
            return false;
        }
        if (!runtime.fname_from_utf8(
                    description.array_inner->type_name,
                    inner_storage.class_name,
                    false,
                    error))
        {
            error = "custom ArrayProperty inner FFieldClass name is unavailable: " +
                    error;
            return false;
        }
        FPropertyPrefixView prefix{};
        // The synthetic metadata is read-only. Its non-null vtable is merely
        // the same structural presence gate used by decode_nested_property;
        // no virtual call is permitted for a custom property.
        prefix.vtable = 1u;
        prefix.class_private = std::bit_cast<std::uintptr_t>(
                &inner_storage.class_name);
        prefix.name_private = {};
        prefix.array_dim = 1;
        prefix.element_size = description.array_inner->element_size;
        prefix.offset_internal = 0;
        std::memcpy(inner_storage.storage.data(), &prefix, sizeof(prefix));
        if (description.array_inner->type_name == "BoolProperty")
        {
            const FBoolPropertyTailView boolean{
                    .field_size = 1u,
                    .byte_offset = 0u,
                    .byte_mask = 1u,
                    .field_mask = 1u,
            };
            std::memcpy(inner_storage.storage.data() + 0x78u,
                        &boolean,
                        sizeof(boolean));
        }
        output.inner_property_address = std::bit_cast<std::uintptr_t>(
                inner_storage.storage.data());
        return true;
    }

    [[nodiscard]] bool same_custom_property(
            const ue4ss::linux::core::CustomPropertyDescription& left,
            const ue4ss::linux::core::CustomPropertyDescription& right) noexcept
    {
        const auto same_type = [](const auto& lhs, const auto& rhs) {
            return lhs.type_name == rhs.type_name &&
                   lhs.element_size == rhs.element_size;
        };
        const bool same_inner =
                left.array_inner.has_value() == right.array_inner.has_value() &&
                (!left.array_inner.has_value() ||
                 same_type(*left.array_inner, *right.array_inner));
        return left.name == right.name &&
               left.owner_class.address == right.owner_class.address &&
               left.owner_class.internal_index == right.owner_class.internal_index &&
               left.owner_class.serial_number == right.owner_class.serial_number &&
               left.offset == right.offset && same_type(left.type, right.type) &&
               same_inner;
    }

    [[nodiscard]] bool is_soft_object_property(std::string_view name) noexcept
    {
        return name == "SoftObjectProperty" || name == "SoftClassProperty";
    }

    [[nodiscard]] bool is_inline_multicast_delegate_property(std::string_view name) noexcept
    {
        return name == "MulticastDelegateProperty" || name == "MulticastInlineDelegateProperty";
    }

    [[nodiscard]] bool is_sparse_multicast_delegate_property(std::string_view name) noexcept
    {
        return name == "MulticastSparseDelegateProperty";
    }

    [[nodiscard]] bool is_multicast_delegate_property(std::string_view name) noexcept
    {
        return is_inline_multicast_delegate_property(name) ||
               is_sparse_multicast_delegate_property(name);
    }

    [[nodiscard]] std::optional<ue4ss::linux::core::UnrealPropertyKind> property_kind(
            const ReflectedProperty& property) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        if (property.type_name == "BoolProperty") return UnrealPropertyKind::boolean;
        if (is_signed_integer_property(property.type_name)) return UnrealPropertyKind::signed_integer;
        if (is_unsigned_integer_property(property.type_name)) return UnrealPropertyKind::unsigned_integer;
        if (property.type_name == "EnumProperty")
        {
            return property.enum_underlying_signed ? UnrealPropertyKind::signed_integer
                                                   : UnrealPropertyKind::unsigned_integer;
        }
        if (property.type_name == "FloatProperty" || property.type_name == "DoubleProperty")
        {
            return UnrealPropertyKind::floating_point;
        }
        if (property.type_name == "NameProperty") return UnrealPropertyKind::name;
        if (property.type_name == "StrProperty") return UnrealPropertyKind::string;
        if (property.type_name == "TextProperty") return UnrealPropertyKind::text;
        if (is_direct_object_property(property.type_name) || property.type_name == "InterfaceProperty")
        {
            return UnrealPropertyKind::object;
        }
        if (property.type_name == "WeakObjectProperty") return UnrealPropertyKind::weak_object;
        if (is_soft_object_property(property.type_name)) return UnrealPropertyKind::soft_object;
        if (property.type_name == "DelegateProperty") return UnrealPropertyKind::delegate;
        if (is_multicast_delegate_property(property.type_name)) return UnrealPropertyKind::multicast_delegate;
        if (property.type_name == "StructProperty" && property.struct_address != 0u)
        {
            return UnrealPropertyKind::structure;
        }
        if (property.type_name == "ArrayProperty" && property.inner_property_address != 0u)
        {
            return UnrealPropertyKind::array;
        }
        if (property.type_name == "SetProperty" && property.inner_property_address != 0u)
        {
            return UnrealPropertyKind::set;
        }
        if (property.type_name == "MapProperty" && property.key_property_address != 0u &&
            property.value_property_address != 0u)
        {
            return UnrealPropertyKind::map;
        }
        return std::nullopt;
    }

    [[nodiscard]] bool is_supported_scalar_property(const ReflectedProperty& property) noexcept
    {
        return property.type_name == "BoolProperty" || is_signed_integer_property(property.type_name) ||
               is_unsigned_integer_property(property.type_name) || property.type_name == "EnumProperty" ||
               (property.type_name == "FloatProperty" && property.element_size == sizeof(float)) ||
               (property.type_name == "DoubleProperty" && property.element_size == sizeof(double)) ||
               (property.type_name == "NameProperty" &&
                property.element_size == sizeof(ue4ss::linux::core::RawFName)) ||
               (property.type_name == "StrProperty" &&
                property.element_size == sizeof(ue4ss::linux::core::UnrealStringHeader)) ||
               (property.type_name == "TextProperty" &&
                property.element_size == sizeof(FTextView)) ||
               (is_direct_object_property(property.type_name) && property.element_size == sizeof(void*));
    }

    [[nodiscard]] std::int64_t sign_extend_integer(std::uint64_t value, std::int32_t size) noexcept
    {
        switch (size)
        {
        case 1:
            return static_cast<std::int8_t>(value);
        case 2:
            return static_cast<std::int16_t>(value);
        case 4:
            return static_cast<std::int32_t>(value);
        case 8:
            return static_cast<std::int64_t>(value);
        default:
            return 0;
        }
    }

    [[nodiscard]] bool populate_property_description(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            std::uint64_t field_address,
            std::uint64_t owner_address,
            ue4ss::linux::core::ReflectedPropertyDescription& output,
            std::string& error) noexcept
    {
        output = {};
        FPropertyPrefixView property{};
        ue4ss::linux::core::RawFName field_class_name{};
        if (field_address == 0u || !safe_read(field_address, property) ||
            property.vtable == 0u || property.class_private == 0u ||
            !safe_read(property.class_private, field_class_name) ||
            !runtime.fname_to_utf8(field_class_name, output.type_name, error) ||
            !runtime.fname_to_utf8(property.name_private, output.name, error))
        {
            error = error.empty() ? "FProperty metadata could not be decoded" : error;
            return false;
        }
        if (!output.type_name.ends_with("Property") || property.array_dim <= 0 ||
            property.array_dim > 1024 || property.element_size <= 0 ||
            property.element_size > k_max_property_element_size || property.offset_internal < 0)
        {
            error = "FProperty has an invalid class, ArrayDim, ElementSize, or Offset_Internal";
            output = {};
            return false;
        }
        output.address = field_address;
        output.owner_address = owner_address;
        output.name_private = property.name_private;
        output.array_dim = property.array_dim;
        output.element_size = property.element_size;
        output.offset = property.offset_internal;
        output.flags = property.property_flags;

        if (output.type_name == "BoolProperty")
        {
            FBoolPropertyTailView boolean{};
            if (!safe_read(field_address + 0x78u, boolean) || boolean.field_size == 0u ||
                boolean.byte_offset >= boolean.field_size || boolean.byte_mask == 0u ||
                boolean.field_mask == 0u)
            {
                error = "FBoolProperty bitfield metadata is invalid";
                output = {};
                return false;
            }
            output.bool_field_size = boolean.field_size;
            output.bool_byte_offset = boolean.byte_offset;
            output.bool_byte_mask = boolean.byte_mask;
            output.bool_field_mask = boolean.field_mask;
        }

        const bool has_property_class =
                output.type_name == "ObjectProperty" ||
                output.type_name == "ObjectPtrProperty" ||
                output.type_name == "ClassProperty" ||
                output.type_name == "WeakObjectProperty" ||
                output.type_name == "LazyObjectProperty" ||
                output.type_name == "SoftObjectProperty" ||
                output.type_name == "SoftClassProperty";
        if (has_property_class &&
            !safe_read(field_address + 0x78u, output.property_class_address))
        {
            error = "FObjectPropertyBase has an unreadable PropertyClass";
            output = {};
            return false;
        }
        if (output.type_name == "StructProperty" &&
            (!safe_read(field_address + 0x78u, output.struct_address) ||
             output.struct_address == 0u))
        {
            error = "FStructProperty has an unreadable or null UScriptStruct";
            output = {};
            return false;
        }
        if ((output.type_name == "ArrayProperty" || output.type_name == "SetProperty") &&
            (!safe_read(field_address + 0x78u, output.inner_property_address) ||
             output.inner_property_address == 0u))
        {
            error = "container FProperty has an unreadable or null inner FProperty";
            output = {};
            return false;
        }
        if (output.type_name == "MapProperty" &&
            (!safe_read(field_address + 0x78u, output.key_property_address) ||
             output.key_property_address == 0u ||
             !safe_read(field_address + 0x80u, output.value_property_address) ||
             output.value_property_address == 0u))
        {
            error = "FMapProperty has an unreadable or null key/value FProperty";
            output = {};
            return false;
        }
        if (output.type_name == "EnumProperty" &&
            (!safe_read(field_address + 0x78u, output.enum_underlying_property_address) ||
             output.enum_underlying_property_address == 0u ||
             !safe_read(field_address + 0x80u, output.enum_address) ||
             output.enum_address == 0u))
        {
            error = "FEnumProperty has an unreadable or null UnderlyingProp/UEnum";
            output = {};
            return false;
        }
        if (output.type_name == "InterfaceProperty" &&
            (!safe_read(field_address + k_ue51_interface_class_offset,
                        output.interface_class_address) ||
             output.interface_class_address == 0u))
        {
            error = "FInterfaceProperty has an unreadable or null InterfaceClass";
            output = {};
            return false;
        }
        return true;
    }

    template <typename DecodeName>
    [[nodiscard]] bool decode_function_property(std::uint64_t field_address,
                                                std::int32_t storage_size,
                                                DecodeName&& decode_name,
                                                ReflectedProperty& output,
                                                std::string& error) noexcept
    {
        output = {};
        FPropertyPrefixView property{};
        ue4ss::linux::core::RawFName field_class_name{};
        if (!safe_read(field_address, property) || property.vtable == 0u || property.class_private == 0u ||
            !safe_read(property.class_private, field_class_name) ||
            !decode_name(field_class_name, output.type_name, error) ||
            !decode_name(property.name_private, output.name, error))
        {
            error = error.empty() ? "UFunction parameter metadata could not be decoded" : error;
            return false;
        }
        if (!output.type_name.ends_with("Property") || property.array_dim != 1)
        {
            error = "UFunction parameter '" + output.name + "' has FFieldClass='" + output.type_name +
                    "' ArrayDim=" + std::to_string(property.array_dim) + " instead of a supported scalar FProperty";
            return false;
        }
        std::int32_t effective_element_size = property.element_size;
        if (output.type_name == "BoolProperty")
        {
            if (!safe_read(field_address + 0x78u, output.boolean) || output.boolean.field_size == 0u ||
                output.boolean.byte_mask == 0u || output.boolean.field_mask == 0u ||
                output.boolean.byte_offset >= output.boolean.field_size)
            {
                error = "UFunction FBoolProperty parameter bitfield metadata is invalid";
                return false;
            }
            effective_element_size = output.boolean.field_size;
        }
        if (effective_element_size <= 0 || effective_element_size > k_max_property_element_size ||
            property.offset_internal < 0 || property.offset_internal > storage_size - effective_element_size)
        {
            error = "UFunction parameter '" + output.name + "' falls outside ParmsSize";
            return false;
        }
        output.address = field_address;
        output.next = property.next;
        output.element_size = effective_element_size;
        output.offset = property.offset_internal;
        output.flags = property.property_flags;
        if (output.type_name == "StructProperty" &&
            (!safe_read(field_address + 0x78u, output.struct_address) || output.struct_address == 0u))
        {
            error = "UFunction FStructProperty parameter has an unreadable or null UScriptStruct";
            output = {};
            return false;
        }
        if (output.type_name == "ArrayProperty" &&
            (!safe_read(field_address + 0x78u, output.inner_property_address) ||
             output.inner_property_address == 0u))
        {
            error = "UFunction FArrayProperty parameter has an unreadable or null Inner FProperty";
            output = {};
            return false;
        }
        if (output.type_name == "SetProperty" &&
            (!safe_read(field_address + 0x78u, output.inner_property_address) ||
             output.inner_property_address == 0u))
        {
            error = "UFunction FSetProperty parameter has an unreadable or null Element FProperty";
            output = {};
            return false;
        }
        if (output.type_name == "MapProperty" &&
            (!safe_read(field_address + 0x78u, output.key_property_address) ||
             output.key_property_address == 0u ||
             !safe_read(field_address + 0x80u, output.value_property_address) ||
             output.value_property_address == 0u))
        {
            error = "UFunction FMapProperty parameter has an unreadable or null key/value FProperty";
            output = {};
            return false;
        }
        if (output.type_name == "InterfaceProperty" &&
            (!safe_read(field_address + k_ue51_interface_class_offset,
                        output.interface_class_address) ||
             output.interface_class_address == 0u))
        {
            error = "UFunction FInterfaceProperty parameter has an unreadable or null InterfaceClass";
            output = {};
            return false;
        }
        if (!decode_enum_property_metadata(field_address, decode_name, output, error))
        {
            output = {};
            return false;
        }
        return true;
    }

    [[nodiscard]] bool decode_nested_property(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                              std::uint64_t property_address,
                                              ReflectedProperty& output,
                                              std::string& error,
                                              bool allow_nonzero_offset = false) noexcept
    {
        FPropertyPrefixView prefix{};
        if (!safe_read(property_address, prefix) || prefix.element_size <= 0)
        {
            error = "nested FProperty metadata is unreadable or invalid";
            return false;
        }
        const auto decode_name = [&runtime](ue4ss::linux::core::RawFName name,
                                            std::string& decoded,
                                            std::string& decode_error) {
            return runtime.fname_to_utf8(name, decoded, decode_error);
        };
        // Array/Set inner properties normally use offset zero. FMapProperty's
        // ValueProp carries its aligned offset within the pair, so nested
        // metadata must be bounded against the maximum supported pair span
        // instead of its own element size.
        if (!decode_function_property(
                    property_address, k_max_property_element_size, decode_name, output, error))
        {
            return false;
        }
        if (!allow_nonzero_offset && output.offset != 0)
        {
            error = "nested container FProperty has a non-zero element offset";
            output = {};
            return false;
        }
        return true;
    }

    [[nodiscard]] std::uint32_t plain_property_alignment(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ReflectedProperty& property) noexcept;

    [[nodiscard]] bool validate_script_set_view(const FScriptSetView& set,
                                                std::string& error) noexcept
    {
        if (ue4ss::linux::core::script_set_storage_size() != sizeof(FScriptSetView) ||
            ue4ss::linux::core::script_map_storage_size() != sizeof(FScriptSetView))
        {
            error = "UEPseudo FScriptSet/FScriptMap storage size disagrees with the UE 5.1 Linux mirror";
            return false;
        }
        if (set.elements.num < 0 || set.elements.max < set.elements.num ||
            set.elements.max > k_max_script_array_elements ||
            (set.elements.max == 0 && set.elements.data != 0u) ||
            (set.elements.max > 0 && set.elements.data == 0u) ||
            set.allocation_flags_num_bits != set.elements.num ||
            set.allocation_flags_max_bits < set.allocation_flags_num_bits ||
            set.allocation_flags_max_bits > k_max_script_array_elements ||
            (set.allocation_flags_max_bits & 31) != 0 ||
            (set.allocation_flags_max_bits <= 128 && set.allocation_flags_secondary != 0u) ||
            (set.allocation_flags_max_bits > 128 && set.allocation_flags_secondary == 0u) ||
            set.num_free_indices < 0 || set.num_free_indices > set.elements.num ||
            (set.num_free_indices == 0 && set.first_free_index != -1 &&
             !(set.elements.num == 0 && set.first_free_index == 0)) ||
            (set.num_free_indices > 0 &&
             (set.first_free_index < 0 || set.first_free_index >= set.elements.num)) ||
            set.hash_size < 0 || set.hash_size > k_max_script_array_elements ||
            (set.hash_size != 0 && !std::has_single_bit(static_cast<std::uint32_t>(set.hash_size))) ||
            (set.hash_size <= 1 && set.hash_secondary != 0u) ||
            (set.hash_size > 1 && set.hash_secondary == 0u))
        {
            error = "FScriptSet/FScriptMap has an invalid sparse-array or hash header";
            return false;
        }
        return true;
    }

    [[nodiscard]] bool script_set_index_is_allocated(const FScriptSetView& set,
                                                      std::int32_t index,
                                                      bool& output,
                                                      std::string& error) noexcept
    {
        output = false;
        if (index < 0 || index >= set.allocation_flags_num_bits)
        {
            error = "FScriptSet allocation-bit index is out of range";
            return false;
        }
        const std::size_t word_index = static_cast<std::size_t>(index) / 32u;
        std::uint32_t word{};
        if (set.allocation_flags_secondary == 0u)
        {
            if (word_index >= set.allocation_flags_inline.size())
            {
                error = "FScriptSet inline allocation flags exceed their fixed capacity";
                return false;
            }
            word = set.allocation_flags_inline[word_index];
        }
        else
        {
            const std::uint64_t offset = static_cast<std::uint64_t>(word_index) * sizeof(std::uint32_t);
            if (set.allocation_flags_secondary > std::numeric_limits<std::uint64_t>::max() - offset ||
                !safe_read(set.allocation_flags_secondary + offset, word))
            {
                error = "FScriptSet heap allocation flags are unreadable";
                return false;
            }
        }
        output = (word & (std::uint32_t{1} << (static_cast<std::uint32_t>(index) & 31u))) != 0u;
        return true;
    }

    [[nodiscard]] bool collect_struct_fields(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                             std::uint64_t struct_address,
                                             std::vector<ReflectedProperty>& output,
                                             std::string& struct_name,
                                             std::string& error,
                                             std::size_t depth = 0u) noexcept
    {
        output.clear();
        struct_name.clear();
        if (depth >= 32u || struct_address == 0u)
        {
            error = "UScriptStruct recursion exceeds the safety limit";
            return false;
        }
        ue4ss::linux::core::ReadOnlyUObjectHandle struct_handle;
        std::string class_error;
        if (!runtime.handle_from_address(struct_address, struct_handle) ||
            !runtime.is_a(struct_handle, "ScriptStruct", class_error))
        {
            error = class_error.empty() ? "FStructProperty does not reference a live UScriptStruct" : class_error;
            return false;
        }
        UObjectBaseView object{};
        if (!safe_read(struct_address, object) || !runtime.fname_to_utf8(object.name_private, struct_name, error))
        {
            error = error.empty() ? "UScriptStruct name metadata is unreadable" : error;
            return false;
        }

        std::vector<std::uint64_t> hierarchy;
        std::unordered_set<std::uint64_t> visited_structs;
        std::uint64_t current = struct_address;
        for (std::size_t level = 0; current != 0u && level < k_max_super_depth; ++level)
        {
            if (!visited_structs.emplace(current).second)
            {
                error = "UScriptStruct inheritance is cyclic";
                return false;
            }
            hierarchy.push_back(current);
            if (!safe_read(current + k_super_struct_offset, current))
            {
                error = "UScriptStruct SuperStruct is unreadable";
                return false;
            }
        }
        if (current != 0u)
        {
            error = "UScriptStruct hierarchy exceeds the safety limit";
            return false;
        }

        std::unordered_set<std::uint64_t> visited_fields;
        for (auto hierarchy_it = hierarchy.rbegin(); hierarchy_it != hierarchy.rend(); ++hierarchy_it)
        {
            std::int32_t properties_size{};
            std::uint64_t field_address{};
            if (!safe_read(*hierarchy_it + k_properties_size_offset, properties_size) || properties_size <= 0 ||
                properties_size > k_max_properties_size ||
                !safe_read(*hierarchy_it + k_child_properties_offset, field_address))
            {
                error = "UScriptStruct field layout is unreadable or invalid";
                return false;
            }
            const auto decode_name = [&runtime](ue4ss::linux::core::RawFName name,
                                                std::string& decoded,
                                                std::string& decode_error) {
                return runtime.fname_to_utf8(name, decoded, decode_error);
            };
            while (field_address != 0u)
            {
                if (visited_fields.size() >= k_max_property_fields || !visited_fields.emplace(field_address).second)
                {
                    error = "UScriptStruct property chain is cyclic or exceeds the safety limit";
                    return false;
                }
                ReflectedProperty property;
                if (!decode_function_property(field_address, properties_size, decode_name, property, error))
                {
                    return false;
                }
                output.push_back(std::move(property));
                FPropertyPrefixView prefix{};
                if (!safe_read(field_address, prefix))
                {
                    error = "UScriptStruct property chain became unreadable";
                    return false;
                }
                field_address = prefix.next;
            }
        }
        return true;
    }

    [[nodiscard]] bool validate_plain_struct(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                             std::uint64_t struct_address,
                                             std::string& error,
                                             std::unordered_set<std::uint64_t>& active,
                                             std::size_t depth) noexcept;

    [[nodiscard]] bool resolve_script_interface_pointer(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::ReadOnlyUObjectHandle& object,
            std::uint64_t interface_class_address,
            std::uint64_t& output,
            std::string& error) noexcept;

    [[nodiscard]] bool read_multicast_invocation_list(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ReflectedProperty& property,
            std::uint64_t value_address,
            FScriptArrayView& output,
            std::string& error) noexcept
    {
        output = {};
        try
        {
            if (is_inline_multicast_delegate_property(property.type_name))
            {
                if (property.element_size != static_cast<std::int32_t>(sizeof(FScriptArrayView)) ||
                    !safe_read(value_address, output))
                {
                    error = "inline FMulticastScriptDelegate storage is unreadable or incompatible";
                    return false;
                }
            }
            else if (is_sparse_multicast_delegate_property(property.type_name))
            {
                using GetMulticastDelegateFunction = const FScriptArrayView* (*)(const void*, const void*);
                if (property.element_size != 1)
                {
                    error = "FSparseDelegate storage is not one byte on this Unreal build";
                    return false;
                }
                std::uint64_t signature_function_address{};
                ue4ss::linux::core::ReadOnlyUObjectHandle signature_function;
                std::string class_error;
                if (!safe_read(property.address + 0x78u, signature_function_address) ||
                    signature_function_address == 0u ||
                    !runtime.handle_from_address(signature_function_address, signature_function) ||
                    !runtime.is_a(signature_function, "Function", class_error))
                {
                    error = class_error.empty()
                                    ? "sparse multicast delegate does not reference a live signature UFunction"
                                    : class_error;
                    return false;
                }
                GetMulticastDelegateFunction get_delegate{};
                if (!load_fproperty_virtual(
                            property.address,
                            k_multicast_get_delegate_slot,
                            get_delegate,
                            error))
                {
                    return false;
                }
#if !defined(UE4SS_LINUX_TESTING)
                std::array<std::uint8_t, 0x20> code{};
                constexpr std::array<std::uint8_t, 3> bound_flag_gate{0x80u, 0x3eu, 0x00u};
                if (!safe_read_bytes(
                            std::bit_cast<std::uintptr_t>(get_delegate), code.data(), code.size()) ||
                    std::search(code.begin(), code.end(), bound_flag_gate.begin(), bound_flag_gate.end()) ==
                            code.end())
                {
                    error = "sparse GetMulticastDelegate candidate lacks the FSparseDelegate bound-flag gate";
                    return false;
                }
#endif
                const auto* invocation_list = get_delegate(
                        std::bit_cast<const void*>(static_cast<std::uintptr_t>(property.address)),
                        std::bit_cast<const void*>(static_cast<std::uintptr_t>(value_address)));
                if (invocation_list == nullptr)
                {
                    return true;
                }
                if (!safe_read(std::bit_cast<std::uintptr_t>(invocation_list), output))
                {
                    error = "sparse FMulticastScriptDelegate invocation list is unreadable";
                    return false;
                }
            }
            else
            {
                error = "property is not a supported multicast delegate";
                return false;
            }
            if (output.num < 0 || output.max < output.num ||
                output.max > k_max_script_array_elements ||
                (output.max == 0 && output.data != 0u) ||
                (output.max > 0 && output.data == 0u))
            {
                error = "FMulticastScriptDelegate has an invalid invocation-list header";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while resolving a multicast delegate invocation list";
            return false;
        }
    }

    [[nodiscard]] bool validate_plain_property(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                               const ReflectedProperty& property,
                                               std::string& error,
                                               std::unordered_set<std::uint64_t>& active,
                                               std::size_t depth) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        const auto kind = property_kind(property);
        if (depth >= 32u || !kind.has_value())
        {
            error = "property '" + property.name + "' has non-plain type '" + property.type_name + "'";
            return false;
        }
        if (*kind == UnrealPropertyKind::structure)
        {
            return validate_plain_struct(runtime, property.struct_address, error, active, depth + 1u);
        }
        if (*kind == UnrealPropertyKind::array)
        {
            ReflectedProperty inner;
            if (!decode_nested_property(runtime, property.inner_property_address, inner, error))
            {
                return false;
            }
            return validate_plain_property(runtime, inner, error, active, depth + 1u);
        }
        if (*kind == UnrealPropertyKind::set)
        {
            ReflectedProperty element;
            if (!decode_nested_property(runtime, property.inner_property_address, element, error))
            {
                return false;
            }
            ue4ss::linux::core::UnrealScriptSetLayout layout;
            if (!ue4ss::linux::core::calculate_script_set_layout(
                        element.element_size,
                        static_cast<std::int32_t>(plain_property_alignment(runtime, element)),
                        layout,
                        error))
            {
                return false;
            }
            return validate_plain_property(runtime, element, error, active, depth + 1u);
        }
        if (*kind == UnrealPropertyKind::map)
        {
            ReflectedProperty key;
            ReflectedProperty value;
            if (!decode_nested_property(runtime, property.key_property_address, key, error, true) ||
                !decode_nested_property(runtime, property.value_property_address, value, error, true))
            {
                return false;
            }
            ue4ss::linux::core::UnrealScriptMapLayout layout;
            if (!ue4ss::linux::core::calculate_script_map_layout(
                        key.element_size,
                        static_cast<std::int32_t>(plain_property_alignment(runtime, key)),
                        value.element_size,
                        static_cast<std::int32_t>(plain_property_alignment(runtime, value)),
                        layout,
                        error))
            {
                return false;
            }
            return validate_plain_property(runtime, key, error, active, depth + 1u) &&
                   validate_plain_property(runtime, value, error, active, depth + 1u);
        }
        return true;
    }

    [[nodiscard]] bool validate_plain_struct(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                             std::uint64_t struct_address,
                                             std::string& error,
                                             std::unordered_set<std::uint64_t>& active,
                                             std::size_t depth) noexcept
    {
        if (depth >= 32u || !active.emplace(struct_address).second)
        {
            error = "recursive UScriptStruct value layout is unsupported";
            return false;
        }
        std::vector<ReflectedProperty> fields;
        std::string name;
        if (!collect_struct_fields(runtime, struct_address, fields, name, error, depth))
        {
            active.erase(struct_address);
            return false;
        }
        for (const auto& field : fields)
        {
            if (!validate_plain_property(runtime, field, error, active, depth + 1u))
            {
                error = "UScriptStruct '" + name + "' field '" + field.name + "': " + error;
                active.erase(struct_address);
                return false;
            }
        }
        active.erase(struct_address);
        return true;
    }

    [[nodiscard]] bool validate_plain_struct(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                             std::uint64_t struct_address,
                                             std::string& error) noexcept
    {
        std::unordered_set<std::uint64_t> active;
        return validate_plain_struct(runtime, struct_address, error, active, 0u);
    }

    [[nodiscard]] bool decode_plain_value(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                          const ReflectedProperty& property,
                                          std::uint64_t address,
                                          ue4ss::linux::core::UnrealPropertyValue& output,
                                          std::string& error,
                                          std::size_t depth = 0u) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        output = {};
        const auto kind = property_kind(property);
        if (!kind.has_value())
        {
            error = "plain value decoder does not support '" + property.type_name + "'";
            return false;
        }
        output.kind = *kind;
        if (*kind == UnrealPropertyKind::string)
        {
            return property.element_size == static_cast<std::int32_t>(sizeof(ue4ss::linux::core::UnrealStringHeader)) &&
                           runtime.read_unreal_string(address, output.text, error) ||
                   (error = error.empty() ? "plain FString storage has an incompatible layout" : error, false);
        }
        if (*kind == UnrealPropertyKind::text)
        {
            FTextView text{};
            if (property.element_size != static_cast<std::int32_t>(sizeof(text)) || !safe_read(address, text))
            {
                error = "FText storage is unreadable or has an incompatible UE 5.1 layout";
                return false;
            }
            if (text.data == 0u)
            {
                output.text.clear();
                return true;
            }

            std::uint64_t vtable{};
            std::uint64_t display_string_function{};
            if (!safe_read(text.data, vtable) || vtable == 0u ||
                vtable > std::numeric_limits<std::uint64_t>::max() -
                                 k_ue51_linux_itextdata_display_string_slot ||
                !safe_read(vtable + k_ue51_linux_itextdata_display_string_slot,
                           display_string_function) ||
                display_string_function == 0u || !is_executable_mapping(display_string_function))
            {
                error = "FText ITextData::GetDisplayString slot is unreadable or not executable";
                return false;
            }

            // A C++ reference return is represented as a pointer by the SysV
            // x86_64 ABI. Copy the returned FString immediately so no
            // engine-owned pointer crosses this boundary.
            using GetDisplayStringFunction = const ue4ss::linux::core::UnrealStringHeader* (*)(const void*);
            try
            {
                const auto get_display_string = std::bit_cast<GetDisplayStringFunction>(
                        static_cast<std::uintptr_t>(display_string_function));
                const auto* display_string = get_display_string(
                        std::bit_cast<const void*>(static_cast<std::uintptr_t>(text.data)));
                if (display_string == nullptr ||
                    !runtime.read_unreal_string(
                            std::bit_cast<std::uintptr_t>(display_string), output.text, error))
                {
                    error = error.empty() ? "FText display FString is null or invalid" : error;
                    return false;
                }
                return true;
            }
            catch (const std::exception& exception)
            {
                error = std::string{"ITextData::GetDisplayString raised an exception: "} + exception.what();
                return false;
            }
            catch (...)
            {
                error = "ITextData::GetDisplayString raised an unknown exception";
                return false;
            }
        }
        if (*kind == UnrealPropertyKind::boolean)
        {
            std::uint8_t byte{};
            if (!safe_read(address + property.boolean.byte_offset, byte))
            {
                error = "plain BoolProperty storage is unreadable";
                return false;
            }
            output.boolean = (byte & property.boolean.field_mask) != 0u;
            return true;
        }
        if (*kind == UnrealPropertyKind::signed_integer || *kind == UnrealPropertyKind::unsigned_integer)
        {
            if (property.element_size != 1 && property.element_size != 2 && property.element_size != 4 &&
                property.element_size != 8)
            {
                error = "plain integer property has an unsupported element size";
                return false;
            }
            std::uint64_t raw{};
            if (!safe_read_bytes(address, &raw, static_cast<std::size_t>(property.element_size)))
            {
                error = "plain integer property storage is unreadable";
                return false;
            }
            if (*kind == UnrealPropertyKind::signed_integer)
            {
                output.signed_integer = sign_extend_integer(raw, property.element_size);
            }
            else
            {
                output.unsigned_integer = raw;
            }
            if (property.enum_address != 0u)
            {
                std::string enum_error;
                if (!runtime.handle_from_address(property.enum_address, output.enum_object) ||
                    !runtime.is_a(output.enum_object, "Enum", enum_error))
                {
                    error = enum_error.empty() ? "numeric enum metadata does not reference a live UEnum"
                                               : enum_error;
                    return false;
                }
                output.enum_property_name = property.name;
            }
            return true;
        }
        if (*kind == UnrealPropertyKind::floating_point)
        {
            if (property.type_name == "FloatProperty" && property.element_size == sizeof(float))
            {
                float value{};
                if (!safe_read(address, value))
                {
                    error = "plain float storage is unreadable";
                    return false;
                }
                output.floating_point = value;
                return true;
            }
            if (property.type_name == "DoubleProperty" && property.element_size == sizeof(double) &&
                safe_read(address, output.floating_point))
            {
                return true;
            }
            error = "plain floating-point property has an incompatible layout";
            return false;
        }
        if (*kind == UnrealPropertyKind::name)
        {
            return safe_read(address, output.name) && runtime.fname_to_utf8(output.name, output.text, error);
        }
        if (*kind == UnrealPropertyKind::object)
        {
            if (property.type_name == "InterfaceProperty")
            {
                FScriptInterfaceView script_interface{};
                if (property.element_size != static_cast<std::int32_t>(sizeof(script_interface)) ||
                    property.interface_class_address == 0u ||
                    !safe_read(address, script_interface))
                {
                    error = "FScriptInterface storage or InterfaceClass metadata is incompatible";
                    return false;
                }
                output.object_is_null = script_interface.object == 0u;
                if (output.object_is_null)
                {
                    if (script_interface.interface_pointer != 0u)
                    {
                        error = "null FScriptInterface retains a non-null interface pointer";
                        return false;
                    }
                    return true;
                }
                std::uint64_t expected_interface_pointer{};
                if (!runtime.handle_from_address(script_interface.object, output.object) ||
                    !resolve_script_interface_pointer(runtime,
                                                      output.object,
                                                      property.interface_class_address,
                                                      expected_interface_pointer,
                                                      error))
                {
                    error = error.empty()
                                    ? "FScriptInterface object is outside GUObjectArray"
                                    : error;
                    return false;
                }
                if (script_interface.interface_pointer != expected_interface_pointer)
                {
                    error = "FScriptInterface pointer does not match UClass::Interfaces metadata";
                    return false;
                }
                return true;
            }
            std::uint64_t object_address{};
            if (!is_direct_object_property(property.type_name) ||
                property.element_size != static_cast<std::int32_t>(sizeof(void*)) ||
                !safe_read(address, object_address))
            {
                error = "plain UObject storage is unreadable";
                return false;
            }
            output.object_is_null = object_address == 0u;
            return output.object_is_null || runtime.handle_from_address(object_address, output.object) ||
                   (error = "plain UObject points outside GUObjectArray", false);
        }
        if (*kind == UnrealPropertyKind::weak_object)
        {
            FWeakObjectPtrView weak{};
            if (property.element_size != static_cast<std::int32_t>(sizeof(weak)) || !safe_read(address, weak))
            {
                error = "FWeakObjectPtr storage is unreadable or has an incompatible layout";
                return false;
            }
            output.object_is_null = weak.object_serial_number == 0 ||
                                    !runtime.handle_from_weak(
                                            weak.object_index, weak.object_serial_number, output.object);
            return true;
        }
        if (*kind == UnrealPropertyKind::soft_object)
        {
            FSoftObjectPtrView soft{};
            if (property.element_size != static_cast<std::int32_t>(sizeof(soft)))
            {
                error = "FSoftObjectPtr element size is " + std::to_string(property.element_size) +
                        ", expected " + std::to_string(sizeof(soft)) + " for UE 5.1";
                return false;
            }
            if (!safe_read(address, soft) ||
                !runtime.fname_to_utf8(
                        soft.asset_path.package_name, output.soft_package_text, error) ||
                !runtime.fname_to_utf8(
                        soft.asset_path.asset_name, output.soft_asset_text, error) ||
                !runtime.read_unreal_string(
                        address + offsetof(FSoftObjectPtrView, sub_path_string),
                        output.soft_sub_path,
                        error))
            {
                error = error.empty() ? "FSoftObjectPtr storage is unreadable or incompatible" : error;
                return false;
            }
            output.soft_package_name = soft.asset_path.package_name;
            output.soft_asset_name = soft.asset_path.asset_name;
            const bool package_is_none = output.soft_package_name.comparison_index == 0u &&
                                         output.soft_package_name.number == 0u;
            const bool asset_is_none = output.soft_asset_name.comparison_index == 0u &&
                                       output.soft_asset_name.number == 0u;
            if (package_is_none && !asset_is_none)
            {
                error = "FTopLevelAssetPath has an asset name without a package name";
                return false;
            }
            output.text = package_is_none ? "None" : output.soft_package_text;
            if (!package_is_none && !asset_is_none)
            {
                output.text.push_back('.');
                output.text += output.soft_asset_text;
            }
            std::string legacy_name_error;
            if (!runtime.fname_from_utf8(output.text, output.name, false, legacy_name_error))
            {
                error = "combined FSoftObjectPath name validation failed: " + legacy_name_error;
                return false;
            }
            output.signed_integer = soft.tag_at_last_test;
            output.object_is_null = soft.weak_ptr.object_serial_number == 0 ||
                                    !runtime.handle_from_weak(
                                            soft.weak_ptr.object_index,
                                            soft.weak_ptr.object_serial_number,
                                            output.object);
            return true;
        }
        if (*kind == UnrealPropertyKind::delegate)
        {
            FScriptDelegateView delegate{};
            if (property.element_size != static_cast<std::int32_t>(sizeof(delegate)) ||
                !safe_read(address, delegate) ||
                !runtime.fname_to_utf8(delegate.function_name, output.text, error))
            {
                error = error.empty() ? "FScriptDelegate storage is unreadable or incompatible" : error;
                return false;
            }
            output.name = delegate.function_name;
            output.object_is_null = delegate.object.object_serial_number == 0 ||
                                    !runtime.handle_from_weak(delegate.object.object_index,
                                                              delegate.object.object_serial_number,
                                                              output.object);
            return true;
        }
        if (*kind == UnrealPropertyKind::multicast_delegate)
        {
            FScriptArrayView invocation_list{};
            if (!read_multicast_invocation_list(runtime, property, address, invocation_list, error))
            {
                return false;
            }
            output.multicast_sparse = is_sparse_multicast_delegate_property(property.type_name);
            output.array_elements.reserve(static_cast<std::size_t>(invocation_list.num));
            ReflectedProperty delegate_property{
                    .type_name = "DelegateProperty",
                    .element_size = static_cast<std::int32_t>(sizeof(FScriptDelegateView)),
            };
            for (std::int32_t index = 0; index < invocation_list.num; ++index)
            {
                const std::uint64_t offset = static_cast<std::uint64_t>(index) * sizeof(FScriptDelegateView);
                if (invocation_list.data > std::numeric_limits<std::uint64_t>::max() - offset)
                {
                    error = "FMulticastScriptDelegate entry address overflows";
                    return false;
                }
                auto binding = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
                if (!decode_plain_value(runtime,
                                        delegate_property,
                                        invocation_list.data + offset,
                                        *binding,
                                        error,
                                        depth + 1u))
                {
                    error = "FMulticastScriptDelegate binding " + std::to_string(index) + ": " + error;
                    return false;
                }
                output.array_elements.push_back(std::move(binding));
            }
            return true;
        }
        if (*kind == UnrealPropertyKind::soft_object)
        {
            FSoftObjectPtrView soft{};
            std::string sub_path;
            if (property.element_size != static_cast<std::int32_t>(sizeof(soft)) ||
                !safe_read(address, soft) ||
                !runtime.read_unreal_string(
                        address + offsetof(FSoftObjectPtrView, sub_path_string),
                        sub_path,
                        error))
            {
                error = error.empty() ? "FSoftObjectPtr release storage is invalid" : error;
                return false;
            }
            const FSoftObjectPtrView empty{};
            if (!safe_write_bytes(address, &empty, sizeof(empty)))
            {
                error = "released FSoftObjectPtr storage is not writable";
                return false;
            }
            runtime.release_unreal_string(soft.sub_path_string);
            return true;
        }
        if (*kind == UnrealPropertyKind::structure)
        {
            if (depth >= 32u || !validate_plain_struct(runtime, property.struct_address, error))
            {
                return false;
            }
            std::vector<ReflectedProperty> fields;
            if (!collect_struct_fields(runtime, property.struct_address, fields, output.struct_type, error, depth + 1u))
            {
                return false;
            }
            output.struct_fields.reserve(fields.size());
            for (const auto& field : fields)
            {
                auto value = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
                if (!decode_plain_value(runtime,
                                        field,
                                        address + static_cast<std::uint64_t>(field.offset),
                                        *value,
                                        error,
                                        depth + 1u))
                {
                    return false;
                }
                output.struct_fields.push_back({field.name, std::move(value)});
            }
            return true;
        }
        if (*kind == UnrealPropertyKind::array)
        {
            if (depth >= 32u)
            {
                error = "nested FScriptArray recursion exceeds the safety limit";
                return false;
            }
            FScriptArrayView array{};
            if (!safe_read(address, array) || array.num < 0 || array.max < array.num ||
                array.max > k_max_script_array_elements ||
                (array.max == 0 && array.data != 0u) || (array.max > 0 && array.data == 0u))
            {
                error = "FScriptArray has an invalid array header";
                return false;
            }
            ReflectedProperty inner;
            if (!decode_nested_property(runtime, property.inner_property_address, inner, error))
            {
                return false;
            }
            std::unordered_set<std::uint64_t> active;
            if (!validate_plain_property(runtime, inner, error, active, depth + 1u))
            {
                error = "FScriptArray Inner is unsupported: " + error;
                return false;
            }
            if (array.num > 0 &&
                static_cast<std::uint64_t>(array.num) >
                        std::numeric_limits<std::uint64_t>::max() /
                                static_cast<std::uint64_t>(inner.element_size))
            {
                error = "FScriptArray byte size overflows the address space";
                return false;
            }
            output.array_elements.reserve(static_cast<std::size_t>(array.num));
            for (std::int32_t index = 0; index < array.num; ++index)
            {
                const std::uint64_t offset = static_cast<std::uint64_t>(index) *
                                             static_cast<std::uint64_t>(inner.element_size);
                if (array.data > std::numeric_limits<std::uint64_t>::max() - offset)
                {
                    error = "FScriptArray element address overflows";
                    return false;
                }
                auto value = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
                if (!decode_plain_value(runtime, inner, array.data + offset, *value, error, depth + 1u))
                {
                    error = "FScriptArray element " + std::to_string(index) + ": " + error;
                    return false;
                }
                output.array_elements.push_back(std::move(value));
            }
            return true;
        }
        if (*kind == UnrealPropertyKind::set)
        {
            if (depth >= 32u || property.element_size !=
                                        static_cast<std::int32_t>(
                                                ue4ss::linux::core::script_set_storage_size()))
            {
                error = "FScriptSet recursion or storage size is incompatible with UEPseudo";
                return false;
            }
            FScriptSetView set{};
            ReflectedProperty element;
            if (!safe_read(address, set) || !validate_script_set_view(set, error) ||
                !decode_nested_property(runtime, property.inner_property_address, element, error))
            {
                error = error.empty() ? "FScriptSet storage is unreadable" : error;
                return false;
            }
            std::unordered_set<std::uint64_t> active;
            if (!validate_plain_property(runtime, element, error, active, depth + 1u))
            {
                error = "FScriptSet Element is unsupported: " + error;
                return false;
            }
            ue4ss::linux::core::UnrealScriptSetLayout layout;
            if (!ue4ss::linux::core::calculate_script_set_layout(
                        element.element_size,
                        static_cast<std::int32_t>(plain_property_alignment(runtime, element)),
                        layout,
                        error) ||
                layout.sparse_stride <= 0 || layout.sparse_stride > k_max_property_element_size)
            {
                error = error.empty() ? "UEPseudo returned an invalid FScriptSet layout" : error;
                return false;
            }
            output.set_elements.reserve(
                    static_cast<std::size_t>(set.elements.num - set.num_free_indices));
            std::int32_t allocated_count{};
            for (std::int32_t index = 0; index < set.elements.num; ++index)
            {
                bool allocated{};
                if (!script_set_index_is_allocated(set, index, allocated, error))
                {
                    return false;
                }
                if (!allocated)
                {
                    continue;
                }
                ++allocated_count;
                const std::uint64_t offset = static_cast<std::uint64_t>(index) *
                                             static_cast<std::uint64_t>(layout.sparse_stride);
                if (set.elements.data > std::numeric_limits<std::uint64_t>::max() - offset)
                {
                    error = "FScriptSet element address overflows";
                    return false;
                }
                auto value = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
                if (!decode_plain_value(runtime,
                                        element,
                                        set.elements.data + offset,
                                        *value,
                                        error,
                                        depth + 1u))
                {
                    error = "FScriptSet element " + std::to_string(index) + ": " + error;
                    return false;
                }
                output.set_elements.push_back(std::move(value));
            }
            if (allocated_count != set.elements.num - set.num_free_indices)
            {
                error = "FScriptSet allocation flags disagree with NumFreeIndices";
                return false;
            }
            return true;
        }
        if (*kind == UnrealPropertyKind::map)
        {
            if (depth >= 32u || property.element_size !=
                                        static_cast<std::int32_t>(
                                                ue4ss::linux::core::script_map_storage_size()))
            {
                error = "FScriptMap recursion or storage size is incompatible with UEPseudo";
                return false;
            }
            FScriptSetView map{};
            ReflectedProperty key;
            ReflectedProperty value;
            if (!safe_read(address, map) || !validate_script_set_view(map, error) ||
                !decode_nested_property(runtime, property.key_property_address, key, error, true) ||
                !decode_nested_property(runtime, property.value_property_address, value, error, true))
            {
                error = error.empty() ? "FScriptMap storage is unreadable" : error;
                return false;
            }
            std::unordered_set<std::uint64_t> active;
            if (!validate_plain_property(runtime, key, error, active, depth + 1u) ||
                !validate_plain_property(runtime, value, error, active, depth + 1u))
            {
                error = "FScriptMap key/value is unsupported: " + error;
                return false;
            }
            ue4ss::linux::core::UnrealScriptMapLayout layout;
            if (!ue4ss::linux::core::calculate_script_map_layout(
                        key.element_size,
                        static_cast<std::int32_t>(plain_property_alignment(runtime, key)),
                        value.element_size,
                        static_cast<std::int32_t>(plain_property_alignment(runtime, value)),
                        layout,
                        error) ||
                layout.value_offset <= 0 || layout.set.sparse_stride <= layout.value_offset ||
                layout.set.sparse_stride > k_max_property_element_size)
            {
                error = error.empty() ? "UEPseudo returned an invalid FScriptMap layout" : error;
                return false;
            }
            output.map_entries.reserve(
                    static_cast<std::size_t>(map.elements.num - map.num_free_indices));
            std::int32_t allocated_count{};
            for (std::int32_t index = 0; index < map.elements.num; ++index)
            {
                bool allocated{};
                if (!script_set_index_is_allocated(map, index, allocated, error))
                {
                    return false;
                }
                if (!allocated)
                {
                    continue;
                }
                ++allocated_count;
                const std::uint64_t offset = static_cast<std::uint64_t>(index) *
                                             static_cast<std::uint64_t>(layout.set.sparse_stride);
                if (map.elements.data > std::numeric_limits<std::uint64_t>::max() - offset ||
                    map.elements.data + offset > std::numeric_limits<std::uint64_t>::max() -
                                                         static_cast<std::uint64_t>(layout.value_offset))
                {
                    error = "FScriptMap pair address overflows";
                    return false;
                }
                auto decoded_key = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
                auto decoded_value = std::make_shared<ue4ss::linux::core::UnrealPropertyValue>();
                const std::uint64_t pair_address = map.elements.data + offset;
                if (!decode_plain_value(runtime, key, pair_address, *decoded_key, error, depth + 1u) ||
                    !decode_plain_value(runtime,
                                        value,
                                        pair_address + static_cast<std::uint64_t>(layout.value_offset),
                                        *decoded_value,
                                        error,
                                        depth + 1u))
                {
                    error = "FScriptMap pair " + std::to_string(index) + ": " + error;
                    return false;
                }
                output.map_entries.push_back({std::move(decoded_key), std::move(decoded_value)});
            }
            if (allocated_count != map.elements.num - map.num_free_indices)
            {
                error = "FScriptMap allocation flags disagree with NumFreeIndices";
                return false;
            }
            return true;
        }
        error = "plain value decoder reached an invalid property kind";
        return false;
    }

    [[nodiscard]] const ue4ss::linux::core::UnrealPropertyValue* find_struct_field(
            const ue4ss::linux::core::UnrealPropertyValue& value,
            std::string_view name) noexcept
    {
        const auto found = std::find_if(value.struct_fields.begin(), value.struct_fields.end(), [name](const auto& field) {
            return field.name == name && field.value != nullptr;
        });
        return found == value.struct_fields.end() ? nullptr : found->value.get();
    }

    [[nodiscard]] bool validate_script_array_header(const FScriptArrayView& array,
                                                    std::string& error) noexcept
    {
        if (array.num < 0 || array.max < array.num || array.max > k_max_script_array_elements ||
            (array.max == 0 && array.data != 0u) || (array.max > 0 && array.data == 0u))
        {
            error = "FScriptArray has an invalid array header";
            return false;
        }
        return true;
    }

    [[nodiscard]] bool class_is_child_of(std::uint64_t candidate,
                                         std::uint64_t requested,
                                         std::string& error) noexcept
    {
        if (candidate == 0u || requested == 0u)
        {
            error = "interface class hierarchy contains a null UClass";
            return false;
        }
        std::unordered_set<std::uint64_t> visited;
        for (std::size_t depth = 0; candidate != 0u && depth < k_max_super_depth; ++depth)
        {
            if (!visited.emplace(candidate).second)
            {
                error = "interface class hierarchy is cyclic";
                return false;
            }
            if (candidate == requested)
            {
                return true;
            }
            if (!safe_read(candidate + k_super_struct_offset, candidate))
            {
                error = "interface UClass::SuperStruct became unreadable";
                return false;
            }
        }
        if (candidate != 0u)
        {
            error = "interface class hierarchy exceeds the safety limit";
        }
        return false;
    }

    [[nodiscard]] bool resolve_script_interface_pointer(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::ReadOnlyUObjectHandle& object,
            std::uint64_t interface_class_address,
            std::uint64_t& output,
            std::string& error) noexcept
    {
        output = 0u;
        if (!runtime.is_valid(object))
        {
            error = "FScriptInterface assignment requires a live UObject";
            return false;
        }
        ue4ss::linux::core::ReadOnlyUObjectHandle interface_class;
        std::string class_error;
        if (!runtime.handle_from_address(interface_class_address, interface_class) ||
            !runtime.is_a(interface_class, "Class", class_error))
        {
            error = class_error.empty()
                            ? "FInterfaceProperty::InterfaceClass is not a live UClass"
                            : class_error;
            return false;
        }

        UObjectBaseView target{};
        if (!safe_read(object.address, target) || target.class_private == 0u)
        {
            error = "FScriptInterface target class is unreadable";
            return false;
        }
        std::unordered_set<std::uint64_t> visited_classes;
        std::uint64_t current_class = target.class_private;
        for (std::size_t depth = 0; current_class != 0u && depth < k_max_super_depth; ++depth)
        {
            if (!visited_classes.emplace(current_class).second)
            {
                error = "UClass hierarchy is cyclic while resolving an interface";
                return false;
            }
            FScriptArrayView interfaces{};
            if (!safe_read(current_class + k_ue51_uclass_interfaces_offset, interfaces) ||
                !validate_script_array_header(interfaces, error) ||
                interfaces.num > k_max_implemented_interfaces ||
                (interfaces.num > 0 &&
                 (interfaces.data % alignof(FImplementedInterfaceView) != 0u ||
                  interfaces.data > std::numeric_limits<std::uint64_t>::max() -
                                            static_cast<std::uint64_t>(interfaces.num) *
                                                    sizeof(FImplementedInterfaceView))))
            {
                error = error.empty() ? "UClass::Interfaces has an invalid UE 5.1 layout" : error;
                return false;
            }
            for (std::int32_t index = 0; index < interfaces.num; ++index)
            {
                FImplementedInterfaceView implemented{};
                const std::uint64_t entry_address = interfaces.data +
                        static_cast<std::uint64_t>(index) * sizeof(implemented);
                ue4ss::linux::core::ReadOnlyUObjectHandle implemented_class;
                if (!safe_read(entry_address, implemented) || implemented.interface_class == 0u ||
                    implemented.implemented_by_k2 > 1u ||
                    !runtime.handle_from_address(implemented.interface_class, implemented_class) ||
                    !runtime.is_a(implemented_class, "Class", class_error))
                {
                    error = class_error.empty()
                                    ? "UClass::Interfaces contains invalid FImplementedInterface metadata"
                                    : class_error;
                    return false;
                }
                std::string hierarchy_error;
                if (!class_is_child_of(
                            implemented.interface_class, interface_class_address, hierarchy_error))
                {
                    if (!hierarchy_error.empty())
                    {
                        error = hierarchy_error;
                        return false;
                    }
                    continue;
                }
                if (implemented.implemented_by_k2 != 0u)
                {
                    // Blueprint-only interfaces intentionally have no native
                    // C++ interface subobject. FScriptInterface still retains
                    // the UObject so reflected interface calls can dispatch.
                    output = 0u;
                    return true;
                }
                const std::int64_t adjusted = static_cast<std::int64_t>(object.address) +
                                              static_cast<std::int64_t>(implemented.pointer_offset);
                if (adjusted <= 0 ||
                    static_cast<std::uint64_t>(adjusted) >
                            std::numeric_limits<std::uintptr_t>::max())
                {
                    error = "FImplementedInterface::PointerOffset overflows the target UObject";
                    return false;
                }
                output = static_cast<std::uint64_t>(adjusted);
                std::uint64_t interface_vtable{};
                if (!safe_read(output, interface_vtable) || !is_executable_mapping(interface_vtable))
                {
                    error = "native interface pointer does not reference an executable vtable";
                    output = 0u;
                    return false;
                }
                return true;
            }
            if (!safe_read(current_class + k_super_struct_offset, current_class))
            {
                error = "UClass::SuperStruct became unreadable while resolving an interface";
                return false;
            }
        }
        if (current_class != 0u)
        {
            error = "UClass hierarchy exceeds the interface safety limit";
        }
        else
        {
            error = "UObject does not implement FInterfaceProperty::InterfaceClass";
        }
        return false;
    }

    [[nodiscard]] std::uint32_t plain_property_alignment(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ReflectedProperty& property) noexcept
    {
        std::uint32_t runtime_alignment{};
        std::string alignment_error;
        if (runtime.fproperty_abi_available() &&
            runtime.reflected_value_alignment(property.address, runtime_alignment, alignment_error))
        {
            return runtime_alignment;
        }
        if (property.type_name == "StructProperty" && property.struct_address != 0u)
        {
            std::int32_t alignment{};
            if (safe_read(property.struct_address + k_properties_size_offset + sizeof(std::int32_t), alignment) &&
                alignment > 0 && alignment <= 4096 && std::has_single_bit(static_cast<std::uint32_t>(alignment)))
            {
                return static_cast<std::uint32_t>(alignment);
            }
        }
        const std::uint32_t bounded = static_cast<std::uint32_t>(
                std::clamp(property.element_size, 1, static_cast<std::int32_t>(alignof(std::max_align_t))));
        return std::bit_floor(bounded);
    }

    [[nodiscard]] bool encode_plain_value(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                          const ReflectedProperty& property,
                                          std::uint64_t address,
                                          const ue4ss::linux::core::UnrealPropertyValue& value,
                                          std::string& error,
                                          std::size_t depth = 0u) noexcept;

    void release_raw_script_set_allocations(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            FScriptSetView& set) noexcept
    {
        runtime.release_unreal_storage(
                std::bit_cast<void*>(static_cast<std::uintptr_t>(set.elements.data)));
        runtime.release_unreal_storage(
                std::bit_cast<void*>(static_cast<std::uintptr_t>(set.allocation_flags_secondary)));
        runtime.release_unreal_storage(
                std::bit_cast<void*>(static_cast<std::uintptr_t>(set.hash_secondary)));
        set = {};
    }

    [[nodiscard]] bool allocate_dense_script_set_storage(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::UnrealScriptSetLayout& layout,
            std::size_t count,
            FScriptSetView& output,
            std::string& error) noexcept
    {
        output = {};
        output.first_free_index = -1;
        if (count > static_cast<std::size_t>(k_max_script_array_elements) ||
            count > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
            layout.sparse_stride <= 0 || layout.sparse_alignment <= 0 ||
            layout.hash_next_id_offset < 0 || layout.hash_index_offset < 0 ||
            layout.hash_next_id_offset > layout.sparse_stride - static_cast<std::int32_t>(sizeof(std::int32_t)) ||
            layout.hash_index_offset > layout.sparse_stride - static_cast<std::int32_t>(sizeof(std::int32_t)))
        {
            error = "dense FScriptSet allocation request or UEPseudo layout is invalid";
            return false;
        }
        if (count == 0u)
        {
            output.allocation_flags_max_bits = 128;
            return true;
        }
        if (count > std::numeric_limits<std::size_t>::max() /
                            static_cast<std::size_t>(layout.sparse_stride))
        {
            error = "dense FScriptSet element byte count overflows";
            return false;
        }

        void* elements{};
        if (!runtime.allocate_unreal_storage(
                    count * static_cast<std::size_t>(layout.sparse_stride),
                    static_cast<std::uint32_t>(layout.sparse_alignment),
                    elements,
                    error))
        {
            return false;
        }
        output.elements = {
                .data = std::bit_cast<std::uintptr_t>(elements),
                .num = static_cast<std::int32_t>(count),
                .max = static_cast<std::int32_t>(count),
        };
        std::memset(elements, 0, count * static_cast<std::size_t>(layout.sparse_stride));
        output.allocation_flags_num_bits = static_cast<std::int32_t>(count);
        if (count <= 128u)
        {
            output.allocation_flags_max_bits = 128;
            for (std::size_t index = 0; index < count; ++index)
            {
                output.allocation_flags_inline[index / 32u] |=
                        std::uint32_t{1} << static_cast<std::uint32_t>(index % 32u);
            }
        }
        else
        {
            const std::size_t word_count = (count + 31u) / 32u;
            if (word_count > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) / 32u)
            {
                release_raw_script_set_allocations(runtime, output);
                error = "dense FScriptSet allocation-bit count overflows";
                return false;
            }
            void* flags{};
            if (!runtime.allocate_unreal_storage(
                        word_count * sizeof(std::uint32_t), alignof(std::uint32_t), flags, error))
            {
                release_raw_script_set_allocations(runtime, output);
                return false;
            }
            output.allocation_flags_secondary = std::bit_cast<std::uintptr_t>(flags);
            output.allocation_flags_max_bits = static_cast<std::int32_t>(word_count * 32u);
            std::memset(flags, 0xff, word_count * sizeof(std::uint32_t));
            const std::size_t remainder = count % 32u;
            if (remainder != 0u)
            {
                static_cast<std::uint32_t*>(flags)[word_count - 1u] =
                        (std::uint32_t{1} << static_cast<std::uint32_t>(remainder)) - 1u;
            }
        }

        const std::uint32_t hash_size = ue4ss::linux::core::script_set_hash_size(
                static_cast<std::uint32_t>(count));
        if (hash_size == 0u || hash_size > static_cast<std::uint32_t>(k_max_script_array_elements) ||
            !std::has_single_bit(hash_size))
        {
            release_raw_script_set_allocations(runtime, output);
            error = "UEPseudo returned an invalid default FScriptSet hash size";
            return false;
        }
        output.hash_size = static_cast<std::int32_t>(hash_size);
        if (hash_size == 1u)
        {
            output.hash_inline = -1;
        }
        else
        {
            void* hash{};
            if (!runtime.allocate_unreal_storage(
                        static_cast<std::size_t>(hash_size) * sizeof(std::int32_t),
                        alignof(std::int32_t),
                        hash,
                        error))
            {
                release_raw_script_set_allocations(runtime, output);
                return false;
            }
            output.hash_secondary = std::bit_cast<std::uintptr_t>(hash);
            std::fill_n(static_cast<std::int32_t*>(hash), hash_size, -1);
        }
        return true;
    }

    [[nodiscard]] std::int32_t* script_set_hash_buckets(FScriptSetView& set) noexcept
    {
        if (set.hash_size == 1)
        {
            return &set.hash_inline;
        }
        return set.hash_size > 1
                       ? std::bit_cast<std::int32_t*>(static_cast<std::uintptr_t>(set.hash_secondary))
                       : nullptr;
    }

    [[nodiscard]] bool destroy_initialized_set_elements(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ReflectedProperty& element,
            const ue4ss::linux::core::UnrealScriptSetLayout& layout,
            FScriptSetView& set,
            std::size_t initialized,
            std::string& error) noexcept
    {
        bool success = true;
        for (std::size_t index = initialized; index > 0u; --index)
        {
            std::string destroy_error;
            const std::uint64_t address = set.elements.data +
                                          static_cast<std::uint64_t>(index - 1u) *
                                                  static_cast<std::uint64_t>(layout.sparse_stride);
            if (!runtime.destroy_reflected_value(element.address, address, destroy_error))
            {
                success = false;
                if (error.empty()) error = std::move(destroy_error);
            }
        }
        release_raw_script_set_allocations(runtime, set);
        return success;
    }

    [[nodiscard]] bool build_dense_script_set(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ReflectedProperty& element,
            const std::vector<std::shared_ptr<ue4ss::linux::core::UnrealPropertyValue>>& values,
            ue4ss::linux::core::UnrealScriptSetLayout& layout,
            FScriptSetView& output,
            std::string& error,
            std::size_t depth) noexcept
    {
        std::uint32_t element_alignment{};
        if (!runtime.reflected_value_alignment(element.address, element_alignment, error) ||
            !ue4ss::linux::core::calculate_script_set_layout(
                    element.element_size,
                    static_cast<std::int32_t>(element_alignment),
                    layout,
                    error) ||
            !allocate_dense_script_set_storage(runtime, layout, values.size(), output, error))
        {
            return false;
        }

        std::vector<std::uint32_t> hashes;
        hashes.reserve(values.size());
        std::size_t initialized{};
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            const std::uint64_t address = output.elements.data +
                                          static_cast<std::uint64_t>(index) *
                                                  static_cast<std::uint64_t>(layout.sparse_stride);
            if (values[index] == nullptr ||
                !runtime.initialize_reflected_value(element.address, address, error))
            {
                error = error.empty() ? "TSet contains a null element" : error;
                static_cast<void>(destroy_initialized_set_elements(
                        runtime, element, layout, output, initialized, error));
                return false;
            }
            ++initialized;
            if (!encode_plain_value(runtime, element, address, *values[index], error, depth + 1u))
            {
                static_cast<void>(destroy_initialized_set_elements(
                        runtime, element, layout, output, initialized, error));
                return false;
            }
            std::uint32_t hash{};
            if (!runtime.reflected_value_hash(element.address, address, hash, error))
            {
                static_cast<void>(destroy_initialized_set_elements(
                        runtime, element, layout, output, initialized, error));
                return false;
            }
            for (std::size_t previous = 0; previous < hashes.size(); ++previous)
            {
                if (hashes[previous] != hash) continue;
                bool identical{};
                const std::uint64_t previous_address = output.elements.data +
                                                       static_cast<std::uint64_t>(previous) *
                                                               static_cast<std::uint64_t>(layout.sparse_stride);
                if (!runtime.reflected_values_identical(
                            element.address, previous_address, address, identical, error))
                {
                    static_cast<void>(destroy_initialized_set_elements(
                            runtime, element, layout, output, initialized, error));
                    return false;
                }
                if (identical)
                {
                    error = "TSet assignment contains duplicate elements";
                    static_cast<void>(destroy_initialized_set_elements(
                            runtime, element, layout, output, initialized, error));
                    return false;
                }
            }
            hashes.push_back(hash);
        }

        std::int32_t* buckets = script_set_hash_buckets(output);
        if (!values.empty() && buckets == nullptr)
        {
            error = "dense FScriptSet has no hash buckets";
            static_cast<void>(destroy_initialized_set_elements(
                    runtime, element, layout, output, initialized, error));
            return false;
        }
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            const std::int32_t bucket = static_cast<std::int32_t>(
                    hashes[index] & static_cast<std::uint32_t>(output.hash_size - 1));
            std::byte* storage = std::bit_cast<std::byte*>(
                    static_cast<std::uintptr_t>(output.elements.data +
                                                static_cast<std::uint64_t>(index) *
                                                        static_cast<std::uint64_t>(layout.sparse_stride)));
            const std::int32_t next = buckets[bucket];
            const std::int32_t current = static_cast<std::int32_t>(index);
            std::memcpy(storage + layout.hash_next_id_offset, &next, sizeof(next));
            std::memcpy(storage + layout.hash_index_offset, &bucket, sizeof(bucket));
            buckets[bucket] = current;
        }
        return true;
    }

    [[nodiscard]] bool destroy_initialized_map_pairs(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ReflectedProperty& key,
            const ReflectedProperty& value,
            const ue4ss::linux::core::UnrealScriptMapLayout& layout,
            FScriptSetView& map,
            std::size_t initialized,
            std::string& error) noexcept
    {
        bool success = true;
        for (std::size_t index = initialized; index > 0u; --index)
        {
            const std::uint64_t pair_address = map.elements.data +
                                               static_cast<std::uint64_t>(index - 1u) *
                                                       static_cast<std::uint64_t>(layout.set.sparse_stride);
            std::string destroy_error;
            if (!runtime.destroy_reflected_value(
                        value.address,
                        pair_address + static_cast<std::uint64_t>(layout.value_offset),
                        destroy_error))
            {
                success = false;
                if (error.empty()) error = destroy_error;
            }
            destroy_error.clear();
            if (!runtime.destroy_reflected_value(key.address, pair_address, destroy_error))
            {
                success = false;
                if (error.empty()) error = std::move(destroy_error);
            }
        }
        release_raw_script_set_allocations(runtime, map);
        return success;
    }

    [[nodiscard]] bool build_dense_script_map(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ReflectedProperty& key,
            const ReflectedProperty& value,
            const std::vector<ue4ss::linux::core::UnrealMapEntryValue>& entries,
            ue4ss::linux::core::UnrealScriptMapLayout& layout,
            FScriptSetView& output,
            std::string& error,
            std::size_t depth) noexcept
    {
        std::uint32_t key_alignment{};
        std::uint32_t value_alignment{};
        if (!runtime.reflected_value_alignment(key.address, key_alignment, error) ||
            !runtime.reflected_value_alignment(value.address, value_alignment, error) ||
            !ue4ss::linux::core::calculate_script_map_layout(
                    key.element_size,
                    static_cast<std::int32_t>(key_alignment),
                    value.element_size,
                    static_cast<std::int32_t>(value_alignment),
                    layout,
                    error) ||
            layout.value_offset <= 0 || layout.value_offset >= layout.set.sparse_stride ||
            !allocate_dense_script_set_storage(runtime, layout.set, entries.size(), output, error))
        {
            return false;
        }

        std::vector<std::uint32_t> hashes;
        hashes.reserve(entries.size());
        std::size_t initialized{};
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            const auto& entry = entries[index];
            const std::uint64_t pair_address = output.elements.data +
                                               static_cast<std::uint64_t>(index) *
                                                       static_cast<std::uint64_t>(layout.set.sparse_stride);
            const std::uint64_t value_address = pair_address +
                                                static_cast<std::uint64_t>(layout.value_offset);
            if (entry.key == nullptr || entry.value == nullptr)
            {
                error = "TMap assignment contains a null key or value";
                static_cast<void>(destroy_initialized_map_pairs(
                        runtime, key, value, layout, output, initialized, error));
                return false;
            }
            if (!runtime.initialize_reflected_value(key.address, pair_address, error))
            {
                static_cast<void>(destroy_initialized_map_pairs(
                        runtime, key, value, layout, output, initialized, error));
                return false;
            }
            if (!runtime.initialize_reflected_value(value.address, value_address, error))
            {
                std::string destroy_error;
                static_cast<void>(runtime.destroy_reflected_value(key.address, pair_address, destroy_error));
                static_cast<void>(destroy_initialized_map_pairs(
                        runtime, key, value, layout, output, initialized, error));
                return false;
            }
            ++initialized;
            if (!encode_plain_value(runtime, key, pair_address, *entry.key, error, depth + 1u) ||
                !encode_plain_value(runtime, value, value_address, *entry.value, error, depth + 1u))
            {
                static_cast<void>(destroy_initialized_map_pairs(
                        runtime, key, value, layout, output, initialized, error));
                return false;
            }
            std::uint32_t hash{};
            if (!runtime.reflected_value_hash(key.address, pair_address, hash, error))
            {
                static_cast<void>(destroy_initialized_map_pairs(
                        runtime, key, value, layout, output, initialized, error));
                return false;
            }
            for (std::size_t previous = 0; previous < hashes.size(); ++previous)
            {
                if (hashes[previous] != hash) continue;
                bool identical{};
                const std::uint64_t previous_address = output.elements.data +
                                                       static_cast<std::uint64_t>(previous) *
                                                               static_cast<std::uint64_t>(layout.set.sparse_stride);
                if (!runtime.reflected_values_identical(
                            key.address, previous_address, pair_address, identical, error))
                {
                    static_cast<void>(destroy_initialized_map_pairs(
                            runtime, key, value, layout, output, initialized, error));
                    return false;
                }
                if (identical)
                {
                    error = "TMap assignment contains duplicate keys";
                    static_cast<void>(destroy_initialized_map_pairs(
                            runtime, key, value, layout, output, initialized, error));
                    return false;
                }
            }
            hashes.push_back(hash);
        }

        std::int32_t* buckets = script_set_hash_buckets(output);
        if (!entries.empty() && buckets == nullptr)
        {
            error = "dense FScriptMap has no hash buckets";
            static_cast<void>(destroy_initialized_map_pairs(
                    runtime, key, value, layout, output, initialized, error));
            return false;
        }
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            const std::int32_t bucket = static_cast<std::int32_t>(
                    hashes[index] & static_cast<std::uint32_t>(output.hash_size - 1));
            std::byte* storage = std::bit_cast<std::byte*>(
                    static_cast<std::uintptr_t>(output.elements.data +
                                                static_cast<std::uint64_t>(index) *
                                                        static_cast<std::uint64_t>(layout.set.sparse_stride)));
            const std::int32_t next = buckets[bucket];
            const std::int32_t current = static_cast<std::int32_t>(index);
            std::memcpy(storage + layout.set.hash_next_id_offset, &next, sizeof(next));
            std::memcpy(storage + layout.set.hash_index_offset, &bucket, sizeof(bucket));
            buckets[bucket] = current;
        }
        return true;
    }

    [[nodiscard]] bool release_plain_value(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                           const ReflectedProperty& property,
                                           std::uint64_t address,
                                           std::string& error,
                                           std::size_t depth = 0u) noexcept;

    [[nodiscard]] bool release_plain_array_storage(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ReflectedProperty& inner,
            const FScriptArrayView& array,
            std::string& error,
            std::size_t depth) noexcept
    {
        if (depth >= 32u || !validate_script_array_header(array, error))
        {
            error = error.empty() ? "FScriptArray release recursion exceeds the safety limit" : error;
            return false;
        }
        for (std::int32_t index = 0; index < array.num; ++index)
        {
            const std::uint64_t offset = static_cast<std::uint64_t>(index) *
                                         static_cast<std::uint64_t>(inner.element_size);
            if (array.data > std::numeric_limits<std::uint64_t>::max() - offset ||
                !release_plain_value(runtime, inner, array.data + offset, error, depth + 1u))
            {
                error = "FScriptArray element release failed at index " + std::to_string(index) + ": " + error;
                return false;
            }
        }
        runtime.release_unreal_storage(
                std::bit_cast<void*>(static_cast<std::uintptr_t>(array.data)));
        return true;
    }

    [[nodiscard]] bool release_plain_value(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                           const ReflectedProperty& property,
                                           std::uint64_t address,
                                           std::string& error,
                                           std::size_t depth) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        const auto kind = property_kind(property);
        if (depth >= 32u || !kind.has_value())
        {
            error = "plain value release does not support '" + property.type_name + "'";
            return false;
        }
        if (*kind == UnrealPropertyKind::string)
        {
            ue4ss::linux::core::UnrealStringHeader string{};
            std::string decoded;
            if (!safe_read(address, string) || !runtime.read_unreal_string(address, decoded, error))
            {
                return false;
            }
            const ue4ss::linux::core::UnrealStringHeader empty{};
            if (!safe_write_bytes(address, &empty, sizeof(empty)))
            {
                error = "released FString header is not writable";
                return false;
            }
            runtime.release_unreal_string(string);
            return true;
        }
        if (*kind == UnrealPropertyKind::text)
        {
            if (property.element_size != static_cast<std::int32_t>(sizeof(FTextView)) ||
                !runtime.destroy_reflected_value(property.address, address, error))
            {
                error = error.empty() ? "FText lifecycle destruction failed" : error;
                return false;
            }
            const FTextView empty{};
            return safe_write_bytes(address, &empty, sizeof(empty)) ||
                   (error = "destroyed FText storage is not writable", false);
        }
        if (*kind == UnrealPropertyKind::soft_object)
        {
            FSoftObjectPtrView soft{};
            std::string sub_path;
            if (property.element_size != static_cast<std::int32_t>(sizeof(soft)) ||
                !safe_read(address, soft) ||
                !runtime.read_unreal_string(
                        address + offsetof(FSoftObjectPtrView, sub_path_string),
                        sub_path,
                        error))
            {
                error = error.empty() ? "FSoftObjectPtr lifecycle release failed" : error;
                return false;
            }
            const FSoftObjectPtrView empty{};
            if (!safe_write_bytes(address, &empty, sizeof(empty)))
            {
                error = "released FSoftObjectPtr storage is not writable";
                return false;
            }
            runtime.release_unreal_string(soft.sub_path_string);
            return true;
        }
        if (*kind == UnrealPropertyKind::structure)
        {
            std::vector<ReflectedProperty> fields;
            std::string struct_name;
            if (!collect_struct_fields(runtime, property.struct_address, fields, struct_name, error, depth + 1u))
            {
                return false;
            }
            for (const auto& field : fields)
            {
                if (!release_plain_value(runtime,
                                         field,
                                         address + static_cast<std::uint64_t>(field.offset),
                                         error,
                                         depth + 1u))
                {
                    return false;
                }
            }
            return true;
        }
        if (*kind == UnrealPropertyKind::array)
        {
            FScriptArrayView array{};
            ReflectedProperty inner;
            if (!safe_read(address, array) ||
                !decode_nested_property(runtime, property.inner_property_address, inner, error) ||
                !release_plain_array_storage(runtime, inner, array, error, depth + 1u))
            {
                return false;
            }
            const FScriptArrayView empty{};
            return safe_write_bytes(address, &empty, sizeof(empty)) ||
                   (error = "released FScriptArray header is not writable", false);
        }
        if (*kind == UnrealPropertyKind::set || *kind == UnrealPropertyKind::map)
        {
            FScriptSetView container{};
            if (property.element_size != static_cast<std::int32_t>(sizeof(container)) ||
                !safe_read(address, container) || !validate_script_set_view(container, error))
            {
                error = error.empty() ? "FScriptSet/FScriptMap release header is invalid" : error;
                return false;
            }
            if (!runtime.destroy_reflected_value(property.address, address, error))
            {
                return false;
            }
            const FScriptSetView empty{};
            return safe_write_bytes(address, &empty, sizeof(empty)) ||
                   (error = "released FScriptSet/FScriptMap header is not writable", false);
        }
        if (*kind == UnrealPropertyKind::multicast_delegate)
        {
            if (is_sparse_multicast_delegate_property(property.type_name))
            {
                if (!runtime.destroy_reflected_value(property.address, address, error))
                {
                    return false;
                }
                const std::uint8_t unbound{};
                return safe_write_bytes(address, &unbound, sizeof(unbound)) ||
                       (error = "destroyed FSparseDelegate storage is not writable", false);
            }
            FScriptArrayView invocation_list{};
            if (property.element_size != static_cast<std::int32_t>(sizeof(invocation_list)) ||
                !safe_read(address, invocation_list) ||
                !validate_script_array_header(invocation_list, error))
            {
                error = error.empty() ? "FMulticastScriptDelegate release header is invalid" : error;
                return false;
            }
            const FScriptArrayView empty{};
            if (!safe_write_bytes(address, &empty, sizeof(empty)))
            {
                error = "released FMulticastScriptDelegate header is not writable";
                return false;
            }
            runtime.release_unreal_storage(
                    std::bit_cast<void*>(static_cast<std::uintptr_t>(invocation_list.data)));
            return true;
        }
        return true;
    }

    [[nodiscard]] bool encode_plain_value(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                          const ReflectedProperty& property,
                                          std::uint64_t address,
                                          const ue4ss::linux::core::UnrealPropertyValue& value,
                                          std::string& error,
                                          std::size_t depth) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        const auto expected = property_kind(property);
        if (!expected.has_value() || depth >= 32u)
        {
            error = "plain value encoder does not support '" + property.type_name + "'";
            return false;
        }
        if (*expected == UnrealPropertyKind::string)
        {
            if (value.kind != UnrealPropertyKind::string ||
                property.element_size != static_cast<std::int32_t>(sizeof(ue4ss::linux::core::UnrealStringHeader)))
            {
                error = "FString field assignment requires a string or FString";
                return false;
            }
            return runtime.assign_unreal_string(address, value.text, error);
        }
        if (*expected == UnrealPropertyKind::text)
        {
            if (value.kind != UnrealPropertyKind::text ||
                property.element_size != static_cast<std::int32_t>(sizeof(FTextView)))
            {
                error = "TextProperty assignment requires FText userdata";
                return false;
            }
            return runtime.assign_unreal_text(property.address, address, value.text, error);
        }
        if (*expected == UnrealPropertyKind::soft_object)
        {
            if (value.kind != UnrealPropertyKind::soft_object ||
                property.element_size != static_cast<std::int32_t>(sizeof(FSoftObjectPtrView)))
            {
                error = "SoftObjectProperty assignment requires compatible TSoftObjectPtr userdata";
                return false;
            }
            FSoftObjectPtrView previous{};
            std::string previous_sub_path;
            if (!safe_read(address, previous) ||
                !runtime.read_unreal_string(
                        address + offsetof(FSoftObjectPtrView, sub_path_string),
                        previous_sub_path,
                        error))
            {
                error = error.empty() ? "destination FSoftObjectPtr is invalid" : error;
                return false;
            }
            ue4ss::linux::core::RawFName package_name = value.soft_package_name;
            ue4ss::linux::core::RawFName asset_name = value.soft_asset_name;
            std::string package_text;
            std::string asset_text;
            if (!runtime.fname_to_utf8(package_name, package_text, error) ||
                !runtime.fname_to_utf8(asset_name, asset_text, error) ||
                (!value.soft_package_text.empty() && value.soft_package_text != package_text) ||
                (!value.soft_asset_text.empty() && value.soft_asset_text != asset_text))
            {
                error = error.empty() ? "FTopLevelAssetPath component identity validation failed" : error;
                return false;
            }
            FSoftObjectPtrView replacement{};
            replacement.weak_ptr.object_index = -1;
            replacement.tag_at_last_test = static_cast<std::int32_t>(value.signed_integer);
            replacement.asset_path = {
                    .package_name = package_name,
                    .asset_name = asset_name,
            };
            if (!value.object_is_null)
            {
                if (!runtime.is_valid(value.object) || value.object.internal_index < 0 ||
                    value.object.serial_number <= 0)
                {
                    error = "SoftObjectProperty assignment contains a stale weak UObject";
                    return false;
                }
                replacement.weak_ptr = {
                        .object_index = value.object.internal_index,
                        .object_serial_number = value.object.serial_number,
                };
            }
            if (!runtime.assign_unreal_string(
                        std::bit_cast<std::uintptr_t>(&replacement.sub_path_string),
                        value.soft_sub_path,
                        error))
            {
                return false;
            }
            if (!safe_write_bytes(address, &replacement, sizeof(replacement)))
            {
                runtime.release_unreal_string(replacement.sub_path_string);
                error = "destination FSoftObjectPtr storage is not writable";
                return false;
            }
            runtime.release_unreal_string(previous.sub_path_string);
            return true;
        }
        if (*expected == UnrealPropertyKind::structure)
        {
            if (value.kind != UnrealPropertyKind::structure ||
                !validate_plain_struct(runtime, property.struct_address, error))
            {
                error = error.empty() ? "StructProperty assignment requires a compatible Lua table" : error;
                return false;
            }
            std::vector<ReflectedProperty> fields;
            std::string struct_name;
            if (!collect_struct_fields(runtime, property.struct_address, fields, struct_name, error, depth + 1u))
            {
                return false;
            }
            for (const auto& field : fields)
            {
                if (const auto* supplied = find_struct_field(value, field.name); supplied != nullptr &&
                    !encode_plain_value(runtime,
                                        field,
                                        address + static_cast<std::uint64_t>(field.offset),
                                        *supplied,
                                        error,
                                        depth + 1u))
                {
                    return false;
                }
            }
            return true;
        }
        if (*expected == UnrealPropertyKind::array)
        {
            if (value.kind != UnrealPropertyKind::array)
            {
                error = "ArrayProperty assignment requires TArray userdata";
                return false;
            }
            std::unordered_set<std::uint64_t> active;
            if (!validate_plain_property(runtime, property, error, active, depth + 1u))
            {
                return false;
            }
            ReflectedProperty inner;
            if (!decode_nested_property(runtime, property.inner_property_address, inner, error))
            {
                return false;
            }
            FScriptArrayView previous{};
            ue4ss::linux::core::UnrealPropertyValue previous_copy;
            if (!safe_read(address, previous) || !validate_script_array_header(previous, error) ||
                !decode_plain_value(runtime, property, address, previous_copy, error, depth + 1u))
            {
                error = "destination ArrayProperty is invalid: " + error;
                return false;
            }
            if (value.array_elements.size() > static_cast<std::size_t>(k_max_script_array_elements) ||
                (!value.array_elements.empty() &&
                 value.array_elements.size() > std::numeric_limits<std::size_t>::max() /
                                                       static_cast<std::size_t>(inner.element_size)))
            {
                error = "ArrayProperty assignment exceeds the bounded element or byte count";
                return false;
            }

            FScriptArrayView replacement{};
            replacement.num = static_cast<std::int32_t>(value.array_elements.size());
            replacement.max = replacement.num;
            if (replacement.num > 0)
            {
                const std::size_t bytes = value.array_elements.size() *
                                          static_cast<std::size_t>(inner.element_size);
                void* allocation{};
                if (!runtime.allocate_unreal_storage(bytes, plain_property_alignment(runtime, inner), allocation, error))
                {
                    return false;
                }
                replacement.data = std::bit_cast<std::uintptr_t>(allocation);
                std::memset(allocation, 0, bytes);
                std::int32_t initialized{};
                for (; initialized < replacement.num; ++initialized)
                {
                    const auto& element = value.array_elements[static_cast<std::size_t>(initialized)];
                    if (element == nullptr ||
                        !encode_plain_value(runtime,
                                            inner,
                                            replacement.data + static_cast<std::uint64_t>(initialized) *
                                                                       static_cast<std::uint64_t>(inner.element_size),
                                            *element,
                                            error,
                                            depth + 1u))
                    {
                        FScriptArrayView partial = replacement;
                        partial.num = initialized;
                        std::string cleanup_error;
                        static_cast<void>(release_plain_array_storage(
                                runtime, inner, partial, cleanup_error, depth + 1u));
                        error = error.empty() ? "ArrayProperty element conversion failed" : error;
                        return false;
                    }
                }
            }
            if (!safe_write_bytes(address, &replacement, sizeof(replacement)))
            {
                std::string cleanup_error;
                static_cast<void>(release_plain_array_storage(
                        runtime, inner, replacement, cleanup_error, depth + 1u));
                error = "destination ArrayProperty header is not writable";
                return false;
            }
            std::string release_error;
            if (!release_plain_array_storage(runtime, inner, previous, release_error, depth + 1u))
            {
                error = "previous ArrayProperty storage could not be released: " + release_error;
                return false;
            }
            return true;
        }
        if (*expected == UnrealPropertyKind::set)
        {
            if (value.kind != UnrealPropertyKind::set ||
                property.element_size != static_cast<std::int32_t>(sizeof(FScriptSetView)) ||
                (property.flags & k_property_flag_no_destructor) != 0u)
            {
                error = "SetProperty assignment requires compatible TSet userdata and lifecycle metadata";
                return false;
            }
            ReflectedProperty element;
            std::unordered_set<std::uint64_t> active;
            if (!decode_nested_property(runtime, property.inner_property_address, element, error) ||
                !validate_plain_property(runtime, element, error, active, depth + 1u))
            {
                error = "SetProperty Element is unsupported: " + error;
                return false;
            }
            FScriptSetView previous{};
            if (!safe_read(address, previous) || !validate_script_set_view(previous, error))
            {
                error = "destination SetProperty is invalid: " + error;
                return false;
            }
            ue4ss::linux::core::UnrealScriptSetLayout layout;
            FScriptSetView replacement{};
            if (!build_dense_script_set(
                        runtime, element, value.set_elements, layout, replacement, error, depth + 1u))
            {
                return false;
            }
            if (!safe_write_bytes(address, &replacement, sizeof(replacement)))
            {
                static_cast<void>(destroy_initialized_set_elements(
                        runtime, element, layout, replacement, value.set_elements.size(), error));
                error = "destination SetProperty header is not writable";
                return false;
            }
            std::string release_error;
            if (!runtime.destroy_reflected_value(
                        property.address,
                        std::bit_cast<std::uintptr_t>(&previous),
                        release_error))
            {
                error = "SetProperty replacement committed but previous storage could not be released: " +
                        release_error;
                return false;
            }
            return true;
        }
        if (*expected == UnrealPropertyKind::map)
        {
            if (value.kind != UnrealPropertyKind::map ||
                property.element_size != static_cast<std::int32_t>(sizeof(FScriptSetView)) ||
                (property.flags & k_property_flag_no_destructor) != 0u)
            {
                error = "MapProperty assignment requires compatible TMap userdata and lifecycle metadata";
                return false;
            }
            ReflectedProperty key;
            ReflectedProperty mapped;
            std::unordered_set<std::uint64_t> active;
            if (!decode_nested_property(runtime, property.key_property_address, key, error, true) ||
                !decode_nested_property(runtime, property.value_property_address, mapped, error, true) ||
                !validate_plain_property(runtime, key, error, active, depth + 1u) ||
                !validate_plain_property(runtime, mapped, error, active, depth + 1u))
            {
                error = "MapProperty key/value is unsupported: " + error;
                return false;
            }
            FScriptSetView previous{};
            if (!safe_read(address, previous) || !validate_script_set_view(previous, error))
            {
                error = "destination MapProperty is invalid: " + error;
                return false;
            }
            ue4ss::linux::core::UnrealScriptMapLayout layout;
            FScriptSetView replacement{};
            if (!build_dense_script_map(
                        runtime, key, mapped, value.map_entries, layout, replacement, error, depth + 1u))
            {
                return false;
            }
            if (!safe_write_bytes(address, &replacement, sizeof(replacement)))
            {
                static_cast<void>(destroy_initialized_map_pairs(
                        runtime, key, mapped, layout, replacement, value.map_entries.size(), error));
                error = "destination MapProperty header is not writable";
                return false;
            }
            std::string release_error;
            if (!runtime.destroy_reflected_value(
                        property.address,
                        std::bit_cast<std::uintptr_t>(&previous),
                        release_error))
            {
                error = "MapProperty replacement committed but previous storage could not be released: " +
                        release_error;
                return false;
            }
            return true;
        }
        if (*expected == UnrealPropertyKind::boolean && value.kind == UnrealPropertyKind::boolean)
        {
            std::uint8_t byte{};
            const std::uint64_t byte_address = address + property.boolean.byte_offset;
            if (!safe_read(byte_address, byte))
            {
                error = "plain BoolProperty storage is unreadable";
                return false;
            }
            byte = static_cast<std::uint8_t>((byte & ~property.boolean.field_mask) |
                                             (value.boolean ? property.boolean.byte_mask : 0u));
            return safe_write_bytes(byte_address, &byte, sizeof(byte)) ||
                   (error = "plain BoolProperty storage is not writable", false);
        }
        if (*expected == UnrealPropertyKind::signed_integer && value.kind == UnrealPropertyKind::signed_integer)
        {
            if ((property.element_size != 1 && property.element_size != 2 && property.element_size != 4 &&
                 property.element_size != 8) ||
                (property.element_size < 8 &&
                 (value.signed_integer < -(std::int64_t{1} << (property.element_size * 8 - 1)) ||
                  value.signed_integer > (std::int64_t{1} << (property.element_size * 8 - 1)) - 1)))
            {
                error = "plain signed integer value does not fit '" + property.name + "'";
                return false;
            }
            const std::uint64_t raw = static_cast<std::uint64_t>(value.signed_integer);
            return safe_write_bytes(address, &raw, static_cast<std::size_t>(property.element_size)) ||
                   (error = "plain signed integer storage is not writable", false);
        }
        if (*expected == UnrealPropertyKind::unsigned_integer &&
            (value.kind == UnrealPropertyKind::unsigned_integer ||
             (value.kind == UnrealPropertyKind::signed_integer && value.signed_integer >= 0)))
        {
            const std::uint64_t raw = value.kind == UnrealPropertyKind::unsigned_integer
                                              ? value.unsigned_integer
                                              : static_cast<std::uint64_t>(value.signed_integer);
            if ((property.element_size != 1 && property.element_size != 2 && property.element_size != 4 &&
                 property.element_size != 8) ||
                (property.element_size < 8 && raw >= (std::uint64_t{1} << (property.element_size * 8))))
            {
                error = "plain unsigned integer value does not fit '" + property.name + "'";
                return false;
            }
            return safe_write_bytes(address, &raw, static_cast<std::size_t>(property.element_size)) ||
                   (error = "plain unsigned integer storage is not writable", false);
        }
        if (*expected == UnrealPropertyKind::floating_point &&
            (value.kind == UnrealPropertyKind::floating_point || value.kind == UnrealPropertyKind::signed_integer))
        {
            const double converted = value.kind == UnrealPropertyKind::floating_point
                                             ? value.floating_point
                                             : static_cast<double>(value.signed_integer);
            if (property.type_name == "FloatProperty" && property.element_size == sizeof(float))
            {
                const float narrowed = static_cast<float>(converted);
                return safe_write_bytes(address, &narrowed, sizeof(narrowed)) ||
                       (error = "plain float storage is not writable", false);
            }
            return property.type_name == "DoubleProperty" && property.element_size == sizeof(double) &&
                           safe_write_bytes(address, &converted, sizeof(converted)) ||
                   (error = "plain double storage is not writable", false);
        }
        if (*expected == UnrealPropertyKind::name && value.kind == UnrealPropertyKind::name)
        {
            return safe_write_bytes(address, &value.name, sizeof(value.name)) ||
                   (error = "plain FName storage is not writable", false);
        }
        if (*expected == UnrealPropertyKind::delegate && value.kind == UnrealPropertyKind::delegate)
        {
            if (property.element_size != static_cast<std::int32_t>(sizeof(FScriptDelegateView)))
            {
                error = "FScriptDelegate storage has an incompatible element size";
                return false;
            }
            FScriptDelegateView replacement{};
            if (!value.object_is_null)
            {
                std::string function_name;
                if (!runtime.is_valid(value.object) ||
                    value.object.internal_index < 0 || value.object.serial_number <= 0 ||
                    (value.name.comparison_index == 0u && value.name.number == 0u) ||
                    !runtime.fname_to_utf8(value.name, function_name, error))
                {
                    error = error.empty()
                                    ? "DelegateProperty assignment requires a live UObject and non-None FName"
                                    : error;
                    return false;
                }
                replacement.object = {
                        .object_index = value.object.internal_index,
                        .object_serial_number = value.object.serial_number,
                };
                replacement.function_name = value.name;
            }
            return safe_write_bytes(address, &replacement, sizeof(replacement)) ||
                   (error = "FScriptDelegate storage is not writable", false);
        }
        if (*expected == UnrealPropertyKind::object && value.kind == UnrealPropertyKind::object)
        {
            if (!value.object_is_null && !runtime.is_valid(value.object))
            {
                error = "plain UObject assignment is stale";
                return false;
            }
            if (property.type_name == "InterfaceProperty")
            {
                if (property.element_size != static_cast<std::int32_t>(sizeof(FScriptInterfaceView)) ||
                    property.interface_class_address == 0u)
                {
                    error = "FInterfaceProperty assignment has incompatible metadata";
                    return false;
                }
                FScriptInterfaceView replacement{};
                if (!value.object_is_null)
                {
                    replacement.object = value.object.address;
                    if (!resolve_script_interface_pointer(runtime,
                                                          value.object,
                                                          property.interface_class_address,
                                                          replacement.interface_pointer,
                                                          error))
                    {
                        return false;
                    }
                }
                FScriptInterfaceView previous{};
                if (!safe_read(address, previous) ||
                    !safe_write_bytes(address, &replacement, sizeof(replacement)))
                {
                    error = "FScriptInterface destination is not readable and writable";
                    return false;
                }
                ue4ss::linux::core::UnrealPropertyValue validated;
                std::string validation_error;
                if (!decode_plain_value(runtime, property, address, validated, validation_error) ||
                    validated.object_is_null != value.object_is_null ||
                    (!value.object_is_null && validated.object.address != value.object.address))
                {
                    static_cast<void>(safe_write_bytes(address, &previous, sizeof(previous)));
                    error = validation_error.empty()
                                    ? "FScriptInterface failed post-write validation and was rolled back"
                                    : validation_error;
                    return false;
                }
                return true;
            }
            if (!is_direct_object_property(property.type_name) ||
                property.element_size != static_cast<std::int32_t>(sizeof(void*)))
            {
                error = "plain UObject assignment has incompatible FProperty metadata";
                return false;
            }
            const std::uint64_t object_address = value.object_is_null ? 0u : value.object.address;
            return safe_write_bytes(address, &object_address, sizeof(object_address)) ||
                   (error = "plain UObject storage is not writable", false);
        }
        error = "Lua value does not match plain field '" + property.name + "' of type '" + property.type_name + "'";
        return false;
    }

    [[nodiscard]] bool marshal_parameter(const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
                                         const ReflectedProperty& property,
                                         const ue4ss::linux::core::UnrealPropertyValue& value,
                                         std::vector<std::byte>& params,
                                         std::string& error) noexcept
    {
        if (property.offset < 0 || static_cast<std::size_t>(property.offset) > params.size() ||
            params.size() - static_cast<std::size_t>(property.offset) <
                    static_cast<std::size_t>(property.element_size))
        {
            error = "parameter '" + property.name + "' falls outside the allocated UFunction buffer";
            return false;
        }
        return encode_plain_value(runtime,
                                  property,
                                  std::bit_cast<std::uintptr_t>(params.data() + property.offset),
                                  value,
                                  error);
    }

    template <std::size_t Size>
    [[nodiscard]] bool contains_bytes(const std::array<std::uint8_t, Size>& haystack,
                                      std::initializer_list<std::uint8_t> needle) noexcept
    {
        return needle.size() != 0u && std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end()) != haystack.end();
    }

    [[nodiscard]] bool is_executable_mapping(std::uint64_t address) noexcept;

    [[nodiscard]] bool validate_process_event_code(std::uint64_t address, std::string& error) noexcept
    {
        std::array<std::uint8_t, 0x400> code{};
        if (!is_executable_mapping(address) || !safe_read_bytes(address, code.data(), code.size()))
        {
            error = "ProcessEvent candidate is not a readable executable mapping";
            return false;
        }

        // These independent instruction fragments prove the UObject and
        // UFunction roles of the SysV arguments and the UE 5.1 layout. They
        // intentionally avoid accepting a vtable offset by itself.
        const bool object_flags_gate = contains_bytes(code, {0xf6, 0x47, 0x0b, 0x60});
        const bool function_flags_gate = contains_bytes(code, {0xf6, 0x86, 0xb1, 0x00, 0x00, 0x00, 0x04});
        const bool callspace_gate = contains_bytes(code, {0xff, 0x90, 0x70, 0x02, 0x00, 0x00});
        const bool remote_call_gate = contains_bytes(code, {0xff, 0x90, 0x78, 0x02, 0x00, 0x00});
        const bool event_graph_gate = contains_bytes(code, {0x4c, 0x8b, 0xa3, 0xc8, 0x00, 0x00, 0x00}) &&
                                      contains_bytes(code, {0x8b, 0x83, 0xd0, 0x00, 0x00, 0x00});
        const bool parms_size_gate = contains_bytes(code, {0x41, 0x0f, 0xb7, 0x84, 0x24, 0xb6, 0x00, 0x00, 0x00}) &&
                                     contains_bytes(code, {0x41, 0x0f, 0xb7, 0x94, 0x24, 0xb6, 0x00, 0x00, 0x00});
        if (!object_flags_gate || !function_flags_gate || !callspace_gate || !remote_call_gate ||
            !event_graph_gate || !parms_size_gate)
        {
            error = "UE 5.1 ProcessEvent candidate failed structural instruction validation";
            return false;
        }
        return true;
    }

    [[nodiscard]] bool is_executable_mapping(std::uint64_t address) noexcept
    {
        try
        {
            std::ifstream maps{"/proc/self/maps"};
            std::string line;
            while (std::getline(maps, line))
            {
                unsigned long long start{};
                unsigned long long end{};
                std::array<char, 5> permissions{};
                if (std::sscanf(line.c_str(), "%llx-%llx %4s", &start, &end, permissions.data()) == 3 &&
                    address >= start && address < end)
                {
                    return permissions[0] == 'r' && permissions[2] == 'x';
                }
            }
        }
        catch (...)
        {
        }
        return false;
    }

    [[nodiscard]] bool read_registry(std::uint64_t address, ChunkedObjectArrayView& view) noexcept
    {
        if (address == 0u || !safe_read(address + k_obj_objects_offset, view) || view.objects == 0u ||
            view.num_elements <= 0 || view.num_elements > view.max_elements || view.max_elements > k_reasonable_element_limit ||
            view.num_chunks <= 0 || view.num_chunks > view.max_chunks)
        {
            return false;
        }
        const std::int32_t required_chunks = (view.num_elements + k_elements_per_chunk - 1) / k_elements_per_chunk;
        return view.num_chunks >= required_chunks;
    }

    [[nodiscard]] bool read_item(const ChunkedObjectArrayView& registry,
                                 std::int32_t index,
                                 ObjectItemView& item) noexcept
    {
        if (index < 0 || index >= registry.num_elements)
        {
            return false;
        }
        const std::int32_t chunk_index = index / k_elements_per_chunk;
        const std::int32_t within_chunk = index % k_elements_per_chunk;
        std::uint64_t chunk{};
        if (!safe_read(registry.objects + static_cast<std::uint64_t>(chunk_index) * sizeof(std::uint64_t), chunk) || chunk == 0u)
        {
            return false;
        }
        return safe_read(chunk + static_cast<std::uint64_t>(within_chunk) * sizeof(ObjectItemView), item);
    }

    [[nodiscard]] bool read_stable_object(const ChunkedObjectArrayView& registry,
                                          std::int32_t index,
                                          ObjectItemView& item,
                                          UObjectBaseView& object) noexcept
    {
        ObjectItemView after{};
        if (!read_item(registry, index, item) || item.object == 0u || !safe_read(item.object, object) ||
            object.internal_index != index || object.vtable == 0u || object.class_private == 0u ||
            !read_item(registry, index, after))
        {
            return false;
        }
        return after.object == item.object && after.serial_number == item.serial_number;
    }

    [[nodiscard]] std::uint64_t fname_key(ue4ss::linux::core::RawFName name) noexcept
    {
        return static_cast<std::uint64_t>(name.comparison_index) | (static_cast<std::uint64_t>(name.number) << 32u);
    }

    enum class InlineMulticastMutation
    {
        add,
        remove,
        clear,
    };

    [[nodiscard]] bool mutate_sparse_multicast_delegate(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::ReadOnlyUObjectHandle& owner,
            const ReflectedProperty& property,
            std::uint64_t value_address,
            InlineMulticastMutation operation,
            const ue4ss::linux::core::ReadOnlyUObjectHandle* target,
            ue4ss::linux::core::RawFName function_name,
            std::string& error) noexcept
    {
        using AddDelegateFunction = void (*)(const void*, FScriptDelegateView, void*, void*);
        using RemoveDelegateFunction = void (*)(const void*, const FScriptDelegateView*, void*, void*);
        using ClearDelegateFunction = void (*)(const void*, void*, void*);
        if (!is_sparse_multicast_delegate_property(property.type_name) || property.element_size != 1)
        {
            error = "property is not a UE 5.1 FSparseDelegate";
            return false;
        }
        ue4ss::linux::core::UnrealPropertyValue before;
        if (!decode_plain_value(runtime, property, value_address, before, error) ||
            !before.multicast_sparse)
        {
            error = error.empty() ? "sparse multicast delegate ABI gate failed" : error;
            return false;
        }

        FScriptDelegateView requested{};
        if (operation != InlineMulticastMutation::clear)
        {
            std::string decoded_function;
            ue4ss::linux::core::ReadOnlyUObjectHandle reflected_function;
            if (target == nullptr || !runtime.is_valid(*target) || target->internal_index < 0 ||
                target->serial_number <= 0 ||
                (function_name.comparison_index == 0u && function_name.number == 0u) ||
                !runtime.fname_to_utf8(function_name, decoded_function, error) ||
                !runtime.find_function(*target, decoded_function, reflected_function, error))
            {
                error = error.empty()
                                ? "sparse multicast binding requires a live target and existing non-None UFunction"
                                : error;
                return false;
            }
            requested = {
                    .object = {
                            .object_index = target->internal_index,
                            .object_serial_number = target->serial_number,
                    },
                    .function_name = function_name,
            };
        }

        const void* property_pointer =
                std::bit_cast<const void*>(static_cast<std::uintptr_t>(property.address));
        void* owner_pointer = std::bit_cast<void*>(static_cast<std::uintptr_t>(owner.address));
        void* value_pointer = std::bit_cast<void*>(static_cast<std::uintptr_t>(value_address));
        switch (operation)
        {
        case InlineMulticastMutation::add:
        {
            AddDelegateFunction add{};
            if (!load_fproperty_virtual(
                        property.address, k_multicast_add_delegate_slot, add, error))
            {
                return false;
            }
            add(property_pointer, requested, owner_pointer, value_pointer);
            break;
        }
        case InlineMulticastMutation::remove:
        {
            RemoveDelegateFunction remove{};
            if (!load_fproperty_virtual(
                        property.address, k_multicast_remove_delegate_slot, remove, error))
            {
                return false;
            }
            remove(property_pointer, &requested, owner_pointer, value_pointer);
            break;
        }
        case InlineMulticastMutation::clear:
        {
            ClearDelegateFunction clear{};
            if (!load_fproperty_virtual(
                        property.address, k_multicast_clear_delegate_slot, clear, error))
            {
                return false;
            }
            clear(property_pointer, owner_pointer, value_pointer);
            break;
        }
        }

        ue4ss::linux::core::UnrealPropertyValue after;
        if (!decode_plain_value(runtime, property, value_address, after, error) ||
            !after.multicast_sparse)
        {
            error = error.empty() ? "sparse multicast delegate failed post-mutation validation" : error;
            return false;
        }
        const auto same_binding = [&requested](const auto& binding) {
            return binding != nullptr &&
                   binding->kind == ue4ss::linux::core::UnrealPropertyKind::delegate &&
                   !binding->object_is_null &&
                   binding->object.internal_index == requested.object.object_index &&
                   binding->object.serial_number == requested.object.object_serial_number &&
                   binding->name.comparison_index == requested.function_name.comparison_index &&
                   binding->name.number == requested.function_name.number;
        };
        const bool contains = std::ranges::any_of(after.array_elements, same_binding);
        if ((operation == InlineMulticastMutation::add && !contains) ||
            (operation == InlineMulticastMutation::remove && contains) ||
            (operation == InlineMulticastMutation::clear && !after.array_elements.empty()))
        {
            error = "sparse multicast delegate virtual mutation did not produce the requested binding state";
            return false;
        }
        return true;
    }

    [[nodiscard]] bool mutate_inline_multicast_delegate(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::ReadOnlyUObjectHandle& owner,
            std::string_view property_name,
            InlineMulticastMutation operation,
            const ue4ss::linux::core::ReadOnlyUObjectHandle* target,
            ue4ss::linux::core::RawFName function_name,
            const ue4ss::linux::core::GameThreadScheduler& scheduler,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "multicast delegate mutation is permitted only inside a validated game-thread callback";
                return false;
            }
            if (!runtime.is_valid(owner))
            {
                error = "a live owner UObject is required for multicast delegate mutation";
                return false;
            }
            UObjectBaseView owner_object{};
            if (!safe_read(owner.address, owner_object))
            {
                error = "multicast delegate owner became unreadable";
                return false;
            }
            ReflectedProperty property;
            const auto decode_name = [&runtime](ue4ss::linux::core::RawFName name,
                                                std::string& decoded,
                                                std::string& decode_error) {
                return runtime.fname_to_utf8(name, decoded, decode_error);
            };
            if (!find_reflected_property(
                        owner_object.class_private, property_name, decode_name, property, error))
            {
                return false;
            }
            if (is_sparse_multicast_delegate_property(property.type_name))
            {
                if (property.offset < 0 ||
                    owner.address > std::numeric_limits<std::uint64_t>::max() -
                                            static_cast<std::uint64_t>(property.offset))
                {
                    error = "sparse multicast delegate property address overflows";
                    return false;
                }
                return mutate_sparse_multicast_delegate(
                        runtime,
                        owner,
                        property,
                        owner.address + static_cast<std::uint64_t>(property.offset),
                        operation,
                        target,
                        function_name,
                        error);
            }
            if (!is_inline_multicast_delegate_property(property.type_name) ||
                property.element_size != static_cast<std::int32_t>(sizeof(FScriptArrayView)))
            {
                error = "property '" + std::string{property_name} +
                        "' is not an inline FMulticastScriptDelegate; sparse delegates require a separate ABI gate";
                return false;
            }
            if (property.offset < 0 ||
                owner.address > std::numeric_limits<std::uint64_t>::max() -
                                        static_cast<std::uint64_t>(property.offset))
            {
                error = "multicast delegate property address overflows";
                return false;
            }
            const std::uint64_t value_address =
                    owner.address + static_cast<std::uint64_t>(property.offset);
            FScriptArrayView previous{};
            ue4ss::linux::core::UnrealPropertyValue validated;
            if (!safe_read(value_address, previous) ||
                !validate_script_array_header(previous, error) ||
                !decode_plain_value(runtime, property, value_address, validated, error))
            {
                error = error.empty() ? "inline multicast delegate storage is invalid" : error;
                return false;
            }

            std::vector<FScriptDelegateView> bindings(static_cast<std::size_t>(previous.num));
            if (!bindings.empty() &&
                !safe_read_bytes(previous.data,
                                 bindings.data(),
                                 bindings.size() * sizeof(FScriptDelegateView)))
            {
                error = "inline multicast delegate invocation list became unreadable";
                return false;
            }

            FScriptDelegateView requested{};
            if (operation != InlineMulticastMutation::clear)
            {
                std::string decoded_function;
                ue4ss::linux::core::ReadOnlyUObjectHandle reflected_function;
                if (target == nullptr || !runtime.is_valid(*target) || target->internal_index < 0 ||
                    target->serial_number <= 0 ||
                    (function_name.comparison_index == 0u && function_name.number == 0u) ||
                    !runtime.fname_to_utf8(function_name, decoded_function, error) ||
                    !runtime.find_function(*target, decoded_function, reflected_function, error))
                {
                    error = error.empty()
                                    ? "multicast binding requires a live target and an existing non-None UFunction"
                                    : error;
                    return false;
                }
                requested = {
                        .object = {
                                .object_index = target->internal_index,
                                .object_serial_number = target->serial_number,
                        },
                        .function_name = function_name,
                };
            }
            const auto same_binding = [&requested](const FScriptDelegateView& binding) {
                return binding.object.object_index == requested.object.object_index &&
                       binding.object.object_serial_number == requested.object.object_serial_number &&
                       binding.function_name.comparison_index == requested.function_name.comparison_index &&
                       binding.function_name.number == requested.function_name.number;
            };
            switch (operation)
            {
            case InlineMulticastMutation::add:
                if (std::none_of(bindings.begin(), bindings.end(), same_binding))
                {
                    bindings.push_back(requested);
                }
                break;
            case InlineMulticastMutation::remove:
                std::erase_if(bindings, same_binding);
                break;
            case InlineMulticastMutation::clear:
                bindings.clear();
                break;
            }

            if (bindings.size() == static_cast<std::size_t>(previous.num))
            {
                bool unchanged = true;
                for (std::size_t index = 0; index < bindings.size(); ++index)
                {
                    FScriptDelegateView existing{};
                    if (!safe_read(previous.data + index * sizeof(FScriptDelegateView), existing) ||
                        std::memcmp(&bindings[index], &existing, sizeof(existing)) != 0)
                    {
                        unchanged = false;
                        break;
                    }
                }
                if (unchanged)
                {
                    return true;
                }
            }

            FScriptArrayView replacement{};
            replacement.num = static_cast<std::int32_t>(bindings.size());
            replacement.max = replacement.num;
            void* allocation{};
            if (!bindings.empty())
            {
                if (!runtime.allocate_unreal_storage(bindings.size() * sizeof(FScriptDelegateView),
                                                     alignof(FScriptDelegateView),
                                                     allocation,
                                                     error))
                {
                    return false;
                }
                std::memcpy(allocation,
                            bindings.data(),
                            bindings.size() * sizeof(FScriptDelegateView));
                replacement.data = std::bit_cast<std::uintptr_t>(allocation);
            }
            if (!safe_write_bytes(value_address, &replacement, sizeof(replacement)))
            {
                runtime.release_unreal_storage(allocation);
                error = "inline multicast delegate header is not writable";
                return false;
            }
            runtime.release_unreal_storage(
                    std::bit_cast<void*>(static_cast<std::uintptr_t>(previous.data)));
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while mutating an inline multicast delegate";
            return false;
        }
    }

    struct DataTableMapRow
    {
        ue4ss::linux::core::RawFName name{};
        std::uint64_t storage{};
    };

    [[nodiscard]] std::uint32_t hash_fname(ue4ss::linux::core::RawFName name) noexcept
    {
        // UE 5.1 FNameEntryId uses 16 offset bits and 13 block bits. This is
        // the engine's GetTypeHash(FNameEntryHandle) formula from NameTypes.
        constexpr std::uint32_t block_offset_bits = 16u;
        constexpr std::uint32_t max_block_bits = 13u;
        const std::uint32_t block = name.comparison_index >> block_offset_bits;
        const std::uint32_t offset = name.comparison_index & ((1u << block_offset_bits) - 1u);
        return (block << (32u - max_block_bits)) + block +
               (offset << block_offset_bits) + offset + (offset >> 4u) + name.number;
    }

    [[nodiscard]] bool build_dense_data_table_map(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const std::vector<DataTableMapRow>& rows,
            ue4ss::linux::core::UnrealScriptMapLayout& layout,
            FScriptSetView& output,
            std::string& error) noexcept
    {
        if (!ue4ss::linux::core::calculate_script_map_layout(
                    sizeof(ue4ss::linux::core::RawFName),
                    alignof(ue4ss::linux::core::RawFName),
                    sizeof(std::uint64_t),
                    alignof(std::uint64_t),
                    layout,
                    error) ||
            layout.value_offset <= 0 || layout.value_offset >= layout.set.sparse_stride ||
            !allocate_dense_script_set_storage(runtime, layout.set, rows.size(), output, error))
        {
            return false;
        }

        std::int32_t* buckets = script_set_hash_buckets(output);
        if (!rows.empty() && buckets == nullptr)
        {
            release_raw_script_set_allocations(runtime, output);
            error = "dense UDataTable RowMap has no hash buckets";
            return false;
        }
        std::unordered_set<std::uint64_t> unique_names;
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            const auto& row = rows[index];
            const std::uint64_t identity =
                    (static_cast<std::uint64_t>(row.name.number) << 32u) |
                    row.name.comparison_index;
            if (row.name.comparison_index == 0u || row.storage == 0u ||
                !unique_names.emplace(identity).second)
            {
                release_raw_script_set_allocations(runtime, output);
                error = "UDataTable replacement contains a null or duplicate row";
                return false;
            }
            const std::uint64_t pair_address =
                    output.elements.data + static_cast<std::uint64_t>(index) *
                                                   static_cast<std::uint64_t>(layout.set.sparse_stride);
            if (!safe_write_bytes(pair_address, &row.name, sizeof(row.name)) ||
                !safe_write_bytes(pair_address + static_cast<std::uint64_t>(layout.value_offset),
                                  &row.storage,
                                  sizeof(row.storage)))
            {
                release_raw_script_set_allocations(runtime, output);
                error = "UDataTable replacement RowMap storage is not writable";
                return false;
            }
            const std::uint32_t hash = hash_fname(row.name);
            const std::int32_t bucket = static_cast<std::int32_t>(
                    hash & static_cast<std::uint32_t>(output.hash_size - 1));
            std::byte* storage = std::bit_cast<std::byte*>(
                    static_cast<std::uintptr_t>(pair_address));
            const std::int32_t next = buckets[bucket];
            const std::int32_t current = static_cast<std::int32_t>(index);
            std::memcpy(storage + layout.set.hash_next_id_offset, &next, sizeof(next));
            std::memcpy(storage + layout.set.hash_index_offset, &bucket, sizeof(bucket));
            buckets[bucket] = current;
        }
        return validate_script_set_view(output, error);
    }

    [[nodiscard]] bool collect_data_table_row_fields(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const ue4ss::linux::core::ReadOnlyUObjectHandle& row_struct,
            std::vector<ReflectedProperty>& fields,
            std::int32_t& properties_size,
            std::string& error) noexcept
    {
        fields.clear();
        properties_size = 0;
        std::string struct_name;
        if (!runtime.is_valid(row_struct) ||
            !safe_read(row_struct.address + k_properties_size_offset, properties_size) ||
            properties_size <= 0 || properties_size > k_max_properties_size ||
            !collect_struct_fields(runtime, row_struct.address, fields, struct_name, error))
        {
            error = error.empty() ? "UDataTable RowStruct layout is invalid" : error;
            return false;
        }
        std::unordered_set<std::uint64_t> active;
        for (const auto& field : fields)
        {
            if (field.offset < 0 || field.element_size <= 0 ||
                field.offset > properties_size - field.element_size ||
                !validate_plain_property(runtime, field, error, active, 0u))
            {
                error = error.empty()
                                ? "UDataTable RowStruct contains an unsupported field layout"
                                : "UDataTable RowStruct field '" + field.name + "': " + error;
                return false;
            }
        }
        return true;
    }

    void destroy_data_table_row_storage(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const std::vector<ReflectedProperty>& fields,
            std::uint64_t storage,
            std::string& error) noexcept
    {
        for (auto field = fields.rbegin(); field != fields.rend(); ++field)
        {
            std::string destroy_error;
            if (!runtime.destroy_reflected_value(
                        field->address,
                        storage + static_cast<std::uint64_t>(field->offset),
                        destroy_error) &&
                error.empty())
            {
                error = "UDataTable row field '" + field->name +
                        "' destruction failed: " + destroy_error;
            }
        }
        runtime.release_unreal_storage(
                std::bit_cast<void*>(static_cast<std::uintptr_t>(storage)));
    }

    [[nodiscard]] bool allocate_data_table_row_storage(
            const ue4ss::linux::core::ReadOnlyUnrealRuntime& runtime,
            const std::vector<ReflectedProperty>& fields,
            std::int32_t properties_size,
            const ue4ss::linux::core::UnrealPropertyValue& value,
            std::uint64_t& output,
            std::string& error) noexcept
    {
        using ue4ss::linux::core::UnrealPropertyKind;
        output = 0u;
        if (value.kind != UnrealPropertyKind::structure)
        {
            error = "UDataTable:AddRow requires a struct table or UScriptStruct";
            return false;
        }
        std::uint32_t alignment = 1u;
        for (const auto& field : fields)
        {
            std::uint32_t field_alignment{};
            if (!runtime.reflected_value_alignment(field.address, field_alignment, error))
            {
                return false;
            }
            alignment = std::max(alignment, field_alignment);
        }
        void* allocation{};
        if (!runtime.allocate_unreal_storage(
                    static_cast<std::size_t>(properties_size), alignment, allocation, error))
        {
            return false;
        }
        output = std::bit_cast<std::uintptr_t>(allocation);
        std::memset(allocation, 0, static_cast<std::size_t>(properties_size));

        std::size_t initialized{};
        for (; initialized < fields.size(); ++initialized)
        {
            const auto& field = fields[initialized];
            if (!runtime.initialize_reflected_value(
                        field.address,
                        output + static_cast<std::uint64_t>(field.offset),
                        error))
            {
                break;
            }
        }
        if (initialized != fields.size())
        {
            std::vector<ReflectedProperty> initialized_fields{
                    fields.begin(), fields.begin() + static_cast<std::ptrdiff_t>(initialized)};
            destroy_data_table_row_storage(runtime, initialized_fields, output, error);
            output = 0u;
            return false;
        }

        for (const auto& supplied : value.struct_fields)
        {
            const auto found = std::find_if(fields.begin(), fields.end(), [&](const auto& field) {
                return field.name == supplied.name;
            });
            if (found == fields.end() || supplied.value == nullptr ||
                !encode_plain_value(runtime,
                                    *found,
                                    output + static_cast<std::uint64_t>(found->offset),
                                    *supplied.value,
                                    error))
            {
                if (error.empty())
                {
                    error = "UDataTable:AddRow contains unknown field '" + supplied.name + "'";
                }
                std::string cleanup_error;
                destroy_data_table_row_storage(runtime, fields, output, cleanup_error);
                output = 0u;
                return false;
            }
        }
        return true;
    }
} // namespace

namespace ue4ss::linux::core
{
    bool ReadOnlyUnrealRuntime::initialize(const ResolvedUnrealState& resolved, std::string& detail) noexcept
    {
        m_fname_to_string = nullptr;
        m_fname_constructor = nullptr;
        m_realloc = nullptr;
        m_free = nullptr;
        m_allocator = nullptr;
        m_guobject_array = 0u;
        m_static_construct_object = nullptr;
        m_process_event = nullptr;
        m_fproperty_abi_ready = false;
        m_fproperty_import_text_ready = false;
        m_uenum_names_offset = 0u;

        constexpr std::uint64_t required = UE4SS_PS_GUOBJECT_ARRAY | UE4SS_PS_FNAME_TO_STRING |
                                           UE4SS_PS_FNAME_CTOR | UE4SS_PS_GMALLOC | UE4SS_PS_ENGINE_VERSION;
        if ((resolved.available_mask & required) != required || resolved.engine_major != 5u || resolved.engine_minor != 1u)
        {
            detail = "read-only FName ABI is approved only for resolved Unreal 5.1";
            return false;
        }
        if (!is_executable_mapping(resolved.fname_to_string) || !is_executable_mapping(resolved.fname_ctor))
        {
            detail = "FName::ToString or the FName constructor is not inside a readable executable mapping";
            return false;
        }

        std::uint64_t allocator{};
        std::uint64_t allocator_vtable{};
        std::uint64_t realloc_function{};
        std::uint64_t free_function{};
        if (!safe_read(resolved.gmalloc, allocator) || allocator == 0u)
        {
            detail = "GMalloc is not initialized or its global pointer is unreadable";
            return false;
        }
        if (!safe_read(allocator, allocator_vtable) || allocator_vtable == 0u)
        {
            detail = "the resolved FMalloc object has no readable vtable";
            return false;
        }
        if (!safe_read(allocator_vtable + k_fmalloc_realloc_vtable_offset, realloc_function) ||
            realloc_function == 0u || !is_executable_mapping(realloc_function))
        {
            detail = "FMalloc::Realloc at the UE 5.1 Linux vtable offset is not executable";
            return false;
        }
        if (!safe_read(allocator_vtable + k_fmalloc_free_vtable_offset, free_function) || free_function == 0u ||
            !is_executable_mapping(free_function))
        {
            detail = "FMalloc::Free at the UE 5.1 Linux vtable offset is not executable";
            return false;
        }

        m_fname_to_string = std::bit_cast<FNameToStringFunction>(static_cast<std::uintptr_t>(resolved.fname_to_string));
        m_fname_constructor = std::bit_cast<FNameConstructorFunction>(static_cast<std::uintptr_t>(resolved.fname_ctor));
        m_realloc = std::bit_cast<ReallocFunction>(static_cast<std::uintptr_t>(realloc_function));
        m_free = std::bit_cast<FreeFunction>(static_cast<std::uintptr_t>(free_function));
        m_allocator = std::bit_cast<void*>(static_cast<std::uintptr_t>(allocator));
        m_guobject_array = resolved.guobject_array;
        if ((resolved.available_mask & UE4SS_PS_STATIC_CONSTRUCT_OBJECT) != 0u &&
            is_executable_mapping(resolved.static_construct_object))
        {
            m_static_construct_object = std::bit_cast<StaticConstructObjectFunction>(
                    static_cast<std::uintptr_t>(resolved.static_construct_object));
        }

        std::string none;
        std::string error;
        if (!fname_to_utf8({}, none, error))
        {
            m_fname_to_string = nullptr;
            m_fname_constructor = nullptr;
            m_realloc = nullptr;
            m_free = nullptr;
            m_allocator = nullptr;
            m_guobject_array = 0u;
            m_static_construct_object = nullptr;
            detail = "FName runtime self-test failed: " + error;
            return false;
        }
        if (none != "None")
        {
            m_fname_to_string = nullptr;
            m_fname_constructor = nullptr;
            m_realloc = nullptr;
            m_free = nullptr;
            m_allocator = nullptr;
            m_guobject_array = 0u;
            m_static_construct_object = nullptr;
            detail = "FName(0, 0) returned an unexpected value: " + none;
            return false;
        }

        RawFName constructed_none{};
        if (!fname_from_utf8("None", constructed_none, false, error) || constructed_none.comparison_index != 0u ||
            constructed_none.number != 0u)
        {
            m_fname_to_string = nullptr;
            m_fname_constructor = nullptr;
            m_realloc = nullptr;
            m_free = nullptr;
            m_allocator = nullptr;
            m_guobject_array = 0u;
            m_static_construct_object = nullptr;
            detail = "FName constructor self-test failed: " +
                     (error.empty() ? std::string{"'None' did not resolve to FName(0, 0)"} : error);
            return false;
        }

        detail = "UE 5.1 SysV FName construction/ToString and FMalloc Realloc/Free passed bounded self-tests";
        return true;
    }

    bool ReadOnlyUnrealRuntime::is_ready() const noexcept
    {
        return m_fname_to_string != nullptr && m_fname_constructor != nullptr && m_realloc != nullptr &&
               m_free != nullptr && m_allocator != nullptr && m_guobject_array != 0u;
    }

    bool ReadOnlyUnrealRuntime::initialize_process_event(std::string& detail) noexcept
    {
        m_process_event = nullptr;
        detail.clear();
        try
        {
            if (!is_ready())
            {
                detail = "validated UObject runtime is required before resolving ProcessEvent";
                return false;
            }
            const ObjectQueryResult packages = find_all_of("Package", 2u);
            if (!packages.success || packages.objects.empty())
            {
                detail = "no live UPackage sample is available for ProcessEvent vtable validation";
                return false;
            }

            std::uint64_t candidate{};
            std::uint64_t windows_candidate{};
            std::uint64_t following_candidate{};
            for (const auto& package : packages.objects)
            {
                UObjectBaseView object{};
                std::uint64_t current{};
                if (!safe_read(package.address, object) || object.vtable == 0u ||
                    !safe_read(object.vtable + k_ue51_linux_process_event_vtable_offset, current) || current == 0u)
                {
                    detail = "UPackage ProcessEvent vtable slot is unreadable";
                    return false;
                }
                if (candidate == 0u)
                {
                    candidate = current;
                    if (!safe_read(object.vtable + k_ue51_msvc_process_event_vtable_offset, windows_candidate) ||
                        !safe_read(object.vtable + k_ue51_linux_process_event_vtable_offset + sizeof(void*), following_candidate))
                    {
                        detail = "neighboring UObject vtable slots are unreadable";
                        return false;
                    }
                }
                else if (candidate != current)
                {
                    detail = "independent UPackage instances disagree on the Linux ProcessEvent vtable target";
                    return false;
                }
            }
            if (candidate == windows_candidate || candidate == following_candidate)
            {
                detail = "Linux ProcessEvent candidate aliases a neighboring UE 5.1 vtable entry";
                return false;
            }

            std::string error;
            if (!validate_process_event_code(candidate, error))
            {
                detail = std::move(error);
                return false;
            }
            m_process_event = std::bit_cast<ProcessEventFunction>(static_cast<std::uintptr_t>(candidate));
            std::array<char, 192> formatted{};
            std::snprintf(formatted.data(),
                          formatted.size(),
                          "UE 5.1 Linux UObject::ProcessEvent resolved at 0x%llx from vtable +0x%llx and passed six structural code gates",
                          static_cast<unsigned long long>(candidate),
                          static_cast<unsigned long long>(k_ue51_linux_process_event_vtable_offset));
            detail = formatted.data();
            return true;
        }
        catch (const std::exception& exception)
        {
            detail = exception.what();
            return false;
        }
        catch (...)
        {
            detail = "unknown exception while resolving ProcessEvent";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::process_event_available() const noexcept
    {
        return m_process_event != nullptr;
    }

#if defined(UE4SS_LINUX_TESTING)
    void ReadOnlyUnrealRuntime::set_process_event_for_testing(void* address) noexcept
    {
        m_process_event = std::bit_cast<ProcessEventFunction>(address);
    }
#endif

    void* ReadOnlyUnrealRuntime::process_event_address() const noexcept
    {
        return std::bit_cast<void*>(m_process_event);
    }

    bool ReadOnlyUnrealRuntime::initialize_fproperty_abi(std::string& detail) noexcept
    {
        using IdenticalFunction = bool (*)(const void*, const void*, const void*, std::uint32_t);
        using HashFunction = std::uint32_t (*)(const void*, const void*);
        using CopyFunction = void (*)(const void*, void*, const void*);
        using LifecycleFunction = void (*)(const void*, void*);
        using AlignmentFunction = std::int32_t (*)(const void*);
        using ImportTextFunction = const char16_t* (*)(
                const void*, const char16_t*, void*, std::int32_t, void*, std::int32_t, void*);

        m_fproperty_abi_ready = false;
        m_fproperty_import_text_ready = false;
        detail.clear();
        try
        {
            if (!is_ready())
            {
                detail = "validated UObject/FName runtime is required before the FProperty ABI gate";
                return false;
            }

            std::uint64_t sample_address{};
            std::string sample_name;
            ChunkedObjectArrayView registry{};
            if (!read_registry(m_guobject_array, registry))
            {
                detail = "GUObjectArray is unreadable during the FProperty ABI gate";
                return false;
            }
            std::unordered_set<std::uint64_t> visited_classes;
            for (std::int32_t index = 0; index < registry.num_elements && sample_address == 0u; ++index)
            {
                ObjectItemView item{};
                UObjectBaseView object{};
                if (!read_stable_object(registry, index, item, object) || object.class_private == 0u ||
                    !visited_classes.emplace(object.class_private).second)
                {
                    continue;
                }
                std::uint64_t field_address{};
                if (!safe_read(object.class_private + k_child_properties_offset, field_address))
                {
                    continue;
                }
                std::unordered_set<std::uint64_t> visited_fields;
                while (field_address != 0u && visited_fields.size() < k_max_property_fields &&
                       visited_fields.emplace(field_address).second)
                {
                    FPropertyPrefixView property{};
                    RawFName field_class_name{};
                    std::string type_name;
                    std::string property_name;
                    std::string name_error;
                    if (!safe_read(field_address, property) || property.vtable == 0u ||
                        property.class_private == 0u ||
                        !safe_read(property.class_private, field_class_name) ||
                        !fname_to_utf8(field_class_name, type_name, name_error) ||
                        !fname_to_utf8(property.name_private, property_name, name_error))
                    {
                        break;
                    }
                    if (type_name == "IntProperty" &&
                        property.element_size == static_cast<std::int32_t>(sizeof(std::int32_t)))
                    {
                        sample_address = field_address;
                        sample_name = std::move(property_name);
                        break;
                    }
                    field_address = property.next;
                }
            }
            if (sample_address == 0u)
            {
                detail = "no live IntProperty sample was found for the Linux FProperty ABI gate";
                return false;
            }

            IdenticalFunction identical{};
            HashFunction hash{};
            CopyFunction copy{};
            LifecycleFunction destroy{};
            LifecycleFunction initialize{};
            AlignmentFunction alignment{};
            if (!load_fproperty_virtual(sample_address, k_fproperty_identical_slot, identical, detail) ||
                !load_fproperty_virtual(sample_address, k_fproperty_hash_slot, hash, detail) ||
                !load_fproperty_virtual(
                        sample_address, k_fproperty_copy_single_to_script_vm_slot, copy, detail) ||
                !load_fproperty_virtual(sample_address, k_fproperty_destroy_value_slot, destroy, detail) ||
                !load_fproperty_virtual(sample_address, k_fproperty_initialize_value_slot, initialize, detail) ||
                !load_fproperty_virtual(sample_address, k_fproperty_min_alignment_slot, alignment, detail))
            {
                return false;
            }

            const std::int32_t left = 0x1234567;
            const std::int32_t equal = left;
            const std::int32_t different = left + 1;
            std::int32_t copied{};
            std::int32_t initialized = -1;
            const bool same = identical(
                    std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)), &left, &equal, 0u);
            const bool differs = identical(
                    std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)), &left, &different, 0u);
            const std::uint32_t hash_a = hash(
                    std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)), &left);
            const std::uint32_t hash_b = hash(
                    std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)), &equal);
            copy(std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)), &copied, &left);
            initialize(std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)), &initialized);
            destroy(std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)), &initialized);
            const std::int32_t min_alignment = alignment(
                    std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)));
            if (!same || differs || hash_a != hash_b || copied != left || initialized != 0 ||
                min_alignment != static_cast<std::int32_t>(alignof(std::int32_t)))
            {
                detail = "UE 5.1 Linux FProperty virtual slots failed their IntProperty behavior probes";
                return false;
            }

            m_fproperty_abi_ready = true;
            detail = "UE 5.1 Linux FProperty Itanium vtable passed Identical/hash/copy/init/destroy/alignment probes on '" +
                     sample_name + "'";

            ImportTextFunction import_text{};
            std::string import_detail;
            if (load_fproperty_virtual(
                        sample_address, k_fproperty_import_text_slot, import_text, import_detail))
            {
                constexpr char16_t probe_text[] = u"731";
                std::int32_t imported = -1;
                const char16_t* remaining = import_text(
                        std::bit_cast<const void*>(static_cast<std::uintptr_t>(sample_address)),
                        probe_text,
                        &imported,
                        0,
                        nullptr,
                        0x08000000,
                        nullptr);
                if (remaining == probe_text + 3 && imported == 731)
                {
                    m_fproperty_import_text_ready = true;
                    detail += "; ImportText=available";
                }
                else
                {
                    detail += "; ImportText=unavailable (IntProperty behavior probe failed)";
                }
            }
            else
            {
                detail += "; ImportText=unavailable (" + import_detail + ")";
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            detail = exception.what();
            return false;
        }
        catch (...)
        {
            detail = "unknown exception while validating the Linux FProperty ABI";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::fproperty_abi_available() const noexcept
    {
        return m_fproperty_abi_ready;
    }

    bool ReadOnlyUnrealRuntime::fproperty_import_text_available() const noexcept
    {
        return m_fproperty_import_text_ready;
    }

    bool ReadOnlyUnrealRuntime::import_property_text(
            std::uint64_t property_address,
            std::string_view text,
            std::uint64_t value_address,
            std::int32_t port_flags,
            const ReadOnlyUObjectHandle& owner,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        using ImportTextFunction = const char16_t* (*)(
                const void*, const char16_t*, void*, std::int32_t, void*, std::int32_t, void*);
        constexpr std::int32_t property_pointer_direct = 0;
        constexpr std::int32_t ppf_restrict_import_types = 0x20;
        constexpr std::int32_t ppf_use_deprecated_properties = 0x08000000;
        constexpr std::uint64_t cpf_config = 0x4000u;

        error.clear();
        try
        {
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "FProperty:ImportText can only execute on the validated game thread";
                return false;
            }
            if (!m_fproperty_import_text_ready)
            {
                error = "Linux FProperty ImportText ABI gate has not passed";
                return false;
            }
            if (!is_valid(owner))
            {
                error = "FProperty:ImportText requires a live owner UObject";
                return false;
            }

            FPropertyPrefixView property{};
            if (!safe_read(property_address, property) || property.array_dim != 1 ||
                property.element_size <= 0 || property.element_size > k_max_property_element_size)
            {
                error = "FProperty:ImportText requires a bounded scalar FProperty";
                return false;
            }
            if ((port_flags & ppf_restrict_import_types) != 0 &&
                (property.property_flags & cpf_config) != 0u)
            {
                error = "FProperty:ImportText rejected a Config property while RestrictImportTypes is set";
                return false;
            }
            const auto value_size = static_cast<std::uint64_t>(property.element_size);
            if (value_address == 0u ||
                value_address > std::numeric_limits<std::uint64_t>::max() - (value_size - 1u))
            {
                error = "FProperty:ImportText value address is invalid";
                return false;
            }
            std::byte first{};
            std::byte last{};
            if (!safe_read(value_address, first) ||
                !safe_read(value_address + value_size - 1u, last))
            {
                error = "FProperty:ImportText value storage is unreadable";
                return false;
            }

            ImportTextFunction import_text{};
            if (!load_fproperty_virtual(
                        property_address, k_fproperty_import_text_slot, import_text, error))
            {
                return false;
            }
            std::u16string buffer = ue4ss::linux::utf8_to_unreal(text);
            buffer.push_back(u'\0');
            const char16_t* begin = buffer.data();
            const char16_t* end = begin + buffer.size();
            const char16_t* remaining = import_text(
                    std::bit_cast<const void*>(static_cast<std::uintptr_t>(property_address)),
                    begin,
                    std::bit_cast<void*>(static_cast<std::uintptr_t>(value_address)),
                    property_pointer_direct,
                    std::bit_cast<void*>(static_cast<std::uintptr_t>(owner.address)),
                    port_flags | ppf_use_deprecated_properties,
                    nullptr);
            const auto remaining_address = std::bit_cast<std::uintptr_t>(remaining);
            const auto begin_address = std::bit_cast<std::uintptr_t>(begin);
            const auto end_address = std::bit_cast<std::uintptr_t>(end);
            if (remaining == nullptr || remaining_address < begin_address ||
                remaining_address >= end_address)
            {
                error = "FProperty:ImportText rejected the input or returned an invalid parse position";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while executing FProperty:ImportText";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::reflected_value_hash(std::uint64_t property_address,
                                                     std::uint64_t value_address,
                                                     std::uint32_t& output,
                                                     std::string& error) const noexcept
    {
        using HashFunction = std::uint32_t (*)(const void*, const void*);
        output = 0u;
        error.clear();
        try
        {
            FPropertyPrefixView property{};
            HashFunction hash{};
            if (!m_fproperty_abi_ready)
            {
                error = "Linux FProperty ABI gate has not passed";
                return false;
            }
            if (!safe_read(property_address, property) || property.array_dim != 1 ||
                property.element_size <= 0 || property.element_size > k_max_property_element_size)
            {
                error = "FProperty does not expose a bounded value for hashing";
                return false;
            }
            std::byte readable{};
            if (!safe_read(value_address, readable) ||
                !load_fproperty_virtual(property_address, k_fproperty_hash_slot, hash, error))
            {
                error = error.empty() ? "FProperty value is unreadable while hashing" : error;
                return false;
            }
            output = hash(std::bit_cast<const void*>(static_cast<std::uintptr_t>(property_address)),
                          std::bit_cast<const void*>(static_cast<std::uintptr_t>(value_address)));
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the FProperty hash boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::reflected_values_identical(std::uint64_t property_address,
                                                           std::uint64_t left_address,
                                                           std::uint64_t right_address,
                                                           bool& output,
                                                           std::string& error) const noexcept
    {
        using IdenticalFunction = bool (*)(const void*, const void*, const void*, std::uint32_t);
        output = false;
        error.clear();
        try
        {
            FPropertyPrefixView property{};
            IdenticalFunction identical{};
            std::byte readable{};
            if (!m_fproperty_abi_ready)
            {
                error = "Linux FProperty ABI gate has not passed";
                return false;
            }
            if (!safe_read(property_address, property) || property.array_dim != 1 ||
                property.element_size <= 0 || property.element_size > k_max_property_element_size ||
                !safe_read(left_address, readable) || !safe_read(right_address, readable) ||
                !load_fproperty_virtual(property_address, k_fproperty_identical_slot, identical, error))
            {
                error = error.empty() ? "FProperty identity operands are unreadable" : error;
                return false;
            }
            output = identical(std::bit_cast<const void*>(static_cast<std::uintptr_t>(property_address)),
                               std::bit_cast<const void*>(static_cast<std::uintptr_t>(left_address)),
                               std::bit_cast<const void*>(static_cast<std::uintptr_t>(right_address)),
                               0u);
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the FProperty identity boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::reflected_value_alignment(std::uint64_t property_address,
                                                           std::uint32_t& output,
                                                           std::string& error) const noexcept
    {
        using AlignmentFunction = std::int32_t (*)(const void*);
        output = 0u;
        error.clear();
        try
        {
            FPropertyPrefixView property{};
            AlignmentFunction alignment{};
            if (!m_fproperty_abi_ready)
            {
                error = "Linux FProperty ABI gate has not passed";
                return false;
            }
            if (!safe_read(property_address, property) || property.array_dim != 1 ||
                property.element_size <= 0 || property.element_size > k_max_property_element_size ||
                !load_fproperty_virtual(
                        property_address, k_fproperty_min_alignment_slot, alignment, error))
            {
                error = error.empty() ? "FProperty alignment metadata is invalid" : error;
                return false;
            }
            const std::int32_t result = alignment(
                    std::bit_cast<const void*>(static_cast<std::uintptr_t>(property_address)));
            if (result <= 0 || result > 4096 || !std::has_single_bit(static_cast<std::uint32_t>(result)))
            {
                error = "FProperty::GetMinAlignment returned an invalid alignment";
                return false;
            }
            output = static_cast<std::uint32_t>(result);
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the FProperty alignment boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::initialize_reflected_value(std::uint64_t property_address,
                                                           std::uint64_t value_address,
                                                           std::string& error) const noexcept
    {
        using LifecycleFunction = void (*)(const void*, void*);
        error.clear();
        try
        {
            FPropertyPrefixView property{};
            if (!m_fproperty_abi_ready)
            {
                error = "Linux FProperty ABI gate has not passed";
                return false;
            }
            if (!safe_read(property_address, property) || property.array_dim != 1 ||
                property.element_size <= 0 || property.element_size > k_max_property_element_size)
            {
                error = "FProperty lifecycle metadata is invalid";
                return false;
            }
            if ((property.property_flags & k_property_flag_zero_constructor) != 0u)
            {
                std::vector<std::byte> zero(static_cast<std::size_t>(property.element_size));
                return safe_write_bytes(value_address, zero.data(), zero.size()) ||
                       (error = "zero-constructed FProperty destination is not writable", false);
            }
            LifecycleFunction initialize{};
            if (!load_fproperty_virtual(
                        property_address, k_fproperty_initialize_value_slot, initialize, error))
            {
                return false;
            }
            initialize(std::bit_cast<const void*>(static_cast<std::uintptr_t>(property_address)),
                       std::bit_cast<void*>(static_cast<std::uintptr_t>(value_address)));
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the FProperty initialization boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::copy_reflected_value(std::uint64_t property_address,
                                                     std::uint64_t destination_address,
                                                     std::uint64_t source_address,
                                                     std::string& error) const noexcept
    {
        using CopyFunction = void (*)(const void*, void*, const void*);
        error.clear();
        try
        {
            FPropertyPrefixView property{};
            CopyFunction copy{};
            std::byte readable{};
            if (!m_fproperty_abi_ready)
            {
                error = "Linux FProperty ABI gate has not passed";
                return false;
            }
            if (!safe_read(property_address, property) || property.array_dim != 1 ||
                property.element_size <= 0 || property.element_size > k_max_property_element_size ||
                !safe_read(destination_address, readable) || !safe_read(source_address, readable) ||
                !load_fproperty_virtual(
                        property_address,
                        k_fproperty_copy_single_to_script_vm_slot,
                        copy,
                        error))
            {
                error = error.empty() ? "FProperty copy operands or metadata are unreadable" : error;
                return false;
            }
            copy(std::bit_cast<const void*>(static_cast<std::uintptr_t>(property_address)),
                 std::bit_cast<void*>(static_cast<std::uintptr_t>(destination_address)),
                 std::bit_cast<const void*>(static_cast<std::uintptr_t>(source_address)));
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the FProperty copy boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::destroy_reflected_value(std::uint64_t property_address,
                                                        std::uint64_t value_address,
                                                        std::string& error) const noexcept
    {
        using LifecycleFunction = void (*)(const void*, void*);
        error.clear();
        try
        {
            FPropertyPrefixView property{};
            if (!m_fproperty_abi_ready)
            {
                error = "Linux FProperty ABI gate has not passed";
                return false;
            }
            if (!safe_read(property_address, property) || property.array_dim != 1 ||
                property.element_size <= 0 || property.element_size > k_max_property_element_size)
            {
                error = "FProperty lifecycle metadata is invalid";
                return false;
            }
            if ((property.property_flags & k_property_flag_no_destructor) != 0u)
            {
                return true;
            }
            LifecycleFunction destroy{};
            if (!load_fproperty_virtual(property_address, k_fproperty_destroy_value_slot, destroy, error))
            {
                return false;
            }
            destroy(std::bit_cast<const void*>(static_cast<std::uintptr_t>(property_address)),
                    std::bit_cast<void*>(static_cast<std::uintptr_t>(value_address)));
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the FProperty destruction boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::assign_unreal_text(std::uint64_t property_address,
                                                   std::uint64_t address,
                                                   std::string_view input,
                                                   std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (m_process_event == nullptr || !m_fproperty_abi_ready)
            {
                error = "FText construction requires validated ProcessEvent and FProperty ABIs";
                return false;
            }
            ReflectedProperty destination_property;
            if (!decode_nested_property(*this, property_address, destination_property, error, true) ||
                destination_property.type_name != "TextProperty" ||
                destination_property.element_size != static_cast<std::int32_t>(sizeof(FTextView)))
            {
                error = error.empty() ? "FText destination does not reference a UE 5.1 TextProperty" : error;
                return false;
            }
            // Validate UTF-8 before allocating any engine object.
            static_cast<void>(ue4ss::linux::utf8_to_unreal(input));

            const auto libraries = find_by_path("/Script/Engine.Default__KismetTextLibrary", 1u);
            if (!libraries.success || libraries.objects.size() != 1u)
            {
                error = "UKismetTextLibrary CDO is unavailable for FText construction";
                return false;
            }
            ReadOnlyUObjectHandle conversion_function;
            if (!find_function(libraries.objects.front(), "Conv_StringToText", conversion_function, error))
            {
                error = "UKismetTextLibrary::Conv_StringToText is unavailable: " + error;
                return false;
            }

            std::uint8_t declared_parameter_count{};
            std::uint16_t params_size{};
            std::uint16_t declared_return_offset{};
            std::uint64_t field_address{};
            if (!safe_read(conversion_function.address + 0xb4u, declared_parameter_count) ||
                !safe_read(conversion_function.address + 0xb6u, params_size) ||
                !safe_read(conversion_function.address + 0xb8u, declared_return_offset) ||
                !safe_read(conversion_function.address + k_child_properties_offset, field_address) ||
                declared_parameter_count != 2u || params_size == 0u || params_size > 65535u)
            {
                error = "Conv_StringToText UFunction layout is incompatible with the validated bridge";
                return false;
            }

            std::optional<ReflectedProperty> input_property;
            std::optional<ReflectedProperty> return_property;
            std::unordered_set<std::uint64_t> visited;
            std::size_t reflected_parameter_count{};
            const auto decode_name = [this](RawFName name,
                                            std::string& decoded,
                                            std::string& decode_error) {
                return fname_to_utf8(name, decoded, decode_error);
            };
            while (field_address != 0u)
            {
                if (visited.size() >= k_max_property_fields || !visited.emplace(field_address).second)
                {
                    error = "Conv_StringToText FProperty chain is cyclic or exceeds the safety limit";
                    return false;
                }
                FPropertyPrefixView prefix{};
                if (!safe_read(field_address, prefix))
                {
                    error = "Conv_StringToText FProperty chain became unreadable";
                    return false;
                }
                if ((prefix.property_flags & k_property_flag_parameter) != 0u)
                {
                    ReflectedProperty property;
                    if (!decode_function_property(field_address, params_size, decode_name, property, error))
                    {
                        return false;
                    }
                    ++reflected_parameter_count;
                    if ((property.flags & k_property_flag_return_parameter) != 0u)
                    {
                        if (return_property.has_value())
                        {
                            error = "Conv_StringToText exposes multiple return properties";
                            return false;
                        }
                        return_property = property;
                    }
                    else
                    {
                        if (input_property.has_value())
                        {
                            error = "Conv_StringToText exposes multiple input properties";
                            return false;
                        }
                        input_property = property;
                    }
                }
                field_address = prefix.next;
            }
            if (reflected_parameter_count != declared_parameter_count || !input_property.has_value() ||
                !return_property.has_value() || input_property->type_name != "StrProperty" ||
                input_property->element_size != static_cast<std::int32_t>(sizeof(UnrealStringHeader)) ||
                return_property->type_name != "TextProperty" ||
                return_property->element_size != static_cast<std::int32_t>(sizeof(FTextView)) ||
                return_property->offset != declared_return_offset)
            {
                error = "Conv_StringToText signature is not FString -> FText";
                return false;
            }

            std::vector<std::byte> params(params_size);
            UnrealStringHeader input_string{};
            bool return_initialized{};
            const auto cleanup = [this,
                                  &input_string,
                                  &return_initialized,
                                  &return_property,
                                  &params](void*) noexcept {
                if (return_initialized && return_property.has_value())
                {
                    std::string cleanup_error;
                    static_cast<void>(destroy_reflected_value(
                            return_property->address,
                            std::bit_cast<std::uintptr_t>(params.data() + return_property->offset),
                            cleanup_error));
                }
                release_unreal_string(input_string);
            };
            auto cleanup_guard = std::unique_ptr<void, decltype(cleanup)>{
                    reinterpret_cast<void*>(std::uintptr_t{1}), cleanup};
            if (!allocate_unreal_string(input, input_string, error))
            {
                return false;
            }
            std::memcpy(params.data() + input_property->offset, &input_string, sizeof(input_string));
            const std::uint64_t return_address =
                    std::bit_cast<std::uintptr_t>(params.data() + return_property->offset);
            if (!initialize_reflected_value(return_property->address, return_address, error))
            {
                return false;
            }
            return_initialized = true;

            m_process_event(
                    std::bit_cast<void*>(static_cast<std::uintptr_t>(libraries.objects.front().address)),
                    std::bit_cast<void*>(static_cast<std::uintptr_t>(conversion_function.address)),
                    params.data());

            UnrealPropertyValue constructed;
            if (!decode_plain_value(*this, *return_property, return_address, constructed, error) ||
                constructed.kind != UnrealPropertyKind::text || constructed.text != input)
            {
                error = error.empty() ? "Conv_StringToText did not round-trip its UTF-8 input" : error;
                return false;
            }
            if (!copy_reflected_value(property_address, address, return_address, error))
            {
                return false;
            }
            UnrealPropertyValue assigned;
            if (!decode_plain_value(*this, destination_property, address, assigned, error) ||
                assigned.kind != UnrealPropertyKind::text || assigned.text != input)
            {
                error = error.empty() ? "FText destination failed post-copy validation" : error;
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while constructing an Unreal FText";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::is_valid(const ReadOnlyUObjectHandle& handle) const noexcept
    {
        if (!is_ready() || handle.address == 0u || handle.internal_index < 0)
        {
            return false;
        }
        ChunkedObjectArrayView registry{};
        ObjectItemView item{};
        UObjectBaseView object{};
        return read_registry(m_guobject_array, registry) &&
               read_stable_object(registry, handle.internal_index, item, object) &&
               item.object == handle.address && item.serial_number == handle.serial_number;
    }

    bool ReadOnlyUnrealRuntime::handle_from_address(std::uint64_t address, ReadOnlyUObjectHandle& output) const noexcept
    {
        output = {};
        if (!is_ready() || address == 0u)
        {
            return false;
        }
        UObjectBaseView object{};
        ChunkedObjectArrayView registry{};
        ObjectItemView item{};
        UObjectBaseView stable_object{};
        if (!safe_read(address, object) || object.internal_index < 0 ||
            !read_registry(m_guobject_array, registry) ||
            !read_stable_object(registry, object.internal_index, item, stable_object) || item.object != address)
        {
            return false;
        }
        output = {address, object.internal_index, item.serial_number};
        return true;
    }

    bool ReadOnlyUnrealRuntime::handle_from_weak(std::int32_t internal_index,
                                                 std::int32_t serial_number,
                                                 ReadOnlyUObjectHandle& output) const noexcept
    {
        output = {};
        if (!is_ready() || internal_index < 0 || serial_number == 0)
        {
            return false;
        }
        ChunkedObjectArrayView registry{};
        ObjectItemView item{};
        UObjectBaseView object{};
        if (!read_registry(m_guobject_array, registry) || internal_index >= registry.num_elements ||
            !read_stable_object(registry, internal_index, item, object) || item.object == 0u ||
            item.serial_number != serial_number)
        {
            return false;
        }
        output = {item.object, internal_index, serial_number};
        return true;
    }

    bool ReadOnlyUnrealRuntime::snapshot_native_tchar_string(
            std::uint64_t address, std::string& output, std::string& error) const noexcept
    {
        constexpr std::size_t max_code_units = 65536u;
        output.clear();
        error.clear();
        try
        {
            if (!is_ready() || address == 0u)
            {
                error = "validated Unreal runtime and native TCHAR address are required";
                return false;
            }
            std::u16string copied;
            copied.reserve(128u);
            for (std::size_t index = 0; index < max_code_units; ++index)
            {
                char16_t code_unit{};
                if (!safe_read(address + index * sizeof(char16_t), code_unit))
                {
                    error = "native TCHAR buffer became unreadable before its terminator";
                    return false;
                }
                if (code_unit == u'\0')
                {
                    output = ue4ss::linux::unreal_to_utf8(copied);
                    return true;
                }
                copied.push_back(code_unit);
            }
            error = "native TCHAR buffer exceeds the 65536-code-unit safety limit";
            return false;
        }
        catch (const std::exception& exception)
        {
            output.clear();
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output.clear();
            error = "unknown exception while snapshotting a native TCHAR string";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::snapshot_native_fstring(
            std::uint64_t address, std::string& output, std::string& error) const noexcept
    {
        output.clear();
        error.clear();
        if (!is_ready() || address == 0u)
        {
            error = "validated Unreal runtime and native FString address are required";
            return false;
        }
        return read_unreal_string(address, output, error);
    }

    bool ReadOnlyUnrealRuntime::snapshot_native_furl(
            std::uint64_t address, UnrealFURLSnapshot& output, std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            NativeFURLView url{};
            if (!is_ready() || address == 0u || !safe_read(address, url))
            {
                error = "validated Unreal runtime and readable native FURL are required";
                return false;
            }
            if (!read_unreal_string(address + offsetof(NativeFURLView, protocol), output.protocol, error) ||
                !read_unreal_string(address + offsetof(NativeFURLView, host), output.host, error) ||
                !read_unreal_string(address + offsetof(NativeFURLView, map), output.map, error) ||
                !read_unreal_string(address + offsetof(NativeFURLView, redirect_url), output.redirect_url, error) ||
                !read_unreal_string(address + offsetof(NativeFURLView, portal), output.portal, error))
            {
                output = {};
                error = error.empty() ? "native FURL contains an invalid FString" : error;
                return false;
            }
            if (url.options.num < 0 || url.options.max < url.options.num ||
                url.options.max > 1024 ||
                (url.options.max == 0 && url.options.data != 0u) ||
                (url.options.max > 0 && url.options.data == 0u) ||
                (url.options.num > 0 &&
                 url.options.data > std::numeric_limits<std::uint64_t>::max() -
                                            static_cast<std::uint64_t>(url.options.num) *
                                                    sizeof(UnrealStringHeader)))
            {
                output = {};
                error = "native FURL options have an invalid TArray header";
                return false;
            }
            output.port = url.port;
            output.valid = url.valid;
            output.options.reserve(static_cast<std::size_t>(url.options.num));
            for (std::int32_t index = 0; index < url.options.num; ++index)
            {
                std::string option;
                if (!read_unreal_string(
                            url.options.data + static_cast<std::uint64_t>(index) *
                                                       sizeof(UnrealStringHeader),
                            option,
                            error))
                {
                    output = {};
                    error = error.empty() ? "native FURL contains an invalid option FString" : error;
                    return false;
                }
                output.options.push_back(std::move(option));
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            output = {};
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output = {};
            error = "unknown exception while snapshotting a native FURL";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::world_from_native_context(
            std::uint64_t address, ReadOnlyUObjectHandle& output, std::string& error) const noexcept
    {
        constexpr std::uint64_t ue51_this_current_world_offset = 0x2c0u;
        output = {};
        error.clear();
        std::uint64_t world{};
        if (!is_ready() || address == 0u ||
            !safe_read(address + ue51_this_current_world_offset, world))
        {
            error = "UE 5.1 FWorldContext::ThisCurrentWorld at +0x2c0 is unreadable";
            return false;
        }
        if (world == 0u)
        {
            return true;
        }
        if (!handle_from_address(world, output))
        {
            error = "FWorldContext::ThisCurrentWorld is outside the validated GUObjectArray";
            return false;
        }
        std::string class_error;
        if (!is_a(output, "World", class_error))
        {
            output = {};
            error = class_error.empty() ? "FWorldContext::ThisCurrentWorld failed the UWorld identity gate"
                                        : std::move(class_error);
            return false;
        }
        return true;
    }

    bool ReadOnlyUnrealRuntime::is_a(const ReadOnlyUObjectHandle& handle,
                                     std::string_view short_class_name,
                                     std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (short_class_name.empty() || !is_valid(handle))
            {
                error = "valid UObject handle and non-empty class name are required";
                return false;
            }
            UObjectBaseView object{};
            if (!safe_read(handle.address, object))
            {
                error = "UObject became unreadable";
                return false;
            }
            std::uint64_t current_class = object.class_private;
            for (std::size_t depth = 0; current_class != 0u && depth < k_max_super_depth; ++depth)
            {
                UObjectBaseView class_object{};
                std::string class_name;
                if (!safe_read(current_class, class_object) || !fname_to_utf8(class_object.name_private, class_name, error))
                {
                    error = error.empty() ? "class hierarchy became unreadable" : error;
                    return false;
                }
                if (class_name == short_class_name)
                {
                    return true;
                }
                if (!safe_read(current_class + k_super_struct_offset, current_class))
                {
                    error = "UStruct::SuperStruct became unreadable";
                    return false;
                }
            }
            return false;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while checking UObject::IsA";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::is_a(const ReadOnlyUObjectHandle& handle,
                                     const ReadOnlyUObjectHandle& requested_class,
                                     std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!is_valid(handle) || !is_valid(requested_class))
            {
                error = "live UObject and UClass handles are required for exact IsA";
                return false;
            }
            UObjectBaseView object{};
            if (!safe_read(handle.address, object))
            {
                error = "UObject became unreadable";
                return false;
            }
            std::uint64_t current_class = object.class_private;
            for (std::size_t depth = 0; current_class != 0u && depth < k_max_super_depth; ++depth)
            {
                if (current_class == requested_class.address)
                {
                    return true;
                }
                if (!safe_read(current_class + k_super_struct_offset, current_class))
                {
                    error = "UStruct::SuperStruct became unreadable";
                    return false;
                }
            }
            return false;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while checking exact UObject::IsA";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::implements_interface(
            const ReadOnlyUObjectHandle& handle,
            const ReadOnlyUObjectHandle& interface_class,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!is_valid(handle) || !is_valid(interface_class))
            {
                error = "live UObject and interface UClass handles are required";
                return false;
            }
            std::uint64_t ignored_interface_pointer{};
            return resolve_script_interface_pointer(
                    *this, handle, interface_class.address, ignored_interface_pointer, error);
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while checking UObject interface implementation";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::describe_object(const ReadOnlyUObjectHandle& handle,
                                                ReadOnlyUObjectDescription& output,
                                                std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            ChunkedObjectArrayView registry{};
            ObjectItemView item{};
            UObjectBaseView object{};
            if (!read_registry(m_guobject_array, registry) ||
                !read_stable_object(registry, handle.internal_index, item, object) ||
                item.object != handle.address || item.serial_number != handle.serial_number)
            {
                error = "UObject handle is stale or no longer present in GUObjectArray";
                return false;
            }

            std::unordered_map<std::uint64_t, std::string> names;
            const auto decode_name = [this, &names, &error](RawFName name, std::string& decoded) {
                const std::uint64_t key = fname_key(name);
                if (const auto found = names.find(key); found != names.end())
                {
                    decoded = found->second;
                    return true;
                }
                if (!fname_to_utf8(name, decoded, error))
                {
                    return false;
                }
                names.emplace(key, decoded);
                return true;
            };

            std::string object_name;
            if (!decode_name(object.name_private, object_name))
            {
                return false;
            }
            UObjectBaseView object_class{};
            if (!safe_read(object.class_private, object_class) || object_class.vtable == 0u ||
                !decode_name(object_class.name_private, output.class_name))
            {
                error = error.empty() ? "UObject class metadata is unreadable" : error;
                return false;
            }

            struct PathPart
            {
                UObjectBaseView object;
                std::string name;
                std::string class_name;
            };
            std::vector<PathPart> reverse_path;
            reverse_path.reserve(8u);
            std::uint64_t current_address = handle.address;
            for (std::size_t depth = 0; current_address != 0u && depth < k_max_outer_depth; ++depth)
            {
                UObjectBaseView current{};
                if (!safe_read(current_address, current) || current.vtable == 0u || current.class_private == 0u)
                {
                    error = "outer chain contains an unreadable UObject";
                    return false;
                }
                UObjectBaseView current_class{};
                PathPart part{.object = current};
                if (!safe_read(current.class_private, current_class) ||
                    !decode_name(current.name_private, part.name) ||
                    !decode_name(current_class.name_private, part.class_name))
                {
                    error = error.empty() ? "outer chain metadata could not be decoded" : error;
                    return false;
                }
                reverse_path.push_back(std::move(part));
                current_address = current.outer_private;
            }
            if (current_address != 0u)
            {
                error = "outer chain exceeded the safety depth limit";
                return false;
            }

            std::string path;
            for (auto iterator = reverse_path.rbegin(); iterator != reverse_path.rend(); ++iterator)
            {
                if (!path.empty())
                {
                    const auto previous = std::prev(iterator);
                    bool direct_subobject_of_package = false;
                    if (previous->class_name != "Package" && previous->object.outer_private != 0u)
                    {
                        UObjectBaseView previous_outer{};
                        UObjectBaseView previous_outer_class{};
                        std::string previous_outer_class_name;
                        direct_subobject_of_package = safe_read(previous->object.outer_private, previous_outer) &&
                                                      safe_read(previous_outer.class_private, previous_outer_class) &&
                                                      decode_name(previous_outer_class.name_private, previous_outer_class_name) &&
                                                      previous_outer_class_name == "Package";
                    }
                    path.push_back(direct_subobject_of_package ? ':' : '.');
                }
                path += iterator->name;
            }

            output.handle = handle;
            output.object_flags = object.object_flags;
            output.internal_object_flags = item.flags;
            output.class_address = object.class_private;
            output.outer_address = object.outer_private;
            output.name_private = object.name_private;
            output.name = std::move(object_name);
            output.path_name = std::move(path);
            output.full_name = output.class_name + " " + output.path_name;
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while describing a UObject";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::find_outer_of_class(
            const ReadOnlyUObjectHandle& handle,
            std::string_view short_class_name,
            bool include_self,
            ReadOnlyUObjectHandle& output,
            std::string& error) const noexcept
    {
        const ReadOnlyUObjectHandle input = handle;
        output = {};
        error.clear();
        try
        {
            if (!is_valid(input) || short_class_name.empty())
            {
                error = "a live UObject and non-empty outer class name are required";
                return false;
            }
            ReadOnlyUObjectDescription initial;
            if (!describe_object(input, initial, error))
            {
                return false;
            }
            constexpr std::uint32_t class_default_object_flag = 0x10u;
            constexpr std::uint32_t begin_destroyed_flag = 0x8000u;
            constexpr std::uint32_t unreachable_internal_flag = 1u << 28u;
            if ((initial.object_flags & class_default_object_flag) != 0u)
            {
                return true;
            }

            std::uint64_t current_address = include_self ? input.address : initial.outer_address;
            std::unordered_set<std::uint64_t> visited;
            for (std::size_t depth = 0; current_address != 0u && depth < k_max_outer_depth; ++depth)
            {
                if (!visited.emplace(current_address).second)
                {
                    error = "UObject outer chain is cyclic while resolving " +
                            std::string{short_class_name};
                    return false;
                }
                ReadOnlyUObjectHandle current;
                ReadOnlyUObjectDescription description;
                if (!handle_from_address(current_address, current) ||
                    !describe_object(current, description, error))
                {
                    error = error.empty() ? "UObject outer is outside the validated registry" : error;
                    return false;
                }
                if ((description.object_flags & begin_destroyed_flag) != 0u ||
                    (static_cast<std::uint32_t>(description.internal_object_flags) &
                     unreachable_internal_flag) != 0u)
                {
                    return true;
                }
                std::string class_error;
                if (is_a(current, short_class_name, class_error))
                {
                    output = current;
                    return true;
                }
                if (!class_error.empty())
                {
                    error = std::move(class_error);
                    return false;
                }
                current_address = description.outer_address;
            }
            if (current_address != 0u)
            {
                error = "UObject outer chain exceeds the safety depth limit";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            output = {};
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output = {};
            error = "unknown exception while resolving a UObject outer class";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::get_class_default_object(const ReadOnlyUObjectHandle& unreal_class,
                                                         ReadOnlyUObjectHandle& output,
                                                         std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            std::string class_error;
            if (!is_valid(unreal_class) || !is_a(unreal_class, "Class", class_error))
            {
                error = class_error.empty() ? "UClass::GetCDO requires a live UClass" : class_error;
                return false;
            }

            std::uint64_t cdo_address{};
            if (!safe_read(unreal_class.address + k_ue51_class_default_object_offset, cdo_address) ||
                cdo_address == 0u)
            {
                error = "UClass::ClassDefaultObject at the validated UE 5.1 offset is null or unreadable";
                return false;
            }
            if (!handle_from_address(cdo_address, output))
            {
                error = "UClass::ClassDefaultObject is not a live GUObjectArray entry";
                return false;
            }

            ReadOnlyUObjectDescription description;
            if (!describe_object(output, description, error))
            {
                output = {};
                error = error.empty() ? "UClass::ClassDefaultObject metadata is unreadable" : error;
                return false;
            }
            constexpr std::uint32_t class_default_object_flag = 0x10u;
            if ((description.object_flags & class_default_object_flag) == 0u ||
                description.class_address != unreal_class.address)
            {
                output = {};
                error = "UClass::ClassDefaultObject failed its RF_ClassDefaultObject/class identity gate";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            output = {};
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output = {};
            error = "unknown exception while resolving UClass::ClassDefaultObject";
            return false;
        }
    }

    ObjectQueryResult ReadOnlyUnrealRuntime::find_objects(
            std::size_t limit,
            std::optional<std::string_view> short_class_name,
            std::optional<std::string_view> object_short_name,
            std::uint32_t required_flags,
            std::uint32_t banned_flags,
            bool exact_class) const noexcept
    {
        ObjectQueryResult result;
        try
        {
            if (!is_ready() ||
                ((!short_class_name.has_value() || short_class_name->empty()) &&
                 (!object_short_name.has_value() || object_short_name->empty())))
            {
                result.detail = "FindObjects requires the validated runtime and a class or object name";
                return result;
            }
            ChunkedObjectArrayView registry{};
            if (!read_registry(m_guobject_array, registry))
            {
                result.detail = "GUObjectArray is not readable while executing FindObjects";
                return result;
            }

            std::unordered_map<std::uint64_t, std::string> names;
            const auto decode_name = [this, &names](RawFName name, std::string& decoded, std::string& error) {
                const std::uint64_t key = fname_key(name);
                if (const auto found = names.find(key); found != names.end())
                {
                    decoded = found->second;
                    return true;
                }
                if (!fname_to_utf8(name, decoded, error))
                {
                    return false;
                }
                names.emplace(key, decoded);
                return true;
            };

            const std::size_t reserve_count = limit == 0u ? 64u : std::min<std::size_t>(limit, 64u);
            result.objects.reserve(reserve_count);
            for (std::int32_t index = 0; index < registry.num_elements; ++index)
            {
                ++result.scanned_objects;
                ObjectItemView item{};
                UObjectBaseView object{};
                if (!read_stable_object(registry, index, item, object))
                {
                    continue;
                }
                if ((object.object_flags & required_flags) != required_flags ||
                    (object.object_flags & banned_flags) != 0u)
                {
                    continue;
                }

                bool name_matches = !object_short_name.has_value();
                if (object_short_name.has_value())
                {
                    std::string object_name;
                    std::string error;
                    if (!decode_name(object.name_private, object_name, error))
                    {
                        result.detail = "object name became unreadable at index " + std::to_string(index) +
                                        (error.empty() ? std::string{} : ": " + error);
                        return result;
                    }
                    name_matches = object_name == *object_short_name;
                }
                if (!name_matches)
                {
                    continue;
                }

                bool class_matches = !short_class_name.has_value();
                if (short_class_name.has_value())
                {
                    std::uint64_t current_class = object.class_private;
                    for (std::size_t depth = 0; current_class != 0u && depth < k_max_super_depth; ++depth)
                    {
                        UObjectBaseView class_object{};
                        std::string class_name;
                        std::string error;
                        if (!safe_read(current_class, class_object) || class_object.vtable == 0u ||
                            !decode_name(class_object.name_private, class_name, error))
                        {
                            result.detail = "class hierarchy became unreadable at object index " +
                                            std::to_string(index) +
                                            (error.empty() ? std::string{} : ": " + error);
                            return result;
                        }
                        if (class_name == *short_class_name)
                        {
                            class_matches = true;
                            break;
                        }
                        if (exact_class)
                        {
                            break;
                        }
                        if (!safe_read(current_class + k_super_struct_offset, current_class))
                        {
                            result.detail = "UStruct::SuperStruct is unreadable at object index " +
                                            std::to_string(index);
                            return result;
                        }
                    }
                }
                if (!class_matches)
                {
                    continue;
                }

                result.objects.push_back({item.object, index, item.serial_number});
                if (limit != 0u && result.objects.size() >= limit)
                {
                    result.truncated = index + 1 < registry.num_elements;
                    break;
                }
            }
            result.success = true;
            result.detail = "FindObjects scanned " + std::to_string(result.scanned_objects) +
                            " object slots and returned " + std::to_string(result.objects.size()) +
                            (result.truncated ? " (limited)" : "");
            return result;
        }
        catch (const std::exception& exception)
        {
            result.detail = exception.what();
            return result;
        }
        catch (...)
        {
            result.detail = "unknown exception while executing FindObjects";
            return result;
        }
    }

    bool ReadOnlyUnrealRuntime::for_each_uobject(
            const std::function<bool(const ReadOnlyUObjectHandle&,
                                     std::int32_t,
                                     std::int32_t)>& callback,
            std::int32_t& visited_objects,
            std::string& error) const noexcept
    {
        visited_objects = 0;
        error.clear();
        try
        {
            if (!is_ready() || !callback)
            {
                error = "ForEachUObject requires the validated runtime and a callback";
                return false;
            }
            ChunkedObjectArrayView registry{};
            if (!read_registry(m_guobject_array, registry))
            {
                error = "GUObjectArray is not readable while executing ForEachUObject";
                return false;
            }

            constexpr std::int32_t unreachable_flag = 1 << 28;
            for (std::int32_t index = 0; index < registry.num_elements; ++index)
            {
                ObjectItemView item{};
                UObjectBaseView object{};
                if (!read_stable_object(registry, index, item, object) ||
                    (item.flags & unreachable_flag) != 0)
                {
                    continue;
                }
                ++visited_objects;
                if (!callback({item.object, index, item.serial_number},
                              index / k_elements_per_chunk,
                              index % k_elements_per_chunk))
                {
                    break;
                }
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while executing ForEachUObject";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::static_construct_object_available() const noexcept
    {
        return is_ready() && m_static_construct_object != nullptr;
    }

    bool ReadOnlyUnrealRuntime::construct_object(
            const UnrealObjectConstructionRequest& request,
            const GameThreadScheduler& scheduler,
            ReadOnlyUObjectHandle& output,
            std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            if (!static_construct_object_available())
            {
                error = "StaticConstructObject resolver is unavailable";
                return false;
            }
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "StaticConstructObject can only execute on the validated game thread";
                return false;
            }
            std::string validation_error;
            if (!is_valid(request.unreal_class) ||
                !is_a(request.unreal_class, "Class", validation_error))
            {
                error = validation_error.empty() ? "StaticConstructObject requires a live UClass"
                                                 : std::move(validation_error);
                return false;
            }
            if (!is_valid(request.outer))
            {
                error = "StaticConstructObject requires a live outer UObject";
                return false;
            }
            if (request.object_template.has_value() && !is_valid(*request.object_template))
            {
                error = "StaticConstructObject template is not a live UObject";
                return false;
            }
            if (request.external_package.has_value())
            {
                if (!is_valid(*request.external_package) ||
                    !is_a(*request.external_package, "Package", validation_error))
                {
                    error = validation_error.empty()
                                    ? "StaticConstructObject external package is not a live UPackage"
                                    : std::move(validation_error);
                    return false;
                }
            }
            constexpr std::uint32_t all_object_flags = 0x0fffffffu;
            constexpr std::uint32_t all_internal_object_flags = 0xff800000u;
            if ((request.object_flags & ~all_object_flags) != 0u)
            {
                error = "StaticConstructObject contains unknown EObjectFlags bits";
                return false;
            }
            if ((request.internal_object_flags & ~all_internal_object_flags) != 0u)
            {
                error = "StaticConstructObject contains unknown EInternalObjectFlags bits";
                return false;
            }
            std::string decoded_name;
            if (!fname_to_utf8(request.name, decoded_name, validation_error))
            {
                error = "StaticConstructObject FName is invalid: " + validation_error;
                return false;
            }

            StaticConstructObjectParametersView parameters{
                    .unreal_class = request.unreal_class.address,
                    .outer = request.outer.address,
                    .name = request.name,
                    .object_flags = request.object_flags,
                    .internal_object_flags = request.internal_object_flags,
                    .copy_transients_from_class_defaults =
                            static_cast<std::uint8_t>(request.copy_transients_from_class_defaults),
                    .assume_template_is_archetype =
                            static_cast<std::uint8_t>(request.assume_template_is_archetype),
                    .object_template = request.object_template.has_value()
                            ? request.object_template->address
                            : 0u,
                    .external_package = request.external_package.has_value()
                            ? request.external_package->address
                            : 0u,
            };
            void* constructed = m_static_construct_object(&parameters);
            if (constructed == nullptr)
            {
                return true;
            }
            if (!handle_from_address(std::bit_cast<std::uintptr_t>(constructed), output))
            {
                error = "StaticConstructObject returned an object without a stable GUObjectArray handle";
                return false;
            }
            ReadOnlyUObjectDescription description;
            if (!describe_object(output, description, error) ||
                description.class_address != request.unreal_class.address ||
                description.outer_address != request.outer.address)
            {
                output = {};
                if (error.empty())
                {
                    error = "StaticConstructObject result failed its class/outer identity gate";
                }
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            output = {};
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output = {};
            error = "unknown exception while executing StaticConstructObject";
            return false;
        }
    }

    ObjectQueryResult ReadOnlyUnrealRuntime::find_all_of(std::string_view short_class_name,
                                                         std::size_t limit) const noexcept
    {
        ObjectQueryResult result;
        try
        {
            if (!is_ready() || short_class_name.empty() || limit == 0u)
            {
                result.detail = "read-only runtime, class name, and positive result limit are required";
                return result;
            }
            ChunkedObjectArrayView registry{};
            if (!read_registry(m_guobject_array, registry))
            {
                result.detail = "GUObjectArray is not readable while executing FindAllOf";
                return result;
            }

            std::unordered_map<std::uint64_t, std::string> names;
            const auto decode_name = [this, &names](RawFName name, std::string& decoded, std::string& error) {
                const std::uint64_t key = fname_key(name);
                if (const auto found = names.find(key); found != names.end())
                {
                    decoded = found->second;
                    return true;
                }
                if (!fname_to_utf8(name, decoded, error))
                {
                    return false;
                }
                names.emplace(key, decoded);
                return true;
            };

            result.objects.reserve(std::min<std::size_t>(limit, 64u));
            for (std::int32_t index = 0; index < registry.num_elements; ++index)
            {
                ++result.scanned_objects;
                ObjectItemView item{};
                UObjectBaseView object{};
                if (!read_stable_object(registry, index, item, object) ||
                    (object.object_flags & k_find_excluded_object_flags) != 0u)
                {
                    continue;
                }

                bool matches{};
                bool is_class_object{};
                std::uint64_t current_class = object.class_private;
                for (std::size_t depth = 0; current_class != 0u && depth < k_max_super_depth; ++depth)
                {
                    UObjectBaseView class_object{};
                    std::string class_name;
                    std::string error;
                    if (!safe_read(current_class, class_object) || class_object.vtable == 0u ||
                        !decode_name(class_object.name_private, class_name, error))
                    {
                        result.detail = "class hierarchy became unreadable at object index " + std::to_string(index) +
                                        (error.empty() ? std::string{} : ": " + error);
                        return result;
                    }
                    matches = matches || class_name == short_class_name;
                    is_class_object = is_class_object || class_name == "Class";
                    if (!safe_read(current_class + k_super_struct_offset, current_class))
                    {
                        result.detail = "UStruct::SuperStruct is unreadable at object index " + std::to_string(index);
                        return result;
                    }
                }

                if (matches && !is_class_object)
                {
                    result.objects.push_back({item.object, index, item.serial_number});
                    if (result.objects.size() >= limit)
                    {
                        result.truncated = index + 1 < registry.num_elements;
                        break;
                    }
                }
            }
            result.success = true;
            result.detail = "FindAllOf scanned " + std::to_string(result.scanned_objects) + " object slots and returned " +
                            std::to_string(result.objects.size()) + (result.truncated ? " (limited)" : "");
            return result;
        }
        catch (const std::exception& exception)
        {
            result.detail = exception.what();
            return result;
        }
        catch (...)
        {
            result.detail = "unknown exception while enumerating GUObjectArray";
            return result;
        }
    }

    ObjectQueryResult ReadOnlyUnrealRuntime::find_by_path(std::string_view path_name,
                                                          std::size_t limit) const noexcept
    {
        ObjectQueryResult result;
        try
        {
            if (!is_ready() || path_name.empty() || limit == 0u)
            {
                result.detail = "read-only runtime, object path, and positive result limit are required";
                return result;
            }
            ChunkedObjectArrayView registry{};
            if (!read_registry(m_guobject_array, registry))
            {
                result.detail = "GUObjectArray is not readable while executing StaticFindObject fallback";
                return result;
            }
            const std::size_t delimiter = path_name.find_last_of(".:");
            const std::string_view short_name = delimiter == std::string_view::npos ? path_name : path_name.substr(delimiter + 1u);

            for (std::int32_t index = 0; index < registry.num_elements; ++index)
            {
                ++result.scanned_objects;
                ObjectItemView item{};
                UObjectBaseView object{};
                if (!read_stable_object(registry, index, item, object))
                {
                    continue;
                }
                std::string name;
                std::string error;
                if (!fname_to_utf8(object.name_private, name, error))
                {
                    result.detail = "object name could not be decoded at index " + std::to_string(index) + ": " + error;
                    return result;
                }
                if (name != short_name)
                {
                    continue;
                }
                const ReadOnlyUObjectHandle handle{item.object, index, item.serial_number};
                ReadOnlyUObjectDescription description;
                if (!describe_object(handle, description, error))
                {
                    result.detail = "candidate object path could not be described: " + error;
                    return result;
                }
                if (description.path_name == path_name)
                {
                    result.objects.push_back(handle);
                    if (result.objects.size() >= limit)
                    {
                        result.truncated = index + 1 < registry.num_elements;
                        break;
                    }
                }
            }
            result.success = true;
            result.detail = "path scan inspected " + std::to_string(result.scanned_objects) + " object slots and returned " +
                            std::to_string(result.objects.size()) + (result.truncated ? " (limited)" : "");
            return result;
        }
        catch (const std::exception& exception)
        {
            result.detail = exception.what();
            return result;
        }
        catch (...)
        {
            result.detail = "unknown exception while scanning UObject paths";
            return result;
        }
    }

    bool ReadOnlyUnrealRuntime::read_enum_names_at_offset(
            const ReadOnlyUObjectHandle& unreal_enum,
            std::uint64_t names_offset,
            std::vector<UnrealEnumEntry>& output,
            std::string& error) const noexcept
    {
        output.clear();
        error.clear();
        try
        {
            std::string class_error;
            if (!is_valid(unreal_enum) || !is_a(unreal_enum, "Enum", class_error))
            {
                error = class_error.empty() ? "UEnum operation requires a live UEnum" : class_error;
                return false;
            }
            FScriptArrayView names{};
            if (names_offset < k_ue51_uenum_names_scan_begin ||
                names_offset > k_ue51_uenum_names_scan_end || names_offset % alignof(void*) != 0u ||
                !safe_read(unreal_enum.address + names_offset, names) ||
                names.num < 0 || names.max < names.num || names.max > k_max_enum_entries ||
                (names.max == 0 && names.data != 0u) || (names.max > 0 && names.data == 0u))
            {
                error = "UE 5.1 UEnum::Names has an invalid TArray header";
                return false;
            }
            if (names.num > 0 &&
                names.data > std::numeric_limits<std::uint64_t>::max() -
                                     static_cast<std::uint64_t>(names.num) * sizeof(FEnumNamePairView))
            {
                error = "UE 5.1 UEnum::Names address range overflows";
                return false;
            }
            output.reserve(static_cast<std::size_t>(names.num));
            std::unordered_set<std::uint64_t> seen_names;
            for (std::int32_t index = 0; index < names.num; ++index)
            {
                FEnumNamePairView pair{};
                const std::uint64_t pair_address = names.data +
                                                   static_cast<std::uint64_t>(index) *
                                                           sizeof(FEnumNamePairView);
                std::string text;
                if (!safe_read(pair_address, pair) || !fname_to_utf8(pair.name, text, error))
                {
                    error = error.empty() ? "UE 5.1 UEnum::Names contains an unreadable FName" : error;
                    output.clear();
                    return false;
                }
                const std::uint64_t name_key = (static_cast<std::uint64_t>(pair.name.number) << 32u) |
                                               pair.name.comparison_index;
                if (!seen_names.emplace(name_key).second)
                {
                    error = "UE 5.1 UEnum::Names contains duplicate FNames";
                    output.clear();
                    return false;
                }
                output.push_back({pair.name, std::move(text), pair.value});
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            output.clear();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while enumerating UEnum::Names";
            output.clear();
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::initialize_uenum_abi(std::string& detail) noexcept
    {
        detail.clear();
        m_uenum_names_offset = 0u;
        try
        {
            const ObjectQueryResult enums = find_all_of("Enum", 32u);
            if (!enums.success || enums.objects.empty())
            {
                detail = enums.detail.empty() ? "no live UEnum samples were found" : enums.detail;
                return false;
            }
            struct Candidate
            {
                std::uint64_t offset{};
                std::size_t successes{};
            };
            std::vector<Candidate> candidates;
            for (std::uint64_t offset = k_ue51_uenum_names_scan_begin;
                 offset <= k_ue51_uenum_names_scan_end;
                 offset += sizeof(void*))
            {
                Candidate candidate{.offset = offset};
                for (const auto& unreal_enum : enums.objects)
                {
                    std::int32_t entry_count{};
                    if (probe_enum_names_header(unreal_enum.address, offset, entry_count))
                    {
                        ++candidate.successes;
                    }
                }
                candidates.push_back(candidate);
            }
            std::ranges::sort(candidates, [](const Candidate& left, const Candidate& right) {
                return left.successes > right.successes;
            });
            const std::size_t required = std::min<std::size_t>(4u, enums.objects.size());
            if (candidates.empty() || candidates.front().successes < required ||
                (candidates.size() > 1u && candidates[1].successes == candidates.front().successes))
            {
                detail = "UE 5.1 Linux UEnum::Names scan did not yield one unique layout candidate; scores=";
                for (const auto& candidate : candidates)
                {
                    if (candidate.successes == 0u) continue;
                    std::array<char, 48> score{};
                    std::snprintf(score.data(),
                                  score.size(),
                                  "+0x%llx:%zu ",
                                  static_cast<unsigned long long>(candidate.offset),
                                  candidate.successes);
                    detail += score.data();
                }
                if (detail.ends_with("scores=")) detail += "none";
                return false;
            }
            m_uenum_names_offset = candidates.front().offset;
            const std::size_t behavior_samples = std::min<std::size_t>(4u, candidates.front().successes);
            std::size_t validated_samples{};
            for (const auto& unreal_enum : enums.objects)
            {
                std::int32_t entry_count{};
                if (!probe_enum_names_header(unreal_enum.address, m_uenum_names_offset, entry_count)) continue;
                std::vector<UnrealEnumEntry> entries;
                std::string validation_error;
                if (!read_enum_names_at_offset(
                            unreal_enum, m_uenum_names_offset, entries, validation_error) ||
                    entries.empty())
                {
                    m_uenum_names_offset = 0u;
                    detail = "selected UEnum::Names candidate failed bounded FName decoding: " +
                             validation_error;
                    return false;
                }
                if (++validated_samples >= behavior_samples) break;
            }
            std::array<char, 192> message{};
            std::snprintf(message.data(),
                          message.size(),
                          "UE 5.1 Linux UEnum::Names validated at +0x%llx across %zu/%zu live enums",
                          static_cast<unsigned long long>(m_uenum_names_offset),
                          candidates.front().successes,
                          enums.objects.size());
            detail = message.data();
            return true;
        }
        catch (const std::exception& exception)
        {
            detail = exception.what();
            return false;
        }
        catch (...)
        {
            detail = "unknown exception while validating the Linux UEnum ABI";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::uenum_abi_available() const noexcept
    {
        return m_uenum_names_offset != 0u;
    }

    bool ReadOnlyUnrealRuntime::enumerate_enum_names(
            const ReadOnlyUObjectHandle& unreal_enum,
            std::vector<UnrealEnumEntry>& output,
            std::string& error) const noexcept
    {
        if (!uenum_abi_available())
        {
            output.clear();
            error = "Linux UEnum ABI gate has not passed";
            return false;
        }
        return read_enum_names_at_offset(unreal_enum, m_uenum_names_offset, output, error);
    }

    bool ReadOnlyUnrealRuntime::edit_enum_name(
            const ReadOnlyUObjectHandle& unreal_enum,
            std::int32_t index,
            RawFName new_name,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            std::vector<UnrealEnumEntry> entries;
            std::string decoded;
            FScriptArrayView names{};
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "UEnum mutation is permitted only inside a validated game-thread callback";
                return false;
            }
            if (!enumerate_enum_names(unreal_enum, entries, error) ||
                index < 0 || static_cast<std::size_t>(index) >= entries.size() ||
                !fname_to_utf8(new_name, decoded, error) || decoded.empty() ||
                !safe_read(unreal_enum.address + m_uenum_names_offset, names))
            {
                error = error.empty() ? "UEnum EditNameAt index or FName is invalid" : error;
                return false;
            }
            return safe_write_bytes(
                           names.data + static_cast<std::uint64_t>(index) *
                                                sizeof(FEnumNamePairView),
                           &new_name,
                           sizeof(new_name)) ||
                   (error = "UEnum name storage is not writable", false);
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while editing a UEnum name";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::edit_enum_value(
            const ReadOnlyUObjectHandle& unreal_enum,
            std::int32_t index,
            std::int64_t new_value,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            std::vector<UnrealEnumEntry> entries;
            FScriptArrayView names{};
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "UEnum mutation is permitted only inside a validated game-thread callback";
                return false;
            }
            if (!enumerate_enum_names(unreal_enum, entries, error) ||
                index < 0 || static_cast<std::size_t>(index) >= entries.size() ||
                !safe_read(unreal_enum.address + m_uenum_names_offset, names))
            {
                error = error.empty() ? "UEnum EditValueAt index is invalid" : error;
                return false;
            }
            return safe_write_bytes(
                           names.data + static_cast<std::uint64_t>(index) *
                                                sizeof(FEnumNamePairView) +
                                   offsetof(FEnumNamePairView, value),
                           &new_value,
                           sizeof(new_value)) ||
                   (error = "UEnum value storage is not writable", false);
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while editing a UEnum value";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::insert_enum_name(
            const ReadOnlyUObjectHandle& unreal_enum,
            RawFName name,
            std::int64_t value,
            std::int32_t index,
            bool shift_values,
            const GameThreadScheduler& scheduler,
            std::int32_t& inserted_index,
            std::string& error) const noexcept
    {
        inserted_index = -1;
        error.clear();
        try
        {
            std::vector<UnrealEnumEntry> entries;
            std::string decoded;
            FScriptArrayView previous{};
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "UEnum mutation is permitted only inside a validated game-thread callback";
                return false;
            }
            if (!enumerate_enum_names(unreal_enum, entries, error) ||
                index < 0 || static_cast<std::size_t>(index) > entries.size() ||
                entries.size() >= static_cast<std::size_t>(k_max_enum_entries) ||
                !fname_to_utf8(name, decoded, error) || decoded.empty() ||
                !safe_read(unreal_enum.address + m_uenum_names_offset, previous))
            {
                error = error.empty() ? "UEnum InsertIntoNames arguments are invalid" : error;
                return false;
            }
            std::vector<FEnumNamePairView> replacement;
            replacement.reserve(entries.size() + 1u);
            for (std::size_t source = 0; source <= entries.size(); ++source)
            {
                if (source == static_cast<std::size_t>(index))
                {
                    replacement.push_back({name, value});
                }
                if (source < entries.size())
                {
                    std::int64_t existing_value = entries[source].value;
                    if (shift_values && value <= existing_value)
                    {
                        if (existing_value == std::numeric_limits<std::int64_t>::max())
                        {
                            error = "UEnum value shift would overflow int64";
                            return false;
                        }
                        ++existing_value;
                    }
                    replacement.push_back({entries[source].name, existing_value});
                }
            }
            void* allocation{};
            const std::size_t bytes = replacement.size() * sizeof(FEnumNamePairView);
            if (!allocate_unreal_storage(bytes, alignof(FEnumNamePairView), allocation, error) ||
                !safe_write_bytes(
                        std::bit_cast<std::uintptr_t>(allocation), replacement.data(), bytes))
            {
                if (allocation != nullptr) release_unreal_storage(allocation);
                error = error.empty() ? "new UEnum name storage is not writable" : error;
                return false;
            }
            const FScriptArrayView next{
                    .data = std::bit_cast<std::uintptr_t>(allocation),
                    .num = static_cast<std::int32_t>(replacement.size()),
                    .max = static_cast<std::int32_t>(replacement.size()),
            };
            if (!safe_write_bytes(
                        unreal_enum.address + m_uenum_names_offset, &next, sizeof(next)))
            {
                release_unreal_storage(allocation);
                error = "UEnum names header is not writable";
                return false;
            }
            if (previous.data != 0u)
            {
                release_unreal_storage(
                        std::bit_cast<void*>(static_cast<std::uintptr_t>(previous.data)));
            }
            inserted_index = index;
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while inserting a UEnum name";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::remove_enum_names(
            const ReadOnlyUObjectHandle& unreal_enum,
            std::int32_t index,
            std::int32_t count,
            bool allow_shrinking,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            std::vector<UnrealEnumEntry> entries;
            FScriptArrayView previous{};
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "UEnum mutation is permitted only inside a validated game-thread callback";
                return false;
            }
            if (!enumerate_enum_names(unreal_enum, entries, error) || index < 0 || count < 0 ||
                static_cast<std::size_t>(index) > entries.size() ||
                static_cast<std::size_t>(count) > entries.size() - static_cast<std::size_t>(index) ||
                !safe_read(unreal_enum.address + m_uenum_names_offset, previous))
            {
                error = error.empty() ? "UEnum RemoveFromNamesAt range is invalid" : error;
                return false;
            }
            if (count == 0) return true;
            std::vector<FEnumNamePairView> replacement;
            replacement.reserve(entries.size() - static_cast<std::size_t>(count));
            for (std::size_t source = 0; source < entries.size(); ++source)
            {
                if (source < static_cast<std::size_t>(index) ||
                    source >= static_cast<std::size_t>(index + count))
                {
                    replacement.push_back({entries[source].name, entries[source].value});
                }
            }
            if (!allow_shrinking)
            {
                if (!replacement.empty() &&
                    !safe_write_bytes(previous.data,
                                      replacement.data(),
                                      replacement.size() * sizeof(FEnumNamePairView)))
                {
                    error = "UEnum compacted name storage is not writable";
                    return false;
                }
                const std::int32_t next_num = static_cast<std::int32_t>(replacement.size());
                return safe_write_bytes(
                               unreal_enum.address + m_uenum_names_offset +
                                       offsetof(FScriptArrayView, num),
                               &next_num,
                               sizeof(next_num)) ||
                       (error = "UEnum names Num is not writable", false);
            }
            FScriptArrayView next{};
            void* allocation{};
            if (!replacement.empty())
            {
                const std::size_t bytes = replacement.size() * sizeof(FEnumNamePairView);
                if (!allocate_unreal_storage(
                            bytes, alignof(FEnumNamePairView), allocation, error) ||
                    !safe_write_bytes(
                            std::bit_cast<std::uintptr_t>(allocation),
                            replacement.data(),
                            bytes))
                {
                    if (allocation != nullptr) release_unreal_storage(allocation);
                    error = error.empty() ? "new compacted UEnum storage is not writable" : error;
                    return false;
                }
                next = {
                        .data = std::bit_cast<std::uintptr_t>(allocation),
                        .num = static_cast<std::int32_t>(replacement.size()),
                        .max = static_cast<std::int32_t>(replacement.size()),
                };
            }
            if (!safe_write_bytes(
                        unreal_enum.address + m_uenum_names_offset, &next, sizeof(next)))
            {
                if (allocation != nullptr) release_unreal_storage(allocation);
                error = "UEnum names header is not writable";
                return false;
            }
            if (previous.data != 0u)
            {
                release_unreal_storage(
                        std::bit_cast<void*>(static_cast<std::uintptr_t>(previous.data)));
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while removing UEnum names";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::enumerate_properties(
            const ReadOnlyUObjectHandle& unreal_struct,
            std::vector<ReflectedPropertyDescription>& output,
            std::string& error,
            bool include_super) const noexcept
    {
        output.clear();
        error.clear();
        try
        {
            std::string class_error;
            const bool is_struct = is_valid(unreal_struct) &&
                                   is_a(unreal_struct, "Struct", class_error);
            class_error.clear();
            const bool is_class = is_valid(unreal_struct) &&
                                  is_a(unreal_struct, "Class", class_error);
            if (!is_struct && !is_class)
            {
                error = class_error.empty() ? "ForEachProperty requires a live UStruct/UClass" : class_error;
                return false;
            }
            std::unordered_set<std::uint64_t> visited_structs;
            std::unordered_set<std::uint64_t> visited_fields;
            std::uint64_t current = unreal_struct.address;
            for (std::size_t depth = 0; current != 0u && depth < k_max_super_depth; ++depth)
            {
                if (!visited_structs.emplace(current).second)
                {
                    error = "UStruct hierarchy is cyclic while enumerating properties";
                    output.clear();
                    return false;
                }
                std::uint64_t field_address{};
                std::int32_t properties_size{};
                if (!safe_read(current + k_properties_size_offset, properties_size) ||
                    properties_size < 0 || properties_size > k_max_properties_size ||
                    !safe_read(current + k_child_properties_offset, field_address))
                {
                    error = "UStruct property size or ChildProperties is invalid while enumerating properties";
                    output.clear();
                    return false;
                }
                while (field_address != 0u)
                {
                    if (visited_fields.size() >= k_max_property_fields ||
                        !visited_fields.emplace(field_address).second)
                    {
                        error = "FProperty chain is cyclic or exceeds the safety limit";
                        output.clear();
                        return false;
                    }
                    FPropertyPrefixView property{};
                    ReflectedPropertyDescription description;
                    if (!safe_read(field_address, property) ||
                        !populate_property_description(
                                *this, field_address, current, description, error))
                    {
                        error = error.empty() ? "FProperty enumeration metadata is invalid" : error;
                        output.clear();
                        return false;
                    }
                    const std::int64_t total_size =
                            static_cast<std::int64_t>(description.element_size) *
                            static_cast<std::int64_t>(description.array_dim);
                    if (total_size <= 0 || description.offset > properties_size - total_size)
                    {
                        error = "FProperty enumeration found storage outside UStruct::PropertiesSize";
                        output.clear();
                        return false;
                    }
                    output.push_back(std::move(description));
                    field_address = property.next;
                }
                if (!include_super)
                {
                    return true;
                }
                if (!safe_read(current + k_super_struct_offset, current))
                {
                    error = "UStruct::SuperStruct is unreadable while enumerating properties";
                    output.clear();
                    return false;
                }
            }
            if (current != 0u)
            {
                error = "UStruct hierarchy exceeds the safety depth limit";
                output.clear();
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            output.clear();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while enumerating reflected properties";
            output.clear();
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::describe_property(
            std::uint64_t address,
            ReflectedPropertyDescription& output,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            return populate_property_description(*this, address, 0u, output, error);
        }
        catch (const std::exception& exception)
        {
            output = {};
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output = {};
            error = "unknown exception while describing an FProperty";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::container_ptr_to_value_ptr(
            const ReflectedPropertyDescription& property,
            const ReadOnlyUObjectHandle& container,
            std::int32_t array_index,
            std::uint64_t& output,
            std::string& error) const noexcept
    {
        output = 0u;
        error.clear();
        try
        {
            if (!is_valid(container) || property.address == 0u || array_index < 0)
            {
                error = "ContainerPtrToValuePtr requires live FProperty/UObject values and a non-negative array index";
                return false;
            }
            ReflectedPropertyDescription current_property;
            if (!describe_property(property.address, current_property, error) ||
                current_property.name != property.name ||
                current_property.type_name != property.type_name ||
                current_property.offset != property.offset ||
                current_property.element_size != property.element_size)
            {
                error = error.empty() ? "FProperty metadata changed after it was exposed to Lua" : error;
                return false;
            }
            if (array_index >= current_property.array_dim)
            {
                error = "ContainerPtrToValuePtr array index exceeds FProperty::ArrayDim";
                return false;
            }

            ReadOnlyUObjectDescription container_description;
            ReadOnlyUObjectHandle container_class;
            if (!describe_object(container, container_description, error) ||
                !handle_from_address(container_description.class_address, container_class))
            {
                error = error.empty() ? "UObject class is not present in the validated registry" : error;
                return false;
            }
            std::vector<ReflectedPropertyDescription> properties;
            if (!enumerate_properties(container_class, properties, error, true) ||
                std::none_of(properties.begin(), properties.end(), [&](const auto& candidate) {
                    return candidate.address == property.address;
                }))
            {
                error = error.empty()
                                ? "FProperty does not belong to the UObject class hierarchy"
                                : error;
                return false;
            }

            const std::uint64_t element_offset =
                    static_cast<std::uint64_t>(array_index) *
                    static_cast<std::uint64_t>(current_property.element_size);
            const std::uint64_t base_offset =
                    static_cast<std::uint64_t>(current_property.offset);
            if (container.address > std::numeric_limits<std::uint64_t>::max() - base_offset ||
                container.address + base_offset >
                        std::numeric_limits<std::uint64_t>::max() - element_offset)
            {
                error = "ContainerPtrToValuePtr address calculation overflowed";
                return false;
            }
            output = container.address + base_offset + element_offset;
            std::vector<std::byte> probe(static_cast<std::size_t>(current_property.element_size));
            if (!safe_read_bytes(output, probe.data(), probe.size()))
            {
                output = 0u;
                error = "ContainerPtrToValuePtr target storage is not fully readable";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            output = 0u;
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output = 0u;
            error = "unknown exception while resolving an FProperty container pointer";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::bind_struct_property(
            const ReadOnlyUObjectHandle& owner,
            std::string_view property_name,
            UnrealStructBinding& output,
            std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            if (!is_valid(owner) || property_name.empty())
            {
                error = "a live UObject and non-empty StructProperty name are required";
                return false;
            }
            UObjectBaseView object{};
            if (!safe_read(owner.address, object))
            {
                error = "UObject became unreadable while binding StructProperty storage";
                return false;
            }
            ReflectedProperty property;
            const auto decode_name = [this](RawFName name,
                                            std::string& decoded,
                                            std::string& decode_error) {
                return fname_to_utf8(name, decoded, decode_error);
            };
            if (!find_reflected_property(
                        object.class_private, property_name, decode_name, property, error))
            {
                return false;
            }
            if (property.type_name != "StructProperty" || property.struct_address == 0u)
            {
                error = "property '" + std::string{property_name} + "' is not a StructProperty";
                return false;
            }
            ReadOnlyUObjectHandle script_struct;
            if (!handle_from_address(property.struct_address, script_struct))
            {
                error = "StructProperty UScriptStruct is absent from the validated object registry";
                return false;
            }
            if (owner.address > std::numeric_limits<std::uint64_t>::max() -
                                        static_cast<std::uint64_t>(property.offset))
            {
                error = "StructProperty storage address overflowed";
                return false;
            }
            output = {
                    .script_struct = script_struct,
                    .owner = owner,
                    .storage_address = owner.address + static_cast<std::uint64_t>(property.offset),
                    .property_address = property.address,
            };
            if (!validate_struct_binding(output, error))
            {
                output = {};
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            output = {};
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output = {};
            error = "unknown exception while binding StructProperty storage";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::bind_struct_storage(
            const ReadOnlyUObjectHandle& script_struct,
            std::uint64_t storage_address,
            std::uint64_t property_address,
            UnrealStructBinding& output,
            std::string& error) const noexcept
    {
        output = {
                .script_struct = script_struct,
                .storage_address = storage_address,
                .property_address = property_address,
        };
        if (!validate_struct_binding(output, error))
        {
            output = {};
            return false;
        }
        return true;
    }

    bool ReadOnlyUnrealRuntime::bind_nested_struct_property(
            const UnrealStructBinding& parent,
            std::string_view property_name,
            UnrealStructBinding& output,
            std::string& error) const noexcept
    {
        output = {};
        if (!validate_struct_binding(parent, error)) return false;
        ReflectedProperty property;
        const auto decode_name = [this](RawFName name,
                                        std::string& decoded,
                                        std::string& decode_error) {
            return fname_to_utf8(name, decoded, decode_error);
        };
        if (!find_reflected_property(parent.script_struct.address,
                                     property_name,
                                     decode_name,
                                     property,
                                     error))
        {
            return false;
        }
        if (property.type_name != "StructProperty" || property.struct_address == 0u)
        {
            error = "UScriptStruct field '" + std::string{property_name} +
                    "' is not a StructProperty";
            return false;
        }
        ReadOnlyUObjectHandle nested_struct;
        if (!handle_from_address(property.struct_address, nested_struct) ||
            parent.storage_address > std::numeric_limits<std::uint64_t>::max() -
                                             static_cast<std::uint64_t>(property.offset))
        {
            error = "nested StructProperty metadata or storage address is invalid";
            return false;
        }
        return bind_struct_storage(
                nested_struct,
                parent.storage_address + static_cast<std::uint64_t>(property.offset),
                property.address,
                output,
                error);
    }

    bool ReadOnlyUnrealRuntime::validate_struct_binding(
            const UnrealStructBinding& binding,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            std::string type_error;
            if (!is_valid(binding.script_struct) ||
                !is_a(binding.script_struct, "ScriptStruct", type_error))
            {
                error = type_error.empty()
                                ? "struct binding requires a live UScriptStruct"
                                : type_error;
                return false;
            }
            if (binding.owner.address != 0u && !is_valid(binding.owner))
            {
                error = "struct binding owner is no longer a live UObject";
                return false;
            }
            std::int32_t properties_size{};
            if (binding.storage_address == 0u ||
                !safe_read(binding.script_struct.address + k_properties_size_offset,
                           properties_size) ||
                properties_size <= 0 || properties_size > k_max_properties_size ||
                binding.storage_address > std::numeric_limits<std::uint64_t>::max() -
                                                  static_cast<std::uint64_t>(properties_size - 1))
            {
                error = "UScriptStruct storage or PropertiesSize is invalid";
                return false;
            }
            std::byte first{};
            std::byte last{};
            if (!safe_read(binding.storage_address, first) ||
                !safe_read(binding.storage_address +
                                   static_cast<std::uint64_t>(properties_size - 1),
                           last))
            {
                error = "UScriptStruct storage span is not fully readable";
                return false;
            }
            if (binding.property_address != 0u)
            {
                ReflectedPropertyDescription property;
                if (!describe_property(binding.property_address, property, error) ||
                    property.type_name != "StructProperty" ||
                    property.struct_address != binding.script_struct.address)
                {
                    error = error.empty()
                                    ? "struct binding FStructProperty metadata no longer matches"
                                    : error;
                    return false;
                }
                if (binding.owner.address != 0u &&
                    binding.owner.address + static_cast<std::uint64_t>(property.offset) !=
                            binding.storage_address)
                {
                    error = "struct binding storage no longer matches its UObject property";
                    return false;
                }
            }
            return validate_plain_struct(*this, binding.script_struct.address, error);
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while validating UScriptStruct storage";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::read_struct_value(
            const UnrealStructBinding& binding,
            UnrealPropertyValue& output,
            std::string& error) const noexcept
    {
        output = {};
        if (!validate_struct_binding(binding, error)) return false;
        std::int32_t properties_size{};
        if (!safe_read(binding.script_struct.address + k_properties_size_offset,
                       properties_size))
        {
            error = "UScriptStruct::PropertiesSize became unreadable";
            return false;
        }
        const ReflectedProperty property{
                .address = binding.property_address,
                .type_name = "StructProperty",
                .element_size = properties_size,
                .struct_address = binding.script_struct.address,
        };
        return decode_plain_value(
                *this, property, binding.storage_address, output, error);
    }

    bool ReadOnlyUnrealRuntime::read_struct_property(
            const UnrealStructBinding& binding,
            std::string_view property_name,
            UnrealPropertyValue& output,
            std::string& error) const noexcept
    {
        output = {};
        if (!validate_struct_binding(binding, error)) return false;
        ReflectedProperty property;
        const auto decode_name = [this](RawFName name,
                                        std::string& decoded,
                                        std::string& decode_error) {
            return fname_to_utf8(name, decoded, decode_error);
        };
        if (!find_reflected_property(binding.script_struct.address,
                                     property_name,
                                     decode_name,
                                     property,
                                     error))
        {
            return false;
        }
        if (binding.storage_address > std::numeric_limits<std::uint64_t>::max() -
                                              static_cast<std::uint64_t>(property.offset))
        {
            error = "UScriptStruct field address overflowed";
            return false;
        }
        return decode_plain_value(
                *this,
                property,
                binding.storage_address + static_cast<std::uint64_t>(property.offset),
                output,
                error);
    }

    bool ReadOnlyUnrealRuntime::write_struct_property(
            const UnrealStructBinding& binding,
            std::string_view property_name,
            const UnrealPropertyValue& value,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        if (!scheduler.is_ready() || !scheduler.is_game_thread())
        {
            error = "UScriptStruct writes are permitted only inside a validated game-thread callback";
            return false;
        }
        if (!validate_struct_binding(binding, error)) return false;
        ReflectedProperty property;
        const auto decode_name = [this](RawFName name,
                                        std::string& decoded,
                                        std::string& decode_error) {
            return fname_to_utf8(name, decoded, decode_error);
        };
        if (!find_reflected_property(binding.script_struct.address,
                                     property_name,
                                     decode_name,
                                     property,
                                     error))
        {
            return false;
        }
        if (binding.storage_address > std::numeric_limits<std::uint64_t>::max() -
                                              static_cast<std::uint64_t>(property.offset))
        {
            error = "UScriptStruct field address overflowed";
            return false;
        }
        return encode_plain_value(
                *this,
                property,
                binding.storage_address + static_cast<std::uint64_t>(property.offset),
                value,
                error);
    }

    bool ReadOnlyUnrealRuntime::enumerate_data_table_rows(
            const ReadOnlyUObjectHandle& data_table,
            ReadOnlyUObjectHandle& row_struct,
            std::vector<UnrealDataTableRow>& rows,
            std::string& error) const noexcept
    {
        row_struct = {};
        rows.clear();
        error.clear();
        try
        {
            std::string type_error;
            if (!is_valid(data_table) || !is_a(data_table, "DataTable", type_error))
            {
                error = type_error.empty()
                                ? "UDataTable operation requires a live UDataTable"
                                : type_error;
                return false;
            }
            std::uint64_t row_struct_address{};
            FScriptSetView row_map{};
            if (!safe_read(data_table.address + k_ue51_data_table_row_struct_offset,
                           row_struct_address) ||
                row_struct_address == 0u ||
                !handle_from_address(row_struct_address, row_struct) ||
                !is_a(row_struct, "ScriptStruct", type_error))
            {
                row_struct = {};
                error = type_error.empty()
                                ? "UDataTable RowStruct is null or not a live UScriptStruct"
                                : type_error;
                return false;
            }
            if (!safe_read(data_table.address + k_ue51_data_table_row_map_offset,
                           row_map) ||
                !validate_script_set_view(row_map, error))
            {
                row_struct = {};
                error = error.empty()
                                ? "UDataTable RowMap is unreadable"
                                : error;
                return false;
            }
            UnrealScriptMapLayout layout;
            if (!calculate_script_map_layout(
                        sizeof(RawFName),
                        alignof(RawFName),
                        sizeof(std::uint64_t),
                        alignof(std::uint64_t),
                        layout,
                        error) ||
                layout.value_offset <= 0 ||
                layout.set.sparse_stride <= layout.value_offset ||
                layout.set.sparse_stride > k_max_property_element_size)
            {
                row_struct = {};
                error = error.empty()
                                ? "UEPseudo returned an invalid DataTable RowMap layout"
                                : error;
                return false;
            }

            rows.reserve(static_cast<std::size_t>(
                    row_map.elements.num - row_map.num_free_indices));
            std::unordered_set<std::string> unique_names;
            std::int32_t allocated_count{};
            for (std::int32_t index = 0; index < row_map.elements.num; ++index)
            {
                bool allocated{};
                if (!script_set_index_is_allocated(
                            row_map, index, allocated, error))
                {
                    rows.clear();
                    row_struct = {};
                    return false;
                }
                if (!allocated) continue;
                ++allocated_count;
                const std::uint64_t offset = static_cast<std::uint64_t>(index) *
                                             static_cast<std::uint64_t>(layout.set.sparse_stride);
                if (row_map.elements.data >
                            std::numeric_limits<std::uint64_t>::max() - offset ||
                    row_map.elements.data + offset >
                            std::numeric_limits<std::uint64_t>::max() -
                                    static_cast<std::uint64_t>(layout.value_offset))
                {
                    error = "UDataTable RowMap pair address overflowed";
                    rows.clear();
                    row_struct = {};
                    return false;
                }
                const std::uint64_t pair_address = row_map.elements.data + offset;
                UnrealDataTableRow row;
                std::uint64_t storage_address{};
                if (!safe_read(pair_address, row.name) ||
                    row.name.comparison_index == 0u ||
                    !fname_to_utf8(row.name, row.name_text, error) ||
                    row.name_text.empty() ||
                    !safe_read(pair_address +
                                       static_cast<std::uint64_t>(layout.value_offset),
                               storage_address) ||
                    storage_address == 0u ||
                    !unique_names.emplace(row.name_text).second ||
                    !bind_struct_storage(
                            row_struct, storage_address, 0u, row.data, error))
                {
                    error = error.empty()
                                    ? "UDataTable RowMap contains an invalid or duplicate row"
                                    : error;
                    rows.clear();
                    row_struct = {};
                    return false;
                }
                rows.push_back(std::move(row));
            }
            if (allocated_count != row_map.elements.num - row_map.num_free_indices)
            {
                error = "UDataTable RowMap allocation flags disagree with NumFreeIndices";
                rows.clear();
                row_struct = {};
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            row_struct = {};
            rows.clear();
            error = exception.what();
            return false;
        }
        catch (...)
        {
            row_struct = {};
            rows.clear();
            error = "unknown exception while enumerating UDataTable rows";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::add_data_table_row(
            const ReadOnlyUObjectHandle& data_table,
            RawFName row_name,
            const UnrealPropertyValue& row_value,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "UDataTable:AddRow can only execute on the validated game thread";
                return false;
            }
            if (!m_fproperty_abi_ready)
            {
                error = "UDataTable:AddRow requires the validated Linux FProperty lifecycle ABI";
                return false;
            }
            std::string row_name_text;
            if (row_name.comparison_index == 0u ||
                !fname_to_utf8(row_name, row_name_text, error) || row_name_text.empty())
            {
                error = error.empty() ? "UDataTable:AddRow requires a non-None row name" : error;
                return false;
            }

            ReadOnlyUObjectHandle row_struct;
            std::vector<UnrealDataTableRow> current_rows;
            if (!enumerate_data_table_rows(data_table, row_struct, current_rows, error))
            {
                return false;
            }
            std::vector<ReflectedProperty> fields;
            std::int32_t properties_size{};
            if (!collect_data_table_row_fields(
                        *this, row_struct, fields, properties_size, error))
            {
                return false;
            }
            std::uint64_t new_storage{};
            if (!allocate_data_table_row_storage(
                        *this, fields, properties_size, row_value, new_storage, error))
            {
                return false;
            }

            std::vector<DataTableMapRow> replacement_rows;
            replacement_rows.reserve(current_rows.size() + 1u);
            std::uint64_t replaced_storage{};
            for (const auto& row : current_rows)
            {
                if (row.name.comparison_index == row_name.comparison_index &&
                    row.name.number == row_name.number)
                {
                    replaced_storage = row.data.storage_address;
                    continue;
                }
                replacement_rows.push_back({row.name, row.data.storage_address});
            }
            replacement_rows.push_back({row_name, new_storage});

            UnrealScriptMapLayout layout;
            FScriptSetView replacement{};
            if (!build_dense_data_table_map(
                        *this, replacement_rows, layout, replacement, error))
            {
                std::string cleanup_error;
                destroy_data_table_row_storage(*this, fields, new_storage, cleanup_error);
                return false;
            }
            const std::uint64_t map_address =
                    data_table.address + k_ue51_data_table_row_map_offset;
            FScriptSetView previous{};
            if (!safe_read(map_address, previous) ||
                !safe_write_bytes(map_address, &replacement, sizeof(replacement)))
            {
                release_raw_script_set_allocations(*this, replacement);
                std::string cleanup_error;
                destroy_data_table_row_storage(*this, fields, new_storage, cleanup_error);
                error = "UDataTable:AddRow could not atomically replace RowMap storage";
                return false;
            }
            release_raw_script_set_allocations(*this, previous);
            if (replaced_storage != 0u)
            {
                destroy_data_table_row_storage(*this, fields, replaced_storage, error);
            }
            return error.empty();
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while adding a UDataTable row";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::remove_data_table_row(
            const ReadOnlyUObjectHandle& data_table,
            RawFName row_name,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "UDataTable:RemoveRow can only execute on the validated game thread";
                return false;
            }
            if (!m_fproperty_abi_ready)
            {
                error = "UDataTable:RemoveRow requires the validated Linux FProperty lifecycle ABI";
                return false;
            }
            ReadOnlyUObjectHandle row_struct;
            std::vector<UnrealDataTableRow> current_rows;
            if (!enumerate_data_table_rows(data_table, row_struct, current_rows, error))
            {
                return false;
            }
            std::vector<DataTableMapRow> replacement_rows;
            replacement_rows.reserve(current_rows.size());
            std::uint64_t removed_storage{};
            for (const auto& row : current_rows)
            {
                if (row.name.comparison_index == row_name.comparison_index &&
                    row.name.number == row_name.number)
                {
                    removed_storage = row.data.storage_address;
                }
                else
                {
                    replacement_rows.push_back({row.name, row.data.storage_address});
                }
            }
            if (removed_storage == 0u) return true;

            std::vector<ReflectedProperty> fields;
            std::int32_t properties_size{};
            if (!collect_data_table_row_fields(
                        *this, row_struct, fields, properties_size, error))
            {
                return false;
            }
            UnrealScriptMapLayout layout;
            FScriptSetView replacement{};
            if (!build_dense_data_table_map(
                        *this, replacement_rows, layout, replacement, error))
            {
                return false;
            }
            const std::uint64_t map_address =
                    data_table.address + k_ue51_data_table_row_map_offset;
            FScriptSetView previous{};
            if (!safe_read(map_address, previous) ||
                !safe_write_bytes(map_address, &replacement, sizeof(replacement)))
            {
                release_raw_script_set_allocations(*this, replacement);
                error = "UDataTable:RemoveRow could not atomically replace RowMap storage";
                return false;
            }
            release_raw_script_set_allocations(*this, previous);
            destroy_data_table_row_storage(*this, fields, removed_storage, error);
            return error.empty();
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while removing a UDataTable row";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::empty_data_table(
            const ReadOnlyUObjectHandle& data_table,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "UDataTable:EmptyTable can only execute on the validated game thread";
                return false;
            }
            if (!m_fproperty_abi_ready)
            {
                error = "UDataTable:EmptyTable requires the validated Linux FProperty lifecycle ABI";
                return false;
            }
            ReadOnlyUObjectHandle row_struct;
            std::vector<UnrealDataTableRow> current_rows;
            if (!enumerate_data_table_rows(data_table, row_struct, current_rows, error))
            {
                return false;
            }
            std::vector<ReflectedProperty> fields;
            std::int32_t properties_size{};
            if (!collect_data_table_row_fields(
                        *this, row_struct, fields, properties_size, error))
            {
                return false;
            }
            UnrealScriptMapLayout layout;
            FScriptSetView replacement{};
            if (!build_dense_data_table_map(*this, {}, layout, replacement, error))
            {
                return false;
            }
            const std::uint64_t map_address =
                    data_table.address + k_ue51_data_table_row_map_offset;
            FScriptSetView previous{};
            if (!safe_read(map_address, previous) ||
                !safe_write_bytes(map_address, &replacement, sizeof(replacement)))
            {
                error = "UDataTable:EmptyTable could not atomically replace RowMap storage";
                return false;
            }
            release_raw_script_set_allocations(*this, previous);
            for (const auto& row : current_rows)
            {
                destroy_data_table_row_storage(
                        *this, fields, row.data.storage_address, error);
            }
            return error.empty();
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while emptying a UDataTable";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::get_super_struct(
            const ReadOnlyUObjectHandle& unreal_struct,
            ReadOnlyUObjectHandle& output,
            std::string& error) const noexcept
    {
        const ReadOnlyUObjectHandle input = unreal_struct;
        output = {};
        error.clear();
        try
        {
            std::string type_error;
            const bool is_struct = is_valid(input) &&
                                   is_a(input, "Struct", type_error);
            type_error.clear();
            const bool is_class = is_valid(input) &&
                                  is_a(input, "Class", type_error);
            if (!is_struct && !is_class)
            {
                error = type_error.empty() ? "GetSuperStruct requires a live UStruct" : type_error;
                return false;
            }
            std::uint64_t address{};
            if (!safe_read(input.address + k_super_struct_offset, address))
            {
                error = "UStruct::SuperStruct is unreadable";
                return false;
            }
            if (address == 0u)
            {
                return true;
            }
            type_error.clear();
            const bool output_is_struct = handle_from_address(address, output) &&
                                          is_a(output, "Struct", type_error);
            type_error.clear();
            const bool output_is_class = output.address != 0u &&
                                         is_a(output, "Class", type_error);
            if (!output_is_struct && !output_is_class)
            {
                output = {};
                error = type_error.empty()
                                ? "UStruct::SuperStruct is not a live UStruct"
                                : type_error;
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while reading UStruct::SuperStruct";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::struct_is_child_of(
            const ReadOnlyUObjectHandle& unreal_struct,
            const ReadOnlyUObjectHandle& requested_parent,
            bool& output,
            std::string& error) const noexcept
    {
        output = false;
        error.clear();
        try
        {
            const auto is_struct_or_class = [this](const ReadOnlyUObjectHandle& candidate,
                                                   std::string& candidate_error) {
                const bool is_struct = is_valid(candidate) &&
                                       is_a(candidate, "Struct", candidate_error);
                candidate_error.clear();
                const bool is_class = is_valid(candidate) &&
                                      is_a(candidate, "Class", candidate_error);
                return is_struct || is_class;
            };
            std::string type_error;
            const bool child_is_struct = is_struct_or_class(unreal_struct, type_error);
            type_error.clear();
            const bool parent_is_struct = is_struct_or_class(requested_parent, type_error);
            if (!child_is_struct || !parent_is_struct)
            {
                error = type_error.empty() ? "IsChildOf requires two live UStruct values" : type_error;
                return false;
            }
            std::unordered_set<std::uint64_t> visited;
            std::uint64_t current = unreal_struct.address;
            for (std::size_t depth = 0; current != 0u && depth < k_max_super_depth; ++depth)
            {
                if (current == requested_parent.address)
                {
                    output = true;
                    return true;
                }
                if (!visited.emplace(current).second)
                {
                    error = "UStruct hierarchy is cyclic while checking IsChildOf";
                    return false;
                }
                if (!safe_read(current + k_super_struct_offset, current))
                {
                    error = "UStruct::SuperStruct became unreadable while checking IsChildOf";
                    return false;
                }
            }
            if (current != 0u)
            {
                error = "UStruct hierarchy exceeds the IsChildOf safety limit";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while checking UStruct::IsChildOf";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::enumerate_functions(
            const ReadOnlyUObjectHandle& unreal_struct,
            std::vector<ReadOnlyUObjectHandle>& output,
            std::string& error) const noexcept
    {
        output.clear();
        error.clear();
        try
        {
            std::string type_error;
            const bool is_struct = is_valid(unreal_struct) &&
                                   is_a(unreal_struct, "Struct", type_error);
            type_error.clear();
            const bool is_class = is_valid(unreal_struct) &&
                                  is_a(unreal_struct, "Class", type_error);
            if (!is_struct && !is_class)
            {
                error = type_error.empty() ? "ForEachFunction requires a live UStruct" : type_error;
                return false;
            }
            std::uint64_t field_address{};
            if (!safe_read(unreal_struct.address + k_children_offset, field_address))
            {
                error = "UStruct::Children is unreadable while enumerating functions";
                return false;
            }
            std::unordered_set<std::uint64_t> visited;
            while (field_address != 0u)
            {
                if (visited.size() >= k_max_property_fields ||
                    !visited.emplace(field_address).second)
                {
                    output.clear();
                    error = "UField child chain is cyclic or exceeds the safety limit";
                    return false;
                }
                ReadOnlyUObjectHandle child;
                std::uint64_t next{};
                if (!handle_from_address(field_address, child) ||
                    !safe_read(field_address + 0x28u, next))
                {
                    output.clear();
                    error = "UField child metadata became unreadable while enumerating functions";
                    return false;
                }
                std::string class_error;
                if (is_a(child, "Function", class_error))
                {
                    output.push_back(child);
                }
                else if (!class_error.empty())
                {
                    output.clear();
                    error = std::move(class_error);
                    return false;
                }
                field_address = next;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            output.clear();
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output.clear();
            error = "unknown exception while enumerating UStruct functions";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::get_function_flags(
            const ReadOnlyUObjectHandle& function,
            std::uint32_t& output,
            std::string& error) const noexcept
    {
        output = 0u;
        error.clear();
        try
        {
            std::string type_error;
            if (!is_valid(function) || !is_a(function, "Function", type_error))
            {
                error = type_error.empty() ? "GetFunctionFlags requires a live UFunction" : type_error;
                return false;
            }
            if (!safe_read(function.address + k_ue51_function_flags_offset, output))
            {
                error = "UFunction::FunctionFlags is unreadable at the validated UE 5.1 offset";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while reading UFunction::FunctionFlags";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::set_function_flags(
            const ReadOnlyUObjectHandle& function,
            std::uint32_t value,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            std::uint32_t original{};
            if (!get_function_flags(function, original, error))
            {
                return false;
            }
            const std::uint64_t address = function.address + k_ue51_function_flags_offset;
            if (!safe_write_bytes(address, &value, sizeof(value)))
            {
                error = "UFunction::FunctionFlags is not writable";
                return false;
            }
            std::uint32_t observed{};
            if (!safe_read(address, observed) || observed != value)
            {
                const bool rolled_back = safe_write_bytes(address, &original, sizeof(original));
                error = rolled_back
                                ? "UFunction::FunctionFlags write verification failed and was rolled back"
                                : "UFunction::FunctionFlags write verification and rollback both failed";
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while writing UFunction::FunctionFlags";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::find_game_engine_tick_slot(std::uint64_t expected_tick_address,
                                                           void**& output,
                                                           std::string& error) const noexcept
    {
        output = nullptr;
        error.clear();
        try
        {
            if (!is_ready() || expected_tick_address == 0u || !is_executable_mapping(expected_tick_address))
            {
                error = "validated read-only runtime and executable UGameEngine::Tick address are required";
                return false;
            }
            const ObjectQueryResult engines = find_all_of("GameEngine", 1u);
            if (!engines.success || engines.objects.empty())
            {
                error = engines.detail.empty() ? "no live UGameEngine instance was found" : engines.detail;
                return false;
            }
            UObjectBaseView engine{};
            if (!safe_read(engines.objects.front().address, engine) || engine.vtable == 0u)
            {
                error = "the live UGameEngine instance has no readable vtable";
                return false;
            }
            std::uint64_t matching_slot{};
            std::size_t matches{};
            std::uint64_t windows_slot_value{};
            std::uint64_t linux_slot_value{};
            for (std::uint64_t offset = 0u; offset < k_ue51_engine_vtable_scan_limit; offset += sizeof(void*))
            {
                const std::uint64_t candidate = engine.vtable + offset;
                std::uint64_t value{};
                if ((candidate % alignof(void*)) != 0u || !safe_read(candidate, value))
                {
                    error = "a candidate UE 5.1 UGameEngine::Tick vtable slot is unreadable or unaligned";
                    return false;
                }
                if (offset == 0x2f0u)
                {
                    windows_slot_value = value;
                }
                else if (offset == 0x2f8u)
                {
                    linux_slot_value = value;
                }
                if (value == expected_tick_address)
                {
                    matching_slot = candidate;
                    ++matches;
                }
            }
            if (matches != 1u)
            {
                std::array<char, 192> detail{};
                std::snprintf(detail.data(),
                              detail.size(),
                              "expected one UE 5.1 UGameEngine::Tick vtable match, found %zu (MSVC=0x%llx Linux+8=0x%llx resolver=0x%llx)",
                              matches,
                              static_cast<unsigned long long>(windows_slot_value),
                              static_cast<unsigned long long>(linux_slot_value),
                              static_cast<unsigned long long>(expected_tick_address));
                error = detail.data();
                return false;
            }
            output = reinterpret_cast<void**>(static_cast<std::uintptr_t>(matching_slot));
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while validating the UGameEngine::Tick vtable slot";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::read_property(const ReadOnlyUObjectHandle& handle,
                                              std::string_view property_name,
                                              UnrealPropertyValue& output,
                                              std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            if (!is_valid(handle))
            {
                error = "a live UObject handle is required for property access";
                return false;
            }
            UObjectBaseView object{};
            if (!safe_read(handle.address, object))
            {
                error = "UObject became unreadable before property access";
                return false;
            }
            ReflectedProperty property;
            const auto decode_name = [this](RawFName name, std::string& decoded, std::string& decode_error) {
                return fname_to_utf8(name, decoded, decode_error);
            };
            if (!find_reflected_property(object.class_private, property_name, decode_name, property, error))
            {
                return false;
            }
            const std::uint64_t value_address = handle.address + static_cast<std::uint64_t>(property.offset);
            if (value_address < handle.address)
            {
                error = "property address overflow";
                return false;
            }

            if (is_supported_scalar_property(property))
            {
                return decode_plain_value(*this, property, value_address, output, error);
            }

            if (property.type_name == "BoolProperty")
            {
                std::uint8_t byte{};
                if (!safe_read(value_address + property.boolean.byte_offset, byte))
                {
                    error = "FBoolProperty storage is unreadable";
                    return false;
                }
                output.kind = UnrealPropertyKind::boolean;
                output.boolean = (byte & property.boolean.field_mask) != 0u;
                return true;
            }
            if (is_signed_integer_property(property.type_name) || is_unsigned_integer_property(property.type_name))
            {
                if (property.element_size != 1 && property.element_size != 2 && property.element_size != 4 &&
                    property.element_size != 8)
                {
                    error = "numeric property has an unsupported element size";
                    return false;
                }
                std::uint64_t raw{};
                if (!safe_read_bytes(value_address, &raw, static_cast<std::size_t>(property.element_size)))
                {
                    error = "numeric property storage is unreadable";
                    return false;
                }
                if (is_signed_integer_property(property.type_name))
                {
                    output.kind = UnrealPropertyKind::signed_integer;
                    output.signed_integer = sign_extend_integer(raw, property.element_size);
                }
                else
                {
                    output.kind = UnrealPropertyKind::unsigned_integer;
                    output.unsigned_integer = raw;
                }
                return true;
            }
            if (property.type_name == "FloatProperty" && property.element_size == sizeof(float))
            {
                float value{};
                if (!safe_read(value_address, value))
                {
                    error = "float property storage is unreadable";
                    return false;
                }
                output.kind = UnrealPropertyKind::floating_point;
                output.floating_point = value;
                return true;
            }
            if (property.type_name == "DoubleProperty" && property.element_size == sizeof(double))
            {
                if (!safe_read(value_address, output.floating_point))
                {
                    error = "double property storage is unreadable";
                    return false;
                }
                output.kind = UnrealPropertyKind::floating_point;
                return true;
            }
            if (property.type_name == "NameProperty" && property.element_size == sizeof(RawFName))
            {
                if (!safe_read(value_address, output.name) || !fname_to_utf8(output.name, output.text, error))
                {
                    error = error.empty() ? "FName property storage is unreadable" : error;
                    return false;
                }
                output.kind = UnrealPropertyKind::name;
                return true;
            }
            if (property.type_name == "StrProperty" && property.element_size == sizeof(UnrealStringHeader))
            {
                output.kind = UnrealPropertyKind::string;
                return read_unreal_string(value_address, output.text, error);
            }
            if (is_direct_object_property(property.type_name) && property.element_size == sizeof(void*))
            {
                std::uint64_t object_address{};
                if (!safe_read(value_address, object_address))
                {
                    error = "object property storage is unreadable";
                    return false;
                }
                output.kind = UnrealPropertyKind::object;
                output.object_is_null = object_address == 0u;
                if (!output.object_is_null && !handle_from_address(object_address, output.object))
                {
                    error = "object property points outside the validated GUObjectArray";
                    return false;
                }
                return true;
            }
            if (property.type_name == "InterfaceProperty" || property.type_name == "WeakObjectProperty" ||
                is_soft_object_property(property.type_name) ||
                property.type_name == "DelegateProperty" || is_multicast_delegate_property(property.type_name))
            {
                return decode_plain_value(*this, property, value_address, output, error);
            }
            if (property.type_name == "StructProperty" && property.struct_address != 0u)
            {
                return decode_plain_value(*this, property, value_address, output, error);
            }
            if (property.type_name == "ArrayProperty" && property.inner_property_address != 0u)
            {
                return decode_plain_value(*this, property, value_address, output, error);
            }
            if ((property.type_name == "SetProperty" && property.inner_property_address != 0u) ||
                (property.type_name == "MapProperty" && property.key_property_address != 0u &&
                 property.value_property_address != 0u))
            {
                return decode_plain_value(*this, property, value_address, output, error);
            }

            error = "property type '" + property.type_name + "' is not supported by the Linux MVP";
            return false;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while reading a reflected property";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::write_property(const ReadOnlyUObjectHandle& handle,
                                               std::string_view property_name,
                                               const UnrealPropertyValue& value,
                                               GameThreadScheduler& scheduler,
                                               std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!scheduler.is_ready())
            {
                error = "reflected property writes require the validated game-thread scheduler";
                return false;
            }
            if (!scheduler.is_game_thread())
            {
                auto* scheduler_pointer = &scheduler;
                return scheduler.execute_sync(
                        [this,
                         handle,
                         property_name = std::string{property_name},
                         value,
                         scheduler_pointer](std::string& operation_error) {
                            return write_property(
                                    handle,
                                    property_name,
                                    value,
                                    *scheduler_pointer,
                                    operation_error);
                        },
                        std::chrono::seconds{2},
                        error);
            }
            if (!is_valid(handle))
            {
                error = "a live UObject handle is required for property writes";
                return false;
            }
            UObjectBaseView object{};
            if (!safe_read(handle.address, object))
            {
                error = "UObject became unreadable before property write";
                return false;
            }
            ReflectedProperty property;
            const auto decode_name = [this](RawFName name, std::string& decoded, std::string& decode_error) {
                return fname_to_utf8(name, decoded, decode_error);
            };
            if (!find_reflected_property(object.class_private, property_name, decode_name, property, error))
            {
                return false;
            }
            const std::uint64_t value_address = handle.address + static_cast<std::uint64_t>(property.offset);

            if (property.type_name == "EnumProperty" && property_kind(property) == std::optional{value.kind})
            {
                return encode_plain_value(*this, property, value_address, value, error);
            }

            if (property.type_name == "BoolProperty" && value.kind == UnrealPropertyKind::boolean)
            {
                std::uint8_t byte{};
                const std::uint64_t byte_address = value_address + property.boolean.byte_offset;
                if (!safe_read(byte_address, byte))
                {
                    error = "FBoolProperty storage is unreadable";
                    return false;
                }
                byte = static_cast<std::uint8_t>((byte & ~property.boolean.field_mask) |
                                                 (value.boolean ? property.boolean.byte_mask : 0u));
                if (!safe_write_bytes(byte_address, &byte, sizeof(byte)))
                {
                    error = "FBoolProperty storage is not writable";
                    return false;
                }
                return true;
            }
            if (is_signed_integer_property(property.type_name) && value.kind == UnrealPropertyKind::signed_integer)
            {
                if ((property.element_size != 1 && property.element_size != 2 && property.element_size != 4 &&
                     property.element_size != 8) ||
                    (property.element_size < 8 &&
                     (value.signed_integer < -(std::int64_t{1} << (property.element_size * 8 - 1)) ||
                      value.signed_integer > (std::int64_t{1} << (property.element_size * 8 - 1)) - 1)))
                {
                    error = "signed integer value does not fit the reflected property";
                    return false;
                }
                const std::uint64_t raw = static_cast<std::uint64_t>(value.signed_integer);
                if (!safe_write_bytes(value_address, &raw, static_cast<std::size_t>(property.element_size)))
                {
                    error = "signed integer property storage is not writable";
                    return false;
                }
                return true;
            }
            if (is_unsigned_integer_property(property.type_name) && value.kind == UnrealPropertyKind::unsigned_integer)
            {
                if ((property.element_size != 1 && property.element_size != 2 && property.element_size != 4 &&
                     property.element_size != 8) ||
                    (property.element_size < 8 && value.unsigned_integer >= (std::uint64_t{1} << (property.element_size * 8))))
                {
                    error = "unsigned integer value does not fit the reflected property";
                    return false;
                }
                if (!safe_write_bytes(value_address,
                                      &value.unsigned_integer,
                                      static_cast<std::size_t>(property.element_size)))
                {
                    error = "unsigned integer property storage is not writable";
                    return false;
                }
                return true;
            }
            if (property.type_name == "FloatProperty" && property.element_size == sizeof(float) &&
                value.kind == UnrealPropertyKind::floating_point)
            {
                const float narrowed = static_cast<float>(value.floating_point);
                if (!safe_write_bytes(value_address, &narrowed, sizeof(narrowed)))
                {
                    error = "float property storage is not writable";
                    return false;
                }
                return true;
            }
            if (property.type_name == "DoubleProperty" && property.element_size == sizeof(double) &&
                value.kind == UnrealPropertyKind::floating_point)
            {
                if (!safe_write_bytes(value_address, &value.floating_point, sizeof(value.floating_point)))
                {
                    error = "double property storage is not writable";
                    return false;
                }
                return true;
            }
            if (property.type_name == "NameProperty" && property.element_size == sizeof(RawFName) &&
                value.kind == UnrealPropertyKind::name)
            {
                std::string decoded;
                if (!fname_to_utf8(value.name, decoded, error) || (!value.text.empty() && decoded != value.text))
                {
                    error = error.empty() ? "FName write value failed identity validation" : error;
                    return false;
                }
                if (!safe_write_bytes(value_address, &value.name, sizeof(value.name)))
                {
                    error = "FName property storage is not writable";
                    return false;
                }
                return true;
            }
            if (property.type_name == "TextProperty" &&
                property.element_size == static_cast<std::int32_t>(sizeof(FTextView)) &&
                value.kind == UnrealPropertyKind::text)
            {
                return assign_unreal_text(property.address, value_address, value.text, error);
            }
            if (property.type_name == "InterfaceProperty" &&
                value.kind == UnrealPropertyKind::object)
            {
                return encode_plain_value(*this, property, value_address, value, error);
            }
            if (is_direct_object_property(property.type_name) && property.element_size == sizeof(void*) &&
                value.kind == UnrealPropertyKind::object)
            {
                if (!value.object_is_null && !is_valid(value.object))
                {
                    error = "object property write requires nil or a live UObject handle";
                    return false;
                }
                const std::uint64_t object_address = value.object_is_null ? 0u : value.object.address;
                if (!safe_write_bytes(value_address, &object_address, sizeof(object_address)))
                {
                    error = "object property storage is not writable";
                    return false;
                }
                return true;
            }
            if (property.type_name == "DelegateProperty" &&
                value.kind == UnrealPropertyKind::delegate)
            {
                return encode_plain_value(*this, property, value_address, value, error);
            }
            if (is_soft_object_property(property.type_name) &&
                value.kind == UnrealPropertyKind::soft_object)
            {
                return encode_plain_value(*this, property, value_address, value, error);
            }
            if (property.type_name == "StructProperty" && property.struct_address != 0u &&
                value.kind == UnrealPropertyKind::structure)
            {
                return encode_plain_value(*this, property, value_address, value, error);
            }
            if (property.type_name == "ArrayProperty" && property.inner_property_address != 0u &&
                value.kind == UnrealPropertyKind::array)
            {
                return encode_plain_value(*this, property, value_address, value, error);
            }
            if (property.type_name == "SetProperty" && property.inner_property_address != 0u &&
                value.kind == UnrealPropertyKind::set)
            {
                return encode_plain_value(*this, property, value_address, value, error);
            }
            if (property.type_name == "MapProperty" && property.key_property_address != 0u &&
                property.value_property_address != 0u && value.kind == UnrealPropertyKind::map)
            {
                return encode_plain_value(*this, property, value_address, value, error);
            }
            if (property.type_name == "StrProperty" && property.element_size == sizeof(UnrealStringHeader) &&
                value.kind == UnrealPropertyKind::string)
            {
                return assign_unreal_string(value_address, value.text, error);
            }
            error = "Lua value type does not match reflected property type '" + property.type_name + "'";
            return false;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while writing a reflected property";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::validate_custom_property(
            const CustomPropertyDescription& property,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (property.name.empty())
            {
                error = "custom property name is empty";
                return false;
            }
            std::string class_error;
            if (!is_valid(property.owner_class) ||
                !is_a(property.owner_class, "Class", class_error))
            {
                error = class_error.empty()
                                ? "custom property owner is not a live UClass"
                                : std::move(class_error);
                return false;
            }
            if (property.offset < 0)
            {
                error = "custom property offset is negative";
                return false;
            }
            const auto expected_size =
                    custom_property_storage_size(property.type.type_name);
            if (!expected_size.has_value())
            {
                error = "custom property type '" + property.type.type_name +
                        "' requires metadata that RegisterCustomProperty does not provide";
                return false;
            }
            if (property.type.element_size != *expected_size)
            {
                error = "custom property type '" + property.type.type_name +
                        "' has size " +
                        std::to_string(property.type.element_size) +
                        ", expected " + std::to_string(*expected_size) +
                        " for the validated UE 5.1 Linux ABI";
                return false;
            }
            if (property.type.type_name == "ArrayProperty")
            {
                if (!property.array_inner.has_value())
                {
                    error = "custom ArrayProperty requires ArrayProperty.Type metadata";
                    return false;
                }
                const auto expected_inner_size = custom_property_storage_size(
                        property.array_inner->type_name);
                if (!expected_inner_size.has_value() ||
                    property.array_inner->type_name == "ArrayProperty")
                {
                    error = "custom ArrayProperty inner type '" +
                            property.array_inner->type_name +
                            "' is unsupported without additional nested metadata";
                    return false;
                }
                if (property.array_inner->element_size != *expected_inner_size)
                {
                    error = "custom ArrayProperty inner type '" +
                            property.array_inner->type_name + "' has size " +
                            std::to_string(property.array_inner->element_size) +
                            ", expected " +
                            std::to_string(*expected_inner_size) +
                            " for the validated UE 5.1 Linux ABI";
                    return false;
                }
            }
            else if (property.array_inner.has_value())
            {
                error = "ArrayProperty metadata was supplied for a non-array custom property";
                return false;
            }
            std::int32_t properties_size{};
            if (!safe_read(property.owner_class.address + k_properties_size_offset,
                           properties_size) ||
                properties_size <= 0 || properties_size > k_max_properties_size)
            {
                error = "custom property owner has an invalid UStruct::PropertiesSize";
                return false;
            }
            const std::int64_t end = static_cast<std::int64_t>(property.offset) +
                                     static_cast<std::int64_t>(*expected_size);
            if (end > properties_size)
            {
                error = "custom property range [" +
                        std::to_string(property.offset) + ", " +
                        std::to_string(end) + ") exceeds owner PropertiesSize " +
                        std::to_string(properties_size);
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while validating a custom property";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::custom_property_owner_matches(
            const ReadOnlyUObjectHandle& handle,
            const ReadOnlyUObjectHandle& owner_class) const noexcept
    {
        if (!is_valid(handle) || !is_valid(owner_class))
        {
            return false;
        }
        if (handle.address == owner_class.address)
        {
            return true;
        }
        std::string ignored;
        if (!is_a(handle, "Struct", ignored))
        {
            ignored.clear();
            return is_a(handle, owner_class, ignored);
        }
        std::uint64_t current = handle.address;
        for (std::size_t depth = 0;
             current != 0u && depth < k_max_super_depth;
             ++depth)
        {
            if (current == owner_class.address)
            {
                return true;
            }
            if (!safe_read(current + k_super_struct_offset, current))
            {
                return false;
            }
        }
        return false;
    }

    bool ReadOnlyUnrealRuntime::read_custom_property(
            const ReadOnlyUObjectHandle& handle,
            const CustomPropertyDescription& property,
            UnrealPropertyValue& output,
            std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            if (!validate_custom_property(property, error) ||
                !custom_property_owner_matches(handle, property.owner_class))
            {
                if (error.empty())
                {
                    error = "custom property does not belong to this UObject class hierarchy";
                }
                return false;
            }
            const std::uint64_t offset =
                    static_cast<std::uint64_t>(property.offset);
            if (handle.address >
                std::numeric_limits<std::uint64_t>::max() - offset)
            {
                error = "custom property address overflows";
                return false;
            }
            ReflectedProperty reflected;
            SyntheticCustomInnerProperty inner;
            if (!make_custom_reflected_property(
                        *this, property, reflected, inner, error))
            {
                return false;
            }
            return decode_plain_value(
                    *this, reflected, handle.address + offset, output, error);
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while reading a custom property";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::write_custom_property(
            const ReadOnlyUObjectHandle& handle,
            const CustomPropertyDescription& property,
            const UnrealPropertyValue& value,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "custom property writes are permitted only inside a validated game-thread callback";
                return false;
            }
            if (!validate_custom_property(property, error) ||
                !custom_property_owner_matches(handle, property.owner_class))
            {
                if (error.empty())
                {
                    error = "custom property does not belong to this UObject class hierarchy";
                }
                return false;
            }
            if (property.type.type_name == "TextProperty" ||
                (property.array_inner.has_value() &&
                 property.array_inner->type_name == "TextProperty"))
            {
                error = "custom TextProperty writes require a real FProperty lifecycle and are unavailable";
                return false;
            }
            if (property.type.type_name == "WeakObjectProperty" ||
                (property.array_inner.has_value() &&
                 property.array_inner->type_name == "WeakObjectProperty"))
            {
                error = "WeakObjectProperty writes are not supported by upstream UE4SS";
                return false;
            }
            const std::uint64_t offset =
                    static_cast<std::uint64_t>(property.offset);
            if (handle.address >
                std::numeric_limits<std::uint64_t>::max() - offset)
            {
                error = "custom property address overflows";
                return false;
            }
            ReflectedProperty reflected;
            SyntheticCustomInnerProperty inner;
            if (!make_custom_reflected_property(
                        *this, property, reflected, inner, error))
            {
                return false;
            }
            return encode_plain_value(
                    *this, reflected, handle.address + offset, value, error);
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while writing a custom property";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::register_custom_property(
            CustomPropertyDescription property,
            std::string& error) noexcept
    {
        error.clear();
        try
        {
            if (!validate_custom_property(property, error))
            {
                return false;
            }
            std::lock_guard lock{m_custom_properties_mutex};
            const auto existing = std::find_if(
                    m_custom_properties.begin(),
                    m_custom_properties.end(),
                    [&property](const CustomPropertyDescription& candidate) {
                        return candidate.owner_class.address ==
                                       property.owner_class.address &&
                               candidate.name == property.name;
                    });
            if (existing != m_custom_properties.end())
            {
                if (same_custom_property(*existing, property))
                {
                    return true;
                }
                error = "a different custom property named '" + property.name +
                        "' is already registered for this UClass";
                return false;
            }
            m_custom_properties.push_back(std::move(property));
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while registering a custom property";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::enumerate_custom_properties(
            const ReadOnlyUObjectHandle& handle,
            std::vector<ReflectedPropertyDescription>& output,
            std::string& error) const noexcept
    {
        output.clear();
        error.clear();
        try
        {
            if (!is_valid(handle))
            {
                error = "a live UObject or UStruct is required for custom property enumeration";
                return false;
            }
            std::vector<CustomPropertyDescription> properties;
            {
                std::lock_guard lock{m_custom_properties_mutex};
                properties = m_custom_properties;
            }
            for (const auto& property : properties)
            {
                if (!custom_property_owner_matches(handle, property.owner_class))
                {
                    continue;
                }
                output.push_back({
                        .name = property.name,
                        .type_name = property.type.type_name,
                        .element_size = property.type.element_size,
                        .offset = property.offset,
                });
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            output.clear();
            error = exception.what();
            return false;
        }
        catch (...)
        {
            output.clear();
            error = "unknown exception while enumerating custom properties";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::read_registered_custom_property(
            const ReadOnlyUObjectHandle& handle,
            std::string_view property_name,
            UnrealPropertyValue& output,
            bool& found,
            std::string& error) const noexcept
    {
        output = {};
        found = false;
        error.clear();
        try
        {
            std::vector<CustomPropertyDescription> properties;
            {
                std::lock_guard lock{m_custom_properties_mutex};
                properties = m_custom_properties;
            }
            for (const auto& property : properties)
            {
                if (property.name != property_name ||
                    !custom_property_owner_matches(handle, property.owner_class))
                {
                    continue;
                }
                found = true;
                return read_custom_property(handle, property, output, error);
            }
            return false;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while finding a registered custom property";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::write_registered_custom_property(
            const ReadOnlyUObjectHandle& handle,
            std::string_view property_name,
            const UnrealPropertyValue& value,
            const GameThreadScheduler& scheduler,
            bool& found,
            std::string& error) const noexcept
    {
        found = false;
        error.clear();
        try
        {
            std::vector<CustomPropertyDescription> properties;
            {
                std::lock_guard lock{m_custom_properties_mutex};
                properties = m_custom_properties;
            }
            for (const auto& property : properties)
            {
                if (property.name != property_name ||
                    !custom_property_owner_matches(handle, property.owner_class))
                {
                    continue;
                }
                found = true;
                return write_custom_property(
                        handle, property, value, scheduler, error);
            }
            return false;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while finding a registered custom property for writing";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::add_multicast_delegate_binding(
            const ReadOnlyUObjectHandle& owner,
            std::string_view property_name,
            const ReadOnlyUObjectHandle& target,
            RawFName function_name,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        return mutate_inline_multicast_delegate(
                *this,
                owner,
                property_name,
                InlineMulticastMutation::add,
                &target,
                function_name,
                scheduler,
                error);
    }

    bool ReadOnlyUnrealRuntime::remove_multicast_delegate_binding(
            const ReadOnlyUObjectHandle& owner,
            std::string_view property_name,
            const ReadOnlyUObjectHandle& target,
            RawFName function_name,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        return mutate_inline_multicast_delegate(
                *this,
                owner,
                property_name,
                InlineMulticastMutation::remove,
                &target,
                function_name,
                scheduler,
                error);
    }

    bool ReadOnlyUnrealRuntime::clear_multicast_delegate_bindings(
            const ReadOnlyUObjectHandle& owner,
            std::string_view property_name,
            const GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        return mutate_inline_multicast_delegate(
                *this,
                owner,
                property_name,
                InlineMulticastMutation::clear,
                nullptr,
                {},
                scheduler,
                error);
    }

    bool ReadOnlyUnrealRuntime::broadcast_multicast_delegate(
            const ReadOnlyUObjectHandle& owner,
            std::string_view property_name,
            const std::vector<UnrealFunctionArgument>& arguments,
            GameThreadScheduler& scheduler,
            std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (!scheduler.is_ready() || !scheduler.is_game_thread())
            {
                error = "multicast delegate Broadcast is permitted only inside a validated game-thread callback";
                return false;
            }
            UnrealPropertyValue delegate;
            if (!read_property(owner, property_name, delegate, error) ||
                delegate.kind != UnrealPropertyKind::multicast_delegate)
            {
                error = error.empty() ? "Broadcast target is not a readable inline multicast delegate" : error;
                return false;
            }
            for (std::size_t index = 0; index < delegate.array_elements.size(); ++index)
            {
                const auto& binding = delegate.array_elements[index];
                if (binding == nullptr || binding->kind != UnrealPropertyKind::delegate ||
                    binding->object_is_null || binding->text.empty())
                {
                    continue;
                }
                ReadOnlyUObjectHandle function;
                std::string binding_error;
                if (!find_function(binding->object, binding->text, function, binding_error))
                {
                    // FMulticastScriptDelegate::ProcessMulticastDelegate skips
                    // stale or no-longer-bound entries as well.
                    continue;
                }
                std::optional<UnrealPropertyValue> ignored_return;
                std::vector<UnrealFunctionOutValue> ignored_out_values;
                if (!invoke_function(binding->object,
                                     function,
                                     arguments,
                                     ignored_return,
                                     ignored_out_values,
                                     scheduler,
                                     binding_error))
                {
                    error = "multicast binding " + std::to_string(index) + " ('" + binding->text +
                            "') failed: " + binding_error;
                    return false;
                }
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while broadcasting an inline multicast delegate";
            return false;
        }
    }

    std::string ReadOnlyUnrealRuntime::process_event_vtable_probe() const noexcept
    {
        try
        {
            const ObjectQueryResult packages = find_all_of("Package", 1u);
            if (!packages.success || packages.objects.empty())
            {
                return "ProcessEvent vtable probe unavailable: no live UPackage sample";
            }
            UObjectBaseView object{};
            if (!safe_read(packages.objects.front().address, object) || object.vtable == 0u)
            {
                return "ProcessEvent vtable probe unavailable: UPackage vtable unreadable";
            }
            std::string report = "UE 5.1 UPackage vtable candidates";
            for (const std::uint64_t offset : {0x258u, 0x260u, 0x268u, 0x270u, 0x278u})
            {
                std::uint64_t candidate{};
                if (!safe_read(object.vtable + offset, candidate))
                {
                    report += " +0x" + std::to_string(offset) + "=unreadable";
                    continue;
                }
                std::array<char, 64> formatted{};
                std::snprintf(formatted.data(), formatted.size(), " +0x%llx=0x%llx%s",
                              static_cast<unsigned long long>(offset),
                              static_cast<unsigned long long>(candidate),
                              is_executable_mapping(candidate) ? "(rx)" : "(!rx)");
                report += formatted.data();
            }
            return report;
        }
        catch (...)
        {
            return "ProcessEvent vtable probe failed at the exception boundary";
        }
    }

    bool ReadOnlyUnrealRuntime::find_function(const ReadOnlyUObjectHandle& context,
                                              std::string_view function_name,
                                              ReadOnlyUObjectHandle& output,
                                              std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            if (!is_valid(context) || function_name.empty())
            {
                error = "a live UObject and non-empty UFunction name are required";
                return false;
            }
            UObjectBaseView object{};
            if (!safe_read(context.address, object))
            {
                error = "UObject became unreadable while finding a UFunction";
                return false;
            }

            std::unordered_set<std::uint64_t> visited;
            std::uint64_t current_class = object.class_private;
            std::size_t fields{};
            for (std::size_t depth = 0; current_class != 0u && depth < k_max_super_depth; ++depth)
            {
                std::uint64_t field_address{};
                if (!safe_read(current_class + 0x48u, field_address))
                {
                    error = "UStruct::Children became unreadable while finding a UFunction";
                    return false;
                }
                while (field_address != 0u)
                {
                    if (++fields > k_max_property_fields || !visited.emplace(field_address).second)
                    {
                        error = "UField child chain is cyclic or exceeds the safety limit";
                        return false;
                    }
                    UObjectBaseView field{};
                    std::uint64_t next{};
                    ReadOnlyUObjectHandle field_handle;
                    std::string name;
                    if (!safe_read(field_address, field) || !safe_read(field_address + 0x28u, next) ||
                        !handle_from_address(field_address, field_handle) || !fname_to_utf8(field.name_private, name, error))
                    {
                        error = error.empty() ? "UField child metadata became unreadable" : error;
                        return false;
                    }
                    std::string class_error;
                    if (name == function_name && is_a(field_handle, "Function", class_error))
                    {
                        output = field_handle;
                        return true;
                    }
                    field_address = next;
                }
                if (!safe_read(current_class + k_super_struct_offset, current_class))
                {
                    error = "UStruct::SuperStruct became unreadable while finding a UFunction";
                    return false;
                }
            }
            error = "UFunction '" + std::string{function_name} + "' was not found in the class hierarchy";
            return false;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while finding a reflected UFunction";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::function_native_address(const ReadOnlyUObjectHandle& function,
                                                        std::uint64_t& output,
                                                        std::string& error) const noexcept
    {
        constexpr std::uint64_t ue51_ufunction_func_offset = 0xd8u;
        output = 0u;
        error.clear();
        try
        {
            std::string class_error;
            if (!is_valid(function) || !is_a(function, "Function", class_error))
            {
                error = class_error.empty() ? "a live UFunction is required" : class_error;
                return false;
            }
            if (function.address > std::numeric_limits<std::uint64_t>::max() - ue51_ufunction_func_offset ||
                !safe_read(function.address + ue51_ufunction_func_offset, output) || output == 0u ||
                !is_executable_mapping(output))
            {
                error = "UFunction::Func is null, unreadable, or not executable";
                output = 0u;
                return false;
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            output = 0u;
            return false;
        }
        catch (...)
        {
            error = "unknown exception while reading UFunction::Func";
            output = 0u;
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::describe_hook_parameters(const ReadOnlyUObjectHandle& function,
                                                         std::uint64_t stack_address,
                                                         std::uint64_t result_address,
                                                         bool include_return,
                                                         std::vector<RemoteUnrealParameter>& output,
                                                         std::string& error) const noexcept
    {
        constexpr std::uint64_t ue51_fframe_locals_offset = 0x28u;
        constexpr std::uint64_t ue51_fframe_out_parms_offset = 0x80u;
        struct FOutParmRecView
        {
            std::uint64_t property{};
            std::uint64_t address{};
            std::uint64_t next{};
        };

        output.clear();
        error.clear();
        try
        {
            std::string class_error;
            if (!is_valid(function) || !is_a(function, "Function", class_error))
            {
                error = class_error.empty() ? "a live UFunction is required for hook parameter reflection" : class_error;
                return false;
            }

            std::uint8_t declared_parameter_count{};
            std::uint16_t params_size{};
            std::uint16_t declared_return_offset{};
            std::uint64_t field_address{};
            if (!safe_read(function.address + 0xb4u, declared_parameter_count) ||
                !safe_read(function.address + 0xb6u, params_size) ||
                !safe_read(function.address + 0xb8u, declared_return_offset) ||
                !safe_read(function.address + k_child_properties_offset, field_address))
            {
                error = "UFunction hook parameter layout is unreadable";
                return false;
            }

            std::uint64_t locals{};
            std::uint64_t out_parms{};
            if (declared_parameter_count != 0u)
            {
                if (stack_address == 0u ||
                    !safe_read(stack_address + ue51_fframe_locals_offset, locals) ||
                    !safe_read(stack_address + ue51_fframe_out_parms_offset, out_parms))
                {
                    error = "UE 5.1 FFrame Locals/OutParms are unreadable";
                    return false;
                }
            }

            std::vector<RemoteUnrealParameter> regular;
            std::optional<RemoteUnrealParameter> returned;
            std::unordered_set<std::uint64_t> visited;
            std::size_t reflected_parameter_count{};
            const auto decode_name = [this](RawFName name, std::string& decoded, std::string& decode_error) {
                return fname_to_utf8(name, decoded, decode_error);
            };
            while (field_address != 0u)
            {
                if (visited.size() >= k_max_property_fields || !visited.emplace(field_address).second)
                {
                    error = "UFunction hook FProperty chain is cyclic or exceeds the safety limit";
                    return false;
                }
                FPropertyPrefixView prefix{};
                if (!safe_read(field_address, prefix))
                {
                    error = "UFunction hook FProperty chain became unreadable";
                    return false;
                }
                if ((prefix.property_flags & k_property_flag_parameter) != 0u)
                {
                    ReflectedProperty property;
                    if (!decode_function_property(field_address, params_size, decode_name, property, error))
                    {
                        return false;
                    }
                    ++reflected_parameter_count;
                    const auto kind = property_kind(property);
                    if (!kind.has_value())
                    {
                        error = "hook parameter '" + property.name + "' has unsupported type '" +
                                property.type_name + "'";
                        return false;
                    }
                    RemoteUnrealParameter parameter{
                            .kind = *kind,
                            .property_address = property.address,
                            .element_size = property.element_size,
                            .boolean_byte_offset = property.boolean.byte_offset,
                            .boolean_byte_mask = property.boolean.byte_mask,
                            .boolean_field_mask = property.boolean.field_mask,
                            .is_return = (property.flags & k_property_flag_return_parameter) != 0u,
                            .is_out = (property.flags & k_property_flag_out_parameter) != 0u,
                            .struct_address = property.struct_address,
                            .inner_property_address = property.inner_property_address,
                            .key_property_address = property.key_property_address,
                            .value_property_address = property.value_property_address,
                            .interface_class_address = property.interface_class_address,
                    };
                    if (parameter.is_return)
                    {
                        if (returned.has_value() || property.offset != declared_return_offset)
                        {
                            error = returned.has_value()
                                            ? "UFunction has multiple reflected return parameters"
                                            : "UFunction ReturnValueOffset disagrees with its return FProperty";
                            return false;
                        }
                        parameter.storage_address = result_address;
                        returned = parameter;
                    }
                    else
                    {
                        if (parameter.is_out)
                        {
                            std::uint64_t current = out_parms;
                            std::unordered_set<std::uint64_t> visited_out;
                            while (current != 0u && visited_out.size() <= declared_parameter_count + 16u &&
                                   visited_out.emplace(current).second)
                            {
                                FOutParmRecView record{};
                                if (!safe_read(current, record))
                                {
                                    error = "FFrame OutParms chain became unreadable";
                                    return false;
                                }
                                if (record.property == property.address)
                                {
                                    parameter.storage_address = record.address;
                                    break;
                                }
                                current = record.next;
                            }
                            if (parameter.storage_address == 0u)
                            {
                                error = "OutParm '" + property.name + "' was not found in FFrame::OutParms";
                                return false;
                            }
                        }
                        else
                        {
                            if (locals == 0u || locals > std::numeric_limits<std::uint64_t>::max() -
                                                               static_cast<std::uint64_t>(property.offset))
                            {
                                error = "FFrame::Locals is null or overflows for parameter '" + property.name + "'";
                                return false;
                            }
                            parameter.storage_address = locals + static_cast<std::uint64_t>(property.offset);
                        }
                        regular.push_back(parameter);
                    }
                }
                field_address = prefix.next;
            }
            if (reflected_parameter_count != declared_parameter_count)
            {
                error = "UFunction NumParms disagrees with its CPF_Parm hook property chain";
                return false;
            }
            if (include_return && returned.has_value())
            {
                if (returned->storage_address == 0u)
                {
                    error = "post-hook return parameter has a null RESULT_DECL";
                    return false;
                }
                output.push_back(*returned);
            }
            output.insert(output.end(), regular.begin(), regular.end());
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            output.clear();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while describing UFunction hook parameters";
            output.clear();
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::read_hook_parameter(const RemoteUnrealParameter& parameter,
                                                    UnrealPropertyValue& output,
                                                    std::string& error) const noexcept
    {
        output = {};
        error.clear();
        try
        {
            if (parameter.storage_address == 0u || parameter.element_size <= 0 ||
                parameter.element_size > k_max_property_element_size)
            {
                error = "remote hook parameter descriptor is invalid";
                return false;
            }
            output.kind = parameter.kind;
            switch (parameter.kind)
            {
            case UnrealPropertyKind::boolean:
            {
                std::uint8_t byte{};
                if (parameter.boolean_field_mask == 0u ||
                    !safe_read(parameter.storage_address + parameter.boolean_byte_offset, byte))
                {
                    error = "remote BoolProperty storage is unreadable";
                    return false;
                }
                output.boolean = (byte & parameter.boolean_field_mask) != 0u;
                return true;
            }
            case UnrealPropertyKind::signed_integer:
            case UnrealPropertyKind::unsigned_integer:
            {
                // Native lifecycle hooks do not have an FProperty descriptor
                // for their C++ arguments. A zero property address therefore
                // denotes a bounded scalar owned by the synchronous detour
                // frame. These descriptors are created only by the core and
                // expire with the hook callback epoch.
                if (parameter.property_address == 0u)
                {
                    if (parameter.element_size != 1 && parameter.element_size != 2 &&
                        parameter.element_size != 4 && parameter.element_size != 8)
                    {
                        error = "native remote integer parameter has an invalid width";
                        return false;
                    }
                    std::uint64_t raw{};
                    if (!safe_read_bytes(parameter.storage_address,
                                         &raw,
                                         static_cast<std::size_t>(parameter.element_size)))
                    {
                        error = "native remote integer parameter storage is unreadable";
                        return false;
                    }
                    if (parameter.kind == UnrealPropertyKind::unsigned_integer)
                    {
                        output.unsigned_integer = raw;
                    }
                    else if (parameter.element_size == 8)
                    {
                        output.signed_integer = static_cast<std::int64_t>(raw);
                    }
                    else
                    {
                        const unsigned bits = static_cast<unsigned>(parameter.element_size * 8);
                        const std::uint64_t sign_bit = std::uint64_t{1} << (bits - 1u);
                        output.signed_integer = static_cast<std::int64_t>((raw ^ sign_bit) - sign_bit);
                    }
                    return true;
                }
                ReflectedProperty property;
                if (!decode_nested_property(*this, parameter.property_address, property, error, true))
                {
                    return false;
                }
                if (property_kind(property) != std::optional{parameter.kind})
                {
                    error = "remote integer descriptor disagrees with its FProperty kind";
                    return false;
                }
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::floating_point:
                if (parameter.element_size == static_cast<std::int32_t>(sizeof(float)))
                {
                    float value{};
                    if (!safe_read(parameter.storage_address, value))
                    {
                        error = "remote float parameter storage is unreadable";
                        return false;
                    }
                    output.floating_point = value;
                    return true;
                }
                if (parameter.element_size == static_cast<std::int32_t>(sizeof(double)) &&
                    safe_read(parameter.storage_address, output.floating_point))
                {
                    return true;
                }
                error = "remote floating-point parameter has invalid or unreadable storage";
                return false;
            case UnrealPropertyKind::name:
                if (parameter.element_size != static_cast<std::int32_t>(sizeof(RawFName)) ||
                    !safe_read(parameter.storage_address, output.name) ||
                    !fname_to_utf8(output.name, output.text, error))
                {
                    error = error.empty() ? "remote FName parameter storage is unreadable" : error;
                    return false;
                }
                return true;
            case UnrealPropertyKind::string:
            {
                if (parameter.element_size != static_cast<std::int32_t>(sizeof(UnrealStringHeader)))
                {
                    error = "remote FString parameter has an incompatible element size";
                    return false;
                }
                return read_unreal_string(parameter.storage_address, output.text, error);
            }
            case UnrealPropertyKind::text:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "TextProperty",
                        .element_size = parameter.element_size,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::object:
            {
                if (parameter.element_size == static_cast<std::int32_t>(sizeof(FScriptInterfaceView)))
                {
                    ReflectedProperty property{
                            .address = parameter.property_address,
                            .type_name = "InterfaceProperty",
                            .element_size = parameter.element_size,
                            .interface_class_address = parameter.interface_class_address,
                    };
                    return decode_plain_value(
                            *this, property, parameter.storage_address, output, error);
                }
                std::uint64_t address{};
                if (parameter.element_size != static_cast<std::int32_t>(sizeof(void*)) ||
                    !safe_read(parameter.storage_address, address))
                {
                    error = "remote UObject parameter storage is unreadable";
                    return false;
                }
                output.object_is_null = address == 0u;
                if (!output.object_is_null && !handle_from_address(address, output.object))
                {
                    error = "remote UObject parameter points outside the validated GUObjectArray";
                    return false;
                }
                return true;
            }
            case UnrealPropertyKind::weak_object:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "WeakObjectProperty",
                        .element_size = parameter.element_size,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::soft_object:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "SoftObjectProperty",
                        .element_size = parameter.element_size,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::delegate:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "DelegateProperty",
                        .element_size = parameter.element_size,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::multicast_delegate:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "MulticastDelegateProperty",
                        .element_size = parameter.element_size,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::structure:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "StructProperty",
                        .element_size = parameter.element_size,
                        .struct_address = parameter.struct_address,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::array:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "ArrayProperty",
                        .element_size = parameter.element_size,
                        .inner_property_address = parameter.inner_property_address,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::set:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "SetProperty",
                        .element_size = parameter.element_size,
                        .inner_property_address = parameter.inner_property_address,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            case UnrealPropertyKind::map:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "MapProperty",
                        .element_size = parameter.element_size,
                        .key_property_address = parameter.key_property_address,
                        .value_property_address = parameter.value_property_address,
                };
                return decode_plain_value(*this, property, parameter.storage_address, output, error);
            }
            }
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while reading a remote hook parameter";
            return false;
        }
        error = "remote hook parameter kind is invalid";
        return false;
    }

    bool ReadOnlyUnrealRuntime::write_hook_parameter(const RemoteUnrealParameter& parameter,
                                                     const UnrealPropertyValue& value,
                                                     std::string& error) const noexcept
    {
        error.clear();
        try
        {
            if (parameter.storage_address == 0u || value.kind != parameter.kind)
            {
                error = "Lua value kind does not match the remote hook parameter";
                return false;
            }
            switch (parameter.kind)
            {
            case UnrealPropertyKind::boolean:
            {
                std::uint8_t byte{};
                const std::uint64_t address = parameter.storage_address + parameter.boolean_byte_offset;
                if (parameter.boolean_field_mask == 0u || !safe_read(address, byte))
                {
                    error = "remote BoolProperty storage is unreadable";
                    return false;
                }
                byte = static_cast<std::uint8_t>((byte & ~parameter.boolean_field_mask) |
                                                 (value.boolean ? parameter.boolean_byte_mask : 0u));
                if (!safe_write_bytes(address, &byte, sizeof(byte)))
                {
                    error = "remote BoolProperty storage is not writable";
                    return false;
                }
                return true;
            }
            case UnrealPropertyKind::signed_integer:
                if ((parameter.element_size != 1 && parameter.element_size != 2 && parameter.element_size != 4 &&
                     parameter.element_size != 8) ||
                    (parameter.element_size < 8 &&
                     (value.signed_integer < -(std::int64_t{1} << (parameter.element_size * 8 - 1)) ||
                      value.signed_integer > (std::int64_t{1} << (parameter.element_size * 8 - 1)) - 1)))
                {
                    error = "signed Lua value does not fit the remote hook parameter";
                    return false;
                }
                {
                    const std::uint64_t raw = static_cast<std::uint64_t>(value.signed_integer);
                    if (!safe_write_bytes(parameter.storage_address, &raw,
                                          static_cast<std::size_t>(parameter.element_size)))
                    {
                        error = "remote signed integer parameter is not writable";
                        return false;
                    }
                }
                return true;
            case UnrealPropertyKind::unsigned_integer:
                if ((parameter.element_size != 1 && parameter.element_size != 2 && parameter.element_size != 4 &&
                     parameter.element_size != 8) ||
                    (parameter.element_size < 8 &&
                     value.unsigned_integer >= (std::uint64_t{1} << (parameter.element_size * 8))))
                {
                    error = "unsigned Lua value does not fit the remote hook parameter";
                    return false;
                }
                if (!safe_write_bytes(parameter.storage_address, &value.unsigned_integer,
                                      static_cast<std::size_t>(parameter.element_size)))
                {
                    error = "remote unsigned integer parameter is not writable";
                    return false;
                }
                return true;
            case UnrealPropertyKind::floating_point:
                if (parameter.element_size == static_cast<std::int32_t>(sizeof(float)))
                {
                    const float narrowed = static_cast<float>(value.floating_point);
                    if (!safe_write_bytes(parameter.storage_address, &narrowed, sizeof(narrowed)))
                    {
                        error = "remote float parameter is not writable";
                        return false;
                    }
                    return true;
                }
                if (parameter.element_size == static_cast<std::int32_t>(sizeof(double)) &&
                    safe_write_bytes(parameter.storage_address, &value.floating_point, sizeof(value.floating_point)))
                {
                    return true;
                }
                error = "remote floating-point parameter is not writable";
                return false;
            case UnrealPropertyKind::name:
                if (parameter.element_size != static_cast<std::int32_t>(sizeof(RawFName)) ||
                    !safe_write_bytes(parameter.storage_address, &value.name, sizeof(value.name)))
                {
                    error = "remote FName parameter is not writable";
                    return false;
                }
                return true;
            case UnrealPropertyKind::string:
                if (parameter.element_size != static_cast<std::int32_t>(sizeof(UnrealStringHeader)))
                {
                    error = "remote FString parameter has an incompatible element size";
                    return false;
                }
                return assign_unreal_string(parameter.storage_address, value.text, error);
            case UnrealPropertyKind::text:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "TextProperty",
                        .element_size = parameter.element_size,
                };
                return encode_plain_value(*this, property, parameter.storage_address, value, error);
            }
            case UnrealPropertyKind::object:
            {
                if (!value.object_is_null && !is_valid(value.object))
                {
                    error = "remote UObject parameter assignment requires nil or a live UObject";
                    return false;
                }
                if (parameter.element_size == static_cast<std::int32_t>(sizeof(FScriptInterfaceView)))
                {
                    ReflectedProperty property{
                            .address = parameter.property_address,
                            .type_name = "InterfaceProperty",
                            .element_size = parameter.element_size,
                            .interface_class_address = parameter.interface_class_address,
                    };
                    return encode_plain_value(
                            *this, property, parameter.storage_address, value, error);
                }
                const std::uint64_t address = value.object_is_null ? 0u : value.object.address;
                if (parameter.element_size != static_cast<std::int32_t>(sizeof(void*)) ||
                    !safe_write_bytes(parameter.storage_address, &address, sizeof(address)))
                {
                    error = "remote UObject parameter is not writable";
                    return false;
                }
                return true;
            }
            case UnrealPropertyKind::weak_object:
                error = "remote WeakObjectProperty mutation is not supported by upstream UE4SS either";
                return false;
            case UnrealPropertyKind::soft_object:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "SoftObjectProperty",
                        .element_size = parameter.element_size,
                };
                return encode_plain_value(*this, property, parameter.storage_address, value, error);
            }
            case UnrealPropertyKind::delegate:
                error = "remote DelegateProperty mutation requires the validated weak-pointer assignment bridge";
                return false;
            case UnrealPropertyKind::multicast_delegate:
                error = "multicast delegates must be changed through Add/Remove/Clear property methods";
                return false;
            case UnrealPropertyKind::structure:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "StructProperty",
                        .element_size = parameter.element_size,
                        .struct_address = parameter.struct_address,
                };
                return encode_plain_value(*this, property, parameter.storage_address, value, error);
            }
            case UnrealPropertyKind::array:
            {
                ReflectedProperty property{
                        .address = parameter.property_address,
                        .type_name = "ArrayProperty",
                        .element_size = parameter.element_size,
                        .inner_property_address = parameter.inner_property_address,
                };
                return encode_plain_value(*this, property, parameter.storage_address, value, error);
            }
            case UnrealPropertyKind::set:
            {
                ReflectedProperty property;
                if (!decode_nested_property(*this, parameter.property_address, property, error, true))
                {
                    return false;
                }
                property.inner_property_address = parameter.inner_property_address;
                return encode_plain_value(*this, property, parameter.storage_address, value, error);
            }
            case UnrealPropertyKind::map:
            {
                ReflectedProperty property;
                if (!decode_nested_property(*this, parameter.property_address, property, error, true))
                {
                    return false;
                }
                property.key_property_address = parameter.key_property_address;
                property.value_property_address = parameter.value_property_address;
                return encode_plain_value(*this, property, parameter.storage_address, value, error);
            }
            }
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while writing a remote hook parameter";
            return false;
        }
        error = "remote hook parameter kind is invalid";
        return false;
    }

    bool ReadOnlyUnrealRuntime::invoke_function(const ReadOnlyUObjectHandle& context,
                                                const ReadOnlyUObjectHandle& function,
                                                const std::vector<UnrealFunctionArgument>& arguments,
                                                std::optional<UnrealPropertyValue>& return_value,
                                                std::vector<UnrealFunctionOutValue>& out_values,
                                                GameThreadScheduler& scheduler,
                                                std::string& error) const noexcept
    {
        return_value.reset();
        out_values.clear();
        error.clear();
        try
        {
            if (m_process_event == nullptr || !scheduler.is_ready())
            {
                error = "ProcessEvent calls require the validated game-thread scheduler";
                return false;
            }
            if (!scheduler.is_game_thread())
            {
                struct DeferredInvocationResult
                {
                    std::optional<UnrealPropertyValue> return_value;
                    std::vector<UnrealFunctionOutValue> out_values;
                };
                auto deferred_result = std::make_shared<DeferredInvocationResult>();
                auto* scheduler_pointer = &scheduler;
                const bool invoked = scheduler.execute_sync(
                        [this,
                         context,
                         function,
                         arguments,
                         deferred_result,
                         scheduler_pointer](std::string& operation_error) {
                            return invoke_function(
                                    context,
                                    function,
                                    arguments,
                                    deferred_result->return_value,
                                    deferred_result->out_values,
                                    *scheduler_pointer,
                                    operation_error);
                        },
                        std::chrono::seconds{2},
                        error);
                if (invoked)
                {
                    return_value = std::move(deferred_result->return_value);
                    out_values = std::move(deferred_result->out_values);
                }
                return invoked;
            }
            if (!is_valid(context) || !is_valid(function))
            {
                error = "live context and UFunction handles are required";
                return false;
            }
            std::string class_error;
            if (!is_a(function, "Function", class_error))
            {
                error = class_error.empty() ? "the supplied reflected object is not a UFunction" : class_error;
                return false;
            }

            std::uint8_t declared_parameter_count{};
            std::uint16_t params_size{};
            std::uint16_t declared_return_offset{};
            std::uint64_t field_address{};
            if (!safe_read(function.address + 0xb4u, declared_parameter_count) ||
                !safe_read(function.address + 0xb6u, params_size) ||
                !safe_read(function.address + 0xb8u, declared_return_offset) ||
                !safe_read(function.address + k_child_properties_offset, field_address) ||
                params_size > 65535u)
            {
                error = "UFunction parameter layout is unreadable";
                return false;
            }

            std::vector<ReflectedProperty> input_properties;
            std::optional<ReflectedProperty> result_property;
            std::unordered_set<std::uint64_t> visited;
            std::size_t reflected_parameter_count{};
            const auto decode_name = [this](RawFName name, std::string& decoded, std::string& decode_error) {
                return fname_to_utf8(name, decoded, decode_error);
            };
            while (field_address != 0u)
            {
                if (visited.size() >= k_max_property_fields || !visited.emplace(field_address).second)
                {
                    error = "UFunction FProperty chain is cyclic or exceeds the safety limit";
                    return false;
                }
                FPropertyPrefixView prefix{};
                if (!safe_read(field_address, prefix))
                {
                    error = "UFunction FProperty chain became unreadable";
                    return false;
                }
                if ((prefix.property_flags & k_property_flag_parameter) != 0u)
                {
                    ReflectedProperty property;
                    if (!decode_function_property(field_address, params_size, decode_name, property, error))
                    {
                        return false;
                    }
                    ++reflected_parameter_count;
                    if ((property.flags & k_property_flag_return_parameter) != 0u)
                    {
                        if (result_property.has_value())
                        {
                            error = "UFunction has multiple reflected return properties";
                            return false;
                        }
                        result_property = property;
                    }
                    else
                    {
                        input_properties.push_back(std::move(property));
                    }
                }
                field_address = prefix.next;
            }
            if (reflected_parameter_count != declared_parameter_count)
            {
                error = "UFunction NumParms disagrees with its CPF_Parm property chain";
                return false;
            }
            if (input_properties.size() != arguments.size())
            {
                error = "UFunction expects " + std::to_string(input_properties.size()) + " argument(s), received " +
                        std::to_string(arguments.size());
                return false;
            }
            if (result_property.has_value() && result_property->offset != declared_return_offset)
            {
                error = "UFunction ReturnValueOffset disagrees with the return FProperty";
                return false;
            }
            const auto is_supported_marshaled_property = [this, &error](const ReflectedProperty& property) {
                if (is_supported_scalar_property(property))
                {
                    return true;
                }
                std::unordered_set<std::uint64_t> active;
                return validate_plain_property(*this, property, error, active, 0u);
            };
            if (result_property.has_value())
            {
                const auto& property = *result_property;
                if (!is_supported_marshaled_property(property))
                {
                    if (error.empty())
                    {
                        error = "UFunction return type '" + property.type_name +
                                "' requires Unreal value lifecycle support and is unavailable";
                    }
                    return false;
                }
            }
            for (const auto& property : input_properties)
            {
                const bool mutable_out = (property.flags & k_property_flag_out_parameter) != 0u &&
                                         (property.flags & k_property_flag_const_parameter) == 0u;
                if (mutable_out && !is_supported_marshaled_property(property))
                {
                    if (error.empty())
                    {
                        error = "UFunction OutParm '" + property.name + "' has unsupported type '" +
                                property.type_name + "'";
                    }
                    return false;
                }
            }

            std::vector<std::byte> params(params_size);
            std::vector<std::size_t> owned_string_offsets;
            std::vector<ReflectedProperty> owned_container_properties;
            std::vector<std::size_t> mutable_out_indices;
            const auto release_owned_values = [this,
                                               &params,
                                               &owned_string_offsets,
                                               &owned_container_properties](void*) noexcept {
                for (auto property = owned_container_properties.rbegin();
                     property != owned_container_properties.rend();
                     ++property)
                {
                    if (property->offset < 0 || static_cast<std::size_t>(property->offset) > params.size() ||
                        params.size() - static_cast<std::size_t>(property->offset) <
                                static_cast<std::size_t>(property->element_size))
                    {
                        continue;
                    }
                    std::string cleanup_error;
                    if (!release_plain_value(
                                *this,
                                *property,
                                std::bit_cast<std::uintptr_t>(params.data() + property->offset),
                                cleanup_error))
                    {
                        std::fprintf(stderr,
                                     "[UE4SS Linux] UFunction container cleanup failed: %s\n",
                                     cleanup_error.c_str());
                    }
                }
                for (const std::size_t offset : owned_string_offsets)
                {
                    if (offset > params.size() || params.size() - offset < sizeof(UnrealStringHeader))
                    {
                        continue;
                    }
                    UnrealStringHeader string{};
                    std::memcpy(&string, params.data() + offset, sizeof(string));
                    release_unreal_string(string);
                    std::memset(params.data() + offset, 0, sizeof(UnrealStringHeader));
                }
            };
            auto value_guard = std::unique_ptr<void, decltype(release_owned_values)>{
                    reinterpret_cast<void*>(std::uintptr_t{1}), release_owned_values};
            for (std::size_t index = 0; index < input_properties.size(); ++index)
            {
                const ReflectedProperty& property = input_properties[index];
                const bool mutable_out = (property.flags & k_property_flag_out_parameter) != 0u &&
                                         (property.flags & k_property_flag_const_parameter) == 0u;
                if (mutable_out)
                {
                    if (!arguments[index].out_placeholder)
                    {
                        error = "UFunction OutParm '" + property.name + "' requires a Lua table placeholder";
                        return false;
                    }
                    mutable_out_indices.push_back(index);
                    if (property.type_name == "StrProperty")
                    {
                        owned_string_offsets.push_back(static_cast<std::size_t>(property.offset));
                    }
                    else if (property.type_name == "TextProperty")
                    {
                        if (!initialize_reflected_value(
                                    property.address,
                                    std::bit_cast<std::uintptr_t>(params.data() + property.offset),
                                    error))
                        {
                            return false;
                        }
                        owned_container_properties.push_back(property);
                    }
                    else if (property.type_name == "StructProperty" || property.type_name == "ArrayProperty" ||
                             property.type_name == "SetProperty" || property.type_name == "MapProperty" ||
                             is_multicast_delegate_property(property.type_name) ||
                             is_soft_object_property(property.type_name))
                    {
                        owned_container_properties.push_back(property);
                    }
                    continue;
                }
                if (arguments[index].out_placeholder && property.type_name != "StructProperty" &&
                    property.type_name != "ArrayProperty" && property.type_name != "SetProperty" &&
                    property.type_name != "MapProperty" && property.type_name != "DelegateProperty")
                {
                    error = "UFunction parameter '" + property.name + "' is not a mutable OutParm";
                    return false;
                }
                UnrealPropertyValue normalized_argument;
                const UnrealPropertyValue* argument_pointer = &arguments[index].value;
                if (property.type_name == "ArrayProperty" &&
                    arguments[index].value.kind == UnrealPropertyKind::structure &&
                    arguments[index].value.struct_fields.empty())
                {
                    normalized_argument.kind = UnrealPropertyKind::array;
                    argument_pointer = &normalized_argument;
                }
                else if (property.type_name == "DelegateProperty" &&
                         arguments[index].value.kind == UnrealPropertyKind::structure)
                {
                    const UnrealPropertyValue* object =
                            find_struct_field(arguments[index].value, "Object");
                    const UnrealPropertyValue* function_name =
                            find_struct_field(arguments[index].value, "FunctionName");
                    normalized_argument.kind = UnrealPropertyKind::delegate;
                    if (object == nullptr || object->kind != UnrealPropertyKind::object)
                    {
                        error = "DelegateProperty argument requires an Object field containing nil or a UObject";
                        return false;
                    }
                    normalized_argument.object = object->object;
                    normalized_argument.object_is_null = object->object_is_null;
                    if (!normalized_argument.object_is_null)
                    {
                        if (function_name == nullptr || function_name->kind != UnrealPropertyKind::name)
                        {
                            error = "DelegateProperty argument requires an FName FunctionName field";
                            return false;
                        }
                        normalized_argument.name = function_name->name;
                        normalized_argument.text = function_name->text;
                    }
                    argument_pointer = &normalized_argument;
                }
                const UnrealPropertyValue& argument = *argument_pointer;
                if (argument.kind == UnrealPropertyKind::object && !argument.object_is_null &&
                    !is_valid(argument.object))
                {
                    error = "UFunction object argument is stale";
                    return false;
                }
                if (property.type_name == "StrProperty" &&
                    property.element_size == static_cast<std::int32_t>(sizeof(UnrealStringHeader)) &&
                    argument.kind == UnrealPropertyKind::string)
                {
                    UnrealStringHeader string{};
                    if (!allocate_unreal_string(argument.text, string, error))
                    {
                        return false;
                    }
                    std::memcpy(params.data() + property.offset, &string, sizeof(string));
                    owned_string_offsets.push_back(static_cast<std::size_t>(property.offset));
                }
                else if (property.type_name == "TextProperty" &&
                         property.element_size == static_cast<std::int32_t>(sizeof(FTextView)) &&
                         argument.kind == UnrealPropertyKind::text)
                {
                    const std::uint64_t destination =
                            std::bit_cast<std::uintptr_t>(params.data() + property.offset);
                    if (!initialize_reflected_value(property.address, destination, error))
                    {
                        return false;
                    }
                    owned_container_properties.push_back(property);
                    if (!assign_unreal_text(property.address, destination, argument.text, error))
                    {
                        return false;
                    }
                }
                else if (((property.type_name == "StructProperty" &&
                           argument.kind == UnrealPropertyKind::structure) ||
                          (property.type_name == "ArrayProperty" &&
                           argument.kind == UnrealPropertyKind::array) ||
                          (property.type_name == "SetProperty" &&
                           argument.kind == UnrealPropertyKind::set) ||
                          (property.type_name == "MapProperty" &&
                           argument.kind == UnrealPropertyKind::map) ||
                          (is_soft_object_property(property.type_name) &&
                           argument.kind == UnrealPropertyKind::soft_object)))
                {
                    const std::uint64_t destination =
                            std::bit_cast<std::uintptr_t>(params.data() + property.offset);
                    if (!encode_plain_value(*this, property, destination, argument, error))
                    {
                        return false;
                    }
                    owned_container_properties.push_back(property);
                }
                else if (!marshal_parameter(*this, property, argument, params, error))
                {
                    return false;
                }
            }

            if (result_property.has_value() && result_property->type_name == "StrProperty")
            {
                owned_string_offsets.push_back(static_cast<std::size_t>(result_property->offset));
            }
            else if (result_property.has_value() && result_property->type_name == "TextProperty")
            {
                if (!initialize_reflected_value(
                            result_property->address,
                            std::bit_cast<std::uintptr_t>(params.data() + result_property->offset),
                            error))
                {
                    return false;
                }
                owned_container_properties.push_back(*result_property);
            }
            else if (result_property.has_value() &&
                     (result_property->type_name == "StructProperty" ||
                      result_property->type_name == "ArrayProperty" ||
                      result_property->type_name == "SetProperty" ||
                      result_property->type_name == "MapProperty" ||
                      is_multicast_delegate_property(result_property->type_name) ||
                      is_soft_object_property(result_property->type_name)))
            {
                owned_container_properties.push_back(*result_property);
            }

            // FString storage is allocated and released through the resolved
            // engine allocator; no allocation crosses the runtime boundary.
            m_process_event(std::bit_cast<void*>(static_cast<std::uintptr_t>(context.address)),
                            std::bit_cast<void*>(static_cast<std::uintptr_t>(function.address)),
                            params.empty() ? nullptr : params.data());

            const auto decode_value = [this, &params, &error](const ReflectedProperty& property,
                                                              UnrealPropertyValue& result) -> bool {
                const std::byte* source = params.data() + property.offset;
                if (is_supported_scalar_property(property))
                {
                    return decode_plain_value(
                            *this,
                            property,
                            std::bit_cast<std::uintptr_t>(source),
                            result,
                            error);
                }
                if (property.type_name == "BoolProperty")
                {
                    const auto byte = std::to_integer<std::uint8_t>(source[property.boolean.byte_offset]);
                    result.kind = UnrealPropertyKind::boolean;
                    result.boolean = (byte & property.boolean.field_mask) != 0u;
                    return true;
                }
                if (is_signed_integer_property(property.type_name))
                {
                    std::uint64_t raw{};
                    std::memcpy(&raw, source, static_cast<std::size_t>(property.element_size));
                    result.kind = UnrealPropertyKind::signed_integer;
                    result.signed_integer = sign_extend_integer(raw, property.element_size);
                    return true;
                }
                if (is_unsigned_integer_property(property.type_name))
                {
                    result.kind = UnrealPropertyKind::unsigned_integer;
                    std::memcpy(&result.unsigned_integer, source, static_cast<std::size_t>(property.element_size));
                    return true;
                }
                if (property.type_name == "FloatProperty" && property.element_size == sizeof(float))
                {
                    float value{};
                    std::memcpy(&value, source, sizeof(value));
                    result.kind = UnrealPropertyKind::floating_point;
                    result.floating_point = value;
                    return true;
                }
                if (property.type_name == "DoubleProperty" && property.element_size == sizeof(double))
                {
                    result.kind = UnrealPropertyKind::floating_point;
                    std::memcpy(&result.floating_point, source, sizeof(result.floating_point));
                    return true;
                }
                if (property.type_name == "NameProperty" && property.element_size == sizeof(RawFName))
                {
                    result.kind = UnrealPropertyKind::name;
                    std::memcpy(&result.name, source, sizeof(result.name));
                    return fname_to_utf8(result.name, result.text, error);
                }
                if (property.type_name == "StrProperty" && property.element_size == sizeof(UnrealStringHeader))
                {
                    result.kind = UnrealPropertyKind::string;
                    return read_unreal_string(
                            std::bit_cast<std::uintptr_t>(params.data() + property.offset), result.text, error);
                }
                if (is_direct_object_property(property.type_name) && property.element_size == sizeof(void*))
                {
                    std::uint64_t address{};
                    std::memcpy(&address, source, sizeof(address));
                    result.kind = UnrealPropertyKind::object;
                    result.object_is_null = address == 0u;
                    if (!result.object_is_null && !handle_from_address(address, result.object))
                    {
                        error = "UFunction produced an object outside the validated GUObjectArray";
                        return false;
                    }
                    return true;
                }
                if ((property.type_name == "StructProperty" && property.struct_address != 0u) ||
                    (property.type_name == "ArrayProperty" && property.inner_property_address != 0u) ||
                    (property.type_name == "SetProperty" && property.inner_property_address != 0u) ||
                    (property.type_name == "MapProperty" && property.key_property_address != 0u &&
                     property.value_property_address != 0u) ||
                    property.type_name == "DelegateProperty" ||
                    is_soft_object_property(property.type_name) ||
                    is_multicast_delegate_property(property.type_name))
                {
                    return decode_plain_value(
                            *this,
                            property,
                            std::bit_cast<std::uintptr_t>(params.data() + property.offset),
                            result,
                            error);
                }
                error = "UFunction value type '" + property.type_name + "' is not supported";
                return false;
            };

            out_values.reserve(mutable_out_indices.size());
            for (const std::size_t index : mutable_out_indices)
            {
                UnrealFunctionOutValue out{.argument_index = index, .name = input_properties[index].name};
                if (!decode_value(input_properties[index], out.value))
                {
                    return false;
                }
                out_values.push_back(std::move(out));
            }

            if (result_property.has_value())
            {
                UnrealPropertyValue result;
                if (!decode_value(*result_property, result))
                {
                    return false;
                }
                return_value = std::move(result);
            }
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the ProcessEvent call boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::fname_to_utf8(RawFName name, std::string& output, std::string& error) const noexcept
    {
        output.clear();
        error.clear();
        if (!is_ready())
        {
            error = "read-only Unreal runtime is not initialized";
            return false;
        }

        try
        {
            UnrealStringHeader string{};
            m_fname_to_string(&name, &string);

            if (string.data == nullptr || string.num <= 0 || string.max < string.num || string.max > k_max_fname_code_units)
            {
                error = "FName::ToString returned an invalid FString header";
                return false;
            }

            std::vector<char16_t> copied(static_cast<std::size_t>(string.num));
            if (!safe_read_bytes(std::bit_cast<std::uintptr_t>(string.data),
                                 copied.data(),
                                 copied.size() * sizeof(char16_t)))
            {
                error = "FName::ToString returned an unreadable UTF-16 buffer";
                return false;
            }

            // The copy is now owned by this runtime. Release the foreign
            // allocation before UTF validation so conversion failures cannot
            // leak Unreal memory.
            m_free(m_allocator, string.data);
            if (copied.back() != u'\0')
            {
                error = "FName::ToString result is not null terminated";
                return false;
            }
            output = ue4ss::linux::unreal_to_utf8(std::u16string_view{copied.data(), copied.size() - 1u});
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the read-only Unreal call boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::fname_from_utf8(std::string_view input,
                                                RawFName& output,
                                                bool add_if_missing,
                                                std::string& error) const noexcept
    {
        output = {};
        error.clear();
        if (!is_ready())
        {
            error = "Unreal FName runtime is not initialized";
            return false;
        }
        if (input.find('\0') != std::string_view::npos)
        {
            error = "FName input contains an embedded null byte";
            return false;
        }
        try
        {
            std::u16string converted = ue4ss::linux::utf8_to_unreal(input);
            if (converted.size() >= static_cast<std::size_t>(k_max_fname_code_units))
            {
                error = "FName input exceeds the bounded UTF-16 length";
                return false;
            }
            converted.push_back(u'\0');
            // EFindName is FNAME_Find=0, FNAME_Add=1 in UE 5.1. The resolver
            // identifies the Linux member constructor, hence the explicit
            // destination (`this`) argument required by the SysV ABI.
            m_fname_constructor(&output, converted.c_str(), add_if_missing ? 1 : 0);
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception at the FName construction boundary";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::read_unreal_string(std::uint64_t address,
                                                   std::string& output,
                                                   std::string& error) const noexcept
    {
        output.clear();
        error.clear();
        UnrealStringHeader string{};
        if (!safe_read(address, string) || string.num < 0 || string.max < string.num ||
            string.max > k_max_unreal_string_code_units ||
            (string.max == 0 && string.data != nullptr) || (string.max > 0 && string.data == nullptr))
        {
            error = "FString has an invalid array header";
            return false;
        }
        if (string.num == 0)
        {
            return true;
        }
        std::vector<char16_t> copied(static_cast<std::size_t>(string.num));
        if (!safe_read_bytes(std::bit_cast<std::uintptr_t>(string.data),
                             copied.data(),
                             copied.size() * sizeof(char16_t)) ||
            copied.back() != u'\0')
        {
            error = "FString buffer is unreadable or not null terminated";
            return false;
        }
        try
        {
            output = ue4ss::linux::unreal_to_utf8(
                    std::u16string_view{copied.data(), copied.size() - 1u});
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::allocate_unreal_string(std::string_view input,
                                                       UnrealStringHeader& output,
                                                       std::string& error) const noexcept
    {
        output = {};
        error.clear();
        if (!is_ready())
        {
            error = "Unreal allocator is not initialized";
            return false;
        }
        if (input.find('\0') != std::string_view::npos)
        {
            error = "FString input contains an embedded null byte";
            return false;
        }
        try
        {
            std::u16string converted = ue4ss::linux::utf8_to_unreal(input);
            if (converted.empty())
            {
                return true;
            }
            if (converted.size() >= static_cast<std::size_t>(k_max_unreal_string_code_units))
            {
                error = "FString input exceeds the bounded UTF-16 length";
                return false;
            }
            converted.push_back(u'\0');
            const std::size_t bytes = converted.size() * sizeof(char16_t);
            void* allocation = m_realloc(m_allocator, nullptr, bytes, 0u);
            if (allocation == nullptr)
            {
                error = "FMalloc::Realloc returned null while constructing FString";
                return false;
            }
            if (!safe_write_bytes(std::bit_cast<std::uintptr_t>(allocation), converted.data(), bytes))
            {
                m_free(m_allocator, allocation);
                error = "new FString allocation is not writable";
                return false;
            }
            output = {
                    .data = static_cast<char16_t*>(allocation),
                    .num = static_cast<std::int32_t>(converted.size()),
                    .max = static_cast<std::int32_t>(converted.size()),
            };
            return true;
        }
        catch (const std::exception& exception)
        {
            error = exception.what();
            return false;
        }
        catch (...)
        {
            error = "unknown exception while constructing FString storage";
            return false;
        }
    }

    bool ReadOnlyUnrealRuntime::allocate_unreal_storage(std::size_t bytes,
                                                        std::uint32_t alignment,
                                                        void*& output,
                                                        std::string& error) const noexcept
    {
        output = nullptr;
        error.clear();
        if (!is_ready() || m_realloc == nullptr || m_allocator == nullptr || bytes == 0u ||
            bytes > static_cast<std::size_t>(k_max_properties_size) *
                            static_cast<std::size_t>(k_max_script_array_elements))
        {
            error = "validated FMalloc and a bounded positive allocation size are required";
            return false;
        }
        if (alignment != 0u && (!std::has_single_bit(alignment) || alignment > 4096u))
        {
            error = "Unreal allocation alignment is invalid";
            return false;
        }
        output = m_realloc(m_allocator, nullptr, bytes, alignment);
        if (output == nullptr)
        {
            error = "FMalloc::Realloc returned null while allocating container storage";
            return false;
        }
        return true;
    }

    void ReadOnlyUnrealRuntime::release_unreal_storage(void* allocation) const noexcept
    {
        if (allocation != nullptr && m_free != nullptr && m_allocator != nullptr)
        {
            m_free(m_allocator, allocation);
        }
    }

    void ReadOnlyUnrealRuntime::release_unreal_string(UnrealStringHeader& string) const noexcept
    {
        if (string.data != nullptr && m_free != nullptr && m_allocator != nullptr)
        {
            m_free(m_allocator, string.data);
        }
        string = {};
    }

    bool ReadOnlyUnrealRuntime::assign_unreal_string(std::uint64_t address,
                                                     std::string_view input,
                                                     std::string& error) const noexcept
    {
        error.clear();
        UnrealStringHeader previous{};
        if (!safe_read(address, previous) || previous.num < 0 || previous.max < previous.num ||
            previous.max > k_max_unreal_string_code_units ||
            (previous.max == 0 && previous.data != nullptr) ||
            (previous.max > 0 && previous.data == nullptr))
        {
            error = "destination FString has an invalid array header";
            return false;
        }

        UnrealStringHeader replacement{};
        if (!allocate_unreal_string(input, replacement, error))
        {
            return false;
        }
        if (!safe_write_bytes(address, &replacement, sizeof(replacement)))
        {
            release_unreal_string(replacement);
            error = "destination FString header is not writable";
            return false;
        }
        release_unreal_string(previous);
        return true;
    }

    FNameRuntimeValidation validate_fname_runtime_abi(const ResolvedUnrealState& resolved) noexcept
    {
        try
        {
            ReadOnlyUnrealRuntime runtime;
            std::string detail;
            const bool success = runtime.initialize(resolved, detail);
            return {success, std::move(detail)};
        }
        catch (const std::exception& exception)
        {
            return {false, exception.what()};
        }
        catch (...)
        {
            return {false, "unknown exception while validating the FName runtime ABI"};
        }
    }

    ReadOnlyObjectApiValidation validate_readonly_object_api(ReadOnlyUnrealRuntime& runtime) noexcept
    {
        try
        {
            if (!runtime.is_ready())
            {
                return {false, "validated read-only Unreal runtime is required"};
            }
            const ObjectQueryResult packages = runtime.find_all_of("Package", 1u);
            if (!packages.success || packages.objects.empty())
            {
                return {false, "read-only FindAllOf('Package') conformance failed: " + packages.detail};
            }
            ReadOnlyUObjectDescription package;
            std::string error;
            if (!runtime.describe_object(packages.objects.front(), package, error) || package.class_name != "Package" ||
                package.path_name.empty())
            {
                return {false, "read-only UObject description conformance failed: " + error};
            }
            return {true,
                    "read-only GUObjectArray traversal, class hierarchy, FName, and outer path passed; sample=" + package.full_name};
        }
        catch (const std::exception& exception)
        {
            return {false, exception.what()};
        }
        catch (...)
        {
            return {false, "unknown exception while validating the read-only UObject API"};
        }
    }

    ReadOnlyObjectApiValidation validate_readonly_object_api(const ResolvedUnrealState& resolved) noexcept
    {
        try
        {
            ReadOnlyUnrealRuntime runtime;
            std::string detail;
            if (!runtime.initialize(resolved, detail))
            {
                return {false, std::move(detail)};
            }
            return validate_readonly_object_api(runtime);
        }
        catch (const std::exception& exception)
        {
            return {false, exception.what()};
        }
        catch (...)
        {
            return {false, "unknown exception while initializing the read-only UObject API"};
        }
    }
} // namespace ue4ss::linux::core
