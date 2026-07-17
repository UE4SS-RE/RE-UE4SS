#pragma once

#include "pattern_resolver.hpp"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ue4ss::linux::core
{
    class GameThreadScheduler;

    struct RawFName
    {
        std::uint32_t comparison_index{};
        std::uint32_t number{};
    };

    struct UnrealStringHeader
    {
        char16_t* data{};
        std::int32_t num{};
        std::int32_t max{};
    };

    static_assert(sizeof(RawFName) == 0x8);
    static_assert(sizeof(UnrealStringHeader) == 0x10);

    struct ReadOnlyUObjectHandle
    {
        std::uint64_t address{};
        std::int32_t internal_index{-1};
        std::int32_t serial_number{};
    };

    struct ReadOnlyUObjectDescription
    {
        ReadOnlyUObjectHandle handle;
        std::uint32_t object_flags{};
        std::int32_t internal_object_flags{};
        std::uint64_t class_address{};
        std::uint64_t outer_address{};
        RawFName name_private{};
        std::string name;
        std::string class_name;
        std::string path_name;
        std::string full_name;
    };

    struct ObjectQueryResult
    {
        bool success{};
        std::int32_t scanned_objects{};
        bool truncated{};
        std::vector<ReadOnlyUObjectHandle> objects;
        std::string detail;
    };

    struct UnrealObjectConstructionRequest
    {
        ReadOnlyUObjectHandle unreal_class;
        ReadOnlyUObjectHandle outer;
        RawFName name;
        std::uint32_t object_flags{};
        std::uint32_t internal_object_flags{};
        bool copy_transients_from_class_defaults{};
        bool assume_template_is_archetype{};
        std::optional<ReadOnlyUObjectHandle> object_template;
        std::optional<ReadOnlyUObjectHandle> external_package;
    };

    struct UnrealFURLSnapshot
    {
        std::string protocol;
        std::string host;
        std::int32_t port{};
        std::int32_t valid{};
        std::string map;
        std::string redirect_url;
        std::vector<std::string> options;
        std::string portal;
    };

    struct UnrealEnumEntry
    {
        RawFName name;
        std::string text;
        std::int64_t value{};
    };

    struct ReflectedPropertyDescription
    {
        std::uint64_t address{};
        std::uint64_t owner_address{};
        RawFName name_private{};
        std::string name;
        std::string type_name;
        std::int32_t array_dim{1};
        std::int32_t element_size{};
        std::int32_t offset{};
        std::uint64_t flags{};
        std::uint8_t bool_field_size{};
        std::uint8_t bool_byte_offset{};
        std::uint8_t bool_byte_mask{};
        std::uint8_t bool_field_mask{};
        std::uint64_t property_class_address{};
        std::uint64_t struct_address{};
        std::uint64_t inner_property_address{};
        std::uint64_t key_property_address{};
        std::uint64_t value_property_address{};
        std::uint64_t enum_address{};
        std::uint64_t enum_underlying_property_address{};
        std::uint64_t interface_class_address{};
    };

    // A non-owning view over live UScriptStruct storage. The script-struct
    // metadata remains registry-validated; the storage belongs to Unreal and
    // is never retained or released by the Linux core.
    struct UnrealStructBinding
    {
        ReadOnlyUObjectHandle script_struct;
        ReadOnlyUObjectHandle owner;
        std::uint64_t storage_address{};
        std::uint64_t property_address{};
    };

    struct UnrealDataTableRow
    {
        RawFName name;
        std::string name_text;
        UnrealStructBinding data;
    };

    struct CustomPropertyTypeDescription
    {
        std::string type_name;
        std::int32_t element_size{};
    };

    struct CustomPropertyDescription
    {
        std::string name;
        ReadOnlyUObjectHandle owner_class;
        std::int32_t offset{};
        CustomPropertyTypeDescription type;
        std::optional<CustomPropertyTypeDescription> array_inner;
    };

    enum class UnrealPropertyKind : std::uint8_t
    {
        boolean,
        signed_integer,
        unsigned_integer,
        floating_point,
        name,
        string,
        text,
        object,
        structure,
        array,
        set,
        map,
        weak_object,
        soft_object,
        delegate,
        multicast_delegate,
    };

    struct UnrealPropertyValue;

    struct UnrealStructFieldValue
    {
        std::string name;
        std::shared_ptr<UnrealPropertyValue> value;
    };

    struct UnrealMapEntryValue
    {
        std::shared_ptr<UnrealPropertyValue> key;
        std::shared_ptr<UnrealPropertyValue> value;
    };

    // Values are copied into core-owned storage. An object value either has a
    // validated GUObjectArray handle or is explicitly null; raw engine-owned
    // strings and pointers never escape this boundary.
    struct UnrealPropertyValue
    {
        UnrealPropertyKind kind{UnrealPropertyKind::signed_integer};
        bool boolean{};
        std::int64_t signed_integer{};
        std::uint64_t unsigned_integer{};
        double floating_point{};
        RawFName name{};
        std::string text;
        ReadOnlyUObjectHandle object;
        bool object_is_null{};
        std::string struct_type;
        std::vector<UnrealStructFieldValue> struct_fields;
        std::vector<std::shared_ptr<UnrealPropertyValue>> array_elements;
        std::vector<std::shared_ptr<UnrealPropertyValue>> set_elements;
        std::vector<UnrealMapEntryValue> map_entries;
        // UE 5.1 stores FSoftObjectPath as FTopLevelAssetPath (package and
        // asset FNames) plus the optional FString subpath. Keep both engine
        // names so a value can round-trip without reconstructing identity.
        RawFName soft_package_name{};
        RawFName soft_asset_name{};
        std::string soft_package_text;
        std::string soft_asset_text;
        // `name`/`text` retain the deprecated combined GetAssetPathName view;
        // this field owns the UTF-8 subpath copy.
        std::string soft_sub_path;
        // Enum metadata is copied as a validated UObject handle and a property
        // name. Lua can materialize the upstream-compatible
        // Enum_<PropertyName> table without exposing a raw engine pointer.
        ReadOnlyUObjectHandle enum_object;
        std::string enum_property_name;
        // Sparse multicast delegates share the logical binding shape with
        // inline delegates but require a different engine-owned storage and
        // mutation ABI. Lua uses this bit to expose the matching wrapper.
        bool multicast_sparse{};
    };

    struct UnrealFunctionArgument
    {
        UnrealPropertyValue value;
        bool out_placeholder{};
    };

    struct UnrealFunctionOutValue
    {
        std::size_t argument_index{};
        std::string name;
        UnrealPropertyValue value;
    };

    // Describes one scalar value living in the active FFrame/RESULT_DECL of a
    // UFunction hook. The descriptor deliberately contains no owning Unreal
    // pointer or C++ object: it is valid only while the synchronous hook
    // callback is executing.
    struct RemoteUnrealParameter
    {
        UnrealPropertyKind kind{UnrealPropertyKind::signed_integer};
        std::uint64_t property_address{};
        std::uint64_t storage_address{};
        std::int32_t element_size{};
        std::uint8_t boolean_byte_offset{};
        std::uint8_t boolean_byte_mask{};
        std::uint8_t boolean_field_mask{};
        bool is_return{};
        bool is_out{};
        std::uint64_t struct_address{};
        std::uint64_t inner_property_address{};
        std::uint64_t key_property_address{};
        std::uint64_t value_property_address{};
        std::uint64_t interface_class_address{};
    };

    // A deliberately small bridge for read-only calls into the running
    // engine. Unreal owns every buffer returned by FName::ToString, so this
    // class copies it to the core runtime and releases it through the same
    // FMalloc instance. No engine allocation crosses the API boundary.
    class ReadOnlyUnrealRuntime
    {
      public:
        [[nodiscard]] bool initialize(const ResolvedUnrealState& resolved, std::string& detail) noexcept;
        [[nodiscard]] bool is_ready() const noexcept;
        [[nodiscard]] bool initialize_process_event(std::string& detail) noexcept;
#if defined(UE4SS_LINUX_TESTING)
        void set_process_event_for_testing(void* address) noexcept;
#endif
        [[nodiscard]] bool process_event_available() const noexcept;
        [[nodiscard]] void* process_event_address() const noexcept;
        [[nodiscard]] bool initialize_fproperty_abi(std::string& detail) noexcept;
        [[nodiscard]] bool fproperty_abi_available() const noexcept;
        [[nodiscard]] bool fproperty_import_text_available() const noexcept;
        [[nodiscard]] bool import_property_text(
                std::uint64_t property_address,
                std::string_view text,
                std::uint64_t value_address,
                std::int32_t port_flags,
                const ReadOnlyUObjectHandle& owner,
                const GameThreadScheduler& scheduler,
                std::string& error) const noexcept;
        [[nodiscard]] bool initialize_uenum_abi(std::string& detail) noexcept;
        [[nodiscard]] bool uenum_abi_available() const noexcept;
        [[nodiscard]] bool reflected_value_hash(std::uint64_t property_address,
                                                std::uint64_t value_address,
                                                std::uint32_t& output,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool reflected_values_identical(std::uint64_t property_address,
                                                      std::uint64_t left_address,
                                                      std::uint64_t right_address,
                                                      bool& output,
                                                      std::string& error) const noexcept;
        [[nodiscard]] bool reflected_value_alignment(std::uint64_t property_address,
                                                      std::uint32_t& output,
                                                      std::string& error) const noexcept;
        [[nodiscard]] bool initialize_reflected_value(std::uint64_t property_address,
                                                      std::uint64_t value_address,
                                                      std::string& error) const noexcept;
        [[nodiscard]] bool copy_reflected_value(std::uint64_t property_address,
                                                std::uint64_t destination_address,
                                                std::uint64_t source_address,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool destroy_reflected_value(std::uint64_t property_address,
                                                   std::uint64_t value_address,
                                                   std::string& error) const noexcept;
        [[nodiscard]] bool fname_to_utf8(RawFName name, std::string& output, std::string& error) const noexcept;
        [[nodiscard]] bool fname_from_utf8(std::string_view input,
                                           RawFName& output,
                                           bool add_if_missing,
                                           std::string& error) const noexcept;
        [[nodiscard]] bool is_valid(const ReadOnlyUObjectHandle& handle) const noexcept;
        [[nodiscard]] bool handle_from_address(std::uint64_t address, ReadOnlyUObjectHandle& output) const noexcept;
        [[nodiscard]] bool handle_from_weak(std::int32_t internal_index,
                                            std::int32_t serial_number,
                                            ReadOnlyUObjectHandle& output) const noexcept;
        [[nodiscard]] bool snapshot_native_tchar_string(std::uint64_t address,
                                                        std::string& output,
                                                        std::string& error) const noexcept;
        [[nodiscard]] bool snapshot_native_fstring(std::uint64_t address,
                                                   std::string& output,
                                                   std::string& error) const noexcept;
        [[nodiscard]] bool snapshot_native_furl(std::uint64_t address,
                                                UnrealFURLSnapshot& output,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool world_from_native_context(std::uint64_t address,
                                                     ReadOnlyUObjectHandle& output,
                                                     std::string& error) const noexcept;
        [[nodiscard]] bool is_a(const ReadOnlyUObjectHandle& handle,
                                std::string_view short_class_name,
                                std::string& error) const noexcept;
        [[nodiscard]] bool is_a(const ReadOnlyUObjectHandle& handle,
                                const ReadOnlyUObjectHandle& requested_class,
                                std::string& error) const noexcept;
        [[nodiscard]] bool implements_interface(const ReadOnlyUObjectHandle& handle,
                                                const ReadOnlyUObjectHandle& interface_class,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool describe_object(const ReadOnlyUObjectHandle& handle,
                                           ReadOnlyUObjectDescription& output,
                                           std::string& error) const noexcept;
        [[nodiscard]] bool find_outer_of_class(const ReadOnlyUObjectHandle& handle,
                                               std::string_view short_class_name,
                                               bool include_self,
                                               ReadOnlyUObjectHandle& output,
                                               std::string& error) const noexcept;
        [[nodiscard]] bool get_class_default_object(const ReadOnlyUObjectHandle& unreal_class,
                                                    ReadOnlyUObjectHandle& output,
                                                    std::string& error) const noexcept;
        [[nodiscard]] ObjectQueryResult find_objects(std::size_t limit,
                                                     std::optional<std::string_view> short_class_name,
                                                     std::optional<std::string_view> object_short_name,
                                                     std::uint32_t required_flags,
                                                     std::uint32_t banned_flags,
                                                     bool exact_class) const noexcept;
        [[nodiscard]] ObjectQueryResult find_all_of(std::string_view short_class_name,
                                                    std::size_t limit = 65536u) const noexcept;
        [[nodiscard]] ObjectQueryResult find_by_path(std::string_view path_name,
                                                     std::size_t limit = 1u) const noexcept;
        [[nodiscard]] bool for_each_uobject(
                const std::function<bool(const ReadOnlyUObjectHandle&,
                                         std::int32_t chunk_index,
                                         std::int32_t object_index)>& callback,
                std::int32_t& visited_objects,
                std::string& error) const noexcept;
        [[nodiscard]] bool static_construct_object_available() const noexcept;
        [[nodiscard]] bool construct_object(const UnrealObjectConstructionRequest& request,
                                            const GameThreadScheduler& scheduler,
                                            ReadOnlyUObjectHandle& output,
                                            std::string& error) const noexcept;
        [[nodiscard]] bool enumerate_enum_names(const ReadOnlyUObjectHandle& unreal_enum,
                                                std::vector<UnrealEnumEntry>& output,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool edit_enum_name(const ReadOnlyUObjectHandle& unreal_enum,
                                          std::int32_t index,
                                          RawFName new_name,
                                          const GameThreadScheduler& scheduler,
                                          std::string& error) const noexcept;
        [[nodiscard]] bool edit_enum_value(const ReadOnlyUObjectHandle& unreal_enum,
                                           std::int32_t index,
                                           std::int64_t new_value,
                                           const GameThreadScheduler& scheduler,
                                           std::string& error) const noexcept;
        [[nodiscard]] bool insert_enum_name(const ReadOnlyUObjectHandle& unreal_enum,
                                            RawFName name,
                                            std::int64_t value,
                                            std::int32_t index,
                                            bool shift_values,
                                            const GameThreadScheduler& scheduler,
                                            std::int32_t& inserted_index,
                                            std::string& error) const noexcept;
        [[nodiscard]] bool remove_enum_names(const ReadOnlyUObjectHandle& unreal_enum,
                                             std::int32_t index,
                                             std::int32_t count,
                                             bool allow_shrinking,
                                             const GameThreadScheduler& scheduler,
                                             std::string& error) const noexcept;
        [[nodiscard]] bool enumerate_properties(const ReadOnlyUObjectHandle& unreal_struct,
                                                std::vector<ReflectedPropertyDescription>& output,
                                                std::string& error,
                                                bool include_super = true) const noexcept;
        [[nodiscard]] bool describe_property(std::uint64_t address,
                                             ReflectedPropertyDescription& output,
                                             std::string& error) const noexcept;
        [[nodiscard]] bool container_ptr_to_value_ptr(
                const ReflectedPropertyDescription& property,
                const ReadOnlyUObjectHandle& container,
                std::int32_t array_index,
                std::uint64_t& output,
                std::string& error) const noexcept;
        [[nodiscard]] bool bind_struct_property(const ReadOnlyUObjectHandle& owner,
                                                std::string_view property_name,
                                                UnrealStructBinding& output,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool bind_struct_storage(const ReadOnlyUObjectHandle& script_struct,
                                               std::uint64_t storage_address,
                                               std::uint64_t property_address,
                                               UnrealStructBinding& output,
                                               std::string& error) const noexcept;
        [[nodiscard]] bool bind_nested_struct_property(
                const UnrealStructBinding& parent,
                std::string_view property_name,
                UnrealStructBinding& output,
                std::string& error) const noexcept;
        [[nodiscard]] bool validate_struct_binding(const UnrealStructBinding& binding,
                                                   std::string& error) const noexcept;
        [[nodiscard]] bool read_struct_value(const UnrealStructBinding& binding,
                                             UnrealPropertyValue& output,
                                             std::string& error) const noexcept;
        [[nodiscard]] bool read_struct_property(const UnrealStructBinding& binding,
                                                std::string_view property_name,
                                                UnrealPropertyValue& output,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool write_struct_property(const UnrealStructBinding& binding,
                                                 std::string_view property_name,
                                                 const UnrealPropertyValue& value,
                                                 const GameThreadScheduler& scheduler,
                                                 std::string& error) const noexcept;
        [[nodiscard]] bool enumerate_data_table_rows(
                const ReadOnlyUObjectHandle& data_table,
                ReadOnlyUObjectHandle& row_struct,
                std::vector<UnrealDataTableRow>& rows,
                std::string& error) const noexcept;
        [[nodiscard]] bool add_data_table_row(
                const ReadOnlyUObjectHandle& data_table,
                RawFName row_name,
                const UnrealPropertyValue& row_value,
                const GameThreadScheduler& scheduler,
                std::string& error) const noexcept;
        [[nodiscard]] bool remove_data_table_row(
                const ReadOnlyUObjectHandle& data_table,
                RawFName row_name,
                const GameThreadScheduler& scheduler,
                std::string& error) const noexcept;
        [[nodiscard]] bool empty_data_table(
                const ReadOnlyUObjectHandle& data_table,
                const GameThreadScheduler& scheduler,
                std::string& error) const noexcept;
        [[nodiscard]] bool get_super_struct(const ReadOnlyUObjectHandle& unreal_struct,
                                            ReadOnlyUObjectHandle& output,
                                            std::string& error) const noexcept;
        [[nodiscard]] bool struct_is_child_of(const ReadOnlyUObjectHandle& unreal_struct,
                                              const ReadOnlyUObjectHandle& requested_parent,
                                              bool& output,
                                              std::string& error) const noexcept;
        [[nodiscard]] bool enumerate_functions(const ReadOnlyUObjectHandle& unreal_struct,
                                               std::vector<ReadOnlyUObjectHandle>& output,
                                               std::string& error) const noexcept;
        [[nodiscard]] bool get_function_flags(const ReadOnlyUObjectHandle& function,
                                              std::uint32_t& output,
                                              std::string& error) const noexcept;
        [[nodiscard]] bool set_function_flags(const ReadOnlyUObjectHandle& function,
                                              std::uint32_t value,
                                              std::string& error) const noexcept;
        [[nodiscard]] bool find_game_engine_tick_slot(std::uint64_t expected_tick_address,
                                                      void**& output,
                                                      std::string& error) const noexcept;
        [[nodiscard]] bool read_property(const ReadOnlyUObjectHandle& handle,
                                         std::string_view property_name,
                                         UnrealPropertyValue& output,
                                         std::string& error) const noexcept;
        [[nodiscard]] bool write_property(const ReadOnlyUObjectHandle& handle,
                                          std::string_view property_name,
                                          const UnrealPropertyValue& value,
                                          const GameThreadScheduler& scheduler,
                                          std::string& error) const noexcept;
        [[nodiscard]] bool register_custom_property(
                CustomPropertyDescription property,
                std::string& error) noexcept;
        [[nodiscard]] bool enumerate_custom_properties(
                const ReadOnlyUObjectHandle& handle,
                std::vector<ReflectedPropertyDescription>& output,
                std::string& error) const noexcept;
        [[nodiscard]] bool read_registered_custom_property(
                const ReadOnlyUObjectHandle& handle,
                std::string_view property_name,
                UnrealPropertyValue& output,
                bool& found,
                std::string& error) const noexcept;
        [[nodiscard]] bool write_registered_custom_property(
                const ReadOnlyUObjectHandle& handle,
                std::string_view property_name,
                const UnrealPropertyValue& value,
                const GameThreadScheduler& scheduler,
                bool& found,
                std::string& error) const noexcept;
        [[nodiscard]] bool validate_custom_property(
                const CustomPropertyDescription& property,
                std::string& error) const noexcept;
        [[nodiscard]] bool custom_property_owner_matches(
                const ReadOnlyUObjectHandle& handle,
                const ReadOnlyUObjectHandle& owner_class) const noexcept;
        [[nodiscard]] bool read_custom_property(
                const ReadOnlyUObjectHandle& handle,
                const CustomPropertyDescription& property,
                UnrealPropertyValue& output,
                std::string& error) const noexcept;
        [[nodiscard]] bool write_custom_property(
                const ReadOnlyUObjectHandle& handle,
                const CustomPropertyDescription& property,
                const UnrealPropertyValue& value,
                const GameThreadScheduler& scheduler,
                std::string& error) const noexcept;
        [[nodiscard]] bool add_multicast_delegate_binding(const ReadOnlyUObjectHandle& owner,
                                                          std::string_view property_name,
                                                          const ReadOnlyUObjectHandle& target,
                                                          RawFName function_name,
                                                          const GameThreadScheduler& scheduler,
                                                          std::string& error) const noexcept;
        [[nodiscard]] bool remove_multicast_delegate_binding(const ReadOnlyUObjectHandle& owner,
                                                             std::string_view property_name,
                                                             const ReadOnlyUObjectHandle& target,
                                                             RawFName function_name,
                                                             const GameThreadScheduler& scheduler,
                                                             std::string& error) const noexcept;
        [[nodiscard]] bool clear_multicast_delegate_bindings(const ReadOnlyUObjectHandle& owner,
                                                             std::string_view property_name,
                                                             const GameThreadScheduler& scheduler,
                                                             std::string& error) const noexcept;
        [[nodiscard]] bool broadcast_multicast_delegate(
                const ReadOnlyUObjectHandle& owner,
                std::string_view property_name,
                const std::vector<UnrealFunctionArgument>& arguments,
                const GameThreadScheduler& scheduler,
                std::string& error) const noexcept;
        [[nodiscard]] std::string process_event_vtable_probe() const noexcept;
        [[nodiscard]] bool find_function(const ReadOnlyUObjectHandle& context,
                                         std::string_view function_name,
                                         ReadOnlyUObjectHandle& output,
                                         std::string& error) const noexcept;
        [[nodiscard]] bool function_native_address(const ReadOnlyUObjectHandle& function,
                                                   std::uint64_t& output,
                                                   std::string& error) const noexcept;
        [[nodiscard]] bool describe_hook_parameters(const ReadOnlyUObjectHandle& function,
                                                    std::uint64_t stack_address,
                                                    std::uint64_t result_address,
                                                    bool include_return,
                                                    std::vector<RemoteUnrealParameter>& output,
                                                    std::string& error) const noexcept;
        [[nodiscard]] bool read_hook_parameter(const RemoteUnrealParameter& parameter,
                                               UnrealPropertyValue& output,
                                               std::string& error) const noexcept;
        [[nodiscard]] bool write_hook_parameter(const RemoteUnrealParameter& parameter,
                                                const UnrealPropertyValue& value,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool invoke_function(const ReadOnlyUObjectHandle& context,
                                           const ReadOnlyUObjectHandle& function,
                                           const std::vector<UnrealFunctionArgument>& arguments,
                                           std::optional<UnrealPropertyValue>& return_value,
                                           std::vector<UnrealFunctionOutValue>& out_values,
                                           const GameThreadScheduler& scheduler,
                                           std::string& error) const noexcept;

        // Internal container bridge. Allocation and release always use the
        // resolved engine FMalloc instance so no ownership crosses runtimes.
        [[nodiscard]] bool allocate_unreal_storage(std::size_t bytes,
                                                   std::uint32_t alignment,
                                                   void*& output,
                                                   std::string& error) const noexcept;
        void release_unreal_storage(void* allocation) const noexcept;
        [[nodiscard]] bool read_unreal_string(std::uint64_t address,
                                              std::string& output,
                                              std::string& error) const noexcept;
        [[nodiscard]] bool assign_unreal_string(std::uint64_t address,
                                                std::string_view input,
                                                std::string& error) const noexcept;
        [[nodiscard]] bool assign_unreal_text(std::uint64_t property_address,
                                              std::uint64_t address,
                                              std::string_view input,
                                              std::string& error) const noexcept;
        void release_unreal_string(UnrealStringHeader& string) const noexcept;

      private:
        using FNameToStringFunction = void (*)(const RawFName*, UnrealStringHeader*);
        using FNameConstructorFunction = void (*)(RawFName*, const char16_t*, std::int32_t);
        using ReallocFunction = void* (*)(void*, void*, std::size_t, std::uint32_t);
        using FreeFunction = void (*)(void*, void*);
        using StaticConstructObjectFunction = void* (*)(const void*);

        [[nodiscard]] bool allocate_unreal_string(std::string_view input,
                                                  UnrealStringHeader& output,
                                                  std::string& error) const noexcept;
        [[nodiscard]] bool read_enum_names_at_offset(const ReadOnlyUObjectHandle& unreal_enum,
                                                     std::uint64_t names_offset,
                                                     std::vector<UnrealEnumEntry>& output,
                                                     std::string& error) const noexcept;

        FNameToStringFunction m_fname_to_string{};
        FNameConstructorFunction m_fname_constructor{};
        ReallocFunction m_realloc{};
        FreeFunction m_free{};
        void* m_allocator{};
        std::uint64_t m_guobject_array{};
        StaticConstructObjectFunction m_static_construct_object{};
        using ProcessEventFunction = void (*)(void*, void*, void*);
        ProcessEventFunction m_process_event{};
        bool m_fproperty_abi_ready{};
        bool m_fproperty_import_text_ready{};
        std::uint64_t m_uenum_names_offset{};
        mutable std::mutex m_custom_properties_mutex;
        std::vector<CustomPropertyDescription> m_custom_properties;
    };

    struct FNameRuntimeValidation
    {
        bool success{};
        std::string detail;
    };

    // Performs one bounded FName(0, 0) -> "None" call. This is intentionally
    // separate from the resolver scan: finding an address does not prove the
    // SysV signature or the FMalloc vtable layout.
    [[nodiscard]] FNameRuntimeValidation validate_fname_runtime_abi(const ResolvedUnrealState& resolved) noexcept;

    struct ReadOnlyObjectApiValidation
    {
        bool success{};
        std::string detail;
    };

    // Confirms the exact UE 5.1 object-array traversal and outer/class
    // layouts without invoking constructors or mutating engine state.
    [[nodiscard]] ReadOnlyObjectApiValidation validate_readonly_object_api(ReadOnlyUnrealRuntime& runtime) noexcept;
    [[nodiscard]] ReadOnlyObjectApiValidation validate_readonly_object_api(const ResolvedUnrealState& resolved) noexcept;
} // namespace ue4ss::linux::core
