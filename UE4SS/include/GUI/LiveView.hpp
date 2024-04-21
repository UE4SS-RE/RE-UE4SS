#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UnrealFlags.hpp>

namespace RC::Unreal
{
    struct FUObjectItem;
    class UObject;
    class UStruct;
    class UClass;
    class FProperty;
} // namespace RC::Unreal

namespace RC::GUI
{
    using namespace RC::Unreal;

    class UFunctionCallerWidget;

    class LiveView
    {
      public:
        using ObjectIteratorCallable = void (LiveView::*)(int32_t, int32_t, const std::function<void(UObject*)>&);

        struct WatchIdentifier
        {
            void* container{};
            void* property{};

            auto operator==(const WatchIdentifier& rhs) const -> bool
            {
                return container == rhs.container && property == rhs.property;
            }
        };

        struct Watch
        {
            enum class AcquisitionMethod
            {
                StaticFindObject,
                FindFirstOf,
            };

            enum class Type
            {
                Property,
                Function,
            };

            static std::mutex s_watch_lock;

            Output::Targets<Output::FileDevice> output{};
            FProperty* property{};
            UObject* container{};
            StringType object_name{};
            StringType property_name{};
            StringType property_value{};
            size_t hash{};
            std::string history{};
            float history_previous_max_scroll_y{};
            AcquisitionMethod acquisition_method{};
            bool write_to_file{};
            bool show_history{};
            bool load_on_startup{};
            bool was_stop_load_on_startup_requested{};
            bool enabled{};
            bool function_is_hooked{};
            std::pair<int, int> function_hook_ids{};

            Watch() = delete;
            Watch(StringType&& object_name, StringType&& property_name);
        };

      private:
        std::string_view m_default_search_buffer{"Search by type, path, and name..."};
        constexpr static size_t m_search_buffer_capacity = 2000;
        char* m_search_by_name_buffer{};
        ObjectIteratorCallable m_object_iterator{&LiveView::guobjectarray_iterator};
        std::unordered_set<UObject*> m_opened_tree_nodes{};
        UObject* m_currently_opened_tree_node{};
        std::string m_current_property_value_buffer{};
        int64_t m_current_enum_value_buffer{};
        float m_top_size{300.0f};
        float m_bottom_size{0.0f};
        UFunctionCallerWidget* m_function_caller_widget{};
        bool m_is_searching_by_name{};
        bool m_search_field_clear_requested{};
        bool m_search_field_cleared{};
        bool m_modal_tried_to_open_nullptr_object_is_open{};
        bool m_modal_edit_property_value_is_open{};
        bool m_modal_edit_property_value_opened_this_frame{};
        bool m_modal_edit_property_value_error_unable_to_edit{};
        bool m_listeners_set{};
        bool m_listeners_allowed{};
        bool m_is_initialized{};

      public:
        LiveView();
        ~LiveView();

      public:
        struct ObjectOrProperty
        {
            union {
                const FUObjectItem* object_item{};
                FProperty* property;
            };
            UObject* object{};
            bool is_object{};

            auto IsUnreachable() const -> bool;
            auto GetFullName() const -> std::string;
        };

      public:
        static std::vector<ObjectOrProperty> s_object_view_history;
        static size_t s_currently_selected_object_index;
        static std::unordered_map<UObject*, std::vector<size_t>> s_history_object_to_index;
        static std::vector<UObject*> s_name_search_results;
        static std::unordered_set<UObject*> s_name_search_results_set;
        static std::string s_name_to_search_by;
        static std::vector<std::unique_ptr<Watch>> s_watches;
        static std::unordered_map<WatchIdentifier, Watch*> s_watch_map;
        static std::unordered_map<void*, std::vector<Watch*>> s_watch_containers;
        static bool s_include_inheritance;
        static bool s_apply_search_filters_when_not_searching;
        static bool s_create_listener_removed;
        static bool s_delete_listener_removed;
        static bool s_selected_item_deleted;
        static bool s_need_to_filter_out_properties;
        static bool s_watches_loaded_from_disk;
        static bool s_filters_loaded_from_disk;
        static bool s_use_regex_for_search;

