#include "unreal_readonly.hpp"
#include "unreal_container_bridge.hpp"
#include "console_exec_hooks.hpp"
#if UE4SS_WITH_LUA_RUNTIME
#include "headless_lua.hpp"
#include "game_thread_scheduler.hpp"
#include "ufunction_hooks.hpp"
#include "object_notifications.hpp"
#include "blueprint_hooks.hpp"
#endif

#include "ue4ss/patternsleuth_abi.h"

#include <Unreal/UObjectGlobals.hpp>

#include <array>
#include <algorithm>
#include <cstddef>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <sys/mman.h>

namespace
{
    using UEPseudoStaticConstructObjectParameters =
            RC::Unreal::UObjectGlobals::Below56::FStaticConstructObjectParameters;

    static_assert(offsetof(UEPseudoStaticConstructObjectParameters, Name) == 0x10);
    static_assert(offsetof(UEPseudoStaticConstructObjectParameters, Template) == 0x28);
    static_assert(offsetof(UEPseudoStaticConstructObjectParameters, PropertyInitCallback) == 0x40);
    static_assert(offsetof(UEPseudoStaticConstructObjectParameters, SubobjectOverrides) == 0x80);
    static_assert(alignof(UEPseudoStaticConstructObjectParameters) == 0x10);
    static_assert(sizeof(UEPseudoStaticConstructObjectParameters) == 0x90);

    struct alignas(16) CapturedStaticConstructObjectParameters
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

    static_assert(alignof(CapturedStaticConstructObjectParameters) == 0x10);
    static_assert(sizeof(CapturedStaticConstructObjectParameters) == 0x90);

    struct TestScriptArray
    {
        std::uint64_t data{};
        std::int32_t num{};
        std::int32_t max{};
    };

#pragma pack(push, 1)
    struct TestNativeFURL
    {
        ue4ss::linux::core::UnrealStringHeader protocol;
        ue4ss::linux::core::UnrealStringHeader host;
        std::int32_t port{};
        std::int32_t valid{};
        ue4ss::linux::core::UnrealStringHeader map;
        ue4ss::linux::core::UnrealStringHeader redirect_url;
        TestScriptArray options;
        ue4ss::linux::core::UnrealStringHeader portal;
    };
#pragma pack(pop)

    static_assert(sizeof(TestNativeFURL) == 0x68);

    struct FakeAllocator
    {
        void** vtable{};
    };

    std::size_t g_free_calls{};
    std::size_t g_realloc_calls{};
    std::size_t g_tick_calls{};
    std::size_t g_native_function_calls{};
    std::size_t g_allocate_uobject_calls{};
    std::size_t g_process_local_script_calls{};
    std::size_t g_process_event_calls{};
    bool g_capture_static_construct_parameters{};
    void* g_static_construct_result{};
    CapturedStaticConstructObjectParameters g_static_construct_parameters{};
    std::array<std::uint64_t, 2> g_fake_output_targets{};
    std::array<std::byte, 0x50> g_fake_output_map_header{};

    void fake_game_engine_tick(void*, float, bool)
    {
        ++g_tick_calls;
    }

    void fake_native_function(void*, void* stack, void* result)
    {
        ++g_native_function_calls;
        if (stack != nullptr && result != nullptr)
        {
            std::uint64_t locals{};
            std::memcpy(&locals, static_cast<std::byte*>(stack) + 0x28u, sizeof(locals));
            std::int32_t value{};
            std::memcpy(&value, reinterpret_cast<const void*>(locals), sizeof(value));
            value += 1;
            std::memcpy(result, &value, sizeof(value));
        }
    }

    __attribute__((noinline)) void fake_process_local_script_function(void*, void*, void*)
    {
        ++g_process_local_script_calls;
        asm volatile("nop; nop; nop; nop; nop; nop; nop; nop" ::: "memory");
    }

    __attribute__((noinline)) void* fake_static_construct_object(const void* parameters)
    {
        ++g_allocate_uobject_calls;
        if (g_capture_static_construct_parameters)
        {
            std::memcpy(&g_static_construct_parameters,
                        parameters,
                        sizeof(g_static_construct_parameters));
        }
        asm volatile("" ::: "memory");
        return g_static_construct_result != nullptr
                ? g_static_construct_result
                : const_cast<void*>(parameters);
    }

    void fake_free(void*, void* allocation)
    {
        ++g_free_calls;
        std::free(allocation);
    }

    void* fake_realloc(void*, void* allocation, std::size_t size, std::uint32_t)
    {
        ++g_realloc_calls;
        return std::realloc(allocation, size);
    }

    static constexpr std::array<const char16_t*, 92> g_names{
            u"None", u"Class", u"Package", u"Derived", u"Base", u"/Script/Test", u"Instance",
            u"IntProperty", u"BoolProperty", u"Health", u"Alive", u"Function", u"GetHealth",
            u"Value", u"ReturnValue", u"NameProperty", u"StrProperty", u"Label", u"Greeting",
            u"GetOutputs", u"OutValue", u"OutGreeting", u"StructProperty", u"ScriptStruct", u"Vector",
            u"DoubleProperty", u"X", u"Y", u"Z", u"Location", u"ScaleVector", u"Factor",
            u"ArrayProperty", u"Targets", u"ObjectProperty", u"GetTargets", u"OutTargets", u"CountTargets",
            u"Messages", u"SetProperty", u"MapProperty", u"Scores", u"Lookup",
            u"WeakObjectProperty", u"DelegateProperty", u"MulticastInlineDelegateProperty", u"InterfaceProperty",
            u"WeakTarget", u"Handler", u"Handlers", u"InterfaceTarget", u"OnHandled", u"UInt32Property",
            u"Enum", u"TestMode", u"TestMode::Disabled", u"TestMode::Enabled", u"TestMode::MAX", u"Mode",
            u"EnumProperty", u"Int8Property", u"EchoMode", u"CountScores", u"GetLookup", u"OutLookup",
            u"EchoDelegate", u"GetHandlers", u"OutHandlers", u"SoftObjectProperty", u"SoftTarget",
            u"/Game/Test.Asset", u"Component", u"EchoSoft", u"/Game/Test", u"Asset",
            u"TextProperty", u"Title", u"MulticastSparseDelegateProperty", u"SparseHandlers",
            u"World", u"Level", u"Actor", u"TestWorld", u"PersistentLevel", u"Hero",
            u"DataTable", u"TestTable", u"RowA", u"RowB",
            u"TestMode::Renamed", u"TestMode::Inserted", u"RowC"};

    void fake_fname_constructor(ue4ss::linux::core::RawFName* output,
                                const char16_t* input,
                                std::int32_t)
    {
        assert(output != nullptr && input != nullptr);
        *output = {};
        const std::u16string_view requested{input};
        for (std::size_t index = 0; index < g_names.size(); ++index)
        {
            if (requested == g_names[index])
            {
                output->comparison_index = static_cast<std::uint32_t>(index);
                return;
            }
        }
    }

    void fake_fname_to_string(const ue4ss::linux::core::RawFName* name,
                              ue4ss::linux::core::UnrealStringHeader* output)
    {
        assert(name != nullptr);
        assert(name->comparison_index < g_names.size());
        assert(name->number == 0u);
        const char16_t* expected = g_names[name->comparison_index];
        const std::size_t code_units = std::char_traits<char16_t>::length(expected) + 1u;
        auto* allocation = static_cast<char16_t*>(std::malloc(code_units * sizeof(char16_t)));
        assert(allocation != nullptr);
        std::memcpy(allocation, expected, code_units * sizeof(char16_t));
        *output = {
                .data = allocation,
                .num = static_cast<std::int32_t>(code_units),
                .max = static_cast<std::int32_t>(code_units),
        };
    }

    void fake_process_event(void*, void* function, void* parameters)
    {
        ++g_process_event_calls;
        assert(function != nullptr && parameters != nullptr);
        ue4ss::linux::core::RawFName function_name{};
        std::memcpy(&function_name, static_cast<std::byte*>(function) + 0x18u, sizeof(function_name));
        if (function_name.comparison_index == 12u)
        {
            std::int32_t input{};
            std::memcpy(&input, parameters, sizeof(input));
            input += 2;
            std::memcpy(static_cast<std::byte*>(parameters) + sizeof(input), &input, sizeof(input));
            return;
        }
        if (function_name.comparison_index == 30u)
        {
            std::array<double, 3> input{};
            double factor{};
            std::memcpy(input.data(), parameters, sizeof(input));
            std::memcpy(&factor, static_cast<std::byte*>(parameters) + 24u, sizeof(factor));
            for (double& component : input)
            {
                component *= factor;
            }
            std::memcpy(static_cast<std::byte*>(parameters) + 32u, input.data(), sizeof(input));
            return;
        }
        if (function_name.comparison_index == 35u)
        {
            auto* allocation = static_cast<std::uint64_t*>(std::malloc(sizeof(g_fake_output_targets)));
            assert(allocation != nullptr);
            std::memcpy(allocation, g_fake_output_targets.data(), sizeof(g_fake_output_targets));
            const std::uint64_t data = reinterpret_cast<std::uintptr_t>(allocation);
            const std::int32_t num = static_cast<std::int32_t>(g_fake_output_targets.size());
            std::memcpy(parameters, &data, sizeof(data));
            std::memcpy(static_cast<std::byte*>(parameters) + 8u, &num, sizeof(num));
            std::memcpy(static_cast<std::byte*>(parameters) + 12u, &num, sizeof(num));
            return;
        }
        if (function_name.comparison_index == 37u)
        {
            std::int32_t num{};
            std::memcpy(&num, static_cast<std::byte*>(parameters) + 8u, sizeof(num));
            std::memcpy(static_cast<std::byte*>(parameters) + 16u, &num, sizeof(num));
            return;
        }
        if (function_name.comparison_index == 61u)
        {
            std::int8_t input{};
            std::memcpy(&input, parameters, sizeof(input));
            std::memcpy(static_cast<std::byte*>(parameters) + 1u, &input, sizeof(input));
            return;
        }
        if (function_name.comparison_index == 62u)
        {
            std::int32_t num{};
            std::int32_t num_free{};
            std::memcpy(&num, static_cast<std::byte*>(parameters) + 8u, sizeof(num));
            std::memcpy(&num_free, static_cast<std::byte*>(parameters) + 0x34u, sizeof(num_free));
            const std::int32_t count = num - num_free;
            std::memcpy(static_cast<std::byte*>(parameters) + 0x50u, &count, sizeof(count));
            return;
        }
        if (function_name.comparison_index == 63u)
        {
            std::memcpy(parameters, g_fake_output_map_header.data(), g_fake_output_map_header.size());
            return;
        }
        if (function_name.comparison_index == 65u)
        {
            std::memcpy(static_cast<std::byte*>(parameters) + 0x10u, parameters, 0x10u);
            return;
        }
        if (function_name.comparison_index == 66u)
        {
            auto* allocation = static_cast<std::byte*>(std::malloc(0x10u));
            assert(allocation != nullptr);
            const std::int32_t object_index = 5;
            const std::int32_t serial_number = 105;
            const ue4ss::linux::core::RawFName bound_function{12u, 0u};
            std::memcpy(allocation, &object_index, sizeof(object_index));
            std::memcpy(allocation + 4u, &serial_number, sizeof(serial_number));
            std::memcpy(allocation + 8u, &bound_function, sizeof(bound_function));
            struct OutputArray
            {
                std::uintptr_t data{};
                std::int32_t num{};
                std::int32_t max{};
            };
            const OutputArray output{
                    .data = reinterpret_cast<std::uintptr_t>(allocation),
                    .num = 1,
                    .max = 1,
            };
            std::memcpy(parameters, &output, sizeof(output));
            return;
        }
        if (function_name.comparison_index == 72u)
        {
            constexpr std::size_t soft_size = 0x30u;
            constexpr std::size_t sub_path_offset = 0x20u;
            std::memcpy(static_cast<std::byte*>(parameters) + soft_size,
                        parameters,
                        soft_size);
            ue4ss::linux::core::UnrealStringHeader input_sub_path{};
            std::memcpy(&input_sub_path,
                        static_cast<std::byte*>(parameters) + sub_path_offset,
                        sizeof(input_sub_path));
            ue4ss::linux::core::UnrealStringHeader output_sub_path{};
            if (input_sub_path.num > 0)
            {
                const std::size_t bytes =
                        static_cast<std::size_t>(input_sub_path.num) * sizeof(char16_t);
                output_sub_path.data = static_cast<char16_t*>(std::malloc(bytes));
                assert(output_sub_path.data != nullptr);
                std::memcpy(output_sub_path.data, input_sub_path.data, bytes);
                output_sub_path.num = input_sub_path.num;
                output_sub_path.max = input_sub_path.num;
            }
            std::memcpy(static_cast<std::byte*>(parameters) + soft_size + sub_path_offset,
                        &output_sub_path,
                        sizeof(output_sub_path));
            return;
        }
        assert(function_name.comparison_index == 19u);
        const std::int32_t out_value = 77;
        std::memcpy(parameters, &out_value, sizeof(out_value));
        constexpr char16_t output_text[] = u"Out ✓";
        auto* allocation = static_cast<char16_t*>(std::malloc(sizeof(output_text)));
        assert(allocation != nullptr);
        std::memcpy(allocation, output_text, sizeof(output_text));
        const ue4ss::linux::core::UnrealStringHeader output_string{
                .data = allocation,
                .num = static_cast<std::int32_t>(std::size(output_text)),
                .max = static_cast<std::int32_t>(std::size(output_text)),
        };
        std::memcpy(static_cast<std::byte*>(parameters) + 8u, &output_string, sizeof(output_string));
    }

    std::int32_t fake_property_element_size(const void* property)
    {
        std::int32_t size{};
        std::memcpy(&size, static_cast<const std::byte*>(property) + 0x38u, sizeof(size));
        return size;
    }

    bool fake_property_identical(const void* property,
                                 const void* left,
                                 const void* right,
                                 std::uint32_t)
    {
        const std::int32_t size = fake_property_element_size(property);
        return size > 0 && std::memcmp(left, right, static_cast<std::size_t>(size)) == 0;
    }

    std::uint32_t fake_property_hash(const void* property, const void* value)
    {
        const std::int32_t size = fake_property_element_size(property);
        const auto* bytes = static_cast<const std::uint8_t*>(value);
        std::uint32_t hash = 2166136261u;
        for (std::int32_t index = 0; index < size; ++index)
        {
            hash = (hash ^ bytes[index]) * 16777619u;
        }
        return hash;
    }

    void fake_property_copy(const void* property, void* destination, const void* source)
    {
        const std::int32_t size = fake_property_element_size(property);
        assert(size > 0);
        std::memcpy(destination, source, static_cast<std::size_t>(size));
    }

    void fake_property_initialize(const void* property, void* destination)
    {
        const std::int32_t size = fake_property_element_size(property);
        assert(size > 0);
        std::memset(destination, 0, static_cast<std::size_t>(size));
    }

    void fake_property_destroy(const void*, void*)
    {
    }

    std::int32_t fake_property_alignment(const void* property)
    {
        const std::int32_t size = fake_property_element_size(property);
        if (size >= 8) return 8;
        if (size >= 4) return 4;
        if (size >= 2) return 2;
        return 1;
    }

    const char16_t* fake_property_import_text(const void* property,
                                              const char16_t* buffer,
                                              void* destination,
                                              std::int32_t pointer_type,
                                              void*,
                                              std::int32_t port_flags,
                                              void*)
    {
        if (property == nullptr || buffer == nullptr || destination == nullptr ||
            fake_property_element_size(property) != static_cast<std::int32_t>(sizeof(std::int32_t)) ||
            pointer_type != 0 || (port_flags & 0x08000000) == 0)
        {
            return nullptr;
        }
        const char16_t* cursor = buffer;
        bool negative = false;
        if (*cursor == u'-')
        {
            negative = true;
            ++cursor;
        }
        if (*cursor < u'0' || *cursor > u'9') return nullptr;
        std::int64_t value{};
        while (*cursor >= u'0' && *cursor <= u'9')
        {
            value = value * 10 + static_cast<std::int64_t>(*cursor - u'0');
            ++cursor;
        }
        if (negative) value = -value;
        if (value < std::numeric_limits<std::int32_t>::min() ||
            value > std::numeric_limits<std::int32_t>::max())
        {
            return nullptr;
        }
        const auto imported = static_cast<std::int32_t>(value);
        std::memcpy(destination, &imported, sizeof(imported));
        return cursor;
    }

    struct FakeScriptArray
    {
        std::uint64_t data{};
        std::int32_t num{};
        std::int32_t max{};
    };

    struct FakeEnumNamePair
    {
        ue4ss::linux::core::RawFName name;
        std::int64_t value{};
    };

    struct FakeScriptSet
    {
        FakeScriptArray elements;
        std::array<std::uint32_t, 4> allocation_flags_inline{};
        std::uint64_t allocation_flags_secondary{};
        std::int32_t allocation_flags_num_bits{};
        std::int32_t allocation_flags_max_bits{};
        std::int32_t first_free_index{-1};
        std::int32_t num_free_indices{};
        std::int32_t hash_inline{};
        std::int32_t hash_inline_padding{};
        std::uint64_t hash_secondary{};
        std::int32_t hash_size{};
        std::int32_t padding{};
    };

    struct FakeWeakObjectPtr
    {
        std::int32_t object_index{};
        std::int32_t object_serial_number{};
    };

    struct FakeScriptDelegate
    {
        FakeWeakObjectPtr object;
        ue4ss::linux::core::RawFName function_name;
    };

    struct FakeImplementedInterface
    {
        std::uint64_t interface_class{};
        std::int32_t pointer_offset{};
        std::uint8_t implemented_by_k2{};
        std::array<std::byte, 3> padding{};
    };

    struct FakeSoftObjectPtr
    {
        FakeWeakObjectPtr weak_ptr;
        std::int32_t tag_at_last_test{};
        std::int32_t padding{};
        ue4ss::linux::core::RawFName package_name;
        ue4ss::linux::core::RawFName asset_name;
        ue4ss::linux::core::UnrealStringHeader sub_path_string;
    };

    struct FakeTextData
    {
        void** vtable{};
        ue4ss::linux::core::UnrealStringHeader display_string;
    };

    struct FakeFText
    {
        std::uint64_t data{};
        std::uint64_t shared_ref_collector{};
        std::uint32_t flags{};
        std::uint32_t unknown{};
    };

    const ue4ss::linux::core::UnrealStringHeader* fake_text_get_display_string(const void* data)
    {
        return &static_cast<const FakeTextData*>(data)->display_string;
    }

    struct FakeUObject
    {
        std::uint64_t vtable{1u};
        std::uint32_t object_flags{};
        std::int32_t internal_index{};
        std::uint64_t class_private{};
        ue4ss::linux::core::RawFName name_private{};
        std::uint64_t outer_private{};
        std::uint64_t field_next{};
        std::array<std::byte, 0x10> padding{};
        std::uint64_t super_struct{};
        std::uint64_t children{};
        std::uint64_t child_properties{};
        std::int32_t properties_size{};
        std::int32_t min_alignment{};
        std::array<std::byte, 0x20> struct_padding{};
        std::int32_t health{};
        std::uint8_t alive{};
        std::array<std::byte, 3> value_padding{};
        ue4ss::linux::core::RawFName label{};
        ue4ss::linux::core::UnrealStringHeader greeting{};
        std::array<double, 3> location{};
        FakeScriptArray targets{};
        std::array<std::byte, 0x10> function_padding{};
        union
        {
            std::uint64_t native_function{};
            FakeScriptArray messages;
        };
        FakeScriptSet scores{};
        FakeScriptSet lookup{};
        FakeWeakObjectPtr weak_target{};
        FakeScriptDelegate handler{};
        FakeScriptArray handlers{};
        std::array<std::uint64_t, 2> interface_target{};
        std::int8_t mode{};
        FakeSoftObjectPtr soft_target{};
        FakeFText title{};
        std::uint8_t sparse_handlers{};
    };

