#include <algorithm>
#include <format>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <regex>

#include <DynamicOutput/DynamicOutput.hpp>
#include <ExceptionHandling.hpp>
#include <Constructs/Views/EnumerateView.hpp>
#include <GUI/GUI.hpp>
#include <GUI/ImGuiUtility.hpp>
#include <GUI/LiveView.hpp>
#include <GUI/LiveView/Filter/DefaultObjectsOnly.hpp>
#include "GUI/LiveView/Filter/ClassNamesFilter.hpp"
#include <GUI/LiveView/Filter/FunctionParamFlags.hpp>
#include <GUI/LiveView/Filter/HasProperty.hpp>
#include <GUI/LiveView/Filter/HasPropertyType.hpp>
#include <GUI/LiveView/Filter/IncludeDefaultObjects.hpp>
#include <GUI/LiveView/Filter/InstancesOnly.hpp>
#include <GUI/LiveView/Filter/NonInstancesOnly.hpp>
#include <GUI/LiveView/Filter/SearchFilter.hpp>
#include <GUI/UFunctionCallerWidget.hpp>
#include <Helpers/String.hpp>
#include <JSON/JSON.hpp>
#include <JSON/Parser/Parser.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/FOutputDevice.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/UKismetNodeHelperLibrary.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <IconsFontAwesome5.h>
#include <misc/cpp/imgui_stdlib.h>
#include <fmt/chrono.h>

namespace RC::GUI
{
    using namespace Unreal;

    static auto SearchFilters = Filter::Types<Filter::InstancesOnly,
                                              Filter::NonInstancesOnly,
                                              Filter::IncludeDefaultObjects,
                                              Filter::DefaultObjectsOnly,
                                              Filter::FunctionParamFlags,
                                              Filter::ClassNamesFilter,
                                              Filter::HasProperty,
                                              Filter::HasPropertyType>{};

    static bool s_live_view_destructed = false;
    static std::unordered_map<const UObject*, std::string> s_object_ptr_to_full_name{};

    static std::mutex s_object_ptr_to_full_name_mutex{};
    std::mutex LiveView::Watch::s_watch_lock{};

    std::vector<LiveView::ObjectOrProperty> LiveView::s_object_view_history{{nullptr, nullptr, false}};
    size_t LiveView::s_currently_selected_object_index{};
    std::unordered_map<UObject*, std::vector<size_t>> LiveView::s_history_object_to_index{{nullptr, {0}}};
    std::vector<UObject*> LiveView::s_name_search_results{};
    std::unordered_set<UObject*> LiveView::s_name_search_results_set{};
    std::string LiveView::s_name_to_search_by{};
    std::vector<std::unique_ptr<LiveView::Watch>> LiveView::s_watches{};
    std::unordered_map<LiveView::WatchIdentifier, LiveView::Watch*> LiveView::s_watch_map;
    std::unordered_map<void*, std::vector<LiveView::Watch*>> LiveView::s_watch_containers{};
    bool LiveView::s_include_inheritance{};
    bool LiveView::s_apply_search_filters_when_not_searching{};
    bool LiveView::s_create_listener_removed{};
    bool LiveView::s_delete_listener_removed{};
    bool LiveView::s_selected_item_deleted{};
    bool LiveView::s_need_to_filter_out_properties{};
    bool LiveView::s_watches_loaded_from_disk{};
    bool LiveView::s_filters_loaded_from_disk{};
    bool LiveView::s_use_regex_for_search{};

    static LiveView* s_live_view{};

    static auto get_object_full_name_cxx_string(UObject* object) -> std::string;

    static auto filter_out_objects(UObject* object) -> bool
    {
        APPLY_PRE_SEARCH_FILTERS(SearchFilters)
        if (LiveView::s_name_search_results_set.contains(object))
        {
            return true;
        }
        APPLY_POST_SEARCH_FILTERS(SearchFilters)
        return false;
    }

    static auto attempt_to_add_search_result(UObject* object) -> void
    {
        // TODO: Stop using the 'HashObject' function when needing the address of an FFieldClassVariant because it's not designed to return an address.
        //       Maybe make the ToFieldClass/ToUClass functions public (append 'Unsafe' to the function names).
        if (LiveView::s_name_to_search_by.empty() ||
            LiveView::s_need_to_filter_out_properties && object->IsA(std::bit_cast<UClass*>(FProperty::StaticClass().HashObject())))
        {
            return;
        }

        auto name_to_search_by = LiveView::s_name_to_search_by;
        std::transform(name_to_search_by.begin(), name_to_search_by.end(), name_to_search_by.begin(), [](char c) {
            return std::tolower(c);
        });

        if (LiveView::s_include_inheritance)
        {
            for (UStruct* super : object->GetClassPrivate()->ForEachSuperStruct())
            {
                auto super_full_name = get_object_full_name_cxx_string(super);
                std::transform(super_full_name.begin(), super_full_name.end(), super_full_name.begin(), [](char c) {
                    return std::tolower(c);
                });
                if (super_full_name.find(name_to_search_by) != super_full_name.npos)
                {
                    LiveView::s_name_search_results.emplace_back(object);
                    LiveView::s_name_search_results_set.emplace(object);
                    break;
                }
            }
        }

        if (filter_out_objects(object))
        {
            return;
        }

        if (LiveView::s_include_inheritance && LiveView::s_name_search_results_set.contains(object))
        {
            return;
        }

        auto object_full_name = get_object_full_name_cxx_string(object);
        std::transform(object_full_name.begin(), object_full_name.end(), object_full_name.begin(), [](char c) {
            return std::tolower(c);
        });

        if (LiveView::s_use_regex_for_search)
        {
            try
            {
                if (std::regex_search(object_full_name.begin(), object_full_name.end(), std::regex(name_to_search_by)))
                {
                    LiveView::s_name_search_results.emplace_back(object);
                    LiveView::s_name_search_results_set.emplace(object);
                }
            }
            catch (std::exception& e)
            {
                UE4SS_ERROR_OUTPUTTER()
                LiveView::s_name_to_search_by.clear();
                s_live_view->set_is_searching_by_name(false);
                s_live_view->set_search_field_clear_requested(true);
            }
            return;
        }

        if (object_full_name.find(name_to_search_by) != object_full_name.npos)
        {
            LiveView::s_name_search_results.emplace_back(object);
            LiveView::s_name_search_results_set.emplace(object);
        }
    }

    static auto remove_search_result(UObject* object) -> void
    {
        LiveView::s_name_search_results.erase(std::remove_if(LiveView::s_name_search_results.begin(),
                                                             LiveView::s_name_search_results.end(),
                                                             [&](const auto& item) {
                                                                 return item == object;
                                                             }),
                                              LiveView::s_name_search_results.end());

        LiveView::s_name_search_results_set.erase(object);

        {
            std::lock_guard<decltype(LiveView::Watch::s_watch_lock)> lock{LiveView::Watch::s_watch_lock};
            auto watch_it = LiveView::s_watch_containers.find(object);
            if (watch_it != LiveView::s_watch_containers.end())
            {
                LiveView::s_watches.erase(std::remove_if(LiveView::s_watches.begin(),
                                                         LiveView::s_watches.end(),
                                                         [&](const auto& item_ptr) {
                                                             const auto& item = *item_ptr;
                                                             if (item.container == object)
                                                             {
                                                                 LiveView::s_watch_map.erase(LiveView::s_watch_map.find({item.container, item.property}));
                                                                 return true;
                                                             }
                                                             else
                                                             {
                                                                 return false;
                                                             }
                                                         }),
                                          LiveView::s_watches.end());
                LiveView::s_watch_containers.erase(watch_it);
            }
        }
    }

    struct FLiveViewCreateListener : public FUObjectCreateListener
    {
        static FLiveViewCreateListener LiveViewCreateListener;

        void NotifyUObjectCreated(const UObjectBase* object, int32 index) override
        {
            if (s_live_view_destructed)
            {
                return;
            }
            attempt_to_add_search_result(std::bit_cast<UObject*>(object));
        }

        void OnUObjectArrayShutdown() override
        {
            UObjectArray::RemoveUObjectCreateListener(this);
            LiveView::s_create_listener_removed = true;
        }
    };
    FLiveViewCreateListener FLiveViewCreateListener::LiveViewCreateListener{};

    struct FLiveViewDeleteListener : public FUObjectDeleteListener
    {
        static FLiveViewDeleteListener LiveViewDeleteListener;

        void NotifyUObjectDeleted(const UObjectBase* object, int32 index) override
        {
            if (s_live_view_destructed)
            {
                return;
            }

            auto as_uobject = std::bit_cast<UObject*>(object);
            if (LiveView::s_history_object_to_index.size() > 1)
            {
                if (auto it = LiveView::s_history_object_to_index.find(as_uobject); it != LiveView::s_history_object_to_index.end())
                {
                    for (const auto& history_index : it->second)
                    {
                        auto& selected_object_or_property = LiveView::s_object_view_history[history_index];
                        if (selected_object_or_property.is_object)
                        {
                            selected_object_or_property.object_item = nullptr;
                            selected_object_or_property.object = nullptr;
                            LiveView::s_history_object_to_index.erase(it);
                        }
                    }
                }
            }

            remove_search_result(as_uobject);

            {
                std::lock_guard lock{s_object_ptr_to_full_name_mutex};
                s_object_ptr_to_full_name.erase(as_uobject);
            }
        }

        void OnUObjectArrayShutdown() override
        {
            UObjectArray::RemoveUObjectDeleteListener(this);
            LiveView::s_delete_listener_removed = true;
        }
    };
    FLiveViewDeleteListener FLiveViewDeleteListener::LiveViewDeleteListener{};

    static auto add_bool_filter_to_json(JSON::Array& json_filters, const StringType& filter_name, bool is_enabled) -> void
    {
        auto& json_object = json_filters.new_object();
        json_object.new_string(STR("FilterName"), filter_name);
        auto& filter_data = json_object.new_object(STR("FilterData"));
        filter_data.new_bool(STR("Enabled"), is_enabled);
    }

    template <typename ContainerType>
    static auto add_array_filter_to_json(JSON::Array& json_filters, const StringType& filter_name, const ContainerType& container, const StringType& array_name) -> void
    {
        auto& json_object = json_filters.new_object();
        json_object.new_string(STR("FilterName"), filter_name);
        auto& filter_data = json_object.new_object(STR("FilterData"));

        if (array_name == STR("ClassNames"))
        {
            filter_data.new_bool(STR("IsExclude"), Filter::ClassNamesFilter::b_is_exclude);
        }
        else if (array_name == STR("FunctionParamFlags"))
        {
            filter_data.new_bool(STR("IncludeReturnProperty"), Filter::FunctionParamFlags::s_include_return_property);
        }

        auto& values_array = filter_data.new_array(array_name);
        for (const auto& value : container)
        {
            if constexpr (std::is_same_v<typename ContainerType::value_type, FName>)
            {
                values_array.new_string(value.ToString());
            }
            else if constexpr (std::is_same_v<typename ContainerType::value_type, bool>)
            {
                values_array.new_bool(value);
            }
            else
            {
                values_array.new_string(value);
            }
        }
    }