      private:
        enum class AffectsHistory
        {
            Yes,
            No
        };

      public:
        enum class UseIndex
        {
            Yes,
            No
        };

      private:
        auto render_info_panel() -> void;
        auto render_info_panel_as_object(const FUObjectItem*, UObject*) -> void;
        auto render_info_panel_as_property(FProperty*) -> void;
        auto render_bottom_panel() -> void;
        auto render_enum() -> void;
        auto render_properties() -> void;
        auto render_object_sub_tree_hierarchy(UObject* object) -> void;
        auto render_struct_sub_tree_hierarchy(UStruct* ustruct) -> void;
        auto render_class(UClass*) -> void;
        auto render_super_struct(UStruct*) -> void;

        enum class ContainerType
        {
            Object,
            Array,
            Struct,
        };
        auto render_property_value(FProperty* property,
                                   ContainerType container_type,
                                   void* container,
                                   FProperty** last_property_in,
                                   bool* tried_to_open_nullptr_object,
                                   bool is_watchable = true,
                                   int32_t first_offset = -1) -> std::variant<std::monostate, UObject*, FProperty*>;

      private:
        auto collapse_all_except(void* except_id) -> void;
        auto search() -> void;
        auto search_by_name() -> void;
        auto select_object(size_t index, const FUObjectItem* object_item, UObject* object, AffectsHistory = AffectsHistory::Yes) -> void;
        auto select_property(size_t index, FProperty* property, AffectsHistory affects_history) -> void;
        auto get_selected_object_or_property() -> const ObjectOrProperty&;
        auto get_selected_object(size_t index = 0, UseIndex = UseIndex::No) -> std::pair<const FUObjectItem*, UObject*>;
        auto get_selected_property(size_t index = 0, UseIndex = UseIndex::No) -> FProperty*;

      private:
        auto guobjectarray_iterator(int32_t int_data_1, int32_t int_data_2, const std::function<void(UObject*)>& callable) -> void;
        auto guobjectarray_by_name_iterator([[maybe_unused]] int32_t int_data_1,
                                            [[maybe_unused]] int32_t int_data_2,
                                            const std::function<void(UObject*)>& callable) -> void;

      public:
        auto set_is_searching_by_name(bool new_value) -> void
        {
            m_is_searching_by_name = new_value;
        }
        auto set_search_field_clear_requested(bool new_value) -> void
        {
            m_search_field_clear_requested = new_value;
        }
        auto was_search_field_clear_requested() -> bool
        {
            return m_search_field_clear_requested;
        }
        auto was_search_field_cleared() -> bool
        {
            return m_search_field_cleared;
        }
        auto set_search_field_cleared(bool new_value) -> void
        {
            m_search_field_cleared = new_value;
        }

      public:
        auto set_listeners() -> void;
        auto unset_listeners() -> void;
        auto initialize() -> void;
        auto render() -> void;
        auto render_watches() -> void;
        auto process_watches() -> void;
        auto set_listeners_allowed(bool new_value) -> void
        {
            m_listeners_allowed = new_value;
        }
        auto are_listeners_allowed() -> bool
        {
            return m_listeners_allowed;
        }

      public:
        static auto process_property_watch(Watch& watch) -> void;
        static auto process_function_pre_watch(Unreal::UnrealScriptFunctionCallableContext& context, void*) -> void;
        static auto process_function_post_watch(Unreal::UnrealScriptFunctionCallableContext& context, void*) -> void;
    };
} // namespace RC::GUI

namespace std
{
    template <>
    struct hash<::RC::GUI::LiveView::WatchIdentifier>
    {
        auto operator()(const ::RC::GUI::LiveView::WatchIdentifier& watch_identifier) const -> size_t
        {
            size_t container_hash = hash<void*>()(watch_identifier.container);
            size_t property_hash = hash<void*>()(watch_identifier.property);
            return container_hash ^ property_hash;
        }
    };
} // namespace std