    struct FakeFFieldClass
    {
        ue4ss::linux::core::RawFName name{};
        std::array<std::byte, 0x38> padding{};
    };

    struct FakeProperty
    {
        std::uint64_t vtable{1u};
        std::uint64_t class_private{};
        std::array<std::byte, 0x10> owner{};
        std::uint64_t next{};
        ue4ss::linux::core::RawFName name{};
        std::uint32_t flags{};
        std::int32_t array_dim{1};
        std::int32_t element_size{};
        std::uint32_t field_padding{};
        std::uint64_t property_flags{};
        std::array<std::byte, 4> replication{};
        std::int32_t offset_internal{};
        std::array<std::byte, 0x28> property_padding{};
        std::uint8_t field_size{};
        std::uint8_t byte_offset{};
        std::uint8_t byte_mask{};
        std::uint8_t field_mask{};
        std::array<std::byte, 12> tail_padding{};
    };

    struct ObjectItemView
    {
        std::uint64_t object{};
        std::int32_t flags{};
        std::int32_t cluster_root_index{};
        std::int32_t serial_number{};
        std::int32_t padding{};
    };

    struct ObjectArrayView
    {
        std::uint64_t objects{};
        std::uint64_t preallocated_objects{};
        std::int32_t max_elements{};
        std::int32_t num_elements{};
        std::int32_t max_chunks{};
        std::int32_t num_chunks{};
    };

    static_assert(offsetof(FakeUObject, super_struct) == 0x40);
    static_assert(offsetof(FakeUObject, child_properties) == 0x50);
    static_assert(offsetof(FakeUObject, properties_size) == 0x58);
    static_assert(offsetof(FakeUObject, health) == 0x80);
    static_assert(offsetof(FakeUObject, label) == 0x88);
    static_assert(offsetof(FakeUObject, greeting) == 0x90);
    static_assert(offsetof(FakeUObject, location) == 0xa0);
    static_assert(offsetof(FakeUObject, targets) == 0xb8);
    static_assert(offsetof(FakeUObject, native_function) == 0xd8);
    static_assert(offsetof(FakeUObject, messages) == 0xd8);
    static_assert(offsetof(FakeUObject, scores) == 0xe8);
    static_assert(offsetof(FakeUObject, lookup) == 0x138);
    static_assert(offsetof(FakeUObject, weak_target) == 0x188);
    static_assert(offsetof(FakeUObject, handler) == 0x190);
    static_assert(offsetof(FakeUObject, handlers) == 0x1a0);
    static_assert(offsetof(FakeUObject, interface_target) == 0x1b0);
    static_assert(offsetof(FakeUObject, mode) == 0x1c0);
    static_assert(offsetof(FakeUObject, soft_target) == 0x1c8);
    static_assert(offsetof(FakeUObject, title) == 0x1f8);
    static_assert(offsetof(FakeUObject, sparse_handlers) == 0x210);
    static_assert(offsetof(FakeProperty, offset_internal) == 0x4c);
    static_assert(offsetof(FakeProperty, field_size) == 0x78);
    static_assert(sizeof(FakeScriptSet) == 0x50);
    static_assert(sizeof(FakeWeakObjectPtr) == 0x8);
    static_assert(sizeof(FakeScriptDelegate) == 0x10);
    static_assert(sizeof(FakeImplementedInterface) == 0x10);
    static_assert(offsetof(FakeSoftObjectPtr, package_name) == 0x10);
    static_assert(offsetof(FakeSoftObjectPtr, sub_path_string) == 0x20);
    static_assert(sizeof(FakeSoftObjectPtr) == 0x30);
    static_assert(sizeof(FakeFText) == 0x18);
    static_assert(sizeof(ObjectItemView) == 0x18);
    static_assert(sizeof(ObjectArrayView) == 0x20);

    std::vector<FakeScriptDelegate> g_sparse_bindings;
    FakeScriptArray g_sparse_invocation_list;

    void refresh_sparse_invocation_list(std::uint8_t* bound)
    {
        g_sparse_invocation_list = {
                .data = reinterpret_cast<std::uintptr_t>(g_sparse_bindings.data()),
                .num = static_cast<std::int32_t>(g_sparse_bindings.size()),
                .max = static_cast<std::int32_t>(g_sparse_bindings.size()),
        };
        if (g_sparse_bindings.empty()) g_sparse_invocation_list.data = 0u;
        if (bound != nullptr) *bound = g_sparse_bindings.empty() ? 0u : 1u;
    }

    const FakeScriptArray* fake_sparse_get_delegate(const void*, const void* value)
    {
        if (value == nullptr || *static_cast<const std::uint8_t*>(value) == 0u) return nullptr;
        return &g_sparse_invocation_list;
    }

    void fake_sparse_add_delegate(const void*,
                                  FakeScriptDelegate requested,
                                  void*,
                                  void* value)
    {
        const auto same = [&requested](const FakeScriptDelegate& binding) {
            return std::memcmp(&binding, &requested, sizeof(binding)) == 0;
        };
        if (std::none_of(g_sparse_bindings.begin(), g_sparse_bindings.end(), same))
        {
            g_sparse_bindings.push_back(requested);
        }
        refresh_sparse_invocation_list(static_cast<std::uint8_t*>(value));
    }

    void fake_sparse_remove_delegate(const void*,
                                     const FakeScriptDelegate* requested,
                                     void*,
                                     void* value)
    {
        if (requested != nullptr)
        {
            std::erase_if(g_sparse_bindings, [requested](const FakeScriptDelegate& binding) {
                return std::memcmp(&binding, requested, sizeof(binding)) == 0;
            });
        }
        refresh_sparse_invocation_list(static_cast<std::uint8_t*>(value));
    }

    void fake_sparse_clear_delegate(const void*, void*, void* value)
    {
        g_sparse_bindings.clear();
        refresh_sparse_invocation_list(static_cast<std::uint8_t*>(value));
    }
}