    static auto internal_save_filters_to_disk() -> void
    {
        auto json = JSON::Object{};
        auto& json_filters = json.new_array(STR("Filters"));
        {
            add_bool_filter_to_json(json_filters, STR("IncludeInheritance"), LiveView::s_include_inheritance);
            add_bool_filter_to_json(json_filters, STR("UseRegexForSearch"), LiveView::s_use_regex_for_search);
            add_bool_filter_to_json(json_filters, STR("ApplySearchFiltersWhenNotSearching"), LiveView::s_apply_search_filters_when_not_searching);
            add_bool_filter_to_json(json_filters, Filter::DefaultObjectsOnly::s_debug_name, Filter::DefaultObjectsOnly::s_enabled);
            add_bool_filter_to_json(json_filters, Filter::IncludeDefaultObjects::s_debug_name, Filter::IncludeDefaultObjects::s_enabled);
            add_bool_filter_to_json(json_filters, Filter::InstancesOnly::s_debug_name, Filter::InstancesOnly::s_enabled);
            add_bool_filter_to_json(json_filters, Filter::NonInstancesOnly::s_debug_name, Filter::NonInstancesOnly::s_enabled);
        }
        {
            add_array_filter_to_json(json_filters, Filter::ClassNamesFilter::s_debug_name, Filter::ClassNamesFilter::list_class_names, STR("ClassNames"));
            add_array_filter_to_json(json_filters, Filter::HasProperty::s_debug_name, Filter::HasProperty::list_properties, STR("Properties"));
            add_array_filter_to_json(json_filters, Filter::HasPropertyType::s_debug_name, Filter::HasPropertyType::list_property_types, STR("PropertyTypes"));
            add_array_filter_to_json(json_filters, Filter::FunctionParamFlags::s_debug_name, Filter::FunctionParamFlags::s_checkboxes, STR("FunctionParamFlags"));
        }

        auto json_file =
                File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + fmt::format(STR("\\liveview\\filters.meta.json")),
                           File::OpenFor::Writing,
                           File::OverwriteExistingFile::Yes,
                           File::CreateIfNonExistent::Yes);
        int32_t json_indent_level{};
        json_file.write_string_to_file(json.serialize(JSON::ShouldFormat::Yes, &json_indent_level));
    }

    static auto save_filters_to_disk() -> void
    {
        TRY([] {
            internal_save_filters_to_disk();
        });
    }

    template <typename T>
    static auto json_array_to_filters_list(JSON::Array& json_array, std::vector<T>& list, StringType type, std::string& internal_value) -> void
    {
        list.clear();
        internal_value.clear();
        json_array.for_each([&](JSON::Value& item) {
            if (!item.is<JSON::String>())
            {
                throw std::runtime_error{fmt::format("Invalid {} in 'filters.meta.json'", to_string(type))};
            }
            list.emplace_back(item.as<JSON::String>()->get_view());
            return LoopAction::Continue;
        });
        for (const auto& class_name : list)
        {
            if constexpr (std::is_same_v<T, FName>)
            {
                internal_value += to_string(class_name.ToString());
            }
            else
            {
                internal_value += to_string(class_name);
            }
            
            if (&class_name != &list.back())
            {
                internal_value += ", ";
            }
        }
    }

    static auto internal_load_filters_from_disk() -> void
    {
        const auto json_file =
                File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + fmt::format(STR("\\liveview\\filters.meta.json")),
                           File::OpenFor::Reading,
                           File::OverwriteExistingFile::No,
                           File::CreateIfNonExistent::Yes);
        auto json_file_contents = json_file.read_all();
        if (json_file_contents.empty())
        {
            return;
        }

        const auto json_global_object = JSON::Parser::parse(json_file_contents);
        const auto& json_filters = json_global_object->get<JSON::Array>(STR("Filters"));
        json_filters.for_each([&](const JSON::Value& filter) {
            if (!filter.is<JSON::Object>())
            {
                throw std::runtime_error{"Invalid filter in 'filters.meta.json'"};
            }
            auto& json_object = *filter.as<JSON::Object>();
            auto filter_name = json_object.get<JSON::String>(STR("FilterName")).get_view();
            auto& filter_data = json_object.get<JSON::Object>(STR("FilterData"));

            if (filter_name == STR("IncludeInheritance"))
            {
                LiveView::s_include_inheritance = filter_data.get<JSON::Bool>(STR("Enabled")).get();
            }
            else if (filter_name == STR("UseRegexForSearch"))
            {
                LiveView::s_use_regex_for_search = filter_data.get<JSON::Bool>(STR("Enabled")).get();
            }
            else if (filter_name == STR("ApplySearchFiltersWhenNotSearching"))
            {
                LiveView::s_apply_search_filters_when_not_searching = filter_data.get<JSON::Bool>(STR("Enabled")).get();
            }
            else if (filter_name == Filter::DefaultObjectsOnly::s_debug_name)
            {
                Filter::DefaultObjectsOnly::s_enabled = filter_data.get<JSON::Bool>(STR("Enabled")).get();
            }
            else if (filter_name == Filter::IncludeDefaultObjects::s_debug_name)
            {
                Filter::IncludeDefaultObjects::s_enabled = filter_data.get<JSON::Bool>(STR("Enabled")).get();
            }
            else if (filter_name == Filter::InstancesOnly::s_debug_name)
            {
                Filter::InstancesOnly::s_enabled = filter_data.get<JSON::Bool>(STR("Enabled")).get();
            }
            else if (filter_name == Filter::NonInstancesOnly::s_debug_name)
            {
                Filter::NonInstancesOnly::s_enabled = filter_data.get<JSON::Bool>(STR("Enabled")).get();
            }
            else if (filter_name == Filter::ClassNamesFilter::s_debug_name)
            {
                Filter::ClassNamesFilter::b_is_exclude = filter_data.get<JSON::Bool>(STR("IsExclude")).get();
                auto& class_names = filter_data.get<JSON::Array>(STR("ClassNames"));
                json_array_to_filters_list(class_names, Filter::ClassNamesFilter::list_class_names, STR("class name"), Filter::ClassNamesFilter::s_internal_class_names);
            }
            else if (filter_name == Filter::HasProperty::s_debug_name)
            {
                auto& properties = filter_data.get<JSON::Array>(STR("Properties"));
                json_array_to_filters_list(properties, Filter::HasProperty::list_properties, STR("property"), Filter::HasProperty::s_internal_properties);
            }
            else if (filter_name == Filter::HasPropertyType::s_debug_name)
            {
                auto& property_types = filter_data.get<JSON::Array>(STR("PropertyTypes"));
                json_array_to_filters_list(property_types,
                                           Filter::HasPropertyType::list_property_types,
                                           STR("property type"),
                                           Filter::HasPropertyType::s_internal_property_types);
            }
            else if (filter_name == Filter::FunctionParamFlags::s_debug_name)
            {
                Filter::FunctionParamFlags::s_include_return_property = filter_data.get<JSON::Bool>(STR("IncludeReturnProperty")).get();
                Filter::FunctionParamFlags::s_checkboxes.fill(false);
                auto& function_param_flags = filter_data.get<JSON::Array>(STR("FunctionParamFlags"));
                if (function_param_flags.get().size() != Filter::FunctionParamFlags::s_checkboxes.size())
                {
                    throw std::runtime_error{"Invalid number of function param flag entires in 'filters.meta.json'"};
                }
                function_param_flags.for_each([](auto index, JSON::Value& flag) {
                    if (!flag.is<JSON::Bool>())
                    {
                        throw std::runtime_error{"Invalid flag in 'filters.meta.json'"};
                    }
                    Filter::FunctionParamFlags::s_checkboxes[index] = flag.as<JSON::Bool>()->get();
                    return LoopAction::Continue;
                });
            }

            return LoopAction::Continue;
        });
    }

    static auto load_filters_from_disk() -> void
    {
        TRY([] {
            internal_load_filters_from_disk();
        });
    }

    static auto toggle_function_watch(LiveView::Watch& watch, bool new_value) -> void
    {
        watch.enabled = new_value;
        if (new_value)
        {
            if (watch.function_is_hooked)
            {
                UObjectGlobals::UnregisterHook(static_cast<UFunction*>(watch.container), watch.function_hook_ids);
            }
            watch.function_hook_ids = UObjectGlobals::RegisterHook(static_cast<UFunction*>(watch.container),
                                                                   &LiveView::process_function_pre_watch,
                                                                   &LiveView::process_function_post_watch,
                                                                   nullptr);
        }
        else
        {
            UObjectGlobals::UnregisterHook(static_cast<UFunction*>(watch.container), watch.function_hook_ids);
        }
    }

    static auto add_watch(const LiveView::WatchIdentifier& watch_id, UObject* object, FProperty* property) -> LiveView::Watch&
    {
        std::lock_guard<decltype(LiveView::Watch::s_watch_lock)> lock{LiveView::Watch::s_watch_lock};
        auto& watch = *LiveView::s_watches.emplace_back(
                std::make_unique<LiveView::Watch>(object->GetOuterPrivate()->GetName() + STR(".") + object->GetName(), property->GetName()));
        watch.enabled = true;
        watch.property = property;
        watch.container = object;
        watch.hash = std::hash<LiveView::WatchIdentifier>()(watch_id);

        LiveView::s_watch_map.emplace(watch_id, &watch);

        auto& watch_container = LiveView::s_watch_containers[watch.container];
        return *watch_container.emplace_back(&watch);
    }

    static auto add_watch(UObject* object, FProperty* property) -> LiveView::Watch&
    {
        return add_watch(LiveView::WatchIdentifier{object, property}, object, property);
    }

    static auto add_watch(const LiveView::WatchIdentifier& watch_id, UFunction* function) -> LiveView::Watch&
    {
        std::lock_guard<decltype(LiveView::Watch::s_watch_lock)> lock{LiveView::Watch::s_watch_lock};
        auto& watch =
                *LiveView::s_watches.emplace_back(std::make_unique<LiveView::Watch>(function->GetOuterPrivate()->GetName() + STR(".") + function->GetName(),
                                                                                    // Workaround for our JSON parser not being able to parse an empty string.
                                                                                    STR(" ")));
        watch.property = nullptr;
        watch.container = function;
        watch.hash = std::hash<LiveView::WatchIdentifier>()(watch_id);
        toggle_function_watch(watch, true);

        LiveView::s_watch_map.emplace(watch_id, &watch);

        auto& watch_container = LiveView::s_watch_containers[function];
        return *watch_container.emplace_back(&watch);
    }

    static auto add_watch(UFunction* function) -> LiveView::Watch&
    {
        return add_watch(LiveView::WatchIdentifier{function, nullptr}, function);
    }

    static auto serialize_watch_to_json_object(const LiveView::Watch& watch) -> std::unique_ptr<JSON::Object>
    {
        auto json_object = std::make_unique<JSON::Object>();
        switch (watch.acquisition_method)
        {
        case LiveView::Watch::AcquisitionMethod::StaticFindObject: {
            auto object_full_name = watch.container->GetFullName();
            auto object_type_space_location = object_full_name.find(STR(" "));
            auto object_typeless_name = StringType{object_full_name.begin() + object_type_space_location + 1, object_full_name.end()};
            json_object->new_string(STR("AcquisitionID"), object_typeless_name);
            break;
        }
        case LiveView::Watch::AcquisitionMethod::FindFirstOf:
            json_object->new_string(STR("AcquisitionID"), watch.container->GetClassPrivate()->GetName());
            break;
        }
        json_object->new_string(STR("PropertyName"), watch.property_name);
        json_object->new_number(STR("AcquisitionMethod"), static_cast<int32_t>(watch.acquisition_method));
        json_object->new_number(STR("WatchType"),
                                watch.container->IsA<UFunction>() ? static_cast<int32_t>(LiveView::Watch::Type::Function)
                                                                  : static_cast<int32_t>(LiveView::Watch::Type::Property));
        return json_object;
    }

    static auto internal_load_watches_from_disk() -> void
    {
        auto working_directory_path = StringType{UE4SSProgram::get_program().get_working_directory()} + fmt::format(STR("\\watches\\watches.meta.json"));
        auto legacy_root_directory_path = StringType{UE4SSProgram::get_program().get_legacy_root_directory()} + fmt::format(STR("\\watches\\watches.meta.json"));
    
        StringType json_file_contents;
        bool is_legacy = !std::filesystem::exists(working_directory_path) && std::filesystem::exists(legacy_root_directory_path);
        auto json_file = File::open(is_legacy ? legacy_root_directory_path : working_directory_path, File::OpenFor::Reading, File::OverwriteExistingFile::No, File::CreateIfNonExistent::Yes);
        
        if (json_file_contents.empty())
        {
            return;
        }

        auto json_global_object = JSON::Parser::parse(json_file_contents);
        const auto& elements = json_global_object->get<JSON::Array>(STR("Watches"));
        elements.for_each([](JSON::Value& element) {
            if (!element.is<JSON::Object>())
            {
                throw std::runtime_error{"Invalid watch in 'watches.meta.json'"};
            }
            auto& json_watch_object = *element.as<JSON::Object>();
            auto acquisition_id = json_watch_object.get<JSON::String>(STR("AcquisitionID")).get_view();
            auto property_name = json_watch_object.get<JSON::String>(STR("PropertyName")).get_view();
            auto acquisition_method =
                    static_cast<LiveView::Watch::AcquisitionMethod>(json_watch_object.get<JSON::Number>(STR("AcquisitionMethod")).get<int64_t>());
            auto watch_type = static_cast<LiveView::Watch::Type>(json_watch_object.get<JSON::Number>(STR("WatchType")).get<int64_t>());

            UObject* object{};
            switch (acquisition_method)
            {
            case LiveView::Watch::AcquisitionMethod::StaticFindObject:
                object = UObjectGlobals::StaticFindObject<UObject*>(nullptr, nullptr, acquisition_id);
                break;
            case LiveView::Watch::AcquisitionMethod::FindFirstOf:
                object = UObjectGlobals::FindFirstOf(acquisition_id);
                break;
            default:
                throw std::runtime_error{"load_watches_from_disk: Unhandled acquisition method"};
            }
            if (!object)
            {
                return LoopAction::Continue;
            }

            LiveView::Watch* watch = [&]() -> LiveView::Watch* {
                if (watch_type == LiveView::Watch::Type::Property)
                {
                    auto property = object->GetPropertyByNameInChain(property_name.data());
                    if (!property)
                    {
                        return nullptr;
                    }
                    return &add_watch(object, property);
                }
                else
                {
                    return &add_watch(static_cast<UFunction*>(object));
                }
            }();

            if (!watch)
            {
                return LoopAction::Continue;
            }

            watch->load_on_startup = true;
            watch->acquisition_method = acquisition_method;

            return LoopAction::Continue;
        });
    }

    static auto load_watches_from_disk() -> void
    {
        TRY([] {
            internal_load_watches_from_disk();
        });
    }

    static auto internal_save_watches_to_disk() -> void
    {
        auto json = JSON::Object{};
        auto& json_uobjects = json.new_array(STR("Watches"));

        {
            std::lock_guard<decltype(LiveView::Watch::s_watch_lock)> lock{LiveView::Watch::s_watch_lock};
            for (const auto& watch : LiveView::s_watches)
            {
                if (!watch->load_on_startup)
                {
                    continue;
                }
                json_uobjects.add_object(serialize_watch_to_json_object(*watch));
            }
        }

        auto json_file = File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + fmt::format(STR("\\watches\\watches.meta.json")),
                                    File::OpenFor::Writing,
                                    File::OverwriteExistingFile::Yes,
                                    File::CreateIfNonExistent::Yes);
        int32_t json_indent_level{};
        json_file.write_string_to_file(json.serialize(JSON::ShouldFormat::Yes, &json_indent_level));
    }

    static auto save_watches_to_disk() -> void
    {
        TRY([] {
            internal_save_watches_to_disk();
        });
    }

    auto LiveView::set_listeners() -> void
    {
        if (m_listeners_set || !are_listeners_allowed())
        {
            return;
        }
        m_listeners_set = true;
        UObjectArray::AddUObjectCreateListener(&FLiveViewCreateListener::LiveViewCreateListener);
        UObjectArray::AddUObjectDeleteListener(&FLiveViewDeleteListener::LiveViewDeleteListener);
    }

    auto LiveView::unset_listeners() -> void
    {
        if (!m_listeners_set || !are_listeners_allowed())
        {
            return;
        }
        m_listeners_set = false;
        UObjectArray::RemoveUObjectCreateListener(&FLiveViewCreateListener::LiveViewCreateListener);
        UObjectArray::RemoveUObjectDeleteListener(&FLiveViewDeleteListener::LiveViewDeleteListener);
    }

    LiveView::Watch::Watch(StringType&& object_name, StringType&& property_name) : object_name(object_name), property_name(property_name)
    {
        auto& file_device = output.get_device<Output::FileDevice>();
        file_device.set_file_name_and_path(StringType{UE4SSProgram::get_program().get_working_directory()} +
                                           fmt::format(STR("\\watches\\ue4ss_watch_{}_{}.txt"), object_name, property_name));
        file_device.set_formatter([](File::StringViewType string) -> File::StringType {
            const auto when_as_string = fmt::format(STR("{:%Y-%m-%d %H:%M:%S}"), std::chrono::system_clock::now());
            return fmt::format(STR("[{}] {}"), when_as_string, string);
        });
    }

    auto LiveView::initialize() -> void
    {
        s_need_to_filter_out_properties = Version::IsBelow(4, 25);
        m_is_initialized = true;
    }

    struct ObjectFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        static constexpr const char* popup_context_item_id_raw = "object_raw_flags_menu";
        static constexpr const char* popup_context_item_id = "object_flags_menu";

        static auto get_raw_flags(UObject* object) -> uint32_t
        {
            return static_cast<uint32_t>(object->GetObjectFlags());
        }

        ObjectFlagsStringifier(UObject* object)
        {
            if (object->HasAnyFlags(RF_NoFlags))
            {
                flag_parts.emplace_back("RF_NoFlags");
            }
            if (object->HasAnyFlags(RF_Public))
            {
                flag_parts.emplace_back("RF_Public");
            }
            if (object->HasAnyFlags(RF_Standalone))
            {
                flag_parts.emplace_back("RF_Standalone");
            }
            if (object->HasAnyFlags(RF_MarkAsNative))
            {
                flag_parts.emplace_back("RF_MarkAsNative");
            }
            if (object->HasAnyFlags(RF_Transactional))
            {
                flag_parts.emplace_back("RF_Transactional");
            }
            if (object->HasAnyFlags(RF_ClassDefaultObject))
            {
                flag_parts.emplace_back("RF_ClassDefaultObject");
            }
            if (object->HasAnyFlags(RF_ArchetypeObject))
            {
                flag_parts.emplace_back("RF_ArchetypeObject");
            }
            if (object->HasAnyFlags(RF_Transient))
            {
                flag_parts.emplace_back("RF_Transient");
            }
            if (object->HasAnyFlags(RF_MarkAsRootSet))
            {
                flag_parts.emplace_back("RF_MarkAsRootSet");
            }
            if (object->HasAnyFlags(RF_TagGarbageTemp))
            {
                flag_parts.emplace_back("RF_TagGarbageTemp");
            }
            if (object->HasAnyFlags(RF_NeedInitialization))
            {
                flag_parts.emplace_back("RF_NeedInitialization");
            }
            if (object->HasAnyFlags(RF_NeedLoad))
            {
                flag_parts.emplace_back("RF_NeedLoad");
            }
            if (object->HasAnyFlags(RF_KeepForCooker))
            {
                flag_parts.emplace_back("RF_KeepForCooker");
            }
            if (object->HasAnyFlags(RF_NeedPostLoad))
            {
                flag_parts.emplace_back("RF_NeedPostLoad");
            }
            if (object->HasAnyFlags(RF_NeedPostLoadSubobjects))
            {
                flag_parts.emplace_back("RF_NeedPostLoadSubobjects");
            }
            if (object->HasAnyFlags(RF_NewerVersionExists))
            {
                flag_parts.emplace_back("RF_NewerVersionExists");
            }
            if (object->HasAnyFlags(RF_BeginDestroyed))
            {
                flag_parts.emplace_back("RF_BeginDestroyed");
            }
            if (object->HasAnyFlags(RF_FinishDestroyed))
            {
                flag_parts.emplace_back("RF_FinishDestroyed");
            }
            if (object->HasAnyFlags(RF_BeingRegenerated))
            {
                flag_parts.emplace_back("RF_BeingRegenerated");
            }
            if (object->HasAnyFlags(RF_DefaultSubObject))
            {
                flag_parts.emplace_back("RF_DefaultSubObject");
            }
            if (object->HasAnyFlags(RF_WasLoaded))
            {
                flag_parts.emplace_back("RF_WasLoaded");
            }
            if (object->HasAnyFlags(RF_TextExportTransient))
            {
                flag_parts.emplace_back("RF_TextExportTransient");
            }
            if (object->HasAnyFlags(RF_LoadCompleted))
            {
                flag_parts.emplace_back("RF_LoadCompleted");
            }
            if (object->HasAnyFlags(RF_InheritableComponentTemplate))
            {
                flag_parts.emplace_back("RF_InheritableComponentTemplate");
            }
            if (object->HasAnyFlags(RF_DuplicateTransient))
            {
                flag_parts.emplace_back("RF_DuplicateTransient");
            }
            if (object->HasAnyFlags(RF_StrongRefOnFrame))
            {
                flag_parts.emplace_back("RF_StrongRefOnFrame");
            }
            if (object->HasAnyFlags(RF_NonPIEDuplicateTransient))
            {
                flag_parts.emplace_back("RF_NonPIEDuplicateTransient");
            }
            if (object->HasAnyFlags(RF_Dynamic))
            {
                flag_parts.emplace_back("RF_Dynamic");
            }
            if (object->HasAnyFlags(RF_WillBeLoaded))
            {
                flag_parts.emplace_back("RF_WillBeLoaded");
            }
            if (object->HasAnyFlags(RF_HasExternalPackage))
            {
                flag_parts.emplace_back("RF_HasExternalPackage");
            }
            if (object->HasAnyFlags(RF_AllFlags))
            {
                flag_parts.emplace_back("RF_AllFlags");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };

    struct ClassFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        static constexpr const char* popup_context_item_id_raw = "class_raw_flags_menu";
        static constexpr const char* popup_context_item_id = "class_flags_menu";

        static auto get_raw_flags(UClass* object) -> uint32_t
        {
            return static_cast<uint32_t>(object->GetClassFlags());
        }

        ClassFlagsStringifier(UClass* uclass)
        {
            if (uclass->HasAnyClassFlags(CLASS_None))
            {
                flag_parts.emplace_back("CLASS_None");
            }
            if (uclass->HasAnyClassFlags(CLASS_Abstract))
            {
                flag_parts.emplace_back("CLASS_Abstract");
            }
            if (uclass->HasAnyClassFlags(CLASS_DefaultConfig))
            {
                flag_parts.emplace_back("CLASS_DefaultConfig");
            }
            if (uclass->HasAnyClassFlags(CLASS_Config))
            {
                flag_parts.emplace_back("CLASS_Config");
            }
            if (uclass->HasAnyClassFlags(CLASS_Transient))
            {
                flag_parts.emplace_back("CLASS_Transient");
            }
            if (uclass->HasAnyClassFlags(CLASS_Parsed))
            {
                flag_parts.emplace_back("CLASS_Parsed");
            }
            if (uclass->HasAnyClassFlags(CLASS_MatchedSerializers))
            {
                flag_parts.emplace_back("CLASS_MatchedSerializers");
            }
            if (uclass->HasAnyClassFlags(CLASS_ProjectUserConfig))
            {
                flag_parts.emplace_back("CLASS_ProjectUserConfig");
            }
            if (uclass->HasAnyClassFlags(CLASS_Native))
            {
                flag_parts.emplace_back("CLASS_Native");
            }
            if (uclass->HasAnyClassFlags(CLASS_NoExport))
            {
                flag_parts.emplace_back("CLASS_NoExport");
            }
            if (uclass->HasAnyClassFlags(CLASS_NotPlaceable))
            {
                flag_parts.emplace_back("CLASS_NotPlaceable");
            }
            if (uclass->HasAnyClassFlags(CLASS_PerObjectConfig))
            {
                flag_parts.emplace_back("CLASS_PerObjectConfig");
            }
            if (uclass->HasAnyClassFlags(CLASS_ReplicationDataIsSetUp))
            {
                flag_parts.emplace_back("CLASS_ReplicationDataIsSetUp");
            }
            if (uclass->HasAnyClassFlags(CLASS_EditInlineNew))
            {
                flag_parts.emplace_back("CLASS_EditInlineNew");
            }
            if (uclass->HasAnyClassFlags(CLASS_CollapseCategories))
            {
                flag_parts.emplace_back("CLASS_CollapseCategories");
            }
            if (uclass->HasAnyClassFlags(CLASS_Interface))
            {
                flag_parts.emplace_back("CLASS_Interface");
            }
            if (uclass->HasAnyClassFlags(CLASS_CustomConstructor))
            {
                flag_parts.emplace_back("CLASS_CustomConstructor");
            }
            if (uclass->HasAnyClassFlags(CLASS_Const))
            {
                flag_parts.emplace_back("CLASS_Const");
            }
            if (uclass->HasAnyClassFlags(CLASS_LayoutChanging))
            {
                flag_parts.emplace_back("CLASS_LayoutChanging");
            }
            if (uclass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
            {
                flag_parts.emplace_back("CLASS_CompiledFromBlueprint");
            }
            if (uclass->HasAnyClassFlags(CLASS_MinimalAPI))
            {
                flag_parts.emplace_back("CLASS_MinimalAPI");
            }
            if (uclass->HasAnyClassFlags(CLASS_RequiredAPI))
            {
                flag_parts.emplace_back("CLASS_RequiredAPI");
            }
            if (uclass->HasAnyClassFlags(CLASS_DefaultToInstanced))
            {
                flag_parts.emplace_back("CLASS_DefaultToInstanced");
            }
            if (uclass->HasAnyClassFlags(CLASS_TokenStreamAssembled))
            {
                flag_parts.emplace_back("CLASS_TokenStreamAssembled");
            }
            if (uclass->HasAnyClassFlags(CLASS_HasInstancedReference))
            {
                flag_parts.emplace_back("CLASS_HasInstancedReference");
            }
            if (uclass->HasAnyClassFlags(CLASS_Hidden))
            {
                flag_parts.emplace_back("CLASS_Hidden");
            }
            if (uclass->HasAnyClassFlags(CLASS_Deprecated))
            {
                flag_parts.emplace_back("CLASS_Deprecated");
            }
            if (uclass->HasAnyClassFlags(CLASS_HideDropDown))
            {
                flag_parts.emplace_back("CLASS_HideDropDown");
            }
            if (uclass->HasAnyClassFlags(CLASS_GlobalUserConfig))
            {
                flag_parts.emplace_back("CLASS_GlobalUserConfig");
            }
            if (uclass->HasAnyClassFlags(CLASS_Intrinsic))
            {
                flag_parts.emplace_back("CLASS_Intrinsic");
            }
            if (uclass->HasAnyClassFlags(CLASS_Constructed))
            {
                flag_parts.emplace_back("CLASS_Constructed");
            }
            if (uclass->HasAnyClassFlags(CLASS_ConfigDoNotCheckDefaults))
            {
                flag_parts.emplace_back("CLASS_ConfigDoNotCheckDefaults");
            }
            if (uclass->HasAnyClassFlags(CLASS_NewerVersionExists))
            {
                flag_parts.emplace_back("CLASS_NewerVersionExists");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };

    struct FunctionFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        static constexpr const char* popup_context_item_id_raw = "func_raw_flags_menu";
        static constexpr const char* popup_context_item_id = "func_flags_menu";

        static auto get_raw_flags(UFunction* function) -> uint32_t
        {
            return static_cast<uint32_t>(function->GetFunctionFlags());
        }

        FunctionFlagsStringifier(UFunction* function)
        {
            if (function->HasAnyFunctionFlags(FUNC_None))
            {
                flag_parts.emplace_back("FUNC_None");
            }
            if (function->HasAnyFunctionFlags(FUNC_Final))
            {
                flag_parts.emplace_back("FUNC_Final");
            }
            if (function->HasAnyFunctionFlags(FUNC_RequiredAPI))
            {
                flag_parts.emplace_back("FUNC_RequiredAPI");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly))
            {
                flag_parts.emplace_back("FUNC_BlueprintAuthorityOnly");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintCosmetic))
            {
                flag_parts.emplace_back("FUNC_BlueprintCosmetic");
            }
            if (function->HasAnyFunctionFlags(FUNC_Net))
            {
                flag_parts.emplace_back("FUNC_Net");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetReliable))
            {
                flag_parts.emplace_back("FUNC_NetReliable");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetRequest))
            {
                flag_parts.emplace_back("FUNC_NetRequest");
            }
            if (function->HasAnyFunctionFlags(FUNC_Exec))
            {
                flag_parts.emplace_back("FUNC_Exec");
            }
            if (function->HasAnyFunctionFlags(FUNC_Native))
            {
                flag_parts.emplace_back("FUNC_Native");
            }
            if (function->HasAnyFunctionFlags(FUNC_Event))
            {
                flag_parts.emplace_back("FUNC_Event");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetResponse))
            {
                flag_parts.emplace_back("FUNC_NetResponse");
            }
            if (function->HasAnyFunctionFlags(FUNC_Static))
            {
                flag_parts.emplace_back("FUNC_Static");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetMulticast))
            {
                flag_parts.emplace_back("FUNC_NetMulticast");
            }
            if (function->HasAnyFunctionFlags(FUNC_UbergraphFunction))
            {
                flag_parts.emplace_back("FUNC_UbergraphFunction");
            }
            if (function->HasAnyFunctionFlags(FUNC_MulticastDelegate))
            {
                flag_parts.emplace_back("FUNC_MulticastDelegate");
            }
            if (function->HasAnyFunctionFlags(FUNC_Public))
            {
                flag_parts.emplace_back("FUNC_Public");
            }
            if (function->HasAnyFunctionFlags(FUNC_Private))
            {
                flag_parts.emplace_back("FUNC_Private");
            }
            if (function->HasAnyFunctionFlags(FUNC_Protected))
            {
                flag_parts.emplace_back("FUNC_Protected");
            }
            if (function->HasAnyFunctionFlags(FUNC_Delegate))
            {
                flag_parts.emplace_back("FUNC_Delegate");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetServer))
            {
                flag_parts.emplace_back("FUNC_NetServer");
            }
            if (function->HasAnyFunctionFlags(FUNC_HasOutParms))
            {
                flag_parts.emplace_back("FUNC_HasOutParms");
            }
            if (function->HasAnyFunctionFlags(FUNC_HasDefaults))
            {
                flag_parts.emplace_back("FUNC_HasDefaults");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetClient))
            {
                flag_parts.emplace_back("FUNC_NetClient");
            }
            if (function->HasAnyFunctionFlags(FUNC_DLLImport))
            {
                flag_parts.emplace_back("FUNC_DLLImport");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintCallable))
            {
                flag_parts.emplace_back("FUNC_BlueprintCallable");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintEvent))
            {
                flag_parts.emplace_back("FUNC_BlueprintEvent");
            }
            if (function->HasAnyFunctionFlags(FUNC_BlueprintPure))
            {
                flag_parts.emplace_back("FUNC_BlueprintPure");
            }
            if (function->HasAnyFunctionFlags(FUNC_EditorOnly))
            {
                flag_parts.emplace_back("FUNC_EditorOnly");
            }
            if (function->HasAnyFunctionFlags(FUNC_Const))
            {
                flag_parts.emplace_back("FUNC_Const");
            }
            if (function->HasAnyFunctionFlags(FUNC_NetValidate))
            {
                flag_parts.emplace_back("FUNC_NetValidate");
            }
            if (function->HasAnyFunctionFlags(FUNC_AllFlags))
            {
                flag_parts.emplace_back("FUNC_AllFlags");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };

    struct PropertyFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        PropertyFlagsStringifier(EPropertyFlags flags)
        {
            if ((flags & CPF_Edit) != 0)
            {
                flag_parts.emplace_back("CPF_Edit");
            }
            if ((flags & CPF_ConstParm) != 0)
            {
                flag_parts.emplace_back("CPF_ConstParm");
            }
            if ((flags & CPF_BlueprintVisible) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintVisible");
            }
            if ((flags & CPF_ExportObject) != 0)
            {
                flag_parts.emplace_back("CPF_ExportObject");
            }
            if ((flags & CPF_BlueprintReadOnly) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintReadOnly");
            }
            if ((flags & CPF_Net) != 0)
            {
                flag_parts.emplace_back("CPF_Net");
            }
            if ((flags & CPF_EditFixedSize) != 0)
            {
                flag_parts.emplace_back("CPF_EditFixedSize");
            }
            if ((flags & CPF_Parm) != 0)
            {
                flag_parts.emplace_back("CPF_Parm");
            }
            if ((flags & CPF_OutParm) != 0)
            {
                flag_parts.emplace_back("CPF_OutParm");
            }
            if ((flags & CPF_ZeroConstructor) != 0)
            {
                flag_parts.emplace_back("CPF_ZeroConstructor");
            }
            if ((flags & CPF_ReturnParm) != 0)
            {
                flag_parts.emplace_back("CPF_ReturnParm");
            }
            if ((flags & CPF_DisableEditOnTemplate) != 0)
            {
                flag_parts.emplace_back("CPF_DisableEditOnTemplate");
            }
            if ((flags & CPF_Transient) != 0)
            {
                flag_parts.emplace_back("CPF_Transient");
            }
            if ((flags & CPF_Config) != 0)
            {
                flag_parts.emplace_back("CPF_Config");
            }
            if ((flags & CPF_DisableEditOnInstance) != 0)
            {
                flag_parts.emplace_back("CPF_DisableEditOnInstance");
            }
            if ((flags & CPF_EditConst) != 0)
            {
                flag_parts.emplace_back("CPF_EditConst");
            }
            if ((flags & CPF_GlobalConfig) != 0)
            {
                flag_parts.emplace_back("CPF_GlobalConfig");
            }
            if ((flags & CPF_InstancedReference) != 0)
            {
                flag_parts.emplace_back("CPF_InstancedReference");
            }
            if ((flags & CPF_DuplicateTransient) != 0)
            {
                flag_parts.emplace_back("CPF_DuplicateTransient");
            }
            if ((flags & CPF_SubobjectReference) != 0)
            {
                flag_parts.emplace_back("CPF_SubobjectReference");
            }
            if ((flags & CPF_SaveGame) != 0)
            {
                flag_parts.emplace_back("CPF_SaveGame");
            }
            if ((flags & CPF_NoClear) != 0)
            {
                flag_parts.emplace_back("CPF_NoClear");
            }
            if ((flags & CPF_ReferenceParm) != 0)
            {
                flag_parts.emplace_back("CPF_ReferenceParm");
            }
            if ((flags & CPF_BlueprintAssignable) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintAssignable");
            }
            if ((flags & CPF_Deprecated) != 0)
            {
                flag_parts.emplace_back("CPF_Deprecated");
            }
            if ((flags & CPF_IsPlainOldData) != 0)
            {
                flag_parts.emplace_back("CPF_IsPlainOldData");
            }
            if ((flags & CPF_RepSkip) != 0)
            {
                flag_parts.emplace_back("CPF_RepSkip");
            }
            if ((flags & CPF_RepNotify) != 0)
            {
                flag_parts.emplace_back("CPF_RepNotify");
            }
            if ((flags & CPF_Interp) != 0)
            {
                flag_parts.emplace_back("CPF_Interp");
            }
            if ((flags & CPF_NonTransactional) != 0)
            {
                flag_parts.emplace_back("CPF_NonTransactional");
            }
            if ((flags & CPF_EditorOnly) != 0)
            {
                flag_parts.emplace_back("CPF_EditorOnly");
            }
            if ((flags & CPF_NoDestructor) != 0)
            {
                flag_parts.emplace_back("CPF_NoDestructor");
            }
            if ((flags & CPF_AutoWeak) != 0)
            {
                flag_parts.emplace_back("CPF_AutoWeak");
            }
            if ((flags & CPF_ContainsInstancedReference) != 0)
            {
                flag_parts.emplace_back("CPF_ContainsInstancedReference");
            }
            if ((flags & CPF_AssetRegistrySearchable) != 0)
            {
                flag_parts.emplace_back("CPF_AssetRegistrySearchable");
            }
            if ((flags & CPF_SimpleDisplay) != 0)
            {
                flag_parts.emplace_back("CPF_SimpleDisplay");
            }
            if ((flags & CPF_AdvancedDisplay) != 0)
            {
                flag_parts.emplace_back("CPF_AdvancedDisplay");
            }
            if ((flags & CPF_Protected) != 0)
            {
                flag_parts.emplace_back("CPF_Protected");
            }
            if ((flags & CPF_BlueprintCallable) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintCallable");
            }
            if ((flags & CPF_BlueprintAuthorityOnly) != 0)
            {
                flag_parts.emplace_back("CPF_BlueprintAuthorityOnly");
            }
            if ((flags & CPF_TextExportTransient) != 0)
            {
                flag_parts.emplace_back("CPF_TextExportTransient");
            }
            if ((flags & CPF_NonPIEDuplicateTransient) != 0)
            {
                flag_parts.emplace_back("CPF_NonPIEDuplicateTransient");
            }
            if ((flags & CPF_ExposeOnSpawn) != 0)
            {
                flag_parts.emplace_back("CPF_ExposeOnSpawn");
            }
            if ((flags & CPF_PersistentInstance) != 0)
            {
                flag_parts.emplace_back("CPF_PersistentInstance");
            }
            if ((flags & CPF_UObjectWrapper) != 0)
            {
                flag_parts.emplace_back("CPF_UObjectWrapper");
            }
            if ((flags & CPF_HasGetValueTypeHash) != 0)
            {
                flag_parts.emplace_back("CPF_HasGetValueTypeHash");
            }
            if ((flags & CPF_NativeAccessSpecifierPublic) != 0)
            {
                flag_parts.emplace_back("CPF_NativeAccessSpecifierPublic");
            }
            if ((flags & CPF_NativeAccessSpecifierProtected) != 0)
            {
                flag_parts.emplace_back("CPF_NativeAccessSpecifierProtected");
            }
            if ((flags & CPF_NativeAccessSpecifierPrivate) != 0)
            {
                flag_parts.emplace_back("CPF_NativeAccessSpecifierPrivate");
            }
            if ((flags & CPF_SkipSerialization) != 0)
            {
                flag_parts.emplace_back("CPF_SkipSerialization");
            }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };

    LiveView::LiveView() : m_function_caller_widget(new UFunctionCallerWidget{})
    {
        m_search_by_name_buffer = new char[m_search_buffer_capacity];
        strncpy_s(m_search_by_name_buffer,
                  m_default_search_buffer.size() + sizeof(char),
                  m_default_search_buffer.data(),
                  m_default_search_buffer.size() + sizeof(char));

        s_live_view = this;
    }

    LiveView::~LiveView()
    {
        s_live_view_destructed = true;
        if (!s_create_listener_removed && m_listeners_set)
        {
            UObjectArray::RemoveUObjectCreateListener(&FLiveViewCreateListener::LiveViewCreateListener);
        }

        if (!s_delete_listener_removed && m_listeners_set)
        {
            UObjectArray::RemoveUObjectDeleteListener(&FLiveViewDeleteListener::LiveViewDeleteListener);
        }

        delete[] m_search_by_name_buffer;
        delete m_function_caller_widget;
    }

    auto LiveView::guobjectarray_iterator(int32_t int_data_1, int32_t int_data_2, const std::function<void(UObject*)>& callable) -> void
    {
        UObjectGlobals::ForEachUObjectInRange(int_data_1, int_data_2, [&](UObject* object, ...) {
            // TODO: Stop using the 'HashObject' function when needing the address of an FFieldClassVariant because it's not designed to return an address.
            //       Maybe make the ToFieldClass/ToUClass functions public (append 'Unsafe' to the function names).
            if (s_need_to_filter_out_properties && object->IsA(std::bit_cast<UClass*>(FProperty::StaticClass().HashObject())))
            {
                return LoopAction::Continue;
            }
            if (s_apply_search_filters_when_not_searching && filter_out_objects(object))
            {
                return LoopAction::Continue;
            }
            callable(object);
            return LoopAction::Continue;
        });
    }

    auto LiveView::select_object(size_t index, const FUObjectItem* object_item, UObject* object, AffectsHistory affects_history) -> void
    {
        if (object_item && object && affects_history == AffectsHistory::Yes)
        {
            s_object_view_history.resize(s_currently_selected_object_index + 1);
            s_object_view_history.emplace_back(ObjectOrProperty{object_item, object, true});
            s_currently_selected_object_index = s_object_view_history.size() - 1;
            auto [history_object, did_emplace] = s_history_object_to_index.emplace(object, std::vector<size_t>{});
            history_object->second.emplace_back(s_currently_selected_object_index);
        }
        else
        {
            s_currently_selected_object_index = index;
        }
    }

    auto LiveView::select_property(size_t index, FProperty* property, AffectsHistory affects_history) -> void
    {
        s_object_view_history.resize(s_currently_selected_object_index + 1);
        auto object_or_property = ObjectOrProperty{};
        object_or_property.is_object = false;
        object_or_property.property = property;
        object_or_property.object = nullptr;
        s_object_view_history.emplace_back(std::move(object_or_property));
        s_currently_selected_object_index = s_object_view_history.size() - 1;
    }

    static auto get_object_full_name(const UObject* object) -> const char*
    {
        if (!UnrealInitializer::StaticStorage::bIsInitialized)
        {
            return "";
        }
        std::lock_guard lock{s_object_ptr_to_full_name_mutex};
        if (auto it = s_object_ptr_to_full_name.find(object); it != s_object_ptr_to_full_name.end())
        {
            return it->second.c_str();
        }
        else
        {
            return s_object_ptr_to_full_name.emplace(object, to_string(object->GetFullName())).first->second.c_str();
        }
    }

    static auto get_object_full_name_cxx_string(UObject* object) -> std::string
    {
        if (!UnrealInitializer::StaticStorage::bIsInitialized)
        {
            return "";
        }
        std::lock_guard lock{s_object_ptr_to_full_name_mutex};
        if (auto it = s_object_ptr_to_full_name.find(object); it != s_object_ptr_to_full_name.end())
        {
            return it->second;
        }
        else
        {
            return s_object_ptr_to_full_name.emplace(object, to_string(object->GetFullName())).first->second;
        }
    }

    auto LiveView::guobjectarray_by_name_iterator(int32_t int_data_1, int32_t int_data_2, const std::function<void(UObject*)>& callable) -> void
    {
        if (int_data_2 > s_name_search_results.size())
        {
            Output::send<LogLevel::Error>(STR("guobjectarray_by_name_iterator: asked to iterate beyond the size of the search result vector ({} > {})\n"),
                                          int_data_2,
                                          s_name_search_results.size());
            return;
        }
        for (size_t i = int_data_1; i < int_data_2; i++)
        {
            callable(s_name_search_results[i]);
        }
    }

    auto LiveView::search_by_name() -> void
    {
        Output::send(STR("Searching by name...\n"));
        s_name_search_results.clear();
        s_name_search_results_set.clear();
        UObjectGlobals::ForEachUObject([&](UObject* object, ...) {
            attempt_to_add_search_result(object);
            return LoopAction::Continue;
        });
    }

    auto LiveView::collapse_all_except(void* except_id) -> void
    {
        // We don't need to do anything if we only have one node open.
        if (m_opened_tree_nodes.size() == 1)
        {
            return;
        }

        for (auto tree_node = m_opened_tree_nodes.begin(); tree_node != m_opened_tree_nodes.end();)
        {
            if (*tree_node != except_id)
            {
                ImGui::GetStateStorage()->SetInt(ImGui::GetID(*tree_node), 0);
                m_opened_tree_nodes.erase(tree_node++);
            }
            else
            {
                ++tree_node;
            }
        }
    }

    auto LiveView::search() -> void
    {
        if (are_listeners_allowed())
        {
            std::string search_buffer{m_search_by_name_buffer};
            if (search_buffer.empty() || s_apply_search_filters_when_not_searching)
            {
                Output::send(STR("Search all chunks\n"));
                s_name_to_search_by.clear();
                m_object_iterator = &LiveView::guobjectarray_iterator;
                m_is_searching_by_name = false;
            }
            else
            {
                Output::send(STR("Search for: {}\n"), search_buffer.empty() ? STR("") : to_wstring(search_buffer));
                s_name_to_search_by = search_buffer;
                m_object_iterator = &LiveView::guobjectarray_by_name_iterator;
                m_is_searching_by_name = true;
                search_by_name();
            }
        }
    }

    auto LiveView::render_object_sub_tree_hierarchy(UObject* object) -> void
    {
        if (!object)
        {
            return;
        }

        auto uclass = object->GetClassPrivate();
        ImGui::Text("%s", get_object_full_name(uclass));
        render_struct_sub_tree_hierarchy(uclass);
    }

    auto LiveView::render_struct_sub_tree_hierarchy(UStruct* ustruct) -> void
    {
        if (!ustruct)
        {
            return;
        }

        auto next_class = ustruct->GetClassPrivate();

        ImGui::Indent();
        ImGui::Text("ClassPrivate");
        if (ImGui::TreeNode(get_object_full_name(next_class)))
        {
            if (next_class && ustruct != next_class)
            {
                render_struct_sub_tree_hierarchy(next_class);
            }
            ImGui::TreePop();
        }
        else
        {
            if (ImGui::IsItemClicked())
            {
                select_object(0, ustruct->GetObjectItem(), ustruct, AffectsHistory::Yes);
            }
        }

        auto next_super = ustruct->GetSuperStruct();

        ImGui::Text("SuperStruct");
        if (ImGui::TreeNode(get_object_full_name(next_super)))
        {
            if (next_super && ustruct != next_super)
            {
                render_struct_sub_tree_hierarchy(next_super);
            }
            ImGui::TreePop();
        }
        else
        {
            if (ImGui::IsItemClicked())
            {
                select_object(0, ustruct->GetObjectItem(), ustruct, AffectsHistory::Yes);
            }
        }

        ImGui::Text("Properties");
        if (ImGui::TreeNodeEx("Show", ImGuiTreeNodeFlags_SpanFullWidth))
        {
            for (FProperty* property : ustruct->ForEachProperty())
            {
                ImGui::TreeNodeEx(to_string(property->GetFullName()).c_str(), ImGuiTreeNodeFlags_Leaf);
                if (ImGui::IsItemClicked())
                {
                    select_property(0, property, AffectsHistory::Yes);
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
        ImGui::Unindent();
    }

    auto LiveView::render_class(UClass* uclass) -> void
    {
        if (!uclass)
        {
            return;
        }
        ImGui::Text("ClassPrivate");
        if (ImGui::TreeNode(get_object_full_name(uclass)))
        {
            auto next_class = uclass->GetClassPrivate();
            if (uclass != next_class)
            {
                render_class(next_class);
                render_super_struct(uclass->GetSuperStruct());
            }
            ImGui::TreePop();
        }
        else
        {
            if (ImGui::IsItemClicked())
            {
                select_object(0, uclass->GetObjectItem(), uclass, AffectsHistory::Yes);
            }
        }

        Output::send(STR("{}\n"), uclass->GetFullName());

        ImGui::Text("Properties");
        for (FProperty* property : uclass->ForEachProperty())
        {
            if (ImGui::TreeNode(to_string(property->GetFullName()).c_str()))
            {
                Output::send(STR("Show property: {}\n"), property->GetFullName());
            }
        }
    }

    auto LiveView::render_super_struct(UStruct* ustruct) -> void
    {
        if (!ustruct)
        {
            return;
        }
        ImGui::Text("SuperStruct");
        if (ImGui::TreeNode(get_object_full_name(ustruct)))
        {
            auto next_super_struct = ustruct->GetSuperStruct();
            if (ustruct != next_super_struct)
            {
                render_super_struct(next_super_struct);
            }
            ImGui::TreePop();
        }
        else
        {
            if (ImGui::IsItemClicked())
            {
                printf_s("Clicked: %S\n", ustruct->GetFullName().c_str());
                select_object(0, ustruct->GetObjectItem(), ustruct, AffectsHistory::Yes);
            }
        }
    }

    auto LiveView::get_selected_object_or_property() -> const ObjectOrProperty&
    {
        return s_object_view_history[s_currently_selected_object_index];
    }

    auto LiveView::get_selected_object(size_t index, UseIndex use_index) -> std::pair<const FUObjectItem*, UObject*>
    {
        const auto& selected_object_or_property = s_object_view_history[use_index == UseIndex::Yes ? index : s_currently_selected_object_index];
        return std::pair{selected_object_or_property.object_item, selected_object_or_property.object};
    }

    auto LiveView::get_selected_property(size_t index, UseIndex use_index) -> FProperty*
    {
        const auto& selected_object_or_property = s_object_view_history[use_index == UseIndex::Yes ? index : s_currently_selected_object_index];
        return selected_object_or_property.property;
    }

    auto LiveView::render_property_value(FProperty* property,
                                         ContainerType container_type,
                                         void* container,
                                         FProperty** last_property_in,
                                         bool* tried_to_open_nullptr_object,
                                         bool is_watchable,
                                         int32 first_offset) -> std::variant<std::monostate, UObject*, FProperty*>
    {
        std::variant<std::monostate, UObject*, FProperty*> next_item_to_render{};
        auto property_offset = property->GetOffset_Internal();

        if (last_property_in && *last_property_in)
        {
            auto property_alignment = property->GetMinAlignment();
            auto last_property_offset = (*last_property_in)->GetOffset_Internal();
            auto last_property_size = (*last_property_in)->GetSize();
            auto last_property_offset_with_alignment = ((last_property_offset + last_property_size + (property_alignment - 1)) & ~(property_alignment - 1));
            if (last_property_offset_with_alignment != property_offset && (!property->IsA<FBoolProperty>() && !(*last_property_in)->IsA<FBoolProperty>()))
            {
                auto unreflected_size = property_offset - last_property_offset_with_alignment;
                auto unreflected_offset = property_offset - unreflected_size;
                ImGui::PushStyleColor(ImGuiCol_Text, g_imgui_text_live_view_unreflected_data_color.Value);
                ImGui::Text("0x%X: Unknown unreflected data", unreflected_offset);
                ImGui::PopStyleColor();
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Offset: 0x%X", unreflected_offset);
                    ImGui::Text("Size: 0x%X", unreflected_size);
                    ImGui::EndTooltip();
                }
            }
        }

        FString property_text{};
        auto container_ptr = property->ContainerPtrToValuePtr<void*>(container);
        property->ExportTextItem(property_text, container_ptr, container_ptr, static_cast<UObject*>(container), NULL);
        auto property_name = to_string(property->GetName());

        bool open_edit_value_popup{};

        auto render_property_value_context_menu = [&](std::string_view id_override = "") {
            if (ImGui::BeginPopupContextItem(id_override.empty() ? property_name.c_str() : fmt::format("context-menu-{}", id_override).c_str()))
            {
                if (ImGui::MenuItem("Copy name"))
                {
                    ImGui::SetClipboardText(property_name.c_str());
                }
                if (ImGui::MenuItem("Copy full name"))
                {
                    ImGui::SetClipboardText(to_string(property->GetFullName()).c_str());
                }
                if (ImGui::MenuItem("Copy value"))
                {
                    ImGui::SetClipboardText(to_string(property_text.GetCharArray()).c_str());
                }
                if (container_type == ContainerType::Object || container_type == ContainerType::Struct)
                {
                    if (ImGui::MenuItem("Edit value"))
                    {
                        open_edit_value_popup = true;
                        m_modal_edit_property_value_is_open = true;
                    }
                }

                if (is_watchable)
                {
                    auto watch_id = WatchIdentifier{container, property};
                    auto property_watcher_it = s_watch_map.find(watch_id);
                    if (property_watcher_it == s_watch_map.end())
                    {
                        ImGui::Separator();
                        if (ImGui::MenuItem("Watch value"))
                        {
                            add_watch(watch_id, static_cast<UObject*>(container), property);
                        }
                    }
                    else
                    {
                        ImGui::Checkbox("Watch value", &property_watcher_it->second->enabled);
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Go to property"))
                {
                    next_item_to_render = property;
                }

                if (property->IsA<FObjectProperty>())
                {
                    if (ImGui::MenuItem("Go to object"))
                    {
                        auto hovered_object = *property->ContainerPtrToValuePtr<UObject*>(container);

                        if (!hovered_object)
                        {
                            *tried_to_open_nullptr_object = true;
                        }
                        else
                        {
                            // Cannot go to another object in the middle of rendering properties.
                            // Doing so would cause the properties to be looked up on an instance with a property-list from another class.
                            // To fix this, we save which instance we want to go to and then we go to it at the end when we're done accessing all properties.
                            next_item_to_render = hovered_object;
                        }
                    }
                }

                ImGui::EndPopup();
            }
        };

        if (first_offset == -1)
        {
            ImGui::Text("0x%X %s:", property_offset, property_name.c_str());
        }
        else
        {
            ImGui::Text("0x%X%s %s:",
                        first_offset,
                        container_type == ContainerType::Array ? fmt::format("").c_str() : fmt::format(" (0x{:X})", property_offset).c_str(),
                        property_name.c_str());
        }
        if (auto struct_property = CastField<FStructProperty>(property); struct_property && struct_property->GetStruct()->GetFirstProperty())
        {
            ImGui::SameLine();
            auto tree_node_id = fmt::format("{}{}", static_cast<void*>(container_ptr), property_name);
            if (ImGui_TreeNodeEx(fmt::format("{}", to_string(property_text.GetCharArray())).c_str(), tree_node_id.c_str(), ImGuiTreeNodeFlags_NoAutoOpenOnLog))
            {
                render_property_value_context_menu(tree_node_id);

                for (FProperty* inner_property : struct_property->GetStruct()->ForEachProperty())
                {
                    FString struct_prop_text_item{};
                    auto struct_prop_container_ptr = inner_property->ContainerPtrToValuePtr<void*>(container_ptr);
                    inner_property->ExportTextItem(struct_prop_text_item, struct_prop_container_ptr, struct_prop_container_ptr, nullptr, NULL);

                    ImGui::Indent();
                    FProperty* last_struct_prop{};
                    next_item_to_render = render_property_value(inner_property,
                                                                ContainerType::Struct,
                                                                container_ptr,
                                                                &last_struct_prop,
                                                                tried_to_open_nullptr_object,
                                                                false,
                                                                property_offset + inner_property->GetOffset_Internal());
                    ImGui::Unindent();

                    if (!std::holds_alternative<std::monostate>(next_item_to_render))
                    {
                        break;
                    }
                }
                ImGui::TreePop();
            }
            render_property_value_context_menu(tree_node_id);
        }
        else if (auto array_property = CastField<FArrayProperty>(property); array_property)
        {
            ImGui::SameLine();
            auto tree_node_id = fmt::format("{}{}", static_cast<void*>(container_ptr), property_name);
            if (ImGui_TreeNodeEx(fmt::format("{}", to_string(property_text.GetCharArray())).c_str(), tree_node_id.c_str(), ImGuiTreeNodeFlags_NoAutoOpenOnLog))
            {
                render_property_value_context_menu(tree_node_id);

                auto script_array = std::bit_cast<FScriptArray*>(container_ptr);
                auto inner_property = array_property->GetInner();
                for (int32_t i = 0; i < script_array->Num(); ++i)
                {
                    auto element_offset = inner_property->GetElementSize() * i;
                    auto element_container_ptr = static_cast<uint8_t*>(*container_ptr) + element_offset;
                    ImGui::Text("[%i]:", i);
                    ImGui::Indent();
                    ImGui::SameLine();
                    next_item_to_render =
                            render_property_value(inner_property, ContainerType::Array, element_container_ptr, nullptr, tried_to_open_nullptr_object, false, element_offset);
                    ImGui::Unindent();

                    if (!std::holds_alternative<std::monostate>(next_item_to_render))
                    {
                        break;
                    }
                }
                if (script_array->Num() < 1)
                {
                    ImGui::Text("-- Empty --");
                }
                ImGui::TreePop();
            }
            render_property_value_context_menu(tree_node_id);
        }
        else if (property->IsA<FEnumProperty>() || (property->IsA<FByteProperty>() && static_cast<FByteProperty*>(property)->IsEnum()))
        {
            UEnum* uenum{};
            if (property->IsA<FByteProperty>())
            {
                uenum = static_cast<FByteProperty*>(property)->GetEnum();
            }
            else
            {
                uenum = static_cast<FEnumProperty*>(property)->GetEnum();
            }
            auto value_raw = *std::bit_cast<uint8*>(container_ptr);
            uint8 enum_index{};
            for (const auto& [key_value_pair, index] : uenum->ForEachName() | views::enumerate)
            {
                if (key_value_pair.Value == value_raw)
                {
                    enum_index = index;
                    break;
                }
            }
            auto value_as_string = Unreal::UKismetNodeHelperLibrary::GetEnumeratorUserFriendlyName(uenum, enum_index);
            ImGui::SameLine();
            ImGui::Text("%S", value_as_string.c_str());
            render_property_value_context_menu();
        }
        else
        {
            ImGui::SameLine();
            ImGui::Text("%S", property_text.GetCharArray());
            render_property_value_context_menu();
        }

        if (last_property_in)
        {
            *last_property_in = property;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%S", property->GetFullName().c_str());
            ImGui::Separator();
            ImGui::Text("Offset: 0x%X", property->GetOffset_Internal());
            ImGui::Text("Size: 0x%X", property->GetSize());
            ImGui::EndTooltip();
        }

        auto obj = container_type == ContainerType::Array ? *static_cast<UObject**>(container) : static_cast<UObject*>(container);
        StringType parent_name{};
        if (container_type == ContainerType::Object)
        {
            parent_name = obj ? obj->GetName() : STR("None");
        }
        auto edit_property_value_modal_name = to_string(fmt::format(STR("Edit value of property: {}->{}"), parent_name, property->GetName()));

        if (open_edit_value_popup)
        {
            ImGui::OpenPopup(edit_property_value_modal_name.c_str());
            if (!m_modal_edit_property_value_opened_this_frame)
            {
                m_modal_edit_property_value_opened_this_frame = true;
                m_current_property_value_buffer = to_string(property_text.GetCharArray());
            }
        }

        if (ImGui::BeginPopupModal(edit_property_value_modal_name.c_str(), &m_modal_edit_property_value_is_open))
        {
            ImGui::Text("Uses the same format as the 'set' UE4 console command.");
            ImGui::Text("The game could crash if the new value is invalid.");
            ImGui::Text("The game can override the new value immediately.");
            ImGui::PushItemWidth(-1.0f);
            ImGui::InputText("##CurrentPropertyValue", &m_current_property_value_buffer);
            if (ImGui::Button("Apply"))
            {
                FOutputDevice placeholder_device{};
                if (!property->ImportText(to_wstring(m_current_property_value_buffer).c_str(), property->ContainerPtrToValuePtr<void>(container), NULL, obj, &placeholder_device))
                {
                    m_modal_edit_property_value_error_unable_to_edit = true;
                    ImGui::OpenPopup("UnableToSetNewPropertyValueError");
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            if (ImGui::BeginPopupModal("UnableToSetNewPropertyValueError",
                                       &m_modal_edit_property_value_error_unable_to_edit,
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Was unable to set new value, please make sure you're using the correct format.");
                ImGui::NewLine();
                ImGui::Text("Technical details:");
                ImGui::Text("FProperty::ImportText returned NULL.");
                ImGui::EndPopup();
            }
            ImGui::EndPopup();
        }

        if (m_modal_edit_property_value_opened_this_frame)
        {
            m_modal_edit_property_value_opened_this_frame = false;
        }
        return next_item_to_render;
    }

    auto LiveView::render_enum() -> void
    {
        const auto currently_selected_object = get_selected_object();
        if (!currently_selected_object.first || !currently_selected_object.second)
        {
            return;
        }

        auto uenum = static_cast<UEnum*>(currently_selected_object.second);
        auto names = uenum->GetEnumNames();
        std::string plus = "+";
        std::string minus = "-";
        int32_t index = -1;

        if (!ImGui::BeginTable("Enum", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            return;
        }
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("FriendlyName");
        ImGui::TableSetupColumn("Value");
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (const auto name : names)
        {
            auto enum_name = name.Key.ToString();
            auto enum_friendly_name = UKismetNodeHelperLibrary::GetEnumeratorUserFriendlyName(uenum, name.Value);

            ImGui::TableNextRow();
            bool open_edit_name_popup{};
            bool open_edit_value_popup{};
            bool open_add_name_popup{};
            ++index;

            ImGui::TableNextColumn();
            ImGui::Text("%S", enum_name.c_str());
            if (ImGui::BeginPopupContextItem(to_string(fmt::format(STR("context-menu-{}"), enum_name)).c_str()))
            {
                if (ImGui::MenuItem("Copy name"))
                {
                    ImGui::SetClipboardText(to_string(enum_name).c_str());
                }
                if (ImGui::MenuItem("Edit name"))
                {
                    open_edit_name_popup = true;
                    m_modal_edit_property_value_is_open = true;
                }
                ImGui::EndPopup();
            }

            ImGui::TableNextColumn();
            ImGui::Text("%S", enum_friendly_name.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%lld", name.Value);
            if (ImGui::BeginPopupContextItem(to_string(fmt::format(STR("context-menu-{}-{}"), enum_name, name.Value)).c_str()))
            {
                if (ImGui::MenuItem("Copy value"))
                {
                    ImGui::SetClipboardText(std::to_string(name.Value).c_str());
                }
                if (ImGui::MenuItem("Edit value"))
                {
                    open_edit_value_popup = true;
                    m_modal_edit_property_value_is_open = true;
                }
                ImGui::EndPopup();
            }

            ImGui::TableNextColumn();
            ImGui::PushID(to_string(fmt::format(STR("button_add_{}"), enum_name)).c_str());
            if (ImGui::Button("+"))
            {
                open_add_name_popup = true;
                m_modal_edit_property_value_is_open = true;
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::PushID(to_string(fmt::format(STR("button_remove_{}"), enum_name)).c_str());
            if (ImGui::Button("-"))
            {
                uenum->RemoveFromNamesAt(index, 1);
            }
            ImGui::PopID();

            std::string edit_enum_name_modal_name = to_string(fmt::format(STR("Edit enum name for: {}"), name.Key.ToString()));

            std::string edit_enum_value_modal_name = to_string(fmt::format(STR("Edit enum value for: {}"), name.Key.ToString()));

            std::string add_enum_name_modal_name = to_string(fmt::format(STR("Enter new enum name after: {}"), name.Key.ToString()));

            if (open_edit_name_popup)
            {
                ImGui::OpenPopup(edit_enum_name_modal_name.c_str());
                if (!m_modal_edit_property_value_opened_this_frame)
                {
                    m_modal_edit_property_value_opened_this_frame = true;
                    m_current_property_value_buffer = to_string(enum_name);
                }
            }

            if (open_edit_value_popup)
            {
                ImGui::OpenPopup(edit_enum_value_modal_name.c_str());
                if (!m_modal_edit_property_value_opened_this_frame)
                {
                    m_modal_edit_property_value_opened_this_frame = true;
                    m_current_enum_value_buffer = name.Value;
                }
            }

            if (open_add_name_popup)
            {
                ImGui::OpenPopup(add_enum_name_modal_name.c_str());
                if (!m_modal_edit_property_value_opened_this_frame)
                {
                    m_modal_edit_property_value_opened_this_frame = true;
                    m_current_property_value_buffer = to_string(enum_name);
                }
            }

            /**
             *
             * ImGui Popup Modal for editing the names in the UEnum names array.
             *
             */
            if (ImGui::BeginPopupModal(edit_enum_name_modal_name.c_str(), &m_modal_edit_property_value_is_open))
            {
                ImGui::Text("Edit the enumerator's name.");
                ImGui::Text("The game could crash if the new name is invalid or if the old name or value is expected to be used elsewhere.");
                ImGui::Text("The game may not use this value without additional patches.");
                ImGui::PushItemWidth(-1.0f);
                ImGui::InputText("##CurrentNameValue", &m_current_property_value_buffer);
                if (ImGui::Button("Apply"))
                {
                    FOutputDevice placeholder_device{};
                    StringType new_name = to_wstring(m_current_property_value_buffer);
                    FName new_key = FName(new_name, FNAME_Add);
                    uenum->EditNameAt(index, new_key);
                    if (uenum->GetEnumNames()[index].Key.ToString() != new_name)
                    {
                        m_modal_edit_property_value_error_unable_to_edit = true;
                        ImGui::OpenPopup("UnableToSetNewEnumNameError");
                    }
                    else
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (ImGui::BeginPopupModal("UnableToSetNewEnumNameError",
                                           &m_modal_edit_property_value_error_unable_to_edit,
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Was unable to set new name.");
                    ImGui::EndPopup();
                }

                ImGui::EndPopup();
            }

            /**
             *
             * ImGui Popup Modal for editing the values in the UEnum names array.
             *
             */
            if (ImGui::BeginPopupModal(edit_enum_value_modal_name.c_str(), &m_modal_edit_property_value_is_open))
            {
                ImGui::Text("Edit the enumerator's value.");
                ImGui::Text("The game could crash if the new value is invalid or if the old name or value is expected to be used elsewhere.");
                ImGui::Text("The game may not use this value without additional patches.");
                ImGui::PushItemWidth(-1.0f);
                ImGuiDataType_ imgui_data_type = Version::IsBelow(4, 15) ? ImGuiDataType_U8 : ImGuiDataType_S64;
                ImGui::InputScalar("##CurrentNameValue", imgui_data_type, &m_current_enum_value_buffer);
                if (ImGui::Button("Apply"))
                {
                    FOutputDevice placeholder_device{};
                    int64_t new_value = m_current_enum_value_buffer;
                    uenum->EditValueAt(index, new_value);

                    if (uenum->GetEnumNames()[index].Value != new_value)
                    {
                        m_modal_edit_property_value_error_unable_to_edit = true;
                        ImGui::OpenPopup("UnableToSetNewEnumValueError");
                    }
                    else
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    m_current_enum_value_buffer = {};
                }

                if (ImGui::BeginPopupModal("UnableToSetNewEnumValueError",
                                           &m_modal_edit_property_value_error_unable_to_edit,
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Was unable to set new value.");
                    ImGui::EndPopup();
                }

                ImGui::EndPopup();
            }

            /**
             *
             * ImGui Popup Modal for adding new enumerators to the UEnum names array.
             *
             */
            if (ImGui::BeginPopupModal(add_enum_name_modal_name.c_str(), &m_modal_edit_property_value_is_open))
            {

                ImGui::Text("Enter the name of the new enumerator at the index of the selected enumerator.");
                ImGui::Text("This shifts all enumerators with a value greater than the current enumerators value up by one.");
                ImGui::Text("The game could crash if the new name is invalid or if the old name or value is expected to be used elsewhere.");
                ImGui::Text("The game may not use this value without additional patches.");
                ImGui::PushItemWidth(-1.0f);
                ImGui::InputText("##CurrentNameValue", &m_current_property_value_buffer);
                if (ImGui::Button("Apply"))
                {
                    FOutputDevice placeholder_device{};
                    StringType new_name = to_wstring(m_current_property_value_buffer);
                    FName new_key = FName(new_name, FNAME_Add);
                    int64 value = names[index].Value;

                    // NOTE: Explicitly giving specifying template params for TPair because Clang can't handle TPair being a templated using statement.
                    uenum->InsertIntoNames(TPair<decltype(new_key), decltype(value)>{new_key, value}, index, true);

                    if (uenum->GetEnumNames()[index].Key.ToString() != new_name)
                    {
                        m_modal_edit_property_value_error_unable_to_edit = true;
                        ImGui::OpenPopup("UnableToAddNewEnumNameError");
                    }
                    else
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    m_current_property_value_buffer.clear();
                }

                if (ImGui::BeginPopupModal("UnableToAddNewEnumNameError",
                                           &m_modal_edit_property_value_error_unable_to_edit,
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Was unable to insert new name.");
                    ImGui::EndPopup();
                }

                ImGui::EndPopup();
            }

            if (m_modal_edit_property_value_opened_this_frame)
            {
                m_modal_edit_property_value_opened_this_frame = false;
            }
        }
        ImGui::EndTable(); // Enum Table
    }

    auto LiveView::render_bottom_panel() -> void
    {
        const auto currently_selected_object = get_selected_object();
        if (!currently_selected_object.first || !currently_selected_object.second)
        {
            return;
        }

        if (currently_selected_object.second->IsA<UEnum>())
        {
            render_enum();
        }
        else
        {
            render_properties();
        }
    }

    auto LiveView::render_properties() -> void
    {
        const auto currently_selected_object = get_selected_object();
        if (!currently_selected_object.first || !currently_selected_object.second)
        {
            return;
        }

        bool instance_is_struct = currently_selected_object.second->IsA<UStruct>();
        auto uclass = instance_is_struct ? static_cast<UClass*>(currently_selected_object.second) : currently_selected_object.second->GetClassPrivate();

        UObject* next_object_to_render{};
        FUObjectItem* next_object_item_to_render{};
        FProperty* next_property_to_render{};

        struct OrderedProperty
        {
            int32_t offset{};
            UStruct* owner{};
            FProperty* property{};
        };
        std::vector<OrderedProperty> all_properties{};

        bool tried_to_open_nullptr_object{};

        FProperty* last_property{};

        auto render_property_text = [&](UClass* uclass, FProperty* property) {
            // New
            auto next_item_variant =
                    render_property_value(property, ContainerType::Object, currently_selected_object.second, &last_property, &tried_to_open_nullptr_object);
            if (auto object_item = std::get_if<UObject*>(&next_item_variant); object_item && *object_item)
            {
                next_object_to_render = *object_item;
                next_object_item_to_render = next_object_to_render->GetObjectItem();
            }
            else if (auto property_item = std::get_if<FProperty*>(&next_item_variant); property_item && *property_item)
            {
                next_property_to_render = *property_item;
            }

            last_property = property;
        };

        if (instance_is_struct)
        {
            // TODO: Redirect to property rendered (this is the property-value renderer even though the name says otherwise).
            return;
        }
        else
        {
            ImGui::Separator();
            for (FProperty* property : uclass->ForEachProperty())
            {
                all_properties.emplace_back(OrderedProperty{property->GetOffset_Internal(), uclass, property});
            }

            for (UStruct* super_struct : uclass->ForEachSuperStruct())
            {
                for (FProperty* property : super_struct->ForEachProperty())
                {
                    all_properties.emplace_back(OrderedProperty{property->GetOffset_Internal(), super_struct, property});
                }
            }

            std::sort(all_properties.begin(), all_properties.end(), [](const OrderedProperty& a, const OrderedProperty& b) {
                return a.offset < b.offset;
            });

            for (const auto& ordered_property : all_properties)
            {
                render_property_text(static_cast<UClass*>(ordered_property.owner), ordered_property.property);
            }

            if (tried_to_open_nullptr_object)
            {
                ImGui::OpenPopup("ObjectPropertyValueOpenNullptrWarning");
                m_modal_tried_to_open_nullptr_object_is_open = true;
            }

            if (ImGui::BeginPopupModal("ObjectPropertyValueOpenNullptrWarning", &m_modal_tried_to_open_nullptr_object_is_open, ImGuiWindowFlags_NoResize))
            {
                ImGui::Text("Cannot open ObjectProperty value because it's nullptr");
                ImGui::EndPopup();
            }

            if (next_object_to_render && next_object_item_to_render)
            {
                select_object(0, next_object_item_to_render, next_object_to_render, AffectsHistory::Yes);
            }

            if (next_property_to_render)
            {
                select_property(0, next_property_to_render, AffectsHistory::Yes);
            }
        }
    }

    auto is_player_controlled(UObject* object) -> bool
    {
        static auto IsPlayerControlled = [](UObject* pawn) -> bool {
            static auto function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, STR("/Script/Engine.Pawn:IsPlayerControlled"));
            if (!function)
            {
                return false;
            }
            struct Params
            {
                bool ReturnValue{};
            };
            Params params{};
            pawn->ProcessEvent(function, &params);
            return params.ReturnValue;
        };

        static auto pawn = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.Pawn"));
        if (!pawn)
        {
            return false;
        }

        if (object->IsA(pawn))
        {
            return IsPlayerControlled(object);
        }

        auto outer = object->GetOuterPrivate();
        while (outer)
        {
            if (outer->IsA(pawn) && IsPlayerControlled(outer))
            {
                return true;
            }
            outer = outer->GetOuterPrivate();
        }

        return false;
    }

    template <typename Stringifier, typename ObjectType>
    auto render_flags(ObjectType* generic_instance, const char* display_label) -> void
    {
        auto raw_unsafe_object_flags = Stringifier::get_raw_flags(generic_instance);
        ImGui::Text("%s (Raw): 0x%X", display_label, raw_unsafe_object_flags);
        if (ImGui::BeginPopupContextItem(Stringifier::popup_context_item_id_raw))
        {
            if (ImGui::MenuItem("Copy raw flags"))
            {
                ImGui::SetClipboardText(fmt::format("0x{:X}", static_cast<uint32_t>(raw_unsafe_object_flags)).c_str());
            }
            ImGui::EndPopup();
        }
        Stringifier flags_stringifier{generic_instance};
        size_t current_flag_line_count{};
        std::string current_flag_line{};
        std::string all_flags{};
        auto create_menu_for_copy_flags = [&](size_t menu_index) {
            if (ImGui::BeginPopupContextItem(fmt::format("{}_{}", Stringifier::popup_context_item_id, menu_index).c_str()))
            {
                if (ImGui::MenuItem("Copy flags"))
                {
                    std::string flags_string_for_copy{};
                    std::for_each(flags_stringifier.flag_parts.begin(), flags_stringifier.flag_parts.end(), [&](const std::string& flag_part) {
                        if (!flags_string_for_copy.empty())
                        {
                            flags_string_for_copy.append(" | ");
                        }
                        flags_string_for_copy.append(flag_part);
                    });
                    ImGui::SetClipboardText(flags_string_for_copy.c_str());
                }
                ImGui::EndPopup();
            }
        };
        ImGui::Text("%s:", display_label);
        create_menu_for_copy_flags(99); // 'menu_index' of '99' because we'll never reach 99 lines of flags and we can't use '0' as that'll be used in the loop below.
        ImGui::Indent();
        for (size_t i = 0; i < flags_stringifier.flag_parts.size(); ++i)
        {
            const auto& flag_part_string = flags_stringifier.flag_parts[i];
            const auto last_element_in_vector = i + 1 >= flags_stringifier.flag_parts.size();

            if (current_flag_line_count < 3)
            {
                current_flag_line.append(flag_part_string + (last_element_in_vector ? "" : " | "));
                ++current_flag_line_count;
            }

            if (current_flag_line_count >= 3 || last_element_in_vector)
            {
                ImGui::Text("%s", current_flag_line.c_str());
                create_menu_for_copy_flags(i);
                all_flags.append(current_flag_line);
                current_flag_line.clear();
                current_flag_line_count = 0;
            }
        }
        ImGui::Unindent();
    }

    auto LiveView::render_info_panel_as_object(const FUObjectItem* object_item, UObject* object) -> void
    {
        if (!object || (!object_item || object_item->IsUnreachable()))
        {
            ImGui::Text("No object selected.");
            return;
        }

        auto object_full_name = get_object_full_name(object);

        ImGui::Text("Selected: %s", to_string(object->GetName()).c_str());
        ImGui::Text("Address: %016llX", std::bit_cast<uintptr_t>(object));
        if (ImGui::BeginPopupContextItem(object_full_name))
        {
            if (ImGui::MenuItem("Copy address"))
            {
                ImGui::SetClipboardText(fmt::format("{:016X}", std::bit_cast<uintptr_t>(object)).c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::Text("ClassPrivate: %s", to_string(object->GetClassPrivate()->GetName()).c_str());
        ImGui::Text("Path: %S", object->GetPathName().c_str());
        render_flags<ObjectFlagsStringifier>(object, "ObjectFlags");
        if (auto as_class = Cast<UClass>(object); as_class)
        {
            render_flags<ClassFlagsStringifier>(as_class, "ClassFlags");
        }
        else if (auto as_function = Cast<UFunction>(object); as_function)
        {
            render_flags<FunctionFlagsStringifier>(as_function, "FunctionFlags");
        }
        ImGui::Text("Player Controlled: %s", is_player_controlled(object) ? "Yes" : "No");
        ImGui::Separator();
        // Potential sizes: 385, -180 (open) | // 385, -286 (closed)
        if (ImGui::CollapsingHeader("Size (total size of class + parents in parentheses)"))
        {
            auto uclass = object->IsA<UStruct>() ? static_cast<UClass*>(object) : object->GetClassPrivate();
            ImGui::Text("The allocated structure may be larger due to alignment");
            ImGui::Indent();
            ImGui::Text("Total: 0x%X", uclass->GetPropertiesSize());

            std::vector<UClass*> all_super_structs{};

            while (uclass)
            {
                all_super_structs.emplace_back(uclass);
                auto current_class = uclass;
                uclass = uclass->GetSuperClass();
                if (uclass == current_class)
                {
                    break;
                }
            }

            for (auto it = all_super_structs.begin(); it != all_super_structs.end(); ++it)
            {
                auto super = *it;
                auto super_size = super->GetPropertiesSize();
                auto supers_super_it = it;
                ++supers_super_it;
                if (supers_super_it != all_super_structs.end())
                {
                    auto supers_super = *supers_super_it;
                    super_size -= supers_super->GetPropertiesSize();
                }
                ImGui::Text("%S: 0x%X (0x%X)", super->GetName().c_str(), super_size, super->GetPropertiesSize());
            }

            ImGui::Unindent();
        }

        render_bottom_panel();
    }

    static auto render_fname(FName name) -> void
    {
        ImGui::Text("ComparisonIndex: 0x%X", name.GetComparisonIndex());
#ifdef WITH_CASE_PRESERVING_NAME
        ImGui::Text("DisplayIndex: 0x%X", name.GetDisplayIndex());
#endif
        ImGui::Text("Number: 0x%X", name.GetNumber());
    }

    auto LiveView::render_info_panel_as_property(FProperty* property) -> void
    {
        if (!property)
        {
            ImGui::Text("Property is nullptr.");
            return;
        }

        FProperty* next_property_to_render{};
        bool tried_to_open_nullptr_property{};
        auto property_full_name = property->GetFullName();

        ImGui::Text("Selected: %S", property->GetName().c_str());
        ImGui::Text("Address: %016llX", std::bit_cast<uintptr_t>(property));
        if (ImGui::BeginPopupContextItem(to_string(property_full_name).c_str()))
        {
            if (ImGui::MenuItem("Copy address"))
            {
                ImGui::SetClipboardText(fmt::format("{:016X}", std::bit_cast<uintptr_t>(property)).c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::Text("Class: %S", property->GetClass().GetName().c_str());
        ImGui::Text("Path: %S", property->GetPathName().c_str());

        ImGui::Separator();

        ImGui::Text("ArrayDim: %i (0x%X)", property->GetArrayDim(), property->GetArrayDim());
        ImGui::Text("ElementSize: %i (0x%X)", property->GetElementSize(), property->GetElementSize());
        auto property_flags = property->GetPropertyFlags();
        ImGui::Text("PropertyFlags (Raw): 0x%llX", property_flags);
        if (ImGui::BeginPopupContextItem("property_raw_flags_menu"))
        {
            if (ImGui::MenuItem("Copy raw flags"))
            {
                ImGui::SetClipboardText(fmt::format("0x{:X}", static_cast<uint64_t>(property_flags)).c_str());
            }
            ImGui::EndPopup();
        }
        PropertyFlagsStringifier property_flags_stringifier{property_flags};
        size_t current_flag_line_count{};
        std::string current_flag_line{};
        std::string all_flags{};
        auto create_menu_for_copy_flags = [&](size_t menu_index) {
            if (ImGui::BeginPopupContextItem(fmt::format("property_flags_menu_{}", menu_index).c_str()))
            {
                if (ImGui::MenuItem("Copy flags"))
                {
                    std::string flags_string_for_copy{};
                    std::for_each(property_flags_stringifier.flag_parts.begin(), property_flags_stringifier.flag_parts.end(), [&](const std::string& flag_part) {
                        if (!flags_string_for_copy.empty())
                        {
                            flags_string_for_copy.append(" | ");
                        }
                        flags_string_for_copy.append(std::move(flag_part));
                    });
                    ImGui::SetClipboardText(flags_string_for_copy.c_str());
                }
                ImGui::EndPopup();
            }
        };
        ImGui::Text("PropertyFlags:");
        create_menu_for_copy_flags(99); // 'menu_index' of '99' because we'll never reach 99 lines of flags and we can't use '0' as that'll be used in the loop below.
        ImGui::Indent();
        for (size_t i = 0; i < property_flags_stringifier.flag_parts.size(); ++i)
        {
            const auto& property_flag_part_string = property_flags_stringifier.flag_parts[i];
            const auto last_element_in_vector = i + 1 >= property_flags_stringifier.flag_parts.size();

            if (current_flag_line_count < 3)
            {
                current_flag_line.append(std::move(property_flag_part_string) + (last_element_in_vector ? "" : " | "));
                ++current_flag_line_count;
            }

            if (current_flag_line_count >= 3 || last_element_in_vector)
            {
                ImGui::Text("%s", current_flag_line.c_str());
                create_menu_for_copy_flags(i);
                all_flags.append(current_flag_line);
                current_flag_line.clear();
                current_flag_line_count = 0;
            }
        }
        ImGui::Unindent();
        ImGui::Text("RepIndex: %i (0x%X)", property->GetRepIndex(), property->GetRepIndex());
        ImGui::Text("OffsetInternal: %i (0x%X)", property->GetOffset_Internal(), property->GetOffset_Internal());
        ImGui::Text("RepNotifyFunc: %S", property->GetRepNotifyFunc().ToString().c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            render_fname(property->GetRepNotifyFunc());
            ImGui::EndTooltip();
        }

        auto render_property_pointer = [](std::string_view pointer_name, FProperty* property) {
            ImGui::Text("%s: %p %S", pointer_name.data(), property, property ? property->GetFullName().c_str() : STR("None"));
            return property;
        };
        int go_to_property_menu_count{};
        auto create_go_to_property_menu = [&](FProperty* goto_to_property) {
            if (ImGui::BeginPopupContextItem(fmt::format("property_link_next_menu_{}", go_to_property_menu_count++).c_str()))
            {
                if (ImGui::MenuItem("Go to property"))
                {
                    if (!goto_to_property)
                    {
                        tried_to_open_nullptr_property = true;
                    }
                    else
                    {
                        // Cannot go to another property in the middle of rendering property.
                        // Doing so would cause the property member-variables to be looked up on the wrong property.
                        // To fix this, we save which property we want to go to and then we go to it after we're done rendering.
                        next_property_to_render = goto_to_property;
                    }
                }
                ImGui::EndPopup();
            }
        };
        create_go_to_property_menu(render_property_pointer("PropertyLinkNext", property->GetPropertyLinkNext()));
        create_go_to_property_menu(render_property_pointer("NextRef", property->GetNextRef()));
        create_go_to_property_menu(render_property_pointer("DestructorLinkNext", property->GetDestructorLinkNext()));
        create_go_to_property_menu(render_property_pointer("PostConstructLinkNext", property->GetPostConstructLinkNext()));

        if (tried_to_open_nullptr_property)
        {
            ImGui::OpenPopup("PropertyOpenNullptrWarning");
            m_modal_tried_to_open_nullptr_object_is_open = true;
        }

        if (ImGui::BeginPopupModal("PropertyOpenNullptrWarning", &m_modal_tried_to_open_nullptr_object_is_open, ImGuiWindowFlags_NoResize))
        {
            ImGui::Text("Cannot go to property because it's nullptr");
            ImGui::EndPopup();
        }

        if (next_property_to_render)
        {
            select_property(0, next_property_to_render, AffectsHistory::Yes);
        }
    }

    auto LiveView::ObjectOrProperty::IsUnreachable() const -> bool
    {
        if (is_object)
        {
            return !object || object->IsUnreachable();
        }
        else
        {
            return !property;
        }
    }

    auto LiveView::ObjectOrProperty::GetFullName() const -> std::string
    {
        if (is_object)
        {
            return to_string(object->GetFullName());
        }
        else
        {
            return to_string(property->GetFullName());
        }
    }

    static auto render_history_menu(const char* str_id) -> std::pair<size_t, bool>
    {
        size_t next_item_index{};
        bool selected_an_item{};
        if (ImGui::BeginPopupContextItem(str_id))
        {
            for (size_t item_index = 0; item_index < LiveView::s_object_view_history.size(); ++item_index)
            {
                const auto& item = LiveView::s_object_view_history[item_index];

                if (item.IsUnreachable())
                {
                    ImGui::BeginDisabled();
                    ImGui::MenuItem("--- Beginning of time ---");
                    ImGui::EndDisabled();
                }
                else
                {
                    if (ImGui::MenuItem(fmt::format("{}. {}", item_index, item.GetFullName()).c_str()))
                    {
                        next_item_index = item_index;
                        selected_an_item = true;
                    }
                }
            }

            ImGui::BeginDisabled();
            ImGui::MenuItem("--- End of time ---");
            ImGui::EndDisabled();

            ImGui::EndPopup();
        }

        return {next_item_index, selected_an_item};
    }

    auto LiveView::render_info_panel() -> void
    {
        ImGui::BeginChild("LiveView_InfoPanel", {-16.0f, m_bottom_size}, true, ImGuiWindowFlags_HorizontalScrollbar);

        size_t next_object_index_to_select{};

        if (ImGui::Button(ICON_FA_ANGLE_DOUBLE_LEFT))
        {
            next_object_index_to_select = s_currently_selected_object_index;
            if (s_currently_selected_object_index > 0 && static_cast<int64_t>(s_currently_selected_object_index) - 1 > 0)
            {
                next_object_index_to_select = s_currently_selected_object_index - 1;
            }
        }
        auto selected_next_object = render_history_menu("InfoPanelHistory_Prev");
        if (selected_next_object.second)
        {
            next_object_index_to_select = selected_next_object.first;
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ANGLE_DOUBLE_RIGHT))
        {
            next_object_index_to_select = s_currently_selected_object_index;
            if (s_currently_selected_object_index + 1 < s_object_view_history.size())
            {
                next_object_index_to_select = s_currently_selected_object_index + 1;
            }
        }
        auto selected_prev_object = render_history_menu("InfoPanelHistory_Next");
        if (selected_prev_object.second)
        {
            next_object_index_to_select = selected_prev_object.first;
        }

        auto currently_selected_object = get_selected_object_or_property();

        if (UE4SSProgram::settings_manager.Experimental.GUIUFunctionCaller)
        {
            ImGui::SameLine();
            if (!currently_selected_object.is_object)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::Button(ICON_FA_SEARCH " Find functions"))
            {
                m_function_caller_widget->open_widget_deferred();
            }
            if (!currently_selected_object.is_object)
            {
                ImGui::EndDisabled();
            }
            ImGui::Separator();
        }

        if (currently_selected_object.is_object)
        {
            render_info_panel_as_object(currently_selected_object.object_item, currently_selected_object.object);
        }
        else
        {
            render_info_panel_as_property(currently_selected_object.property);
        }

        ImGui::EndChild();

        if (next_object_index_to_select > 0)
        {
            select_object(next_object_index_to_select, nullptr, nullptr, AffectsHistory::No);
        }
    }

    static auto object_search_field_always_callback(ImGuiInputTextCallbackData* data) -> int
    {
        auto typed_this = static_cast<LiveView*>(data->UserData);
        if (LiveView::s_apply_search_filters_when_not_searching)
        {
            static constexpr auto message = std::string_view{"Searching is disabled while 'Apply filters when not searching' is enabled.'"};
            strncpy_s(data->Buf, message.size() + 1, message.data(), message.size() + 1);
            data->BufTextLen = message.size();
            data->BufDirty = true;
        }
        if (typed_this->was_search_field_clear_requested() && !typed_this->was_search_field_cleared())
        {
            strncpy_s(data->Buf, 1, "", 1);
            data->BufTextLen = 0;
            data->BufDirty = true;
            typed_this->set_search_field_cleared(true);
        }
        return 1;
    }

    auto LiveView::process_property_watch(Watch& watch) -> void
    {
        FString live_value_fstring{};
        watch.property->ExportTextItem(live_value_fstring, watch.property->ContainerPtrToValuePtr<void>(watch.container), nullptr, nullptr, 0);
        auto live_value_string = StringType{live_value_fstring.GetCharArray()};

        if (watch.property_value == live_value_string)
        {
            return;
        }

        watch.property_value = std::move(live_value_string);

        const auto when_as_string = fmt::format(STR("{:%H:%M:%S}"), std::chrono::system_clock::now());
        watch.history.append(to_string(when_as_string + STR(" ") + watch.property_value + STR("\n")));

        if (watch.write_to_file)
        {
            watch.output.send(STR("{}\n"), watch.property_value);
        }
    }

    auto LiveView::process_function_pre_watch(Unreal::UnrealScriptFunctionCallableContext& context, void*) -> void
    {
        // TODO: Log params in pre-state ?
    }

    auto LiveView::process_function_post_watch(Unreal::UnrealScriptFunctionCallableContext& context, void*) -> void
    {
        if (!UnrealInitializer::StaticStorage::bIsInitialized)
        {
            return;
        }

        auto function = context.TheStack.Node();
        std::lock_guard<decltype(LiveView::Watch::s_watch_lock)> lock{LiveView::Watch::s_watch_lock};
        auto it = s_watch_containers.find(function);
        if (it == s_watch_containers.end())
        {
            return;
        }
        if (it->second.empty())
        {
            return;
        }
        auto& watch = *it->second[0];

        auto num_params = function->GetNumParms();

        const auto when_as_string = fmt::format(STR("{:%H:%M:%S}"), std::chrono::system_clock::now());
        StringType buffer{fmt::format(STR("Received call @ {}.\n"), when_as_string)};

        buffer.append(fmt::format(STR("  Context:\n    {}\n"), context.Context->GetFullName()));

        buffer.append(STR("  Locals:\n"));
        bool has_local_params{};
        for (const auto& param : function->ForEachProperty())
        {
            if (param->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
            {
                continue;
            }
            has_local_params = true;
            FString param_text{};
            auto container_ptr = param->ContainerPtrToValuePtr<void*>(context.TheStack.Locals());
            param->ExportTextItem(param_text, container_ptr, container_ptr, std::bit_cast<UObject*>(function), NULL);
            buffer.append(fmt::format(STR("    {} = {}\n"), param->GetName(), param_text.GetCharArray()));
        }
        if (!has_local_params)
        {
            buffer.append(STR("    <No Local Params>\n"));
        }

        bool has_out_params{};
        buffer.append(STR("  Out:\n"));
        for (const auto& param : function->ForEachProperty())
        {
            if (param->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                continue;
            }
            if (!param->HasAnyPropertyFlags(CPF_OutParm))
            {
                continue;
            }
            has_out_params = true;
            FString param_text{};
            auto container_ptr = FindOutParamValueAddress(context.TheStack, param);
            param->ExportTextItem(param_text, container_ptr, container_ptr, std::bit_cast<UObject*>(function), NULL);
            buffer.append(fmt::format(STR("    {} = {}\n"), param->GetName(), param_text.GetCharArray()));
        }
        if (!has_out_params)
        {
            buffer.append(STR("    <No Out Params>\n"));
        }

        buffer.append(STR("  ReturnValue\n"));
        auto return_property = function->GetReturnProperty();
        if (return_property)
        {
            FString return_property_text{};
            auto container_ptr = context.RESULT_DECL;
            return_property->ExportTextItem(return_property_text, container_ptr, container_ptr, std::bit_cast<UObject*>(function), NULL);
            buffer.append(fmt::format(STR("    {}"), return_property_text.GetCharArray()));
        }
        else
        {
            buffer.append(STR("    <No Return Value>"));
        }

        buffer.append(STR("\n\n"));
        watch.history.append(to_string(buffer));

        if (watch.write_to_file)
        {
            watch.output.send(STR("{}"), buffer);
        }
    }

    auto LiveView::process_watches() -> void
    {
        if (!UnrealInitializer::StaticStorage::bIsInitialized)
        {
            return;
        }

        std::lock_guard<decltype(Watch::s_watch_lock)> lock{Watch::s_watch_lock};
        for (auto& watch : s_watches)
        {
            if (!watch->enabled)
            {
                continue;
            }
            if (!watch->container)
            {
                continue;
            }
            if (watch->container->IsA<UFunction>())
            {
                continue;
            }

            process_property_watch(*watch);
        }
    }

    auto LiveView::render() -> void
    {
        if (!UnrealInitializer::StaticStorage::bIsInitialized)
        {
            return;
        }
        if (!m_is_initialized)
        {
            return;
        }

        if (!s_watches_loaded_from_disk)
        {
            load_watches_from_disk();
            s_watches_loaded_from_disk = true;
        }

        if (!s_filters_loaded_from_disk)
        {
            load_filters_from_disk();
            s_filters_loaded_from_disk = true;
        }

        bool listeners_allowed = are_listeners_allowed();
        if (!listeners_allowed)
        {
            ImGui::BeginDisabled();
        }

        // Update this text if corresponding button's text changes. Textinput width = Spacing + Window margin + Button padding + Button text width
        ImGui::PushItemWidth(-(8.0f + 16.0f + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::CalcTextSize(ICON_FA_COPY " Copy search result").x));
        bool push_inactive_text_color = !m_search_field_cleared;
        if (push_inactive_text_color)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, g_imgui_text_inactive_color.Value);
        }
        auto apply_search_filters_when_not_searching = s_apply_search_filters_when_not_searching;
        if (ImGui::InputText("##Search by name",
                             m_search_by_name_buffer,
                             m_search_buffer_capacity,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways,
                             &object_search_field_always_callback,
                             this))
        {
            if (!apply_search_filters_when_not_searching)
            {
                search();
            }
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::BeginTooltip();
            if (!apply_search_filters_when_not_searching)
            {
                ImGui::Text("Right-click to open search options.");
            }
            else
            {
                ImGui::Text("You can't search with 'Apply filters when not searching' enabled.");
                ImGui::Text("Right click to bring up the menu and disable this option.");
            }
            ImGui::EndTooltip();
        }
        if (push_inactive_text_color)
        {
            ImGui::PopStyleColor();
        }
        ImGui::PopItemWidth();

        if (ImGui::BeginPopupContextItem("##search-options"))
        {
            ImGui::Text("Search options");
            ImGui::SameLine();
            // Making sure the user can't enable filters when not searching, if they are currently actually searching.
            // Otherwise it uses the wrong iterator.
            auto is_searching_by_name = m_is_searching_by_name;
            if (is_searching_by_name)
            {
                ImGui::BeginDisabled();
            }
            ImGui::Checkbox("Apply filters when not searching", &s_apply_search_filters_when_not_searching);
            if (is_searching_by_name)
            {
                ImGui::EndDisabled();
            }
            if (ImGui::BeginTable("search_options_table", 2))
            {
                bool instances_only_enabled = !(Filter::NonInstancesOnly::s_enabled || Filter::DefaultObjectsOnly::s_enabled);
                bool non_instances_only_enabled = !(Filter::InstancesOnly::s_enabled || Filter::DefaultObjectsOnly::s_enabled);
                bool default_objects_only_enabled = !(Filter::NonInstancesOnly::s_enabled || Filter::InstancesOnly::s_enabled);

                //  Row 1
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Checkbox("Include inheritance", &s_include_inheritance);
                ImGui::TableNextColumn();
                if (!instances_only_enabled)
                {
                    ImGui::BeginDisabled();
                }
                ImGui::Checkbox("Instances only", &Filter::InstancesOnly::s_enabled);
                if (!instances_only_enabled)
                {
                    ImGui::EndDisabled();
                }

                // Row 2
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Checkbox("Function parameter flags", &Filter::FunctionParamFlags::s_enabled);
                ImGui::TableNextColumn();
                if (!non_instances_only_enabled)
                {
                    ImGui::BeginDisabled();
                }
                ImGui::Checkbox("Non-instances only", &Filter::NonInstancesOnly::s_enabled);
                if (!non_instances_only_enabled)
                {
                    ImGui::EndDisabled();
                }

                // Row 3
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Checkbox("Include CDOs", &Filter::IncludeDefaultObjects::s_enabled);
                ImGui::TableNextColumn();
                if (!default_objects_only_enabled)
                {
                    ImGui::BeginDisabled();
                }
                ImGui::Checkbox("CDOs only", &Filter::DefaultObjectsOnly::s_enabled);
                if (!default_objects_only_enabled)
                {
                    ImGui::EndDisabled();
                }

                // Row 4
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Checkbox("Use Regex for search", &s_use_regex_for_search);

                // Row 5
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                const char* filter_modes[] = {"Exclude class names", "Include class names"};
                int current_filter_mode = Filter::ClassNamesFilter::b_is_exclude ? 0 : 1;
                if (ImGui::Combo("##FilterMode", &current_filter_mode, filter_modes, IM_ARRAYSIZE(filter_modes)))
                {
                    Filter::ClassNamesFilter::b_is_exclude = current_filter_mode == 0;

                    // Since the filter mode changed, reparse the class names list, reparse s_internal_class_names into list_class_names using new filter mode
                    Filter::ClassNamesFilter::list_class_names.clear();
                    if (!Filter::ClassNamesFilter::s_internal_class_names.empty())
                    {
                        std::istringstream ss(Filter::ClassNamesFilter::s_internal_class_names);
                        std::string class_name;
                        while (std::getline(ss, class_name, ','))
                        {
                            std::erase_if(class_name, isspace);
                            Filter::ClassNamesFilter::list_class_names.emplace_back(to_wstring(class_name));
                        }
                    }
                }

                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputText("##ClassNamesFilter", &Filter::ClassNamesFilter::s_internal_class_names))
                {
                    Filter::ClassNamesFilter::list_class_names.clear();
                    if (!Filter::ClassNamesFilter::s_internal_class_names.empty())
                    {
                        std::istringstream ss(Filter::ClassNamesFilter::s_internal_class_names);
                        std::string class_name;
                        while (std::getline(ss, class_name, ','))
                        {
                            std::erase_if(class_name, isspace);
                            Filter::ClassNamesFilter::list_class_names.emplace_back(to_wstring(class_name));
                        }
                    }
                }

                // Row 6
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Has property");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputText("##HasProperty", &Filter::HasProperty::s_internal_properties))
                {
                    Filter::HasProperty::list_properties.clear();
                    if (!Filter::HasProperty::s_internal_properties.empty())
                    {
                        std::istringstream ss(Filter::HasProperty::s_internal_properties);
                        std::string property_name;
                        while (std::getline(ss, property_name, ','))
                        {
                            std::erase_if(property_name, isspace);
                            Filter::HasProperty::list_properties.emplace_back(to_wstring(property_name));
                        }
                    }
                }

                // Row 7
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Has property of type");
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::InputText("##HasPropertyType", &Filter::HasPropertyType::s_internal_property_types))
                {
                    Filter::HasPropertyType::list_property_types.clear();
                    if (!Filter::HasPropertyType::s_internal_property_types.empty())
                    {
                        std::istringstream ss(Filter::HasPropertyType::s_internal_property_types);
                        std::string property_type_name;
                        while (std::getline(ss, property_type_name, ','))
                        {
                            std::erase_if(property_type_name, isspace);
                            if (!property_type_name.empty())
                            {
                                Filter::HasPropertyType::list_property_types.emplace_back(FName(to_wstring(property_type_name), FNAME_Add));
                            }
                        }
                    }
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_SEARCH " Refresh search"))
                {
                    search();
                }
                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_SAVE " Save filters"))
                {
                    save_filters_to_disk();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Saves your filters to <UE4SS.dll install location>/liveview/filters.meta.json");
                    ImGui::EndTooltip();
                }

                ImGui::EndTable();
            }
            ImGui::EndPopup();
        }

        if (Filter::FunctionParamFlags::s_enabled &&
            ImGui::Begin("##search-option-function-param-flags-required", &Filter::FunctionParamFlags::s_enabled, ImGuiWindowFlags_NoCollapse))
        {
            if (ImGui::BeginTable("search_options_function_param_flags_table", 2))
            {
                static std::array s_all_property_flags{CPF_Edit,
                                                       CPF_ConstParm,
                                                       CPF_BlueprintVisible,
                                                       CPF_ExportObject,
                                                       CPF_BlueprintReadOnly,
                                                       CPF_Net,
                                                       CPF_EditFixedSize,
                                                       CPF_Parm,
                                                       CPF_OutParm,
                                                       CPF_ZeroConstructor,
                                                       CPF_ReturnParm,
                                                       CPF_DisableEditOnTemplate,
                                                       CPF_Transient,
                                                       CPF_Config,
                                                       CPF_DisableEditOnInstance,
                                                       CPF_EditConst,
                                                       CPF_GlobalConfig,
                                                       CPF_InstancedReference,
                                                       CPF_DuplicateTransient,
                                                       CPF_SubobjectReference,
                                                       CPF_SaveGame,
                                                       CPF_NoClear,
                                                       CPF_ReferenceParm,
                                                       CPF_BlueprintAssignable,
                                                       CPF_Deprecated,
                                                       CPF_IsPlainOldData,
                                                       CPF_RepSkip,
                                                       CPF_RepNotify,
                                                       CPF_Interp,
                                                       CPF_NonTransactional,
                                                       CPF_EditorOnly,
                                                       CPF_NoDestructor,
                                                       CPF_AutoWeak,
                                                       CPF_ContainsInstancedReference,
                                                       CPF_AssetRegistrySearchable,
                                                       CPF_SimpleDisplay,
                                                       CPF_AdvancedDisplay,
                                                       CPF_Protected,
                                                       CPF_BlueprintCallable,
                                                       CPF_BlueprintAuthorityOnly,
                                                       CPF_TextExportTransient,
                                                       CPF_NonPIEDuplicateTransient,
                                                       CPF_ExposeOnSpawn,
                                                       CPF_PersistentInstance,
                                                       CPF_UObjectWrapper,
                                                       CPF_HasGetValueTypeHash,
                                                       CPF_NativeAccessSpecifierPublic,
                                                       CPF_NativeAccessSpecifierProtected,
                                                       CPF_NativeAccessSpecifierPrivate,
                                                       CPF_SkipSerialization};

                static_assert(Filter::FunctionParamFlags::s_checkboxes.size() >= s_all_property_flags.size(), "The checkbox array is too small.");

                auto render_column = [](size_t i) {
                    auto property_flag_string = PropertyFlagsStringifier{s_all_property_flags[i]}.flags_string;
                    if (ImGui::Checkbox(property_flag_string.c_str(), &Filter::FunctionParamFlags::s_checkboxes[i]))
                    {
                        if (Filter::FunctionParamFlags::s_checkboxes[i])
                        {
                            Filter::FunctionParamFlags::s_value |= s_all_property_flags[i];
                        }
                        else
                        {
                            Filter::FunctionParamFlags::s_value &= ~s_all_property_flags[i];
                        }
                    }
                };

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Checkbox("Also check return property flags", &Filter::FunctionParamFlags::s_include_return_property);

                for (size_t i = 0; i < s_all_property_flags.size(); ++i)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    render_column(i);
                    ImGui::TableNextColumn();
                    render_column(++i);
                }

                ImGui::EndTable();
            }
            ImGui::End();
        }

        if (!listeners_allowed)
        {
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::BeginTooltip();
                ImGui::Text(ICON_FA_BAN " Feature disabled due to 'General.bUseUObjectArrayCache' being set to 0 in UE4SS-settings.ini.");
                ImGui::EndTooltip();
            }
        }

        if (!m_is_searching_by_name && m_search_field_clear_requested && !ImGui::IsItemActive())
        {
            strncpy_s(m_search_by_name_buffer,
                      m_default_search_buffer.size() + sizeof(char),
                      m_default_search_buffer.data(),
                      m_default_search_buffer.size() + sizeof(char));
            m_search_field_clear_requested = false;
            m_search_field_cleared = false;
        }

        if (ImGui::IsItemClicked())
        {
            if (!m_is_searching_by_name)
            {
                m_search_field_clear_requested = true;
            }
        }

        ImGui::SameLine();

        // Remember to update text width calculations for the last ImGui::PushItemWidth call if this text gets updated.
        if (ImGui::Button(ICON_FA_COPY " Copy search result"))
        {
            StringType result{};
            auto is_below_425 = Version::IsBelow(4, 25);
            for (const auto& search_result : s_name_search_results)
            {
                UE4SSProgram::dump_uobject(search_result, nullptr, result, is_below_425);
            }
            ImGui::SetClipboardText(to_string(result).c_str());
        }

        // Y - Windows title bar offset - Bottom window margin - Splitter height
        auto split_pane_height = ImGui::GetContentRegionAvail().y - 31.0f - 8.0f - 4.0f;
        if (m_bottom_size > 0 && m_bottom_size + m_top_size != split_pane_height)
        {
            // Window height changed, scale panes by ratio
            m_top_size = std::max(ImGui::GetFrameHeight(), std::round(split_pane_height * (m_top_size / (m_top_size + m_bottom_size))));
        }
        m_bottom_size = std::max(ImGui::GetFrameHeight(), split_pane_height - m_top_size);
        ImGui_Splitter(false, 4.0f, &m_top_size, &m_bottom_size, ImGui::GetFrameHeight(), ImGui::GetFrameHeight(), -16.0f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.156f, 0.156f, 0.156f, 1.0f});
        ImGui::BeginChild("LiveView_TreeView", {-16.0f, m_top_size}, true);

        auto do_iteration = [&](int32_t int_data_1 = 0, int32_t int_data_2 = 0) {
            ((*this).*((*this).m_object_iterator))(int_data_1, int_data_2, [&](UObject* object) {
                auto tree_node_name = std::string{get_object_full_name(object)};

                auto render_context_menu = [&] {
                    if (ImGui::BeginPopupContextItem(tree_node_name.c_str()))
                    {
                        if (ImGui::MenuItem(ICON_FA_COPY " Copy Full Name"))
                        {
                            Output::send(STR("Copy Full Name: {}\n"), object->GetFullName());
                            ImGui::SetClipboardText(tree_node_name.c_str());
                        }
                        if (object->IsA<UFunction>())
                        {
                            auto watch_id = WatchIdentifier{object, nullptr};
                            auto function_watcher_it = s_watch_map.find(watch_id);
                            if (function_watcher_it == s_watch_map.end())
                            {
                                ImGui::Separator();
                                if (ImGui::MenuItem(ICON_FA_EYE " Watch value"))
                                {
                                    add_watch(watch_id, static_cast<UFunction*>(object));
                                }
                            }
                            else
                            {
                                ImGui::Checkbox(ICON_FA_EYE " Watch value", &function_watcher_it->second->enabled);
                            }
                        }
                        ImGui::EndPopup();
                    }
                };

                if (ImGui_TreeNodeEx(tree_node_name.c_str(), object))
                {
                    m_currently_opened_tree_node = object;
                    m_opened_tree_nodes.emplace(object);

                    // For some reason, the menu has to be rendered both if the node is open and if it's closed.
                    // Rendering after the 'TreeNodeEx' if-statement only works if the node is closed.
                    render_context_menu();

                    if (auto as_struct = Cast<UStruct>(object); as_struct)
                    {
                        render_struct_sub_tree_hierarchy(as_struct);
                    }
                    else
                    {
                        render_object_sub_tree_hierarchy(object);
                    }

                    ImGui::TreePop();
                }
                else
                {
                    if (ImGui::IsItemClicked())
                    {
                        select_object(0, object->GetObjectItem(), object, AffectsHistory::Yes);
                    }
                }
                collapse_all_except(m_currently_opened_tree_node);
                render_context_menu();
            });
        };

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
        if (!s_apply_search_filters_when_not_searching)
        {
            ImGuiListClipper clipper{};
            clipper.Begin(m_is_searching_by_name ? s_name_search_results.size() : UObjectArray::GetNumElements(), ImGui::GetTextLineHeightWithSpacing());
            while (clipper.Step())
            {
                do_iteration(clipper.DisplayStart, clipper.DisplayEnd);
            }
        }
        else
        {
            do_iteration(0, UObjectArray::GetNumElements());
        }
        ImGui::PopStyleVar();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        render_info_panel();
        if (UE4SSProgram::settings_manager.Experimental.GUIUFunctionCaller)
        {
            const auto& selected_item = get_selected_object_or_property();
            if (selected_item.is_object)
            {
                m_function_caller_widget->render(selected_item.object);
            }
        }
    }

    static auto toggle_all_watches(bool check) -> void
    {
        std::lock_guard<decltype(LiveView::Watch::s_watch_lock)> lock{LiveView::Watch::s_watch_lock};
        for (auto& watch : LiveView::s_watches)
        {
            if (watch->container->IsA<UFunction>())
            {
                toggle_function_watch(*watch, check);
            }
            else
            {
                watch->enabled = check;
            }
        }
    }

    auto LiveView::render_watches() -> void
    {
        if (!s_watches_loaded_from_disk)
        {
            load_watches_from_disk();
            s_watches_loaded_from_disk = true;
        }

        ImGui::BeginChild("watch_render_frame", {-16.0f, -31.0f + -8.0f});

        if (ImGui::Button("All Off"))
        {
            toggle_all_watches(false);
        }
        ImGui::SameLine();
        if (ImGui::Button("All On"))
        {
            toggle_all_watches(true);
        }

        static int num_columns = 3;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {2.0f, 2.0f});
        if (ImGui::BeginTable("watch_table", num_columns, ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight() * 3.0f + 4.0f);
            ImGui::TableSetupColumn("Watch Identifier", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Save", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
            ImGui::TableHeadersRow();

            {
                std::lock_guard<decltype(LiveView::Watch::s_watch_lock)> lock{LiveView::Watch::s_watch_lock};
                for (auto& watch_ptr : s_watches)
                {
                    auto& watch = *watch_ptr;
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox(to_string(fmt::format(STR("##watch-on-off-{}"), watch.hash)).c_str(), &watch.enabled))
                    {
                        if (watch.container->IsA<UFunction>())
                        {
                            toggle_function_watch(watch, watch.enabled);
                        }
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("On/Off");
                        ImGui::EndTooltip();
                    }
                    ImGui::SameLine(0.0f, 2.0f);
                    ImGui::Checkbox(to_string(fmt::format(STR("##watch-write-to-file-{}"), watch.hash)).c_str(), &watch.write_to_file);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("Write to file");
                        ImGui::EndTooltip();
                    }
                    ImGui::SameLine(0.0f, 2.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
                    ImGui::PushID(fmt::format("collapse_history_{}", watch.hash).c_str());
                    ImGui::Selectable(watch.show_history ? ICON_FA_MINUS : ICON_FA_PLUS, &watch.show_history, ImGuiSelectableFlags_NoPadWithHalfSpacing);
                    ImGui::PopID();
                    ImGui::PopStyleVar();

                    ImGui::TableNextColumn();
                    ImGui::Text("%S.%S", watch.object_name.c_str(), watch.property_name.c_str());
                    if (watch.show_history)
                    {
                        ImGui::PushID(fmt::format("history_{}", watch.hash).c_str());
                        ImGui::InputTextMultiline("##history",
                                                  &watch.history,
                                                  {-2.0f, ImGui::GetTextLineHeight() * 10.0f + ImGui::GetStyle().FramePadding.y * 2.0f},
                                                  ImGuiInputTextFlags_ReadOnly);
                        ImGui_AutoScroll("##history", &watch.history_previous_max_scroll_y);
                        ImGui::PopID();
                    }
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox(to_string(fmt::format(STR("##watch-from-disk-{}"), watch.hash)).c_str(), &watch.load_on_startup))
                    {
                        save_watches_to_disk();
                    }
                    ImGui::SetNextWindowSize({690.0f, 0.0f});
                    if (ImGui::BeginPopupContextItem(to_string(fmt::format(STR("##watch-from-disk-settings-popup-{}"), watch.hash)).c_str()))
                    {
                        ImGui::Text("Acquisition Method");
                        ImGui::Text("This determines how the watch will be reacquired.");
                        ImGui::RadioButton("StaticFindObject",
                                           std::bit_cast<int*>(&watch.acquisition_method),
                                           static_cast<int>(Watch::AcquisitionMethod::StaticFindObject));
                        ImGui::RadioButton("FindFirstOf", std::bit_cast<int*>(&watch.acquisition_method), static_cast<int>(Watch::AcquisitionMethod::FindFirstOf));
                        save_watches_to_disk();
                        ImGui::EndPopup();
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("Auto-create on startup.\nRight-click for options.");
                        ImGui::EndTooltip();
                    }
                }
            }

            ImGui::EndTable();
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
    }
} // namespace RC::GUI