int main()
{
    const auto command_parts = ue4ss::linux::core::tokenize_console_command(
            R"(ue4ss_probe alpha "two words" 'three words' "escaped\"quote")");
    assert((command_parts == std::vector<std::string>{
                                     "ue4ss_probe",
                                     "alpha",
                                     "two words",
                                     "three words",
                                     "escaped\"quote"}));

    constexpr std::size_t thunk_page_size = 4096u;
    auto* thunk_page = static_cast<std::uint8_t*>(mmap(
            nullptr,
            thunk_page_size,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0));
    assert(thunk_page != MAP_FAILED);
    std::memset(thunk_page, 0xcc, thunk_page_size);
    auto* thunk_target = thunk_page + 0x100u;
    const std::array<std::uint8_t, 8> target_prologue{
            0x55u,                   // push rbp
            0x48u, 0x89u, 0xe5u,    // mov rbp, rsp
            0x45u, 0x89u, 0xc6u,    // mov r14d, r8d
            0xc3u};                  // ret
    std::memcpy(thunk_target, target_prologue.data(), target_prologue.size());
    thunk_page[0] = 0x45u; // xor r8d, r8d
    thunk_page[1] = 0x31u;
    thunk_page[2] = 0xc0u;
    thunk_page[3] = 0xe9u; // jmp rel32
    const auto relative_target = static_cast<std::int32_t>(
            reinterpret_cast<std::uintptr_t>(thunk_target) -
            (reinterpret_cast<std::uintptr_t>(thunk_page) + 8u));
    std::memcpy(thunk_page + 4u, &relative_target, sizeof(relative_target));
    std::string thunk_error;
    const auto resolved_thunk =
            ue4ss::linux::core::resolve_call_function_by_name_with_arguments_thunk(
                    reinterpret_cast<std::uintptr_t>(thunk_page), thunk_error);
    assert(resolved_thunk.has_value());
    assert(*resolved_thunk == reinterpret_cast<std::uintptr_t>(thunk_target));
    thunk_page[0] = 0x90u;
    assert(!ue4ss::linux::core::resolve_call_function_by_name_with_arguments_thunk(
                    reinterpret_cast<std::uintptr_t>(thunk_page), thunk_error)
                    .has_value());
    assert(thunk_error.find("r8d") != std::string::npos);
    assert(munmap(thunk_page, thunk_page_size) == 0);

    std::array<void*, 4> text_data_vtable{};
    text_data_vtable[3] = reinterpret_cast<void*>(&fake_text_get_display_string);

    std::array<void*, 64> property_vtable{};
    property_vtable[0xb8u / sizeof(void*)] = reinterpret_cast<void*>(&fake_property_identical);
    property_vtable[0x100u / sizeof(void*)] = reinterpret_cast<void*>(&fake_property_hash);
    property_vtable[0x108u / sizeof(void*)] = reinterpret_cast<void*>(&fake_property_copy);
    property_vtable[0x130u / sizeof(void*)] = reinterpret_cast<void*>(&fake_property_destroy);
    property_vtable[0x138u / sizeof(void*)] = reinterpret_cast<void*>(&fake_property_initialize);
    property_vtable[0x150u / sizeof(void*)] = reinterpret_cast<void*>(&fake_property_alignment);
    property_vtable[0xf0u / sizeof(void*)] = reinterpret_cast<void*>(&fake_property_import_text);
    property_vtable[0x170u / sizeof(void*)] = reinterpret_cast<void*>(&fake_sparse_get_delegate);
    property_vtable[0x180u / sizeof(void*)] = reinterpret_cast<void*>(&fake_sparse_add_delegate);
    property_vtable[0x188u / sizeof(void*)] = reinterpret_cast<void*>(&fake_sparse_remove_delegate);
    property_vtable[0x190u / sizeof(void*)] = reinterpret_cast<void*>(&fake_sparse_clear_delegate);

    std::array<void*, 8> allocator_vtable{};
    allocator_vtable[5] = reinterpret_cast<void*>(&fake_realloc);
    allocator_vtable[7] = reinterpret_cast<void*>(&fake_free);
    FakeAllocator allocator{allocator_vtable.data()};
    FakeAllocator* allocator_pointer = &allocator;

    std::array<FakeFFieldClass, 20> field_classes{};
    field_classes[0].name = {7u, 0u};
    field_classes[1].name = {8u, 0u};
    field_classes[2].name = {15u, 0u};
    field_classes[3].name = {16u, 0u};
    field_classes[4].name = {22u, 0u};
    field_classes[5].name = {25u, 0u};
    field_classes[6].name = {32u, 0u};
    field_classes[7].name = {34u, 0u};
    field_classes[8].name = {39u, 0u};
    field_classes[9].name = {40u, 0u};
    field_classes[10].name = {43u, 0u};
    field_classes[11].name = {44u, 0u};
    field_classes[12].name = {45u, 0u};
    field_classes[13].name = {46u, 0u};
    field_classes[14].name = {52u, 0u};
    field_classes[15].name = {59u, 0u};
    field_classes[16].name = {60u, 0u};
    field_classes[17].name = {68u, 0u};
    field_classes[18].name = {75u, 0u};
    field_classes[19].name = {77u, 0u};
    std::array<FakeProperty, 17> properties{};
    properties[0].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[0]);
    properties[0].next = reinterpret_cast<std::uintptr_t>(&properties[1]);
    properties[0].name = {9u, 0u};
    properties[0].element_size = sizeof(std::int32_t);
    properties[0].offset_internal = offsetof(FakeUObject, health);
    properties[1].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[1]);
    properties[1].name = {10u, 0u};
    properties[1].element_size = sizeof(std::uint8_t);
    properties[1].offset_internal = offsetof(FakeUObject, alive);
    properties[1].field_size = sizeof(std::uint8_t);
    properties[1].byte_mask = 0x4u;
    properties[1].field_mask = 0x4u;
    properties[1].next = reinterpret_cast<std::uintptr_t>(&properties[2]);
    properties[2].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[2]);
    properties[2].next = reinterpret_cast<std::uintptr_t>(&properties[3]);
    properties[2].name = {17u, 0u};
    properties[2].element_size = sizeof(ue4ss::linux::core::RawFName);
    properties[2].offset_internal = offsetof(FakeUObject, label);
    properties[3].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[3]);
    properties[3].next = reinterpret_cast<std::uintptr_t>(&properties[4]);
    properties[3].name = {18u, 0u};
    properties[3].element_size = sizeof(ue4ss::linux::core::UnrealStringHeader);
    properties[3].offset_internal = offsetof(FakeUObject, greeting);
    properties[4].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[4]);
    properties[4].next = reinterpret_cast<std::uintptr_t>(&properties[5]);
    properties[4].name = {29u, 0u};
    properties[4].element_size = sizeof(std::array<double, 3>);
    properties[4].offset_internal = offsetof(FakeUObject, location);
    properties[5].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[6]);
    properties[5].next = reinterpret_cast<std::uintptr_t>(&properties[6]);
    properties[5].name = {33u, 0u};
    properties[5].element_size = sizeof(FakeScriptArray);
    properties[5].offset_internal = offsetof(FakeUObject, targets);

    FakeProperty array_inner{};
    array_inner.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[7]);
    array_inner.name = {33u, 0u};
    array_inner.element_size = sizeof(void*);
    array_inner.offset_internal = 0;
    const std::uint64_t array_inner_address = reinterpret_cast<std::uintptr_t>(&array_inner);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[5]) + 0x78u,
                &array_inner_address,
                sizeof(array_inner_address));

    FakeProperty string_array_inner{};
    string_array_inner.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[3]);
    string_array_inner.name = {38u, 0u};
    string_array_inner.element_size = sizeof(ue4ss::linux::core::UnrealStringHeader);
    string_array_inner.offset_internal = 0;
    const std::uint64_t string_array_inner_address = reinterpret_cast<std::uintptr_t>(&string_array_inner);
    properties[6].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[6]);
    properties[6].name = {38u, 0u};
    properties[6].element_size = sizeof(FakeScriptArray);
    properties[6].offset_internal = offsetof(FakeUObject, messages);
    properties[6].next = reinterpret_cast<std::uintptr_t>(&properties[7]);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[6]) + 0x78u,
                &string_array_inner_address,
                sizeof(string_array_inner_address));

    FakeProperty set_inner{};
    set_inner.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[0]);
    set_inner.name = {41u, 0u};
    set_inner.element_size = sizeof(std::int32_t);
    set_inner.offset_internal = 0;
    const std::uint64_t set_inner_address = reinterpret_cast<std::uintptr_t>(&set_inner);
    properties[7].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[8]);
    properties[7].name = {41u, 0u};
    properties[7].element_size = sizeof(FakeScriptSet);
    properties[7].offset_internal = offsetof(FakeUObject, scores);
    properties[7].next = reinterpret_cast<std::uintptr_t>(&properties[8]);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[7]) + 0x78u,
                &set_inner_address,
                sizeof(set_inner_address));

    FakeProperty map_key{};
    map_key.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[14]);
    map_key.name = {13u, 0u};
    map_key.element_size = sizeof(std::int32_t);
    map_key.offset_internal = 0;
    FakeProperty map_value{};
    map_value.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[7]);
    map_value.name = {14u, 0u};
    map_value.element_size = sizeof(void*);
    map_value.offset_internal = 0;
    const std::uint64_t map_key_address = reinterpret_cast<std::uintptr_t>(&map_key);
    const std::uint64_t map_value_address = reinterpret_cast<std::uintptr_t>(&map_value);
    properties[8].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[9]);
    properties[8].name = {42u, 0u};
    properties[8].element_size = sizeof(FakeScriptSet);
    properties[8].offset_internal = offsetof(FakeUObject, lookup);
    properties[8].next = reinterpret_cast<std::uintptr_t>(&properties[9]);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[8]) + 0x78u,
                &map_key_address,
                sizeof(map_key_address));
    std::memcpy(reinterpret_cast<std::byte*>(&properties[8]) + 0x80u,
                &map_value_address,
                sizeof(map_value_address));

    properties[9].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[10]);
    properties[9].name = {47u, 0u};
    properties[9].element_size = sizeof(FakeWeakObjectPtr);
    properties[9].offset_internal = offsetof(FakeUObject, weak_target);
    properties[9].next = reinterpret_cast<std::uintptr_t>(&properties[10]);
    properties[10].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[11]);
    properties[10].name = {48u, 0u};
    properties[10].element_size = sizeof(FakeScriptDelegate);
    properties[10].offset_internal = offsetof(FakeUObject, handler);
    properties[10].next = reinterpret_cast<std::uintptr_t>(&properties[11]);
    properties[11].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[12]);
    properties[11].name = {49u, 0u};
    properties[11].element_size = sizeof(FakeScriptArray);
    properties[11].offset_internal = offsetof(FakeUObject, handlers);
    properties[11].next = reinterpret_cast<std::uintptr_t>(&properties[12]);
    properties[12].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[13]);
    properties[12].name = {50u, 0u};
    properties[12].element_size = 2u * sizeof(void*);
    properties[12].offset_internal = offsetof(FakeUObject, interface_target);
    properties[12].next = reinterpret_cast<std::uintptr_t>(&properties[13]);

    FakeProperty enum_underlying{};
    enum_underlying.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[16]);
    enum_underlying.name = {58u, 0u};
    enum_underlying.element_size = sizeof(std::int8_t);
    enum_underlying.offset_internal = 0;
    const std::uint64_t enum_underlying_address = reinterpret_cast<std::uintptr_t>(&enum_underlying);
    properties[13].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[15]);
    properties[13].name = {58u, 0u};
    properties[13].element_size = sizeof(std::int8_t);
    properties[13].offset_internal = offsetof(FakeUObject, mode);
    properties[13].next = reinterpret_cast<std::uintptr_t>(&properties[14]);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[13]) + 0x78u,
                &enum_underlying_address,
                sizeof(enum_underlying_address));
    properties[14].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[17]);
    properties[14].name = {69u, 0u};
    properties[14].element_size = sizeof(FakeSoftObjectPtr);
    properties[14].offset_internal = offsetof(FakeUObject, soft_target);
    properties[14].next = reinterpret_cast<std::uintptr_t>(&properties[15]);
    properties[15].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[18]);
    properties[15].name = {76u, 0u};
    properties[15].element_size = sizeof(FakeFText);
    properties[15].offset_internal = offsetof(FakeUObject, title);
    properties[15].next = reinterpret_cast<std::uintptr_t>(&properties[16]);
    properties[16].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[19]);
    properties[16].name = {78u, 0u};
    properties[16].element_size = sizeof(std::uint8_t);
    properties[16].offset_internal = offsetof(FakeUObject, sparse_handlers);

    std::array<FakeProperty, 3> vector_fields{};
    for (std::size_t index = 0; index < vector_fields.size(); ++index)
    {
        vector_fields[index].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[5]);
        vector_fields[index].name = {static_cast<std::uint32_t>(26u + index), 0u};
        vector_fields[index].element_size = sizeof(double);
        vector_fields[index].offset_internal = static_cast<std::int32_t>(index * sizeof(double));
        if (index + 1u < vector_fields.size())
        {
            vector_fields[index].next = reinterpret_cast<std::uintptr_t>(&vector_fields[index + 1u]);
        }
    }

    std::array<FakeProperty, 2> function_parameters{};
    function_parameters[0].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[0]);
    function_parameters[0].next = reinterpret_cast<std::uintptr_t>(&function_parameters[1]);
    function_parameters[0].name = {13u, 0u};
    function_parameters[0].element_size = sizeof(std::int32_t);
    function_parameters[0].property_flags = 0x80u; // CPF_Parm
    function_parameters[0].offset_internal = 0;
    function_parameters[1].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[0]);
    function_parameters[1].name = {14u, 0u};
    function_parameters[1].element_size = sizeof(std::int32_t);
    function_parameters[1].property_flags = 0x80u | 0x400u; // CPF_Parm | CPF_ReturnParm
    function_parameters[1].offset_internal = sizeof(std::int32_t);

    std::array<FakeProperty, 2> out_parameters{};
    out_parameters[0].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[0]);
    out_parameters[0].next = reinterpret_cast<std::uintptr_t>(&out_parameters[1]);
    out_parameters[0].name = {20u, 0u};
    out_parameters[0].element_size = sizeof(std::int32_t);
    out_parameters[0].property_flags = 0x80u | 0x100u; // CPF_Parm | CPF_OutParm
    out_parameters[0].offset_internal = 0;
    out_parameters[1].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[3]);
    out_parameters[1].name = {21u, 0u};
    out_parameters[1].element_size = sizeof(ue4ss::linux::core::UnrealStringHeader);
    out_parameters[1].property_flags = 0x80u | 0x100u; // CPF_Parm | CPF_OutParm
    out_parameters[1].offset_internal = 8;

    std::array<FakeProperty, 3> vector_function_parameters{};
    vector_function_parameters[0].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[4]);
    vector_function_parameters[0].next = reinterpret_cast<std::uintptr_t>(&vector_function_parameters[1]);
    vector_function_parameters[0].name = {29u, 0u};
    vector_function_parameters[0].element_size = sizeof(std::array<double, 3>);
    vector_function_parameters[0].property_flags = 0x80u; // CPF_Parm
    vector_function_parameters[0].offset_internal = 0;
    vector_function_parameters[1].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[5]);
    vector_function_parameters[1].next = reinterpret_cast<std::uintptr_t>(&vector_function_parameters[2]);
    vector_function_parameters[1].name = {31u, 0u};
    vector_function_parameters[1].element_size = sizeof(double);
    vector_function_parameters[1].property_flags = 0x80u; // CPF_Parm
    vector_function_parameters[1].offset_internal = 24;
    vector_function_parameters[2].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[4]);
    vector_function_parameters[2].name = {14u, 0u};
    vector_function_parameters[2].element_size = sizeof(std::array<double, 3>);
    vector_function_parameters[2].property_flags = 0x80u | 0x400u; // CPF_Parm | CPF_ReturnParm
    vector_function_parameters[2].offset_internal = 32;

    FakeProperty out_array_parameter{};
    out_array_parameter.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[6]);
    out_array_parameter.name = {36u, 0u};
    out_array_parameter.element_size = sizeof(FakeScriptArray);
    out_array_parameter.property_flags = 0x80u | 0x100u; // CPF_Parm | CPF_OutParm
    out_array_parameter.offset_internal = 0;
    std::memcpy(reinterpret_cast<std::byte*>(&out_array_parameter) + 0x78u,
                &array_inner_address,
                sizeof(array_inner_address));

    std::array<FakeProperty, 2> count_array_parameters{};
    count_array_parameters[0].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[6]);
    count_array_parameters[0].next = reinterpret_cast<std::uintptr_t>(&count_array_parameters[1]);
    count_array_parameters[0].name = {33u, 0u};
    count_array_parameters[0].element_size = sizeof(FakeScriptArray);
    count_array_parameters[0].property_flags = 0x80u; // CPF_Parm
    count_array_parameters[0].offset_internal = 0;
    std::memcpy(reinterpret_cast<std::byte*>(&count_array_parameters[0]) + 0x78u,
                &array_inner_address,
                sizeof(array_inner_address));
    count_array_parameters[1].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[0]);
    count_array_parameters[1].name = {14u, 0u};
    count_array_parameters[1].element_size = sizeof(std::int32_t);
    count_array_parameters[1].property_flags = 0x80u | 0x400u; // CPF_Parm | CPF_ReturnParm
    count_array_parameters[1].offset_internal = 16;

    std::array<FakeProperty, 2> enum_function_parameters{};
    for (std::size_t index = 0; index < enum_function_parameters.size(); ++index)
    {
        enum_function_parameters[index].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[15]);
        enum_function_parameters[index].name = {index == 0u ? 58u : 14u, 0u};
        enum_function_parameters[index].element_size = sizeof(std::int8_t);
        enum_function_parameters[index].property_flags = 0x80u; // CPF_Parm
        enum_function_parameters[index].offset_internal = static_cast<std::int32_t>(index);
        std::memcpy(reinterpret_cast<std::byte*>(&enum_function_parameters[index]) + 0x78u,
                    &enum_underlying_address,
                    sizeof(enum_underlying_address));
    }
    enum_function_parameters[0].next = reinterpret_cast<std::uintptr_t>(&enum_function_parameters[1]);
    enum_function_parameters[1].property_flags |= 0x400u; // CPF_ReturnParm

    std::array<FakeProperty, 2> count_set_parameters{};
    count_set_parameters[0].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[8]);
    count_set_parameters[0].next = reinterpret_cast<std::uintptr_t>(&count_set_parameters[1]);
    count_set_parameters[0].name = {41u, 0u};
    count_set_parameters[0].element_size = sizeof(FakeScriptSet);
    count_set_parameters[0].property_flags = 0x80u; // CPF_Parm
    count_set_parameters[0].offset_internal = 0;
    std::memcpy(reinterpret_cast<std::byte*>(&count_set_parameters[0]) + 0x78u,
                &set_inner_address,
                sizeof(set_inner_address));
    count_set_parameters[1].class_private = reinterpret_cast<std::uintptr_t>(&field_classes[0]);
    count_set_parameters[1].name = {14u, 0u};
    count_set_parameters[1].element_size = sizeof(std::int32_t);
    count_set_parameters[1].property_flags = 0x80u | 0x400u; // CPF_Parm | CPF_ReturnParm
    count_set_parameters[1].offset_internal = sizeof(FakeScriptSet);

    FakeProperty out_map_parameter{};
    out_map_parameter.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[9]);
    out_map_parameter.name = {64u, 0u};
    out_map_parameter.element_size = sizeof(FakeScriptSet);
    out_map_parameter.property_flags = 0x80u | 0x100u; // CPF_Parm | CPF_OutParm
    out_map_parameter.offset_internal = 0;
    std::memcpy(reinterpret_cast<std::byte*>(&out_map_parameter) + 0x78u,
                &map_key_address,
                sizeof(map_key_address));
    std::memcpy(reinterpret_cast<std::byte*>(&out_map_parameter) + 0x80u,
                &map_value_address,
                sizeof(map_value_address));

    std::array<FakeProperty, 2> echo_delegate_parameters{};
    for (std::size_t index = 0; index < echo_delegate_parameters.size(); ++index)
    {
        echo_delegate_parameters[index].class_private =
                reinterpret_cast<std::uintptr_t>(&field_classes[11]);
        echo_delegate_parameters[index].name = {index == 0u ? 48u : 14u, 0u};
        echo_delegate_parameters[index].element_size = sizeof(FakeScriptDelegate);
        echo_delegate_parameters[index].property_flags = 0x80u; // CPF_Parm
        echo_delegate_parameters[index].offset_internal =
                static_cast<std::int32_t>(index * sizeof(FakeScriptDelegate));
    }
    echo_delegate_parameters[0].next =
            reinterpret_cast<std::uintptr_t>(&echo_delegate_parameters[1]);
    echo_delegate_parameters[1].property_flags |= 0x400u; // CPF_ReturnParm

    FakeProperty out_multicast_parameter{};
    out_multicast_parameter.class_private = reinterpret_cast<std::uintptr_t>(&field_classes[12]);
    out_multicast_parameter.name = {67u, 0u};
    out_multicast_parameter.element_size = sizeof(FakeScriptArray);
    out_multicast_parameter.property_flags = 0x80u | 0x100u; // CPF_Parm | CPF_OutParm
    out_multicast_parameter.offset_internal = 0;

    std::array<FakeProperty, 2> echo_soft_parameters{};
    for (std::size_t index = 0; index < echo_soft_parameters.size(); ++index)
    {
        echo_soft_parameters[index].class_private =
                reinterpret_cast<std::uintptr_t>(&field_classes[17]);
        echo_soft_parameters[index].name = {index == 0u ? 69u : 14u, 0u};
        echo_soft_parameters[index].element_size = sizeof(FakeSoftObjectPtr);
        echo_soft_parameters[index].property_flags = 0x80u; // CPF_Parm
        echo_soft_parameters[index].offset_internal =
                static_cast<std::int32_t>(index * sizeof(FakeSoftObjectPtr));
    }
    echo_soft_parameters[0].next = reinterpret_cast<std::uintptr_t>(&echo_soft_parameters[1]);
    echo_soft_parameters[1].property_flags |= 0x400u; // CPF_ReturnParm

    const auto bind_property_vtable = [&property_vtable](FakeProperty& property) {
        property.vtable = reinterpret_cast<std::uintptr_t>(property_vtable.data());
    };
    for (auto& property : properties) bind_property_vtable(property);
    bind_property_vtable(array_inner);
    bind_property_vtable(string_array_inner);
    bind_property_vtable(set_inner);
    bind_property_vtable(map_key);
    bind_property_vtable(map_value);
    bind_property_vtable(enum_underlying);
    for (auto& property : vector_fields) bind_property_vtable(property);
    for (auto& property : function_parameters) bind_property_vtable(property);
    for (auto& property : out_parameters) bind_property_vtable(property);
    for (auto& property : vector_function_parameters) bind_property_vtable(property);
    bind_property_vtable(out_array_parameter);
    for (auto& property : count_array_parameters) bind_property_vtable(property);
    for (auto& property : enum_function_parameters) bind_property_vtable(property);
    for (auto& property : count_set_parameters) bind_property_vtable(property);
    bind_property_vtable(out_map_parameter);
    for (auto& property : echo_delegate_parameters) bind_property_vtable(property);
    bind_property_vtable(out_multicast_parameter);
    for (auto& property : echo_soft_parameters) bind_property_vtable(property);
    constexpr std::uint64_t zero_constructor_no_destructor = 0x200u | 0x1000000000u;
    properties[0].property_flags |= zero_constructor_no_destructor;
    set_inner.property_flags |= zero_constructor_no_destructor;
    map_key.property_flags |= zero_constructor_no_destructor;
    map_value.property_flags |= zero_constructor_no_destructor;
    enum_underlying.property_flags |= zero_constructor_no_destructor;
    properties[13].property_flags |= zero_constructor_no_destructor;

    std::array<FakeUObject, 31> objects{};
    const auto address = [&objects](std::size_t index) {
        return reinterpret_cast<std::uintptr_t>(&objects[index]);
    };
    objects[0] = {.internal_index = 0, .class_private = address(0), .name_private = {1, 0}}; // Class
    objects[1] = {.internal_index = 1, .class_private = address(0), .name_private = {2, 0}}; // Package class
    objects[2] = {.internal_index = 2, .class_private = address(1), .name_private = {5, 0}}; // Package instance
    objects[3] = {.internal_index = 3, .class_private = address(0), .name_private = {4, 0}, .outer_private = address(2)}; // Base
    objects[4] = {.internal_index = 4,
                  .class_private = address(0),
                  .name_private = {3, 0},
                  .outer_private = address(2),
                  .super_struct = address(3),
                  .child_properties = reinterpret_cast<std::uintptr_t>(properties.data()),
                  .properties_size = sizeof(FakeUObject)}; // Derived
    objects[5] = {.object_flags = 0x1u,
                  .internal_index = 5,
                  .class_private = address(4),
                  .name_private = {6, 0},
                  .outer_private = address(2),
                  .health = 42,
                  .alive = 0x4u,
                  .label = {9u, 0u},
                  .location = {1.0, 2.0, 3.0},
                  .mode = -1}; // live instance
    objects[5].vtable = reinterpret_cast<std::uintptr_t>(&fake_game_engine_tick);
    const std::uint64_t interface_class_address = address(3);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[12]) + 0x78u,
                &interface_class_address,
                sizeof(interface_class_address));
    std::array<FakeImplementedInterface, 1> implemented_interfaces{{
            {.interface_class = interface_class_address,
             .pointer_offset = 0,
             .implemented_by_k2 = 0},
    }};
    const FakeScriptArray implemented_interfaces_header{
            .data = reinterpret_cast<std::uintptr_t>(implemented_interfaces.data()),
            .num = static_cast<std::int32_t>(implemented_interfaces.size()),
            .max = static_cast<std::int32_t>(implemented_interfaces.size()),
    };
    std::memcpy(reinterpret_cast<std::byte*>(&objects[4]) + 0x1d0u,
                &implemented_interfaces_header,
                sizeof(implemented_interfaces_header));
    constexpr char16_t title_text[] = u"Hello FText \u2713";
    FakeTextData title_data{
            .vtable = text_data_vtable.data(),
            .display_string = {
                    .data = const_cast<char16_t*>(title_text),
                    .num = static_cast<std::int32_t>(std::size(title_text)),
                    .max = static_cast<std::int32_t>(std::size(title_text)),
            },
    };
    objects[5].title.data = reinterpret_cast<std::uintptr_t>(&title_data);
    objects[6] = {.object_flags = 0x10,
                  .internal_index = 6,
                  .class_private = address(4),
                  .name_private = {6, 0},
                  .outer_private = address(2)}; // excluded CDO
    const std::uint64_t derived_cdo_address = address(6);
    std::memcpy(reinterpret_cast<std::byte*>(&objects[4]) + 0x110u,
                &derived_cdo_address,
                sizeof(derived_cdo_address));
    objects[7] = {.internal_index = 7,
                  .class_private = address(0),
                  .name_private = {11, 0},
                  .outer_private = address(2)}; // Function class
    objects[8] = {.internal_index = 8,
                  .class_private = address(7),
                  .name_private = {12, 0},
                  .outer_private = address(4),
                  .child_properties = reinterpret_cast<std::uintptr_t>(function_parameters.data()),
                  .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // UFunction
    const std::uint8_t function_num_parms = 2u;
    const std::uint16_t function_parms_size = 2u * sizeof(std::int32_t);
    const std::uint16_t function_return_offset = sizeof(std::int32_t);
    const std::uint32_t function_flags = 0x400u;
    std::memcpy(reinterpret_cast<std::byte*>(&objects[8]) + 0xb0u,
                &function_flags,
                sizeof(function_flags));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[8]) + 0xb4u, &function_num_parms, sizeof(function_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[8]) + 0xb6u, &function_parms_size, sizeof(function_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[8]) + 0xb8u, &function_return_offset, sizeof(function_return_offset));
    const std::uint64_t sparse_signature_function = address(8);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[16]) + 0x78u,
                &sparse_signature_function,
                sizeof(sparse_signature_function));
    g_sparse_bindings = {{
            .object = {.object_index = 5, .object_serial_number = 105},
            .function_name = {12u, 0u},
    }};
    refresh_sparse_invocation_list(&objects[5].sparse_handlers);
    objects[9] = {.internal_index = 9,
                  .class_private = address(7),
                  .name_private = {19, 0},
                  .outer_private = address(4),
                  .child_properties = reinterpret_cast<std::uintptr_t>(out_parameters.data()),
                  .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // UFunction with scalar outs
    const std::uint8_t out_function_num_parms = 2u;
    const std::uint16_t out_function_parms_size = 24u;
    const std::uint16_t out_function_return_offset = 0xffffu;
    std::memcpy(reinterpret_cast<std::byte*>(&objects[9]) + 0xb4u, &out_function_num_parms, sizeof(out_function_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[9]) + 0xb6u, &out_function_parms_size, sizeof(out_function_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[9]) + 0xb8u, &out_function_return_offset, sizeof(out_function_return_offset));
    objects[10] = {.internal_index = 10,
                   .class_private = address(0),
                   .name_private = {23, 0},
                   .outer_private = address(2)}; // ScriptStruct class
    objects[11] = {.internal_index = 11,
                   .class_private = address(10),
                   .name_private = {24, 0},
                   .outer_private = address(2),
                   .child_properties = reinterpret_cast<std::uintptr_t>(vector_fields.data()),
                   .properties_size = sizeof(std::array<double, 3>)}; // FVector UScriptStruct
    const std::uint64_t vector_struct_address = address(11);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[4]) + 0x78u,
                &vector_struct_address,
                sizeof(vector_struct_address));
    std::memcpy(reinterpret_cast<std::byte*>(&vector_function_parameters[0]) + 0x78u,
                &vector_struct_address,
                sizeof(vector_struct_address));
    std::memcpy(reinterpret_cast<std::byte*>(&vector_function_parameters[2]) + 0x78u,
                &vector_struct_address,
                sizeof(vector_struct_address));
    objects[12] = {.internal_index = 12,
                   .class_private = address(7),
                   .name_private = {30, 0},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(vector_function_parameters.data()),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Struct UFunction
    const std::uint8_t vector_function_num_parms = 3u;
    const std::uint16_t vector_function_parms_size = 56u;
    const std::uint16_t vector_function_return_offset = 32u;
    std::memcpy(reinterpret_cast<std::byte*>(&objects[12]) + 0xb4u,
                &vector_function_num_parms,
                sizeof(vector_function_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[12]) + 0xb6u,
                &vector_function_parms_size,
                sizeof(vector_function_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[12]) + 0xb8u,
                &vector_function_return_offset,
                sizeof(vector_function_return_offset));
    objects[13] = {.internal_index = 13,
                   .class_private = address(7),
                   .name_private = {35, 0},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(&out_array_parameter),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Array OutParm UFunction
    const std::uint8_t out_array_num_parms = 1u;
    const std::uint16_t out_array_parms_size = sizeof(FakeScriptArray);
    const std::uint16_t out_array_return_offset = 0xffffu;
    std::memcpy(reinterpret_cast<std::byte*>(&objects[13]) + 0xb4u, &out_array_num_parms, sizeof(out_array_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[13]) + 0xb6u, &out_array_parms_size, sizeof(out_array_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[13]) + 0xb8u, &out_array_return_offset, sizeof(out_array_return_offset));
    objects[14] = {.internal_index = 14,
                   .class_private = address(7),
                   .name_private = {37, 0},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(count_array_parameters.data()),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Array input UFunction
    const std::uint8_t count_array_num_parms = 2u;
    const std::uint16_t count_array_parms_size = 20u;
    const std::uint16_t count_array_return_offset = 16u;
    std::memcpy(reinterpret_cast<std::byte*>(&objects[14]) + 0xb4u, &count_array_num_parms, sizeof(count_array_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[14]) + 0xb6u, &count_array_parms_size, sizeof(count_array_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[14]) + 0xb8u, &count_array_return_offset, sizeof(count_array_return_offset));
    objects[15] = {.internal_index = 15,
                   .class_private = address(0),
                   .name_private = {53u, 0u},
                   .outer_private = address(2)}; // UEnum class
    objects[16] = {.internal_index = 16,
                   .class_private = address(15),
                   .name_private = {54u, 0u},
                   .outer_private = address(2)}; // UEnum instance
    auto* enum_names = static_cast<FakeEnumNamePair*>(
            std::malloc(3u * sizeof(FakeEnumNamePair)));
    assert(enum_names != nullptr);
    enum_names[0] = FakeEnumNamePair{{55u, 0u}, -1};
    enum_names[1] = FakeEnumNamePair{{56u, 0u}, 1};
    enum_names[2] = FakeEnumNamePair{{57u, 0u}, 2};
    const FakeScriptArray enum_names_header{
            .data = reinterpret_cast<std::uintptr_t>(enum_names),
            .num = 3,
            .max = 3,
    };
    std::memcpy(reinterpret_cast<std::byte*>(&objects[16]) + 0x40u,
                &enum_names_header,
                sizeof(enum_names_header));
    // A second field deliberately resembles a TArray header, but its payload
    // contains pointer-like values rather than enum values. The Linux ABI gate
    // must reject it without ever passing its FName bits to engine code.
    std::array<FakeEnumNamePair, 2> false_enum_candidate{
            FakeEnumNamePair{{std::numeric_limits<std::uint32_t>::max(), 0u},
                             std::numeric_limits<std::int64_t>::max()},
            FakeEnumNamePair{{std::numeric_limits<std::uint32_t>::max() - 1u, 0u},
                             std::numeric_limits<std::int64_t>::max() - 1},
    };
    const FakeScriptArray false_enum_header{
            .data = reinterpret_cast<std::uintptr_t>(false_enum_candidate.data()),
            .num = static_cast<std::int32_t>(false_enum_candidate.size()),
            .max = static_cast<std::int32_t>(false_enum_candidate.size()),
    };
    std::memcpy(reinterpret_cast<std::byte*>(&objects[16]) + 0x70u,
                &false_enum_header,
                sizeof(false_enum_header));
    const std::uint64_t enum_object_address = address(16);
    std::memcpy(reinterpret_cast<std::byte*>(&properties[13]) + 0x80u,
                &enum_object_address,
                sizeof(enum_object_address));
    for (auto& parameter : enum_function_parameters)
    {
        std::memcpy(reinterpret_cast<std::byte*>(&parameter) + 0x80u,
                    &enum_object_address,
                    sizeof(enum_object_address));
    }
    objects[17] = {.internal_index = 17,
                   .class_private = address(7),
                   .name_private = {61u, 0u},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(enum_function_parameters.data()),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Enum UFunction
    const std::uint8_t enum_function_num_parms = 2u;
    const std::uint16_t enum_function_parms_size = 2u;
    const std::uint16_t enum_function_return_offset = 1u;
    std::memcpy(reinterpret_cast<std::byte*>(&objects[17]) + 0xb4u,
                &enum_function_num_parms,
                sizeof(enum_function_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[17]) + 0xb6u,
                &enum_function_parms_size,
                sizeof(enum_function_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[17]) + 0xb8u,
                &enum_function_return_offset,
                sizeof(enum_function_return_offset));
    objects[18] = {.internal_index = 18,
                   .class_private = address(7),
                   .name_private = {62u, 0u},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(count_set_parameters.data()),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Set input UFunction
    const std::uint8_t count_set_num_parms = 2u;
    const std::uint16_t count_set_parms_size = sizeof(FakeScriptSet) + sizeof(std::int32_t);
    const std::uint16_t count_set_return_offset = sizeof(FakeScriptSet);
    std::memcpy(reinterpret_cast<std::byte*>(&objects[18]) + 0xb4u,
                &count_set_num_parms,
                sizeof(count_set_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[18]) + 0xb6u,
                &count_set_parms_size,
                sizeof(count_set_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[18]) + 0xb8u,
                &count_set_return_offset,
                sizeof(count_set_return_offset));
    objects[19] = {.internal_index = 19,
                   .class_private = address(7),
                   .name_private = {63u, 0u},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(&out_map_parameter),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Map OutParm UFunction
    const std::uint8_t out_map_num_parms = 1u;
    const std::uint16_t out_map_parms_size = sizeof(FakeScriptSet);
    const std::uint16_t out_map_return_offset = 0xffffu;
    std::memcpy(reinterpret_cast<std::byte*>(&objects[19]) + 0xb4u,
                &out_map_num_parms,
                sizeof(out_map_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[19]) + 0xb6u,
                &out_map_parms_size,
                sizeof(out_map_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[19]) + 0xb8u,
                &out_map_return_offset,
                sizeof(out_map_return_offset));
    objects[20] = {.internal_index = 20,
                   .class_private = address(7),
                   .name_private = {65u, 0u},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(echo_delegate_parameters.data()),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Delegate input/return
    const std::uint8_t echo_delegate_num_parms = 2u;
    const std::uint16_t echo_delegate_parms_size = 2u * sizeof(FakeScriptDelegate);
    const std::uint16_t echo_delegate_return_offset = sizeof(FakeScriptDelegate);
    std::memcpy(reinterpret_cast<std::byte*>(&objects[20]) + 0xb4u,
                &echo_delegate_num_parms,
                sizeof(echo_delegate_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[20]) + 0xb6u,
                &echo_delegate_parms_size,
                sizeof(echo_delegate_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[20]) + 0xb8u,
                &echo_delegate_return_offset,
                sizeof(echo_delegate_return_offset));
    objects[21] = {.internal_index = 21,
                   .class_private = address(7),
                   .name_private = {66u, 0u},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(&out_multicast_parameter),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Multicast OutParm
    const std::uint8_t out_multicast_num_parms = 1u;
    const std::uint16_t out_multicast_parms_size = sizeof(FakeScriptArray);
    const std::uint16_t out_multicast_return_offset = 0xffffu;
    std::memcpy(reinterpret_cast<std::byte*>(&objects[21]) + 0xb4u,
                &out_multicast_num_parms,
                sizeof(out_multicast_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[21]) + 0xb6u,
                &out_multicast_parms_size,
                sizeof(out_multicast_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[21]) + 0xb8u,
                &out_multicast_return_offset,
                sizeof(out_multicast_return_offset));
    objects[22] = {.internal_index = 22,
                   .class_private = address(7),
                   .name_private = {72u, 0u},
                   .outer_private = address(4),
                   .child_properties = reinterpret_cast<std::uintptr_t>(echo_soft_parameters.data()),
                   .native_function = reinterpret_cast<std::uintptr_t>(&fake_native_function)}; // Soft ptr input/return
    const std::uint8_t echo_soft_num_parms = 2u;
    const std::uint16_t echo_soft_parms_size = 2u * sizeof(FakeSoftObjectPtr);
    const std::uint16_t echo_soft_return_offset = sizeof(FakeSoftObjectPtr);
    std::memcpy(reinterpret_cast<std::byte*>(&objects[22]) + 0xb4u,
                &echo_soft_num_parms,
                sizeof(echo_soft_num_parms));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[22]) + 0xb6u,
                &echo_soft_parms_size,
                sizeof(echo_soft_parms_size));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[22]) + 0xb8u,
                &echo_soft_return_offset,
                sizeof(echo_soft_return_offset));
    objects[23] = {.internal_index = 23,
                   .class_private = address(0),
                   .name_private = {79u, 0u},
                   .outer_private = address(2)}; // World class
    objects[24] = {.internal_index = 24,
                   .class_private = address(0),
                   .name_private = {80u, 0u},
                   .outer_private = address(2)}; // Level class
    objects[25] = {.internal_index = 25,
                   .class_private = address(0),
                   .name_private = {81u, 0u},
                   .outer_private = address(2)}; // Actor class
    objects[26] = {.internal_index = 26,
                   .class_private = address(23),
                   .name_private = {82u, 0u},
                   .outer_private = address(2)}; // UWorld
    objects[27] = {.internal_index = 27,
                   .class_private = address(24),
                   .name_private = {83u, 0u},
                   .outer_private = address(26)}; // ULevel
    objects[28] = {.internal_index = 28,
                   .class_private = address(25),
                   .name_private = {84u, 0u},
                   .outer_private = address(27)}; // AActor
    objects[29] = {.internal_index = 29,
                   .class_private = address(0),
                   .name_private = {85u, 0u},
                   .outer_private = address(2)}; // DataTable class
    objects[30] = {.internal_index = 30,
                   .class_private = address(29),
                   .name_private = {86u, 0u},
                   .outer_private = address(2)}; // UDataTable instance
    objects[4].children = address(8);
    objects[8].field_next = address(9);
    objects[9].field_next = address(12);
    objects[12].field_next = address(13);
    objects[13].field_next = address(14);
    objects[14].field_next = address(17);
    objects[17].field_next = address(18);
    objects[18].field_next = address(19);
    objects[19].field_next = address(20);
    objects[20].field_next = address(21);
    objects[21].field_next = address(22);

    auto* target_data = static_cast<std::uint64_t*>(std::malloc(2u * sizeof(std::uint64_t)));
    assert(target_data != nullptr);
    target_data[0] = address(2);
    target_data[1] = address(5);
    objects[5].targets = {
            .data = reinterpret_cast<std::uintptr_t>(target_data),
            .num = 2,
            .max = 2,
    };
    g_fake_output_targets = {address(2), address(5)};

    ue4ss::linux::core::UnrealScriptSetLayout set_layout;
    std::string container_layout_error;
    assert(ue4ss::linux::core::calculate_script_set_layout(
            sizeof(std::int32_t), alignof(std::int32_t), set_layout, container_layout_error));
    assert(ue4ss::linux::core::script_set_storage_size() == sizeof(FakeScriptSet));
    std::vector<std::byte> set_storage(static_cast<std::size_t>(set_layout.sparse_stride) * 3u);
    const std::int32_t score_a = 11;
    const std::int32_t score_b = 33;
    std::memcpy(set_storage.data(), &score_a, sizeof(score_a));
    std::memcpy(set_storage.data() + static_cast<std::size_t>(set_layout.sparse_stride) * 2u,
                &score_b,
                sizeof(score_b));
    objects[5].scores = {
            .elements = {
                    .data = reinterpret_cast<std::uintptr_t>(set_storage.data()),
                    .num = 3,
                    .max = 3,
            },
            .allocation_flags_inline = {0b101u, 0u, 0u, 0u},
            .allocation_flags_num_bits = 3,
            .allocation_flags_max_bits = 128,
            .first_free_index = 1,
            .num_free_indices = 1,
            .hash_inline = 0,
            .hash_size = 1,
    };

    ue4ss::linux::core::UnrealScriptMapLayout map_layout;
    assert(ue4ss::linux::core::calculate_script_map_layout(
            sizeof(std::int32_t),
            alignof(std::int32_t),
            sizeof(void*),
            alignof(void*),
            map_layout,
            container_layout_error));
    assert(ue4ss::linux::core::script_map_storage_size() == sizeof(FakeScriptSet));
    std::vector<std::byte> map_storage(static_cast<std::size_t>(map_layout.set.sparse_stride) * 3u);
    const std::int32_t map_key_a = 7;
    const std::int32_t map_key_b = 9;
    const std::uint64_t map_value_a = address(2);
    const std::uint64_t map_value_b = address(5);
    std::memcpy(map_storage.data(), &map_key_a, sizeof(map_key_a));
    std::memcpy(map_storage.data() + map_layout.value_offset, &map_value_a, sizeof(map_value_a));
    std::memcpy(map_storage.data() + static_cast<std::size_t>(map_layout.set.sparse_stride) * 2u,
                &map_key_b,
                sizeof(map_key_b));
    std::memcpy(map_storage.data() + static_cast<std::size_t>(map_layout.set.sparse_stride) * 2u +
                        static_cast<std::size_t>(map_layout.value_offset),
                &map_value_b,
                sizeof(map_value_b));
    objects[5].lookup = {
            .elements = {
                    .data = reinterpret_cast<std::uintptr_t>(map_storage.data()),
                    .num = 3,
                    .max = 3,
            },
            .allocation_flags_inline = {0b101u, 0u, 0u, 0u},
            .allocation_flags_num_bits = 3,
            .allocation_flags_max_bits = 128,
            .first_free_index = 1,
            .num_free_indices = 1,
            .hash_inline = 0,
            .hash_size = 1,
    };
    std::memcpy(g_fake_output_map_header.data(), &objects[5].lookup, sizeof(objects[5].lookup));

    auto* data_table_row_a = static_cast<double*>(std::malloc(3u * sizeof(double)));
    auto* data_table_row_b = static_cast<double*>(std::malloc(3u * sizeof(double)));
    assert(data_table_row_a != nullptr && data_table_row_b != nullptr);
    std::copy_n(std::array<double, 3>{10.0, 20.0, 30.0}.data(), 3u, data_table_row_a);
    std::copy_n(std::array<double, 3>{40.0, 50.0, 60.0}.data(), 3u, data_table_row_b);
    ue4ss::linux::core::UnrealScriptMapLayout data_table_map_layout;
    assert(ue4ss::linux::core::calculate_script_map_layout(
            sizeof(ue4ss::linux::core::RawFName),
            alignof(ue4ss::linux::core::RawFName),
            sizeof(void*),
            alignof(void*),
            data_table_map_layout,
            container_layout_error));
    const std::size_t data_table_map_bytes =
            static_cast<std::size_t>(data_table_map_layout.set.sparse_stride) * 3u;
    auto* data_table_map_storage = static_cast<std::byte*>(std::malloc(data_table_map_bytes));
    assert(data_table_map_storage != nullptr);
    std::memset(data_table_map_storage, 0, data_table_map_bytes);
    const ue4ss::linux::core::RawFName row_a_name{87u, 0u};
    const ue4ss::linux::core::RawFName row_b_name{88u, 0u};
    const std::uint64_t row_a_data = reinterpret_cast<std::uintptr_t>(data_table_row_a);
    const std::uint64_t row_b_data = reinterpret_cast<std::uintptr_t>(data_table_row_b);
    std::memcpy(data_table_map_storage, &row_a_name, sizeof(row_a_name));
    std::memcpy(data_table_map_storage + data_table_map_layout.value_offset,
                &row_a_data,
                sizeof(row_a_data));
    const std::size_t row_b_offset =
            static_cast<std::size_t>(data_table_map_layout.set.sparse_stride) * 2u;
    std::memcpy(data_table_map_storage + row_b_offset,
                &row_b_name,
                sizeof(row_b_name));
    std::memcpy(data_table_map_storage + row_b_offset +
                        static_cast<std::size_t>(data_table_map_layout.value_offset),
                &row_b_data,
                sizeof(row_b_data));
    const FakeScriptSet data_table_row_map{
            .elements = {
                    .data = reinterpret_cast<std::uintptr_t>(data_table_map_storage),
                    .num = 3,
                    .max = 3,
            },
            .allocation_flags_inline = {0b101u, 0u, 0u, 0u},
            .allocation_flags_num_bits = 3,
            .allocation_flags_max_bits = 128,
            .first_free_index = 1,
            .num_free_indices = 1,
            .hash_inline = 0,
            .hash_size = 1,
    };
    const std::uint64_t data_table_row_struct = address(11);
    std::memcpy(reinterpret_cast<std::byte*>(&objects[30]) + 0x28u,
                &data_table_row_struct,
                sizeof(data_table_row_struct));
    std::memcpy(reinterpret_cast<std::byte*>(&objects[30]) + 0x30u,
                &data_table_row_map,
                sizeof(data_table_row_map));
    objects[5].weak_target = {.object_index = 5, .object_serial_number = 105};
    objects[5].handler = {
            .object = {.object_index = 5, .object_serial_number = 105},
            .function_name = {12u, 0u},
    };
    auto* handler_storage = static_cast<FakeScriptDelegate*>(
            std::malloc(2u * sizeof(FakeScriptDelegate)));
    assert(handler_storage != nullptr);
    handler_storage[0] = {
            .object = {.object_index = 5, .object_serial_number = 105},
            .function_name = {12u, 0u},
    };
    handler_storage[1] = {
            .object = {.object_index = 2, .object_serial_number = 102},
            .function_name = {51u, 0u},
    };
    objects[5].handlers = {
            .data = reinterpret_cast<std::uintptr_t>(handler_storage),
            .num = 2,
            .max = 2,
    };
    objects[5].interface_target = {address(5), address(5)};
    constexpr char16_t soft_sub_path[] = u"Component";
    auto* soft_sub_path_storage = static_cast<char16_t*>(std::malloc(sizeof(soft_sub_path)));
    assert(soft_sub_path_storage != nullptr);
    std::memcpy(soft_sub_path_storage, soft_sub_path, sizeof(soft_sub_path));
    objects[5].soft_target = {
            .weak_ptr = {.object_index = 5, .object_serial_number = 105},
            .tag_at_last_test = 42,
            .package_name = {73u, 0u},
            .asset_name = {74u, 0u},
            .sub_path_string = {
                    .data = soft_sub_path_storage,
                    .num = static_cast<std::int32_t>(std::size(soft_sub_path)),
                    .max = static_cast<std::int32_t>(std::size(soft_sub_path)),
            },
    };
    const std::uint64_t object_property_class_address = address(4);
    std::memcpy(reinterpret_cast<std::byte*>(&array_inner) + 0x78u,
                &object_property_class_address,
                sizeof(object_property_class_address));
    std::memcpy(reinterpret_cast<std::byte*>(&map_value) + 0x78u,
                &object_property_class_address,
                sizeof(object_property_class_address));

    std::array<ObjectItemView, 31> items{};
    for (std::size_t index = 0; index < items.size(); ++index)
    {
        items[index] = {
                .object = address(index),
                .cluster_root_index = -1,
                .serial_number = static_cast<std::int32_t>(100u + index),
        };
    }
    items[5].flags = 0x40;
    std::array<std::uint64_t, 1> chunks{reinterpret_cast<std::uintptr_t>(items.data())};
    const ObjectArrayView object_array{
            .objects = reinterpret_cast<std::uintptr_t>(chunks.data()),
            .max_elements = 65536,
            .num_elements = static_cast<std::int32_t>(items.size()),
            .max_chunks = 1,
            .num_chunks = 1,
    };
    std::array<std::byte, 0x30> guobject_array{};
    std::memcpy(guobject_array.data() + 0x10, &object_array, sizeof(object_array));

    constexpr std::uint64_t required = UE4SS_PS_GUOBJECT_ARRAY | UE4SS_PS_FNAME_TO_STRING | UE4SS_PS_FNAME_CTOR |
                                       UE4SS_PS_GMALLOC | UE4SS_PS_STATIC_CONSTRUCT_OBJECT |
                                       UE4SS_PS_ENGINE_VERSION;
    const ue4ss::linux::core::ResolvedUnrealState resolved{
            .available_mask = required,
            .engine_major = 5,
            .engine_minor = 1,
            .guobject_array = reinterpret_cast<std::uintptr_t>(guobject_array.data()),
            .fname_to_string = reinterpret_cast<std::uintptr_t>(&fake_fname_to_string),
            .fname_ctor = reinterpret_cast<std::uintptr_t>(&fake_fname_constructor),
            .gmalloc = reinterpret_cast<std::uintptr_t>(&allocator_pointer),
            .static_construct_object = reinterpret_cast<std::uintptr_t>(&fake_static_construct_object),
    };

    const auto validation = ue4ss::linux::core::validate_fname_runtime_abi(resolved);
    assert(validation.success);
    assert(validation.detail.find("bounded self-tests") != std::string::npos);
    assert(g_free_calls == 1u);

    ue4ss::linux::core::ReadOnlyUnrealRuntime runtime;
    std::string detail;
    assert(runtime.initialize(resolved, detail));
    if (!runtime.initialize_fproperty_abi(detail))
    {
        std::fprintf(stderr, "synthetic FProperty ABI gate failed: %s\n", detail.c_str());
        assert(false);
    }
    assert(runtime.fproperty_abi_available());
    assert(runtime.fproperty_import_text_available());
    if (!runtime.initialize_uenum_abi(detail))
    {
        std::fprintf(stderr, "synthetic UEnum ABI gate failed: %s\n", detail.c_str());
        assert(false);
    }
    assert(runtime.uenum_abi_available());
    assert(detail.find("+0x40") != std::string::npos);

    ue4ss::linux::core::ReadOnlyUObjectHandle instance_handle;
    ue4ss::linux::core::ReadOnlyUObjectHandle derived_class_handle;
    ue4ss::linux::core::ReadOnlyUObjectHandle base_class_handle;
    assert(runtime.handle_from_address(address(5), instance_handle));
    assert(runtime.handle_from_address(address(4), derived_class_handle));
    assert(runtime.handle_from_address(address(3), base_class_handle));
    ue4ss::linux::core::ReadOnlyUObjectDescription instance_description;
    assert(runtime.describe_object(instance_handle, instance_description, detail));
    assert(instance_description.object_flags == 0x1u);
    assert(instance_description.internal_object_flags == 0x40);
    assert(instance_description.name_private.comparison_index == 6u);
    ue4ss::linux::core::ReadOnlyUObjectHandle super_struct;
    assert(runtime.get_super_struct(derived_class_handle, super_struct, detail));
    assert(super_struct.address == address(3));
    assert(runtime.get_super_struct(super_struct, super_struct, detail));
    assert(super_struct.address == 0u && !runtime.is_valid(super_struct));
    bool is_child{};
    assert(runtime.struct_is_child_of(derived_class_handle,
                                      base_class_handle,
                                      is_child,
                                      detail));
    assert(is_child);
    assert(runtime.struct_is_child_of(base_class_handle,
                                      derived_class_handle,
                                      is_child,
                                      detail));
    assert(!is_child);
    std::vector<ue4ss::linux::core::ReadOnlyUObjectHandle> reflected_functions;
    assert(runtime.enumerate_functions(derived_class_handle, reflected_functions, detail));
    assert(reflected_functions.size() == 11u);
    assert(reflected_functions.front().address == address(8));
    std::uint32_t observed_function_flags{};
    assert(runtime.get_function_flags(reflected_functions.front(), observed_function_flags, detail));
    assert(observed_function_flags == function_flags);
    assert(runtime.set_function_flags(reflected_functions.front(), 0x401u, detail));
    assert(runtime.get_function_flags(reflected_functions.front(), observed_function_flags, detail));
    assert(observed_function_flags == 0x401u);
    assert(runtime.set_function_flags(reflected_functions.front(), function_flags, detail));
    std::vector<ue4ss::linux::core::ReflectedPropertyDescription> reflected_properties;
    assert(runtime.enumerate_properties(derived_class_handle, reflected_properties, detail, true));
    const auto health_property = std::find_if(
            reflected_properties.begin(), reflected_properties.end(), [](const auto& property) {
                return property.name == "Health";
            });
    const auto targets_property = std::find_if(
            reflected_properties.begin(), reflected_properties.end(), [](const auto& property) {
                return property.name == "Targets";
            });
    assert(health_property != reflected_properties.end());
    assert(targets_property != reflected_properties.end());
    assert(health_property->owner_address == address(4));
    assert(health_property->name_private.comparison_index == 9u);
    assert(targets_property->inner_property_address == array_inner_address);
    ue4ss::linux::core::ReflectedPropertyDescription inner_property_description;
    assert(runtime.describe_property(
            targets_property->inner_property_address, inner_property_description, detail));
    assert(inner_property_description.type_name == "ObjectProperty");
    assert(inner_property_description.property_class_address == address(4));
    std::uint64_t health_value_address{};
    assert(runtime.container_ptr_to_value_ptr(
            *health_property, instance_handle, 0, health_value_address, detail));
    assert(health_value_address == address(5) + offsetof(FakeUObject, health));
    assert(!runtime.container_ptr_to_value_ptr(
            *health_property, instance_handle, 1, health_value_address, detail));
    assert(detail.find("ArrayDim") != std::string::npos);
    ue4ss::linux::core::ReadOnlyUObjectHandle actor_handle;
    ue4ss::linux::core::ReadOnlyUObjectHandle world_handle;
    ue4ss::linux::core::ReadOnlyUObjectHandle level_handle;
    assert(runtime.handle_from_address(address(28), actor_handle));
    assert(runtime.find_outer_of_class(actor_handle, "World", true, world_handle, detail));
    assert(world_handle.address == address(26));
    assert(runtime.find_outer_of_class(actor_handle, "Level", false, level_handle, detail));
    assert(level_handle.address == address(27));
    assert(runtime.find_outer_of_class(world_handle, "World", true, world_handle, detail));
    assert(world_handle.address == address(26));

    const auto native_string = [](std::u16string& storage) {
        return ue4ss::linux::core::UnrealStringHeader{
                .data = storage.empty() ? nullptr : storage.data(),
                .num = storage.empty() ? 0 : static_cast<std::int32_t>(storage.size() + 1u),
                .max = storage.empty() ? 0 : static_cast<std::int32_t>(storage.size() + 1u),
        };
    };
    std::u16string protocol = u"unreal";
    std::u16string host = u"127.0.0.1";
    std::u16string map = u"/Game/Pal/Maps/PalWorld";
    std::u16string redirect_url;
    std::u16string portal = u"Start";
    std::u16string option_listen = u"listen";
    std::u16string option_game = u"game=Pal";
    std::array<ue4ss::linux::core::UnrealStringHeader, 2> options{
            native_string(option_listen), native_string(option_game)};
    TestNativeFURL native_url{
            .protocol = native_string(protocol),
            .host = native_string(host),
            .port = 8211,
            .valid = 1,
            .map = native_string(map),
            .redirect_url = native_string(redirect_url),
            .options = {
                    .data = reinterpret_cast<std::uintptr_t>(options.data()),
                    .num = static_cast<std::int32_t>(options.size()),
                    .max = static_cast<std::int32_t>(options.size()),
            },
            .portal = native_string(portal),
    };
    std::string native_fstring;
    assert(runtime.snapshot_native_tchar_string(
            reinterpret_cast<std::uintptr_t>(native_url.map.data), native_fstring, detail));
    assert(native_fstring == "/Game/Pal/Maps/PalWorld");
    assert(!runtime.snapshot_native_tchar_string(1u, native_fstring, detail));
    assert(detail.find("unreadable") != std::string::npos);
    assert(runtime.snapshot_native_fstring(
            reinterpret_cast<std::uintptr_t>(&native_url.map), native_fstring, detail));
    assert(native_fstring == "/Game/Pal/Maps/PalWorld");
    ue4ss::linux::core::UnrealFURLSnapshot native_url_snapshot;
    assert(runtime.snapshot_native_furl(
            reinterpret_cast<std::uintptr_t>(&native_url), native_url_snapshot, detail));
    assert(native_url_snapshot.protocol == "unreal");
    assert(native_url_snapshot.host == "127.0.0.1");
    assert(native_url_snapshot.port == 8211 && native_url_snapshot.valid == 1);
    assert(native_url_snapshot.map == "/Game/Pal/Maps/PalWorld");
    assert(native_url_snapshot.redirect_url.empty());
    assert((native_url_snapshot.options == std::vector<std::string>{"listen", "game=Pal"}));
    assert(native_url_snapshot.portal == "Start");

    const auto valid_options = native_url.options;
    native_url.options.num = 1025;
    native_url.options.max = 1025;
    assert(!runtime.snapshot_native_furl(
            reinterpret_cast<std::uintptr_t>(&native_url), native_url_snapshot, detail));
    assert(detail.find("invalid TArray header") != std::string::npos);
    native_url.options = valid_options;
    --options[1].num;
    assert(!runtime.snapshot_native_furl(
            reinterpret_cast<std::uintptr_t>(&native_url), native_url_snapshot, detail));
    assert(detail.find("null terminated") != std::string::npos);
    options[1] = native_string(option_game);

    const auto derived = runtime.find_all_of("Base");
    assert(derived.success);
    assert(derived.objects.size() == 1u);
    assert(derived.objects[0].internal_index == 5);
    assert(runtime.is_valid(derived.objects[0]));

    ue4ss::linux::core::ReadOnlyUObjectDescription description;
    assert(runtime.describe_object(derived.objects[0], description, detail));
    assert(description.name == "Instance");
    assert(description.class_name == "Derived");
    assert(description.path_name == "/Script/Test.Instance");
    assert(description.full_name == "Derived /Script/Test.Instance");

    ue4ss::linux::core::ReadOnlyUObjectHandle derived_cdo;
    assert(runtime.get_class_default_object({address(4), 4, 104}, derived_cdo, detail));
    assert(derived_cdo.address == address(6) && derived_cdo.internal_index == 6);
    ue4ss::linux::core::ReadOnlyUObjectHandle invalid_cdo;
    assert(!runtime.get_class_default_object(derived.objects[0], invalid_cdo, detail));
    assert(detail.find("UClass") != std::string::npos);

    const auto default_objects = runtime.find_objects(
            0u, std::string_view{"Derived"}, std::nullopt, 0x10u, 0u, true);
    assert(default_objects.success && default_objects.objects.size() == 1u &&
           default_objects.objects[0].address == address(6));
    const auto non_default_instance = runtime.find_objects(
            1u, std::string_view{"Base"}, std::string_view{"Instance"}, 0u, 0x10u, false);
    assert(non_default_instance.success && non_default_instance.objects.size() == 1u &&
           non_default_instance.objects[0].address == address(5));
    const auto class_objects = runtime.find_objects(
            0u, std::string_view{"Class"}, std::nullopt, 0u, 0u, true);
    assert(class_objects.success && std::any_of(class_objects.objects.begin(), class_objects.objects.end(),
                                                [derived_class = address(4)](const auto& object) {
                                                    return object.address == derived_class;
                                                }));

    const auto by_path = runtime.find_by_path("/Script/Test.Instance");
    assert(by_path.success && by_path.objects.size() == 1u);
    assert(by_path.objects[0].address == derived.objects[0].address);

    std::int32_t iterated_objects{};
    std::vector<std::pair<std::int32_t, std::int32_t>> iterated_indices;
    assert(runtime.for_each_uobject(
            [&iterated_indices](const auto& handle,
                                std::int32_t chunk_index,
                                std::int32_t object_index) {
                assert(handle.internal_index == object_index);
                iterated_indices.emplace_back(chunk_index, object_index);
                return true;
            },
            iterated_objects,
            detail));
    assert(iterated_objects == static_cast<std::int32_t>(objects.size()));
    assert(iterated_indices.size() == objects.size());
    assert((iterated_indices.front() == std::pair<std::int32_t, std::int32_t>{0, 0}));
    assert((iterated_indices.back() == std::pair<std::int32_t, std::int32_t>{0, 30}));

    const auto enums = runtime.find_all_of("Enum");
    assert(enums.success && enums.objects.size() == 1u && enums.objects[0].address == address(16));
    std::vector<ue4ss::linux::core::UnrealEnumEntry> enum_entries;
    assert(runtime.enumerate_enum_names(enums.objects[0], enum_entries, detail));
    assert(enum_entries.size() == 3u);
    assert(enum_entries[0].text == "TestMode::Disabled" && enum_entries[0].value == -1);
    assert(enum_entries[1].text == "TestMode::Enabled" && enum_entries[1].value == 1);
    assert(enum_entries[2].text == "TestMode::MAX" && enum_entries[2].value == 2);

    ue4ss::linux::core::UnrealPropertyValue property_value;
    assert(runtime.read_property(derived.objects[0], "Health", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::signed_integer);
    assert(property_value.signed_integer == 42);
    assert(runtime.read_property(derived.objects[0], "Alive", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::boolean);
    assert(property_value.boolean);
    assert(runtime.read_property(derived.objects[0], "Label", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::name);
    assert(property_value.text == "Health");
    assert(runtime.read_property(derived.objects[0], "Greeting", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::string);
    assert(property_value.text.empty());
    assert(runtime.read_property(derived.objects[0], "Title", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::text);
    assert(property_value.text == "Hello FText \u2713");
    const ue4ss::linux::core::RemoteUnrealParameter text_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::text,
            .property_address = reinterpret_cast<std::uintptr_t>(&properties[15]),
            .storage_address = reinterpret_cast<std::uintptr_t>(&objects[5].title),
            .element_size = sizeof(FakeFText),
    };
    ue4ss::linux::core::UnrealPropertyValue remote_text;
    assert(runtime.read_hook_parameter(text_parameter, remote_text, detail));
    assert(remote_text.kind == ue4ss::linux::core::UnrealPropertyKind::text &&
           remote_text.text == "Hello FText \u2713");
    FakeFText copied_text{};
    assert(runtime.initialize_reflected_value(
            reinterpret_cast<std::uintptr_t>(&properties[15]),
            reinterpret_cast<std::uintptr_t>(&copied_text),
            detail));
    assert(runtime.copy_reflected_value(
            reinterpret_cast<std::uintptr_t>(&properties[15]),
            reinterpret_cast<std::uintptr_t>(&copied_text),
            reinterpret_cast<std::uintptr_t>(&objects[5].title),
            detail));
    const ue4ss::linux::core::RemoteUnrealParameter copied_text_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::text,
            .property_address = reinterpret_cast<std::uintptr_t>(&properties[15]),
            .storage_address = reinterpret_cast<std::uintptr_t>(&copied_text),
            .element_size = sizeof(FakeFText),
    };
    assert(runtime.read_hook_parameter(copied_text_parameter, remote_text, detail));
    assert(remote_text.text == "Hello FText \u2713");
    assert(runtime.destroy_reflected_value(
            reinterpret_cast<std::uintptr_t>(&properties[15]),
            reinterpret_cast<std::uintptr_t>(&copied_text),
            detail));
    assert(!runtime.write_hook_parameter(text_parameter, remote_text, detail));
    assert(detail.find("ProcessEvent") != std::string::npos ||
           detail.find("KismetTextLibrary CDO") != std::string::npos);
    std::int32_t native_end_play_reason = -2;
    const ue4ss::linux::core::RemoteUnrealParameter native_reason_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::signed_integer,
            .storage_address = reinterpret_cast<std::uintptr_t>(&native_end_play_reason),
            .element_size = sizeof(native_end_play_reason),
    };
    ue4ss::linux::core::UnrealPropertyValue native_reason_value;
    assert(runtime.read_hook_parameter(native_reason_parameter, native_reason_value, detail));
    assert(native_reason_value.signed_integer == -2);
    native_reason_value.signed_integer = 4;
    assert(runtime.write_hook_parameter(native_reason_parameter, native_reason_value, detail));
    assert(native_end_play_reason == 4);
    const auto invalid_native_reason = ue4ss::linux::core::RemoteUnrealParameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::signed_integer,
            .storage_address = reinterpret_cast<std::uintptr_t>(&native_end_play_reason),
            .element_size = 3,
    };
    assert(!runtime.read_hook_parameter(invalid_native_reason, native_reason_value, detail));
    assert(detail.find("invalid width") != std::string::npos);
    if (!runtime.read_property(derived.objects[0], "Mode", property_value, detail))
    {
        std::fprintf(stderr, "synthetic EnumProperty read failed: %s\n", detail.c_str());
        assert(false);
    }
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::signed_integer);
    assert(property_value.signed_integer == -1);
    assert(property_value.enum_object.address == address(16));
    assert(property_value.enum_property_name == "Mode");
    assert(runtime.read_property(derived.objects[0], "Location", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::structure);
    assert(property_value.struct_type == "Vector");
    assert(property_value.struct_fields.size() == 3u);
    assert(property_value.struct_fields[0].name == "X" &&
           property_value.struct_fields[0].value->floating_point == 1.0);
    assert(property_value.struct_fields[1].name == "Y" &&
           property_value.struct_fields[1].value->floating_point == 2.0);
    assert(property_value.struct_fields[2].name == "Z" &&
           property_value.struct_fields[2].value->floating_point == 3.0);
    const ue4ss::linux::core::RemoteUnrealParameter location_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::structure,
            .property_address = reinterpret_cast<std::uintptr_t>(&properties[4]),
            .storage_address = reinterpret_cast<std::uintptr_t>(&objects[5].location),
            .element_size = sizeof(std::array<double, 3>),
            .struct_address = address(11),
    };
    ue4ss::linux::core::UnrealPropertyValue remote_location;
    assert(runtime.read_hook_parameter(location_parameter, remote_location, detail));
    remote_location.struct_fields[0].value->floating_point = 4.0;
    remote_location.struct_fields[1].value->floating_point = 5.0;
    remote_location.struct_fields[2].value->floating_point = 6.0;
    assert(runtime.write_hook_parameter(location_parameter, remote_location, detail));
    assert((objects[5].location == std::array<double, 3>{4.0, 5.0, 6.0}));
    assert(runtime.read_property(derived.objects[0], "Targets", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::array);
    assert(property_value.array_elements.size() == 2u);
    assert(property_value.array_elements[0]->kind == ue4ss::linux::core::UnrealPropertyKind::object);
    assert(property_value.array_elements[0]->object.address == address(2));
    assert(property_value.array_elements[1]->object.address == address(5));
    const ue4ss::linux::core::RemoteUnrealParameter targets_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::array,
            .property_address = reinterpret_cast<std::uintptr_t>(&properties[5]),
            .storage_address = reinterpret_cast<std::uintptr_t>(&objects[5].targets),
            .element_size = sizeof(FakeScriptArray),
            .inner_property_address = reinterpret_cast<std::uintptr_t>(&array_inner),
    };
    assert(runtime.read_property(derived.objects[0], "Messages", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::array &&
           property_value.array_elements.empty());
    assert(runtime.read_property(derived.objects[0], "Scores", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::set);
    assert(property_value.set_elements.size() == 2u);
    assert(property_value.set_elements[0]->signed_integer == 11);
    assert(property_value.set_elements[1]->signed_integer == 33);
    assert(runtime.read_property(derived.objects[0], "Lookup", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::map);
    assert(property_value.map_entries.size() == 2u);
    assert(property_value.map_entries[0].key->kind == ue4ss::linux::core::UnrealPropertyKind::unsigned_integer);
    assert(property_value.map_entries[0].key->unsigned_integer == 7);
    assert(property_value.map_entries[0].value->object.address == address(2));
    assert(property_value.map_entries[1].key->unsigned_integer == 9);
    assert(property_value.map_entries[1].value->object.address == address(5));
    ue4ss::linux::core::UnrealPropertyValue set_replacement;
    assert(runtime.read_property(derived.objects[0], "Scores", set_replacement, detail));
    const ue4ss::linux::core::RemoteUnrealParameter scores_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::set,
            .property_address = reinterpret_cast<std::uintptr_t>(&properties[7]),
            .storage_address = reinterpret_cast<std::uintptr_t>(&objects[5].scores),
            .element_size = sizeof(FakeScriptSet),
            .inner_property_address = reinterpret_cast<std::uintptr_t>(&set_inner),
    };
    assert(runtime.write_hook_parameter(scores_parameter, set_replacement, detail));
    assert(runtime.read_property(derived.objects[0], "Scores", property_value, detail));
    assert(property_value.set_elements.size() == 2u);
    assert(property_value.set_elements[0]->signed_integer == 11);
    assert(property_value.set_elements[1]->signed_integer == 33);
    ue4ss::linux::core::UnrealPropertyValue map_replacement;
    assert(runtime.read_property(derived.objects[0], "Lookup", map_replacement, detail));
    const ue4ss::linux::core::RemoteUnrealParameter lookup_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::map,
            .property_address = reinterpret_cast<std::uintptr_t>(&properties[8]),
            .storage_address = reinterpret_cast<std::uintptr_t>(&objects[5].lookup),
            .element_size = sizeof(FakeScriptSet),
            .key_property_address = reinterpret_cast<std::uintptr_t>(&map_key),
            .value_property_address = reinterpret_cast<std::uintptr_t>(&map_value),
    };
    assert(runtime.write_hook_parameter(lookup_parameter, map_replacement, detail));
    assert(runtime.read_property(derived.objects[0], "Lookup", property_value, detail));
    assert(property_value.map_entries.size() == 2u);
    assert(property_value.map_entries[0].key->unsigned_integer == 7);
    assert(property_value.map_entries[1].key->unsigned_integer == 9);
    assert(runtime.read_property(derived.objects[0], "WeakTarget", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::weak_object);
    assert(!property_value.object_is_null && property_value.object.address == address(5));
    assert(runtime.read_property(derived.objects[0], "Handler", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::delegate);
    assert(!property_value.object_is_null && property_value.object.address == address(5));
    assert(property_value.text == "GetHealth");
    assert(runtime.read_property(derived.objects[0], "Handlers", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::multicast_delegate);
    assert(property_value.array_elements.size() == 2u);
    assert(property_value.array_elements[0]->object.address == address(5));
    assert(property_value.array_elements[0]->text == "GetHealth");
    assert(property_value.array_elements[1]->object.address == address(2));
    assert(property_value.array_elements[1]->text == "OnHandled");
    assert(runtime.read_property(derived.objects[0], "SparseHandlers", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::multicast_delegate);
    assert(property_value.multicast_sparse);
    assert(property_value.array_elements.size() == 1u);
    assert(property_value.array_elements[0]->object.address == address(5));
    assert(property_value.array_elements[0]->text == "GetHealth");
    assert(runtime.read_property(derived.objects[0], "InterfaceTarget", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::object);
    assert(!property_value.object_is_null && property_value.object.address == address(5));
    const ue4ss::linux::core::RemoteUnrealParameter interface_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::object,
            .property_address = reinterpret_cast<std::uintptr_t>(&properties[12]),
            .storage_address = reinterpret_cast<std::uintptr_t>(&objects[5].interface_target),
            .element_size = static_cast<std::int32_t>(2u * sizeof(void*)),
            .interface_class_address = interface_class_address,
    };
    ue4ss::linux::core::UnrealPropertyValue remote_interface;
    assert(runtime.read_hook_parameter(interface_parameter, remote_interface, detail));
    assert(!remote_interface.object_is_null && remote_interface.object.address == address(5));
    ue4ss::linux::core::UnrealPropertyValue null_interface;
    null_interface.kind = ue4ss::linux::core::UnrealPropertyKind::object;
    null_interface.object_is_null = true;
    assert(runtime.write_hook_parameter(interface_parameter, null_interface, detail));
    assert(objects[5].interface_target[0] == 0u && objects[5].interface_target[1] == 0u);
    assert(runtime.write_hook_parameter(interface_parameter, remote_interface, detail));
    assert(objects[5].interface_target[0] == address(5) &&
           objects[5].interface_target[1] == address(5));
    const ue4ss::linux::core::RemoteUnrealParameter messages_parameter{
            .kind = ue4ss::linux::core::UnrealPropertyKind::array,
            .property_address = reinterpret_cast<std::uintptr_t>(&properties[6]),
            .storage_address = reinterpret_cast<std::uintptr_t>(&objects[5].messages),
            .element_size = sizeof(FakeScriptArray),
            .inner_property_address = reinterpret_cast<std::uintptr_t>(&string_array_inner),
    };
    ue4ss::linux::core::RawFName constructed_name{};
    assert(runtime.fname_from_utf8("Greeting", constructed_name, true, detail));
    assert(constructed_name.comparison_index == 18u && constructed_name.number == 0u);
    assert(!runtime.read_property(derived.objects[0], "Missing", property_value, detail));

    ue4ss::linux::core::ReadOnlyUObjectHandle data_table_handle;
    assert(runtime.handle_from_address(address(30), data_table_handle));
    ue4ss::linux::core::ReadOnlyUObjectHandle row_struct_handle;
    std::vector<ue4ss::linux::core::UnrealDataTableRow> data_table_rows;
    assert(runtime.enumerate_data_table_rows(
            data_table_handle, row_struct_handle, data_table_rows, detail));
    assert(row_struct_handle.address == address(11));
    assert(data_table_rows.size() == 2u);
    assert(data_table_rows[0].name_text == "RowA" &&
           data_table_rows[1].name_text == "RowB");
    assert(runtime.read_struct_property(
            data_table_rows[0].data, "X", property_value, detail));
    assert(property_value.kind == ue4ss::linux::core::UnrealPropertyKind::floating_point &&
           property_value.floating_point == 10.0);

    const auto object_api = ue4ss::linux::core::validate_readonly_object_api(resolved);
    assert(object_api.success);

#if UE4SS_WITH_LUA_RUNTIME
    const std::filesystem::path game_directory_fixture =
            std::filesystem::temp_directory_path() /
            ("ue4ss-game-directories-" +
             std::to_string(reinterpret_cast<std::uintptr_t>(&runtime)));
    std::filesystem::remove_all(game_directory_fixture);
    const std::filesystem::path fixture_game = game_directory_fixture / "Pal";
    const std::filesystem::path fixture_executable_directory =
            fixture_game / "Binaries" / "Linux";
    const std::filesystem::path fixture_logic_mod =
            fixture_game / "Content" / "Paks" / "LogicMods" / "Sample";
    const std::filesystem::path fixture_ue4ss_root =
            game_directory_fixture / "ue4ss";
    std::filesystem::create_directories(fixture_executable_directory);
    std::filesystem::create_directories(fixture_logic_mod);
    std::ofstream{fixture_game / "Content" / "Paks" / "LogicMods" / "sample.pak"}
            << "pak fixture\n";
    std::ofstream{fixture_logic_mod / "config.lua"} << "return true\n";

    ue4ss::linux::core::HeadlessLuaState lua;
    const auto binding_result = lua.install_readonly_unreal_bindings(
            runtime,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            fixture_executable_directory,
            fixture_ue4ss_root);
    assert(binding_result.success);
    assert(lua.execute_string(R"lua(
        assert(type(DumpAllObjects) == "function")
        assert(DumpAllObjectsAvailable == true)
        DumpAllObjects()
    )lua").success);
    const std::filesystem::path object_dump =
            fixture_ue4ss_root / "Logs" / "UE4SS_ObjectDump.txt";
    std::ifstream object_dump_stream{object_dump};
    const std::string object_dump_text{
            std::istreambuf_iterator<char>{object_dump_stream},
            std::istreambuf_iterator<char>{}};
    assert(object_dump_text.find("Derived /Script/Test.Instance") !=
           std::string::npos);
    assert(object_dump_text.find("IntProperty Health") !=
           std::string::npos);
    assert(object_dump_text.find("# visited=31 written=31") !=
           std::string::npos);
    std::vector<std::pair<ue4ss::linux::core::LuaModLifecycleAction, std::string>>
            lifecycle_requests;
    lua.configure_mod_lifecycle(
            "SyntheticMod",
            [&lifecycle_requests](
                    ue4ss::linux::core::LuaModLifecycleAction action,
                    std::string_view target,
                    std::string&) {
                lifecycle_requests.emplace_back(action, target);
                return true;
            });
    assert(lua.execute_string(R"lua(
        assert(ModLifecycleAvailable == true)
        RestartCurrentMod()
        UninstallCurrentMod()
        RestartMod("AnotherMod")
        UninstallMod("ThirdMod")
    )lua").success);
    assert(lifecycle_requests.size() == 4u);
    assert(lifecycle_requests[0].first ==
           ue4ss::linux::core::LuaModLifecycleAction::restart);
    assert(lifecycle_requests[0].second == "SyntheticMod");
    assert(lifecycle_requests[1].first ==
           ue4ss::linux::core::LuaModLifecycleAction::uninstall);
    assert(lifecycle_requests[1].second == "SyntheticMod");
    assert(lifecycle_requests[2].first ==
           ue4ss::linux::core::LuaModLifecycleAction::restart);
    assert(lifecycle_requests[2].second == "AnotherMod");
    assert(lifecycle_requests[3].first ==
           ue4ss::linux::core::LuaModLifecycleAction::uninstall);
    assert(lifecycle_requests[3].second == "ThirdMod");
    const auto lua_result = lua.execute_string(R"lua(
        local object = FindFirstOf("Base")
        assert(object ~= nil and object:IsValid())
        assert(object:GetName() == "Instance")
        assert(object:GetPathName() == "/Script/Test.Instance")
        assert(object:GetFullName() == "Derived /Script/Test.Instance")
        assert(tostring(object) == object:GetFullName())
        local ue4ss_major, ue4ss_minor, ue4ss_hotfix = UE4SS.GetVersion()
        assert(ue4ss_major == 3 and ue4ss_minor == 0 and ue4ss_hotfix == 1)
        assert(Key.A == 0x41 and Key.F24 == 0x87 and Key.OEM_CLEAR == 0xFE)
        assert(ModifierKey.SHIFT == 0x10 and ModifierKey.CONTROL == 0x11 and
               ModifierKey.ALT == 0x12)
        assert(UnrealVersion.GetMajor() == 5 and UnrealVersion.GetMinor() == 1)
        assert(UnrealVersion.IsEqual(5, 1) and UnrealVersion:IsAtLeast(5, 0) and
               UnrealVersion.IsAtMost(5, 1) and UnrealVersion.IsBelow(5, 2) and
               UnrealVersion.IsAbove(4, 27))
        assert(FPackageName.IsShortPackageName("Test") and
               not FPackageName:IsShortPackageName("/Game/Test"))
        assert(FPackageName.IsValidLongPackageName("/Game/Test") and
               not FPackageName.IsValidLongPackageName("Game/Test") and
               not FPackageName.IsValidLongPackageName("/Bad Name"))
        assert(ModRef:type() == "ModRef")
        ModRef:SetSharedVariable("SyntheticNumber", 123)
        ModRef:SetSharedVariable("SyntheticString", "shared")
        ModRef:SetSharedVariable("SyntheticObject", object)
        ModRef:SetSharedVariable("SyntheticNil", nil)
        assert(ModRef:GetSharedVariable("SyntheticNumber") == 123 and
               ModRef:GetSharedVariable("SyntheticString") == "shared" and
               ModRef:GetSharedVariable("SyntheticObject") == object and
               ModRef:GetSharedVariable("SyntheticNil") == nil and
               ModRef:GetSharedVariable("MissingShared") == nil)
        assert(type(MainThread) == "thread" and type(AsyncThread) == "thread")
        assert(CustomPropertiesAvailable == true and type(RegisterCustomProperty) == "function")
        assert(type(PropertyTypes) == "table" and
               PropertyTypes.IntProperty.Name == "IntProperty" and
               PropertyTypes.IntProperty.Size == 4 and
               PropertyTypes.IntProperty.FFieldClassPointer ~= 0 and
               PropertyTypes.ArrayProperty.Size == 16 and
               PropertyTypes.ObjectProperty.Size == 8)
        RegisterCustomProperty({
            Name = "CustomHealth",
            Type = PropertyTypes.IntProperty,
            BelongsToClass = "/Script/Test.Derived",
            OffsetInternal = { Property = "Health", RelativeOffset = 0 }
        })
        -- Exact duplicate registration is idempotent across mod reloads.
        RegisterCustomProperty({
            Name = "CustomHealth",
            Type = PropertyTypes.IntProperty,
            BelongsToClass = "Class /Script/Test.Derived",
            OffsetInternal = 0x80
        })
        RegisterCustomProperty({
            Name = "CustomTargets",
            Type = PropertyTypes.ArrayProperty,
            BelongsToClass = "/Script/Test.Derived",
            OffsetInternal = { Property = "Targets", RelativeOffset = 0 },
            ArrayProperty = { Type = PropertyTypes.ObjectProperty }
        })
        assert(object.CustomHealth == 42)
        assert(type(object.CustomTargets) == "userdata" and #object.CustomTargets == 2 and
               object.CustomTargets[2] == object)
        local invalid_size = {
            Name = "IntProperty", Size = 8,
            FFieldClassPointer = 1, StaticPointer = 0
        }
        local bad_size_ok, bad_size_error = pcall(function()
            RegisterCustomProperty({
                Name = "BadSize", Type = invalid_size,
                BelongsToClass = "/Script/Test.Derived", OffsetInternal = 0x80
            })
        end)
        assert(not bad_size_ok and bad_size_error:find("expected 4", 1, true) ~= nil)
        local bad_container_ok, bad_container_error = pcall(function()
            RegisterCustomProperty({
                Name = "BadSet", Type = PropertyTypes.SetProperty,
                BelongsToClass = "/Script/Test.Derived", OffsetInternal = 0xe8
            })
        end)
        assert(not bad_container_ok and bad_container_error:find("metadata", 1, true) ~= nil)
        assert(object.Health == 42)
        assert(object:GetPropertyValue("Health") == 42)
        local reflected_health = object:Reflection():GetProperty("Health")
        assert(reflected_health:IsValid() and reflected_health:GetName() == "Health")
        assert(not object:Reflection():GetProperty("MissingProperty"):IsValid())
        assert(object.Alive == true)
        assert(object.Label:ToString() == "Health")
        assert(object.Label:GetComparisonIndex() == 9)
        assert(object.Greeting:ToString() == "")
        assert(object.Title:type() == "FText")
        assert(object.Title:ToString() == "Hello FText \u{2713}")
        assert(tostring(object.Title) == "Hello FText \u{2713}")
        local local_text = FText("Hello FText \u{2713}")
        assert(local_text:type() == "FText" and local_text:ToString() == "Hello FText \u{2713}")
        assert(local_text == object.Title and local_text ~= FText("different"))
        assert(object.Mode == -1)
        assert(type(Enum_Mode) == "table" and Enum_Mode["TestMode::Disabled"] == -1 and
               Enum_Mode["TestMode::Enabled"] == 1 and Enum_Mode["TestMode::MAX"] == 2)
        local test_enum = FindFirstOf("Enum")
        assert(test_enum ~= nil and test_enum:IsValid() and test_enum:NumEnums() == 3)
        assert(test_enum:GetNameByValue(-1):ToString() == "TestMode::Disabled")
        assert(test_enum:GetNameByValue(999):ToString() == "None")
        local first_enum_name, first_enum_value = test_enum:GetEnumNameByIndex(0)
        assert(first_enum_name:ToString() == "TestMode::Disabled" and first_enum_value == -1)
        local enum_visit_count = 0
        test_enum:ForEachName(function(name, value)
            enum_visit_count = enum_visit_count + 1
            assert(type(value) == "number" and name:ToString():find("TestMode::", 1, true) == 1)
        end)
        assert(enum_visit_count == 3)
        local enum_mutation_ok, enum_mutation_error = pcall(function()
            test_enum:EditValueAt(1, 5)
        end)
        assert(not enum_mutation_ok and
               enum_mutation_error:find("game-thread", 1, true) ~= nil)
        local location = object.Location
        assert(type(location) == "userdata" and location:type() == "UScriptStruct")
        assert(location:IsValid() and location:IsMappedToObject() and location:IsMappedToProperty())
        assert(location:GetStructAddress() ~= 0 and location:GetPropertyAddress() ~= 0)
        assert(location:GetProperty():IsValid() and location:GetProperty():GetName() == "Location")
        assert(location:GetStruct():GetName() == "Vector")
        assert(location.X == 4.0 and location.Y == 5.0 and location.Z == 6.0)
        local data_table = FindFirstOf("DataTable")
        assert(data_table:IsValid() and data_table:type() == "UDataTable" and #data_table == 2)
        assert(data_table:GetRowStruct():GetName() == "Vector")
        local row_names = data_table:GetRowNames()
        assert(#row_names == 2 and row_names[1] == "RowA" and row_names[2] == "RowB")
        local row_map = data_table:GetRowMap()
        assert(row_map.RowA:type() == "UScriptStruct" and row_map.RowA.X == 10.0)
        assert(row_map.RowB.X == 40.0 and not row_map.RowA:IsMappedToProperty())
        assert(data_table:FindRow("RowA").Y == 20.0)
        assert(data_table:FindRow(FName("RowB")).Z == 60.0)
        assert(data_table:FindRow("MissingRow") == nil)
        local all_rows = data_table:GetAllRows()
        assert(#all_rows == 2 and all_rows[1].Name == "RowA" and
               all_rows[1].Data.X == 10.0 and all_rows[2].Name == "RowB")
        local row_visit_count = 0
        data_table:ForEachRow(function(row_name, row)
            row_visit_count = row_visit_count + 1
            assert((row_name == "RowA" and row.X == 10.0) or
                   (row_name == "RowB" and row.X == 40.0))
        end)
        assert(row_visit_count == 2)
        local mutation_ok, mutation_error = pcall(function()
            data_table:RemoveRow("RowA")
        end)
        assert(not mutation_ok and mutation_error:find("game%-thread scheduler") ~= nil)
        local targets = object.Targets
        assert(type(targets) == "userdata" and #targets == 2)
        assert(targets:GetArrayNum() == 2 and targets:GetArrayMax() == 2 and targets:IsValid())
        assert(targets:type() == "TArray")
        assert(targets[1]:GetName() == "/Script/Test" and targets[2] == object)
        local visited = 0
        targets:ForEach(function(index, element)
            assert(index == visited + 1)
            assert(element:get() == targets[index])
            if index == 1 then element:set(object) end
            visited = visited + 1
        end)
        assert(visited == 2 and targets[1] == object)
        assert(type(object.Messages) == "userdata" and #object.Messages == 0)
        local scores = object.Scores
        assert(type(scores) == "userdata" and scores:type() == "TSet" and scores:IsValid())
        assert(#scores == 2 and scores:Contains(11) and scores:Contains(33) and not scores:Contains(22))
        local score_sum = 0
        scores:ForEach(function(score) score_sum = score_sum + score end)
        assert(score_sum == 44)
        assert(scores:Add(44) == 44 and scores:Contains(44) and #scores == 3)
        assert(scores:Remove(44) == 44 and not scores:Contains(44) and #scores == 2)
        local lookup = object.Lookup
        assert(type(lookup) == "userdata" and lookup:type() == "TMap" and lookup:IsValid())
        assert(#lookup == 2 and lookup:Contains(7) and lookup:Contains(9) and not lookup:Contains(8))
        assert(lookup:Find(7):GetName() == "/Script/Test")
        assert(lookup:Find(9) == object)
        local existing_lookup_value = lookup:Find(7)
        assert(lookup:Add(7, existing_lookup_value) == existing_lookup_value and
               #lookup == 2 and lookup:Find(7) == existing_lookup_value)
        assert(lookup:Add(8, object) == object and lookup:Contains(8) and lookup:Find(8) == object)
        assert(lookup:Remove(8) == 8 and not lookup:Contains(8))
        local map_count = 0
        lookup:ForEach(function(key, value)
            assert((key == 7 and value:GetName() == "/Script/Test") or (key == 9 and value == object))
            map_count = map_count + 1
        end)
        assert(map_count == 2)
        assert(object.WeakTarget:type() == "FWeakObjectPtr" and object.WeakTarget:Get() == object)
        assert(object.WeakTarget:get() == object)
        assert(type(object.Handler) == "table" and object.Handler.Object == object)
        assert(object.Handler.FunctionName:ToString() == "GetHealth")
        local handlers = object.Handlers
        assert(type(handlers) == "userdata" and handlers:type() == "MulticastDelegateProperty")
        assert(#handlers == 2)
        local bindings = handlers:GetBindings()
        assert(type(bindings) == "table" and #bindings == 2)
        assert(bindings[1].Object == object and
               bindings[1].FunctionName:ToString() == "GetHealth")
        assert(bindings[2].Object:GetName() == "/Script/Test" and
               bindings[2].FunctionName:ToString() == "OnHandled")
        local sparse_handlers = object.SparseHandlers
        assert(type(sparse_handlers) == "userdata" and
               sparse_handlers:type() == "MulticastSparseDelegateProperty")
        assert(#sparse_handlers == 1)
        local sparse_bindings = sparse_handlers:GetBindings()
        assert(type(sparse_bindings) == "table" and #sparse_bindings == 1)
        assert(sparse_bindings[1].Object == object and
               sparse_bindings[1].FunctionName:ToString() == "GetHealth")
        assert(object.InterfaceTarget == object)
        local soft = object.SoftTarget
        assert(type(soft) == "userdata" and soft:type() == "TSoftObjectPtrUserdata")
        assert(soft:GetWeakPtr():Get() == object and soft:GetTagAtLastTest() == 42)
        local soft_path = soft:GetObjectID()
        assert(soft_path:type() == "FSoftObjectPathUserdata")
        assert(soft_path:GetAssetPathName():ToString() == "/Game/Test.Asset")
        assert(soft_path:GetSubPathString():ToString() == "Component")

        local name = FName("Greeting")
        assert(name:ToString() == "Greeting")
        assert(name:type() == "FName")
        assert(name == FName(18))
        assert(name:Equals(FName("Greeting", EFindName.FNAME_Find)))
        assert(NAME_None:ToString() == "None")
        local string = FString("Grüße")
        assert(string:ToString() == "Grüße" and string:Len() == 5)
        string:Append("!")
        assert(string:EndsWith("!") and string:StartsWith("Gr") and string:Find("ü") == 3)

        local utf8 = FUtf8String("Grüße")
        assert(utf8:type() == "FUtf8String" and utf8:ToString() == "Grüße" and utf8:Len() == 7)
        utf8:Append(FUtf8String("!"))
        assert(utf8:EndsWith("!") and utf8:StartsWith("Gr") and utf8:Find("ü") == 3)
        local utf8_upper = utf8:ToUpper():ToString()
        assert(utf8_upper == "GRüßE!", "unexpected FUtf8String upper value: " .. utf8_upper)
        local ansi = FAnsiString("Linux")
        ansi:Append(" ANSI")
        assert(ansi:type() == "FAnsiString" and ansi:Len() == 10 and
               ansi:Find("ANSI") == 7 and ansi:ToLower():ToString() == "linux ansi")
        ansi:Clear()
        assert(ansi:IsEmpty() and ansi:Len() == 0)

        local directories = IterateGameDirectories()
        assert(type(directories) == "table" and type(directories.Game) == "table")
        assert(directories.Game.__name == "Pal" and
               directories.Game.__absolute_path:match("/Pal$") ~= nil)
        local logic_mods = directories.Game.Content.Paks.LogicMods
        assert(type(logic_mods) == "table" and logic_mods.__name == "LogicMods")
        local directory_entries = 0
        for key, value in pairs(logic_mods) do
            assert(key == "Sample" and type(value) == "table")
            directory_entries = directory_entries + 1
        end
        assert(directory_entries == 1)
        local files = logic_mods.__files
        assert(type(files) == "table" and #files == 1 and files[1].__name == "sample.pak")
        local nested_files = logic_mods.Sample.__files
        assert(type(nested_files) == "table" and #nested_files == 1 and
               nested_files[1].__name == "config.lua")
        assert(CreateLogicModsDirectory() == true)

        local all = FindAllOf("Base")
        assert(type(all) == "table" and #all == 1 and all[1] == object)

        local invalid = CreateInvalidObject()
        assert(type(invalid) == "userdata" and not invalid:IsValid())
        assert(invalid == CreateInvalidObject())

        local current_thread = GetCurrentThreadId()
        assert(current_thread:type() == "ThreadId")
        assert(#current_thread:ToString() > 0)
        assert(current_thread == GetMainModThreadId())
        assert(IsInMainModThread() and not IsInAsyncThread())
        assert(GetAsyncThreadId() ~= current_thread)
        local early_game_thread_ok, early_game_thread_error = pcall(GetGameThreadId)
        assert(not early_game_thread_ok and early_game_thread_error:find("before UGameEngine::Tick") ~= nil)

        local class = StaticFindObject("/Script/Test.Derived")
        assert(class ~= nil and class:IsValid())
        assert(class:type() == "UClass" and object:type() == "UObject")
        assert(class:GetAddress() ~= 0 and object:GetAddress() ~= 0)
        assert(object:GetFName():ToString() == "Instance")
        assert(class:IsAnyClass() and class:IsClass())
        assert(not object:IsAnyClass() and not object:IsClass())
        assert(object:HasAllFlags(0x1) and object:HasAnyFlags(0x1))
        assert(not object:HasAnyFlags(EObjectFlags.RF_ClassDefaultObject))
        assert(object:HasAnyInternalFlags(0x40))
        assert(object:GetClass() == class)
        local interface_class = StaticFindObject("/Script/Test.Base")
        assert(interface_class ~= nil and object:ImplementsInterface(interface_class))
        assert(class:GetSuperStruct() == interface_class)
        assert(not interface_class:GetSuperStruct():IsValid())
        assert(class:IsChildOf(interface_class) and class:IsChildOf(class))
        assert(not interface_class:IsChildOf(class))
        local function_count = 0
        class:ForEachFunction(function(function_object)
            assert(function_object:type() == "UFunction")
            assert(function_object:GetOuter() == class)
            if function_object:GetName() == "GetHealth" then
                assert(function_object:GetFunctionFlags() == 0x400)
                function_object:SetFunctionFlags(0x401)
                assert(function_object:GetFunctionFlags() == 0x401)
                function_object:SetFunctionFlags(0x400)
            end
            function_count = function_count + 1
        end)
        assert(function_count == 11)
        local interface_metadata_seen = false
        local health_property = nil
        class:ForEachProperty(function(property)
            if property:GetName() == "Health" then
                health_property = property
                assert(property:IsA(PropertyTypes.IntProperty))
                assert(not property:IsA(PropertyTypes.BoolProperty))
                assert(property:GetAddress() ~= 0)
                assert(property:GetFName():ToString() == "Health")
                assert(property:GetFullName() == "IntProperty /Script/Test.Derived:Health")
                assert(property:ContainerPtrToValuePtr(object) ~= nil)
                local import_ok, import_error = pcall(function()
                    property:ImportText("5", property:ContainerPtrToValuePtr(object), 0, object)
                end)
                assert(not import_ok and import_error:find("game thread") ~= nil)
            end
            if property:GetName() == "Alive" then
                assert(property:GetByteMask() == 0x4 and property:GetByteOffset() == 0)
                assert(property:GetFieldMask() == 0x4 and property:GetFieldSize() == 1)
            end
            if property:GetName() == "Targets" then
                local inner = property:GetInner()
                assert(inner:type() == "ObjectProperty")
                assert(inner:GetPropertyClass() == class)
            end
            if property:GetName() == "Location" then
                assert(property:GetStruct():GetName() == "Vector")
                assert(property:GetStruct():type() == "UScriptStruct")
            end
            if property:GetName() == "Mode" then
                assert(property:GetEnum():GetName() == "TestMode")
                assert(property:GetEnum():type() == "UEnum")
            end
            if property:GetName() == "InterfaceTarget" then
                assert(property:GetClass():GetName() == "InterfaceProperty")
                assert(property:GetInterfaceClass() == interface_class)
                interface_metadata_seen = true
            end
        end)
        assert(health_property ~= nil)
        assert(interface_metadata_seen)
        assert(class:GetCDO():IsValid() and class:GetCDO():GetClass() == class)
        assert(EObjectFlags.RF_NoFlags == 0 and EObjectFlags.RF_ClassDefaultObject == 0x10 and
               EObjectFlags.RF_AllFlags == 0x0fffffff)
        local default_objects = FindObjects(0, class, nil,
            EObjectFlags.RF_ClassDefaultObject, EObjectFlags.RF_NoFlags, true)
        assert(type(default_objects) == "table" and #default_objects == 1 and
               default_objects[1] == class:GetCDO())
        local non_default_objects = FindObjects(1, "Base", FName("Instance"),
            EObjectFlags.RF_NoFlags, EObjectFlags.RF_ClassDefaultObject, false)
        assert(type(non_default_objects) == "table" and #non_default_objects == 1 and
               non_default_objects[1] == object)
        local classes = FindObjects(0, "Class", nil, 0, 0, true)
        assert(type(classes) == "table" and #classes > 0)
        assert(FindObject("Base", "Instance", 0, EObjectFlags.RF_ClassDefaultObject) == object)
        assert(FindObject(class, FName("Instance"), 0, EObjectFlags.RF_ClassDefaultObject) == object)
        local package = StaticFindObject("/Script/Test")
        assert(object:GetOuter() == package)
        assert(FindObject(class, package, "Instance", false) == object)
        assert(StaticFindObject(class, package, "Instance", false) == object)
        assert(not FindObject("Base", "Missing", 0, 0):IsValid())
        local visited = 0
        ForEachUObject(function(candidate, chunk_index, object_index)
            assert(candidate:IsValid())
            assert(chunk_index == 0 and object_index == visited)
            visited = visited + 1
            return true -- Upstream ignores callback return values for this API.
        end)
        assert(visited == 31)
        local non_class_cdo_ok, non_class_cdo_error = pcall(function() return object:GetCDO() end)
        assert(not non_class_cdo_ok and non_class_cdo_error:find("UClass") ~= nil)
        assert(object:IsA(class))
        assert(not StaticFindObject("/Script/Test.Missing"):IsValid())
        local actor = FindFirstOf("Actor")
        local level = FindFirstOf("Level")
        local world = FindFirstOf("World")
        assert(actor:IsValid() and actor:type() == "AActor")
        assert(level:IsValid() and world:IsValid() and world:type() == "UWorld")
        assert(actor:GetLevel() == level and actor:GetWorld() == world)
        assert(level:GetWorld() == world and world:GetWorld() == world)
    )lua");
    if (!lua_result.success)
    {
        std::fprintf(stderr, "synthetic Lua conformance failed: %s\n", lua_result.detail.c_str());
    }
    assert(lua_result.success);
    std::filesystem::remove_all(
            fixture_game / "Content" / "Paks" / "LogicMods");
    const auto logic_mod_creation_result = lua.execute_string(R"lua(
        assert(CreateLogicModsDirectory() == true)
        local refreshed = IterateGameDirectories()
        assert(type(refreshed.Game.Content.Paks.LogicMods) == "table")
    )lua");
    assert(logic_mod_creation_result.success);
    std::filesystem::remove_all(game_directory_fixture);
    const auto unavailable_scheduler = lua.execute_string(R"lua(
        local ok, message = pcall(function()
            ExecuteInGameThread(function() end)
        end)
        assert(not ok and message:find("not available") ~= nil)
    )lua");
    assert(unavailable_scheduler.success);

    ue4ss::linux::core::ReadOnlyUObjectHandle out_function_handle;
    assert(runtime.find_function(derived.objects[0], "GetOutputs", out_function_handle, detail));
    assert(out_function_handle.address == address(9));

    void* tick_slot = reinterpret_cast<void*>(&fake_game_engine_tick);
    ue4ss::linux::core::GameThreadScheduler scheduler;
    assert(scheduler.start_on_slot(&tick_slot, &fake_game_engine_tick, detail));
    runtime.set_process_event_for_testing(reinterpret_cast<void*>(&fake_process_event));
    ue4ss::linux::core::HeadlessLuaState scheduled_lua;
    assert(scheduled_lua.install_readonly_unreal_bindings(runtime, &scheduler).success);
    const auto scheduled_result = scheduled_lua.execute_string(R"lua(
        scheduled_immediate = false
        scheduled_delayed = false
        scheduled_execute_async = false
        async_thread_identity_ok = false
        async_unreal_access = false
        scheduled_loop_count = 0
        controlled_delay_count = 0
        explicit_delay_count = 0
        frame_delay_count = 0
        frame_loop_count = 0
        retrigger_count = 0
        ExecuteInGameThread(function()
            assert(IsInGameThread())
            assert(GetCurrentThreadId() == GetGameThreadId())
            local object = FindFirstOf("Base")
            local health_property = nil
            object:GetClass():ForEachProperty(function(property)
                if property:GetName() == "Health" then health_property = property end
            end)
            assert(health_property ~= nil)
            assert(ModRef:GetSharedVariable("SyntheticNumber") == 123 and
                   ModRef:GetSharedVariable("SyntheticString") == "shared" and
                   ModRef:GetSharedVariable("SyntheticObject") == object)
            assert(object.CustomHealth == 42)
            assert(object:GetPropertyValue("CustomHealth") == 42)
            local test_enum = FindFirstOf("Enum")
            test_enum:EditNameAt(1, "TestMode::Renamed")
            test_enum:EditValueAt(1, 5)
            assert(test_enum:GetEnumNameByIndex(1):ToString() == "TestMode::Renamed")
            local inserted_index = test_enum:InsertIntoNames("TestMode::Inserted", 3, 1, true)
            assert(inserted_index == 1 and test_enum:NumEnums() == 4)
            local inserted_name, inserted_value = test_enum:GetEnumNameByIndex(1)
            local shifted_name, shifted_value = test_enum:GetEnumNameByIndex(2)
            assert(inserted_name:ToString() == "TestMode::Inserted" and inserted_value == 3)
            assert(shifted_name:ToString() == "TestMode::Renamed" and shifted_value == 6)
            test_enum:RemoveFromNamesAt(1, 1, true)
            assert(test_enum:NumEnums() == 3)
            test_enum:EditNameAt(1, "TestMode::Enabled")
            test_enum:EditValueAt(1, 1)
            object:SetPropertyValue("Health", 72)
            assert(object:GetPropertyValue("Health") == 72 and object.CustomHealth == 72)
            object:SetPropertyValue("Health", 42)
            object.CustomHealth = 74
            assert(object.Health == 74 and object.CustomHealth == 74)
            object.Health = 73
            assert(object.CustomHealth == 73)
            health_property:ImportText(
                "71", health_property:ContainerPtrToValuePtr(object), 0, object)
            assert(object.Health == 71 and object.CustomHealth == 71)
            object:SetPropertyValue("Health", 73)
            object.Alive = false
            object.Label = FName("Greeting")
            object.Greeting = FString("Linux ✓")
            local current_mode = object.Mode
            object.Mode = Enum_Mode["TestMode::Enabled"]
            assert(object:EchoMode(Enum_Mode["TestMode::Enabled"]) == Enum_Mode["TestMode::Enabled"])
            assert(type(Enum_ReturnValue) == "table" and
                   Enum_ReturnValue["TestMode::Disabled"] == -1 and
                   Enum_ReturnValue["TestMode::Enabled"] == 1)
            object.Handler = { Object = object, FunctionName = FName("ScaleVector") }
            assert(object.Handler.Object == object and
                   object.Handler.FunctionName:ToString() == "ScaleVector")
            object.Handler = nil
            assert(object.Handler.Object == nil and object.Handler.FunctionName:ToString() == "None")
            object.Handler = { Object = object, FunctionName = "GetHealth" }
            object.InterfaceTarget = nil
            assert(object.InterfaceTarget == nil)
            object.InterfaceTarget = object
            assert(object.InterfaceTarget == object)
            local soft = object.SoftTarget
            object.SoftTarget = soft
            local rewritten_soft = object.SoftTarget
            assert(rewritten_soft:GetWeakPtr():Get() == object and
                   rewritten_soft:GetObjectID():GetAssetPathName():ToString() == "/Game/Test.Asset" and
                   rewritten_soft:GetObjectID():GetSubPathString():ToString() == "Component")
            local echoed_soft = object:EchoSoft(soft)
            assert(echoed_soft:type() == "TSoftObjectPtrUserdata" and
                   echoed_soft:GetWeakPtr():Get() == object and
                   echoed_soft:GetObjectID():GetAssetPathName():ToString() == "/Game/Test.Asset" and
                   echoed_soft:GetObjectID():GetSubPathString():ToString() == "Component")
            local handlers = object.Handlers
            handlers:Add(object, FName("ScaleVector"))
            assert(#handlers == 3)
            handlers:Add(object, FName("ScaleVector"))
            assert(#handlers == 3)
            handlers:Remove(object, "ScaleVector")
            assert(#handlers == 2)
            handlers:Clear()
            assert(#handlers == 0 and #handlers:GetBindings() == 0)
            handlers:Add(object, "GetHealth")
            local rebound = handlers:GetBindings()
            assert(#rebound == 1 and rebound[1].Object == object and
                   rebound[1].FunctionName:ToString() == "GetHealth")
            handlers:Broadcast(40)
            local sparse_handlers = object.SparseHandlers
            assert(sparse_handlers:type() == "MulticastSparseDelegateProperty" and
                   #sparse_handlers == 1)
            sparse_handlers:Add(object, "ScaleVector")
            assert(#sparse_handlers == 2)
            sparse_handlers:Add(object, FName("ScaleVector"))
            assert(#sparse_handlers == 2)
            sparse_handlers:Remove(object, "ScaleVector")
            assert(#sparse_handlers == 1)
            local sparse_rebound = sparse_handlers:GetBindings()
            assert(#sparse_rebound == 1 and sparse_rebound[1].Object == object and
                   sparse_rebound[1].FunctionName:ToString() == "GetHealth")
            sparse_handlers:Broadcast(40)
            local echoed_delegate = object:EchoDelegate({
                Object = object,
                FunctionName = FName("GetHealth")
            })
            assert(type(echoed_delegate) == "table" and echoed_delegate.Object == object and
                   echoed_delegate.FunctionName:ToString() == "GetHealth")
            local out_handlers = {}
            assert(object:GetHandlers(out_handlers) == nil)
            assert(type(out_handlers.OutHandlers) == "table" and #out_handlers.OutHandlers == 1 and
                   out_handlers.OutHandlers[1].Object == object and
                   out_handlers.OutHandlers[1].FunctionName:ToString() == "GetHealth")
            object.Location = { X = 7.5, Y = 8.5, Z = 9.5 }
            local live_location = object.Location
            live_location.X = 6.5
            assert(object.Location.X == 6.5)
            live_location.X = 7.5
            local data_table = FindFirstOf("DataTable")
            local live_row = data_table:FindRow("RowA")
            live_row.X = 11.5
            assert(data_table:GetRowMap().RowA.X == 11.5)
            data_table:AddRow("RowC", { X = 70.0, Y = 80.0, Z = 90.0 })
            assert(#data_table == 3 and data_table:FindRow("RowC").Y == 80.0)
            data_table:RemoveRow(FName("RowC"))
            assert(#data_table == 2 and data_table:FindRow("RowC") == nil)
            data_table:AddRow("RowC", live_row)
            assert(data_table:FindRow("RowC").X == 11.5)
            data_table:EmptyTable()
            assert(#data_table == 0 and data_table:FindRow("RowA") == nil)
            data_table:AddRow("RowA", { X = 11.5, Y = 20.0, Z = 30.0 })
            data_table:AddRow("RowB", { X = 40.0, Y = 50.0, Z = 60.0 })
            assert(#data_table == 2 and data_table:FindRow("RowB").Z == 60.0)
            local targets = object.Targets
            targets:Empty()
            targets[1] = object
            object.CustomTargets = targets
            assert(#object.Targets == 1 and object.Targets[1] == object)
            object.Messages = { "Alpha", FString("Grüße ✓") }
            assert(object:GetHealth(40) == 42)
            local get_health = StaticFindObject("/Script/Test.Derived:GetHealth")
            assert(get_health:IsValid() and get_health:type() == "UFunction")
            assert(object:CallFunction(get_health, 40) == 42)
            local scaled = object:ScaleVector({ X = 2.0, Y = 3.0, Z = 4.0 }, 2.5)
            assert(type(scaled) == "table")
            assert(scaled.X == 5.0 and scaled.Y == 7.5 and scaled.Z == 10.0)
            local out_targets = {}
            object:GetTargets(out_targets)
            assert(type(out_targets.OutTargets) == "userdata" and #out_targets.OutTargets == 2)
            assert(out_targets.OutTargets[1]:GetName() == "/Script/Test" and out_targets.OutTargets[2] == object)
            assert(object:CountTargets({ object, object }) == 2)
            assert(object:CountTargets({}) == 0)
            local scores_snapshot = object.Scores
            assert(object:CountScores(scores_snapshot) == #scores_snapshot)
            local out_lookup = {}
            assert(object:GetLookup(out_lookup) == nil)
            assert(type(out_lookup.OutLookup) == "userdata" and out_lookup.OutLookup:type() == "TMap")
            assert(#out_lookup.OutLookup == 2 and out_lookup.OutLookup:Contains(7) and
                   out_lookup.OutLookup:Contains(9) and out_lookup.OutLookup:Find(9) == object)
            local out_value = {}
            local out_greeting = {}
            local no_return = object:GetOutputs(out_value, out_greeting)
            assert(no_return == nil)
            assert(out_value.OutValue == 77)
            assert(out_greeting.OutGreeting:ToString() == "Out ✓")
            scheduled_immediate = object.Health == 73 and object.Alive == false and
                                  object.Label:ToString() == "Greeting" and
                                  object.Greeting:ToString() == "Linux ✓" and
                                  object.Mode == Enum_Mode["TestMode::Enabled"] and
                                  object.Location.X == 7.5 and object.Location.Y == 8.5 and object.Location.Z == 9.5 and
                                  #object.Targets == 1 and object.Targets[1] == object and
                                  #object.Messages == 2 and object.Messages[1]:ToString() == "Alpha" and
                                  object.Messages[2]:ToString() == "Grüße ✓"
            ExecuteWithDelay(0, function()
                object.Health = 75
                local result = object:GetHealth(40)
                local succeeded = IsInAsyncThread() and result == 42
                ExecuteInGameThread(function()
                    async_unreal_access = succeeded
                    object.Health = 76
                end)
            end)
        end)
        ExecuteAsync(function()
            scheduled_execute_async = true
            async_thread_identity_ok = IsInAsyncThread() and
                                       GetCurrentThreadId() == GetAsyncThreadId() and
                                       not IsInMainModThread()
        end)
        ExecuteWithDelay(0, function()
            scheduled_delayed = IsInAsyncThread()
        end)
        LoopAsync(1, function()
            scheduled_loop_count = scheduled_loop_count + 1
            return scheduled_loop_count >= 2
        end)

        local cancelled = ExecuteInGameThreadWithDelay(1000, function()
            error("cancelled delayed action executed")
        end)
        assert(IsValidDelayedActionHandle(cancelled))
        assert(IsDelayedActionActive(cancelled))
        assert(GetDelayedActionRate(cancelled) == 1000)
        assert(PauseDelayedAction(cancelled))
        assert(IsDelayedActionPaused(cancelled))
        assert(GetDelayedActionTimeRemaining(cancelled) >= 0)
        assert(UnpauseDelayedAction(cancelled))
        assert(ResetDelayedActionTimer(cancelled))
        assert(SetDelayedActionTimer(cancelled, 1200))
        assert(GetDelayedActionRate(cancelled) == 1200)
        assert(CancelDelayedAction(cancelled))
        assert(not IsValidDelayedActionHandle(cancelled))
        assert(GetDelayedActionRate(cancelled) == -1)

        local controlled = ExecuteInGameThreadWithDelay(1000, function()
            controlled_delay_count = controlled_delay_count + 1
        end)
        assert(PauseDelayedAction(controlled))
        assert(SetDelayedActionTimer(controlled, 0))
        assert(IsDelayedActionActive(controlled))

        local explicit = MakeActionHandle()
        ExecuteInGameThreadWithDelay(explicit, 0, function()
            explicit_delay_count = explicit_delay_count + 1
        end)
        ExecuteInGameThreadWithDelay(explicit, 0, function()
            explicit_delay_count = explicit_delay_count + 100
        end)

        local retrigger = MakeActionHandle()
        RetriggerableExecuteInGameThreadWithDelay(retrigger, 1000, function()
            retrigger_count = retrigger_count + 1
        end)
        local retrigger_result = RetriggerableExecuteInGameThreadWithDelay(retrigger, 0, function()
            retrigger_count = retrigger_count + 100
        end)
        assert(retrigger_result == retrigger)

        frame_delay_handle = ExecuteInGameThreadAfterFrames(2, function()
            frame_delay_count = frame_delay_count + 1
        end)
        assert(GetDelayedActionRate(frame_delay_handle) == 2)
        frame_loop_handle = LoopInGameThreadAfterFrames(1, function()
            frame_loop_count = frame_loop_count + 1
            if frame_loop_count >= 2 then
                assert(CancelDelayedAction(frame_loop_handle))
            end
        end)

        time_loop_handle = LoopInGameThreadWithDelay(0, function()
            if explicit_delay_count > 0 then
                assert(CancelDelayedAction(time_loop_handle))
            end
        end)
    )lua");
    if (!scheduled_result.success)
    {
        std::fprintf(stderr, "scheduled Lua conformance failed: %s\n", scheduled_result.detail.c_str());
    }
    assert(scheduled_result.success);
    const auto tick = reinterpret_cast<ue4ss::linux::core::GameThreadScheduler::TickFunction>(tick_slot);
    // Async Lua needs two native game-thread round trips and schedules a final
    // Lua callback. Its marker property is only written by this thread, making
    // the completion check race-free and independent of CI scheduling speed.
    std::uint64_t scheduled_tick_count{};
    while (objects[5].health != 76 && scheduled_tick_count < 2000u)
    {
        tick(nullptr, 0.016f, false);
        ++scheduled_tick_count;
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    assert(objects[5].health == 76);
    for (std::uint64_t index = 0; index < 64u; ++index)
    {
        tick(nullptr, 0.016f, false);
        ++scheduled_tick_count;
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    const auto scheduled_assertions = scheduled_lua.execute_string(R"lua(
        assert(scheduled_immediate)
        assert(scheduled_delayed)
        assert(scheduled_execute_async)
        assert(async_thread_identity_ok)
        assert(async_unreal_access)
        assert(scheduled_loop_count == 2)
        assert(GetCurrentThreadId() == GetMainModThreadId())
        assert(GetGameThreadId() == GetCurrentThreadId())
        assert(IsInGameThread())
        assert(controlled_delay_count == 1)
        assert(explicit_delay_count == 1)
        assert(retrigger_count == 1)
        assert(frame_delay_count == 1)
        assert(frame_loop_count == 2)
        assert(not IsValidDelayedActionHandle(frame_delay_handle))
        assert(not IsValidDelayedActionHandle(frame_loop_handle))
        assert(not IsValidDelayedActionHandle(time_loop_handle))
        local clear_one = ExecuteInGameThreadWithDelay(1000, function() end)
        local clear_two = LoopInGameThreadAfterFrames(1000, function() end)
        assert(IsValidDelayedActionHandle(clear_one) and IsValidDelayedActionHandle(clear_two))
        assert(ClearAllDelayedActions() == 2)
        assert(not IsValidDelayedActionHandle(clear_one) and
               not IsValidDelayedActionHandle(clear_two))
    )lua");
    if (!scheduled_assertions.success)
    {
        std::fprintf(stderr,
                     "scheduled Lua result assertions failed: %s\n",
                     scheduled_assertions.detail.c_str());
    }
    assert(scheduled_assertions.success);
    assert(g_tick_calls == scheduled_tick_count);
    assert(objects[5].health == 76);
    assert(objects[5].alive == 0u);
    data_table_rows.clear();
    assert(runtime.enumerate_data_table_rows(
            data_table_handle, row_struct_handle, data_table_rows, detail));
    assert(data_table_rows.size() == 2u && data_table_rows[0].name_text == "RowA");
    assert(runtime.read_struct_property(
            data_table_rows[0].data, "X", property_value, detail));
    assert(property_value.floating_point == 11.5);
    assert(objects[5].label.comparison_index == 18u);
    assert(objects[5].greeting.data != nullptr && objects[5].greeting.num > 1);
    assert((objects[5].location == std::array<double, 3>{7.5, 8.5, 9.5}));
    assert(objects[5].targets.num == 1 && objects[5].targets.max == 1 && objects[5].targets.data != 0u);
    std::uint64_t stored_target{};
    std::memcpy(&stored_target, reinterpret_cast<const void*>(objects[5].targets.data), sizeof(stored_target));
    assert(stored_target == address(5));
    ue4ss::linux::core::UnrealPropertyValue empty_targets;
    empty_targets.kind = ue4ss::linux::core::UnrealPropertyKind::array;
    assert(runtime.write_hook_parameter(targets_parameter, empty_targets, detail));
    assert(objects[5].targets.data == 0u && objects[5].targets.num == 0 && objects[5].targets.max == 0);
    assert(runtime.read_property(derived.objects[0], "Messages", property_value, detail));
    assert(property_value.array_elements.size() == 2u);
    assert(property_value.array_elements[0]->text == "Alpha");
    assert(property_value.array_elements[1]->text == "Grüße ✓");
    ue4ss::linux::core::UnrealPropertyValue empty_messages;
    empty_messages.kind = ue4ss::linux::core::UnrealPropertyKind::array;
    assert(runtime.write_hook_parameter(messages_parameter, empty_messages, detail));
    assert(objects[5].messages.data == 0u && objects[5].messages.num == 0 && objects[5].messages.max == 0);
    assert(g_realloc_calls > 0u);
    assert(g_process_event_calls == 16u);
    scheduler.stop();

    const auto function_query = runtime.find_by_path("/Script/Test.Derived:GetHealth");
    assert(function_query.success && function_query.objects.size() == 1u);
    ue4ss::linux::core::UFunctionHookManager hooks;
    assert(hooks.start(runtime, detail));
    ue4ss::linux::core::HeadlessLuaState hook_lua;
    assert(hook_lua.install_readonly_unreal_bindings(runtime, nullptr, &hooks).success);
    assert(hook_lua.execute_string(R"lua(
        hook_pre_count = 0
        hook_post_count = 0
        hook_pre_id, hook_post_id = RegisterHook(
            "/Script/Test.Derived:GetHealth",
            function(Context, Value)
                assert(Context:type() == "RemoteUnrealParam")
                assert(Context:Get() == FindFirstOf("Base"))
                assert(Value:Get() == 41)
                Value:Set(42)
                escaped_hook_parameter = Value
                hook_pre_count = hook_pre_count + 1
            end,
            function(Context, ReturnValue, Value)
                assert(Context:get() == FindFirstOf("Base"))
                assert(ReturnValue:get() == 43)
                assert(Value:get() == 42)
                ReturnValue:set(44)
                hook_post_count = hook_post_count + 1
                return 45
            end)
    )lua").success);
    const auto hooked_native = reinterpret_cast<void (*)(void*, void*, void*)>(objects[8].native_function);
    std::array<std::byte, 0xa0> frame{};
    std::array<std::int32_t, 2> frame_locals{41, 0};
    const std::uint64_t frame_locals_address = reinterpret_cast<std::uintptr_t>(frame_locals.data());
    std::memcpy(frame.data() + 0x28u, &frame_locals_address, sizeof(frame_locals_address));
    std::int32_t hook_return{};
    hooked_native(&objects[5], frame.data(), &hook_return);
    assert(g_native_function_calls == 1u);
    assert(frame_locals[0] == 42);
    assert(hook_return == 45);
    assert(hook_lua.execute_string(R"lua(
        assert(hook_pre_count == 1 and hook_post_count == 1)
        local ok, message = pcall(function() return escaped_hook_parameter:get() end)
        assert(not ok and string.find(message, "no longer valid", 1, true) ~= nil)
        UnregisterHook("/Script/Test.Derived:GetHealth", hook_pre_id, hook_post_id)
    )lua").success);
    frame_locals[0] = 50;
    hook_return = 0;
    hooked_native(&objects[5], frame.data(), &hook_return);
    assert(g_native_function_calls == 2u);
    assert(frame_locals[0] == 50);
    assert(hook_return == 51);
    assert(hook_lua.execute_string(R"lua(
        assert(hook_pre_count == 1 and hook_post_count == 1)
    )lua").success);
    hooks.stop();
    assert(objects[8].native_function == reinterpret_cast<std::uintptr_t>(&fake_native_function));

    ue4ss::linux::core::BlueprintHookManager blueprint_hooks;
    assert(blueprint_hooks.start_for_testing(
            runtime,
            {
                    .process_internal = reinterpret_cast<std::uintptr_t>(&fake_native_function),
                    .process_local_script_function = reinterpret_cast<std::uintptr_t>(&fake_process_local_script_function),
                    .detail = "synthetic ProcessLocalScriptFunction",
            },
            detail));
    std::size_t blueprint_pre_count{};
    std::size_t blueprint_post_count{};
    const auto blueprint_ids = blueprint_hooks.register_hook(
            function_query.objects.front(),
            [&](const ue4ss::linux::core::UFunctionHookInvocation& invocation) {
                assert(invocation.context.address == address(5));
                assert(invocation.function.address == address(8));
                ++blueprint_pre_count;
            },
            [&](const ue4ss::linux::core::UFunctionHookInvocation& invocation) {
                assert(invocation.context.address == address(5));
                assert(invocation.function.address == address(8));
                ++blueprint_post_count;
            },
            detail);
    assert(blueprint_ids.first != 0u && blueprint_ids.second != 0u);
    auto custom_event_lua =
            std::make_unique<ue4ss::linux::core::HeadlessLuaState>();
    assert(custom_event_lua->install_readonly_unreal_bindings(
            runtime, nullptr, nullptr, nullptr, &blueprint_hooks).success);
    assert(custom_event_lua->execute_string(R"lua(
        custom_event_count = 0
        RegisterCustomEvent("gethealth", function(Context, Value)
            assert(Context:get() == FindFirstOf("Base"))
            assert(Value:get() == 61)
            Value:set(62)
            custom_event_count = custom_event_count + 1
            return 73
        end)
    )lua").success);
    std::array<std::byte, 0xa0> blueprint_frame{};
    const std::uint64_t current_native_function = address(8);
    std::memcpy(blueprint_frame.data() + 0x90u, &current_native_function, sizeof(current_native_function));
    std::array<std::int32_t, 2> blueprint_locals{61, 0};
    const std::uint64_t blueprint_locals_address =
            reinterpret_cast<std::uintptr_t>(blueprint_locals.data());
    std::memcpy(
            blueprint_frame.data() + 0x28u,
            &blueprint_locals_address,
            sizeof(blueprint_locals_address));
    std::int32_t blueprint_result{};
    using ScriptFunction = void (*)(void*, void*, void*);
    volatile ScriptFunction process_local = &fake_process_local_script_function;
    process_local(&objects[5], blueprint_frame.data(), &blueprint_result);
    assert(g_process_local_script_calls == 1u);
    assert(blueprint_pre_count == 1u && blueprint_post_count == 1u);
    assert(blueprint_locals[0] == 62);
    assert(blueprint_result == 73);
    assert(custom_event_lua->execute_string(R"lua(
        assert(custom_event_count == 1)
        UnregisterCustomEvent("GETHEALTH")
        UnregisterCustomEvent("GETHEALTH")
    )lua").success);
    assert(blueprint_hooks.unregister_hook(
            function_query.objects.front(), blueprint_ids.first, blueprint_ids.second, detail));
    blueprint_locals[0] = 80;
    blueprint_result = 0;
    process_local(&objects[5], blueprint_frame.data(), &blueprint_result);
    assert(g_process_local_script_calls == 2u);
    assert(blueprint_pre_count == 1u && blueprint_post_count == 1u);
    assert(blueprint_locals[0] == 80 && blueprint_result == 0);
    custom_event_lua.reset();

    // Deterministically hold a detour between its pre-callback and trampoline
    // call while another thread stops the manager. The trampoline must remain
    // executable until this invocation returns.
    std::atomic<bool> blueprint_block_entered{};
    std::atomic<bool> blueprint_block_release{};
    std::atomic<bool> blueprint_stop_returned{};
    const auto blocking_blueprint_ids = blueprint_hooks.register_hook(
            function_query.objects.front(),
            [&](const ue4ss::linux::core::UFunctionHookInvocation&) {
                blueprint_block_entered.store(true, std::memory_order_release);
                while (!blueprint_block_release.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
            },
            {},
            detail);
    assert(blocking_blueprint_ids.first != 0u && blocking_blueprint_ids.second == 0u);
    std::thread blueprint_invocation{[&] {
        process_local(&objects[5], blueprint_frame.data(), nullptr);
    }};
    while (!blueprint_block_entered.load(std::memory_order_acquire))
    {
        std::this_thread::yield();
    }
    std::thread blueprint_stop{[&] {
        blueprint_hooks.stop();
        blueprint_stop_returned.store(true, std::memory_order_release);
    }};
    std::this_thread::sleep_for(std::chrono::milliseconds{2});
    assert(!blueprint_stop_returned.load(std::memory_order_acquire));
    blueprint_block_release.store(true, std::memory_order_release);
    blueprint_invocation.join();
    blueprint_stop.join();
    assert(blueprint_stop_returned.load(std::memory_order_acquire));
    assert(g_process_local_script_calls == 3u);
    process_local(&objects[5], blueprint_frame.data(), nullptr);
    assert(g_process_local_script_calls == 4u);

    void* notification_tick_slot = reinterpret_cast<void*>(&fake_game_engine_tick);
    ue4ss::linux::core::GameThreadScheduler notification_scheduler;
    assert(notification_scheduler.start_on_slot(
            &notification_tick_slot, &fake_game_engine_tick, detail));
    ue4ss::linux::core::ObjectNotificationManager notifications;
    assert(notifications.start(
            runtime,
            notification_scheduler,
            reinterpret_cast<std::uintptr_t>(&fake_static_construct_object),
            detail));
    ue4ss::linux::core::HeadlessLuaState notification_lua;
    assert(notification_lua.install_readonly_unreal_bindings(
            runtime, &notification_scheduler, nullptr, &notifications).success);
    assert(notification_lua.execute_string(R"lua(
        new_object_count = 0
        NotifyOnNewObject("/Script/Test.Derived", function(Object)
            assert(Object == FindFirstOf("Base"))
            new_object_count = new_object_count + 1
            return true
        end)
    )lua").success);
    volatile auto construct = &fake_static_construct_object;
    assert(construct(&objects[5]) == &objects[5]);
    const auto notification_tick = reinterpret_cast<ue4ss::linux::core::GameThreadScheduler::TickFunction>(
            notification_tick_slot);
    notification_tick(nullptr, 0.016f, false);
    assert(g_allocate_uobject_calls == 1u);
    assert(notification_lua.execute_string("assert(new_object_count == 1)").success);
    assert(construct(&objects[5]) == &objects[5]);
    notification_tick(nullptr, 0.016f, false);
    assert(g_allocate_uobject_calls == 2u);
    assert(notification_lua.execute_string("assert(new_object_count == 1)").success);

    g_static_construct_result = &objects[5];
    g_capture_static_construct_parameters = true;
    assert(notification_lua.execute_string(R"lua(
        assert(StaticConstructObjectAvailable)
        local unreal_class = StaticFindObject("/Script/Test.Derived")
        local outer = StaticFindObject("/Script/Test")
        local created = StaticConstructObject(
            unreal_class,
            outer,
            FName("Instance"),
            EObjectFlags.RF_Transient,
            EInternalObjectFlags.GarbageCollectionKeepFlags,
            true,
            false,
            nil,
            0,
            0,
            0)
        assert(created:IsValid())
        assert(created == FindFirstOf("Base"))
    )lua").success);
    assert(g_allocate_uobject_calls == 3u);
    assert(g_static_construct_parameters.unreal_class == address(4));
    assert(g_static_construct_parameters.outer == address(2));
    assert(g_static_construct_parameters.name.comparison_index == 6u);
    assert(g_static_construct_parameters.name.number == 0u);
    assert(g_static_construct_parameters.object_flags == 0x40u);
    assert(g_static_construct_parameters.internal_object_flags == 0x0e000000u);
    assert(g_static_construct_parameters.copy_transients_from_class_defaults == 1u);
    assert(g_static_construct_parameters.assume_template_is_archetype == 0u);
    assert(g_static_construct_parameters.object_template == 0u);
    assert(g_static_construct_parameters.instance_graph == 0u);
    assert(g_static_construct_parameters.external_package == 0u);
    assert(std::all_of(g_static_construct_parameters.property_init_callback.begin(),
                       g_static_construct_parameters.property_init_callback.end(),
                       [](std::byte value) { return value == std::byte{}; }));
    assert(g_static_construct_parameters.subobject_overrides == 0u);
    g_capture_static_construct_parameters = false;
    g_static_construct_result = nullptr;
    notifications.stop();
    notification_scheduler.stop();
#endif

    auto wrong_version = resolved;
    wrong_version.engine_minor = 2;
    assert(!ue4ss::linux::core::validate_fname_runtime_abi(wrong_version).success);

    auto missing_allocator = resolved;
    missing_allocator.gmalloc = 0u;
    assert(!ue4ss::linux::core::validate_fname_runtime_abi(missing_allocator).success);
    assert(g_free_calls > 1u);
    const auto release_fake_set = [&allocator](FakeScriptSet& set) {
        fake_free(&allocator, reinterpret_cast<void*>(set.elements.data));
        if (set.allocation_flags_secondary != 0u)
        {
            fake_free(&allocator, reinterpret_cast<void*>(set.allocation_flags_secondary));
        }
        if (set.hash_secondary != 0u)
        {
            fake_free(&allocator, reinterpret_cast<void*>(set.hash_secondary));
        }
        set = {};
    };
    release_fake_set(objects[5].scores);
    release_fake_set(objects[5].lookup);
    fake_free(&allocator, reinterpret_cast<void*>(objects[5].handlers.data));
    objects[5].handlers = {};
    fake_free(&allocator, objects[5].soft_target.sub_path_string.data);
    objects[5].soft_target = {};
    fake_free(&allocator, objects[5].greeting.data);
    objects[5].greeting = {};
    return 0;
}
