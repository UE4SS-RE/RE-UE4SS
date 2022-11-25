#include <algorithm>
#include <string>
#include <format>
#include <unordered_map>
#include <variant>

#include <GUI/LiveView.hpp>
#include <GUI/GUI.hpp>
#include <GUI/ImGuiUtility.hpp>
#include <ExceptionHandling.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <JSON/JSON.hpp>
#include <JSON/Parser/Parser.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h>

namespace RC::GUI
{
    using namespace Unreal;

    static constexpr int s_max_elements_per_chunk = 64 * 1024;
    static constexpr int s_max_chunk_size_doubled = s_max_elements_per_chunk * 2;
    static constexpr int s_num_items_per_chunk = 1;
    static constexpr int s_chunk_id_start = -s_max_elements_per_chunk;

    static bool s_live_view_destructed = false;
    static std::unordered_map<const UObject*, std::string> s_object_ptr_to_full_name{};

    std::vector<LiveView::ObjectOrProperty> LiveView::s_object_view_history{{nullptr, nullptr, false}};
    size_t LiveView::s_currently_selected_object_index{};
    std::unordered_map<UObject*, std::vector<size_t>> LiveView::s_history_object_to_index{{nullptr, {0}}};
    std::vector<UObject*> LiveView::s_name_search_results{};
    std::unordered_set<UObject*> LiveView::s_name_search_results_set{};
    std::string LiveView::s_name_to_search_by{};
    std::vector<LiveView::Watch> LiveView::s_watches{};
    std::unordered_map<LiveView::WatchIdentifier, LiveView::Watch*> LiveView::s_watch_map;
    std::unordered_map<void*, std::vector<LiveView::Watch*>> LiveView::s_watch_containers{};
    bool LiveView::s_create_listener_removed{};
    bool LiveView::s_delete_listener_removed{};
    bool LiveView::s_selected_item_deleted{};
    bool LiveView::s_need_to_filter_out_properties{};
    bool LiveView::s_watches_loaded_from_disk{};

    static auto get_object_full_name_cxx_string(UObject* object) -> std::string;

    static auto attempt_to_add_search_result(UObject* object) -> void
    {
        // TODO: Stop using the 'HashObject' function when needing the address of an FFieldClassVariant because it's not designed to return an address.
        //       Maybe make the ToFieldClass/ToUClass functions public (append 'Unsafe' to the function names).
        if (LiveView::s_need_to_filter_out_properties && object->IsA(std::bit_cast<UClass*>(FProperty::StaticClass().HashObject())))
        {
            return;
        }

        if (!LiveView::s_name_search_results_set.contains(object))
        {
            auto object_full_name = get_object_full_name_cxx_string(object);
            std::transform(object_full_name.begin(), object_full_name.end(), object_full_name.begin(), [](char c) {
                return std::tolower(c);
            });

            auto name_to_search_by = LiveView::s_name_to_search_by;
            std::transform(name_to_search_by.begin(), name_to_search_by.end(), name_to_search_by.begin(), [](char c) {
                return std::tolower(c);
            });

            if (object_full_name.find(name_to_search_by) != object_full_name.npos)
            {
                LiveView::s_name_search_results.emplace_back(object);
                LiveView::s_name_search_results_set.emplace(object);
            }
        }
    }

    static auto remove_search_result(UObject* object) -> void
    {
        LiveView::s_name_search_results.erase(std::remove_if(LiveView::s_name_search_results.begin(), LiveView::s_name_search_results.end(), [&](const auto& item) {
            return item == object;
        }), LiveView::s_name_search_results.end());

        LiveView::s_name_search_results_set.erase(object);

        auto watch_it = LiveView::s_watch_containers.find(object);
        if (watch_it != LiveView::s_watch_containers.end())
        {
            LiveView::s_watches.erase(std::remove_if(LiveView::s_watches.begin(), LiveView::s_watches.end(), [&](const auto& item) {
                if (item.container == object)
                {
                    LiveView::s_watch_map.erase(LiveView::s_watch_map.find({item.container, item.property}));
                    return true;
                }
                else
                {
                    return false;
                }
            }), LiveView::s_watches.end());
            LiveView::s_watch_containers.erase(watch_it);
        }
    }

    struct FLiveViewCreateListener : public FUObjectCreateListener
    {
        static FLiveViewCreateListener LiveViewCreateListener;

        void NotifyUObjectCreated(const UObjectBase* object, int32 index) override
        {
            if (s_live_view_destructed) { return; }
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
            if (s_live_view_destructed) { return; }
            if (LiveView::s_history_object_to_index.size() <= 1) { return; }

            auto as_uobject = std::bit_cast<UObject*>(object);
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

            remove_search_result(as_uobject);
        }

        void OnUObjectArrayShutdown() override
        {
            UObjectArray::RemoveUObjectDeleteListener(this);
            LiveView::s_delete_listener_removed = true;
        }
    };
    FLiveViewDeleteListener FLiveViewDeleteListener::LiveViewDeleteListener{};

    static auto add_watch(const LiveView::WatchIdentifier& watch_id, UObject* object, FProperty* property) -> LiveView::Watch&
    {
        auto& watch = LiveView::s_watches.emplace_back(LiveView::Watch{
                object->GetOuterPrivate()->GetName() + STR(".") + object->GetName(),
                property->GetName()
        });
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

    static auto serialize_watch_to_json_object(const LiveView::Watch& watch) -> std::unique_ptr<JSON::Object>
    {
        auto json_object = std::make_unique<JSON::Object>();
        switch (watch.acquisition_method)
        {
            case LiveView::Watch::AcquisitionMethod::StaticFindObject:
            {
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
        return json_object;
    }

    static auto internal_load_watches_from_disk() -> void
    {
        auto json_file = File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + std::format(STR("\\watches\\watches.meta.json")),
                                    File::OpenFor::Reading,
                                    File::OverwriteExistingFile::No,
                                    File::CreateIfNonExistent::Yes);
        auto json_file_contents = json_file.read_all();
        if (json_file_contents.empty()) { return; }

        auto json_global_object = JSON::Parser::parse(json_file_contents);
        const auto& elements = json_global_object->get<JSON::Array>(STR("Watches"));
        elements.for_each([](JSON::Value& element) {
            auto& json_watch_object = *element.as<JSON::Object>();
            auto acquisition_id = json_watch_object.get<JSON::String>(STR("AcquisitionID")).get_view();
            auto property_name = json_watch_object.get<JSON::String>(STR("PropertyName")).get_view();
            auto acquisition_method = static_cast<LiveView::Watch::AcquisitionMethod>(json_watch_object.get<JSON::Number>(STR("AcquisitionMethod")).get<int64_t>());

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
            if (!object) { return LoopAction::Continue; }

            auto property = object->GetPropertyByNameInChain(property_name.data());
            if (!property) { return LoopAction::Continue; }

            auto& watch = add_watch(object, property);
            watch.load_on_startup = true;
            watch.acquisition_method = acquisition_method;

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

        for (const auto& watch : LiveView::s_watches)
        {
            if (!watch.load_on_startup) { continue; }
            json_uobjects.add_object(serialize_watch_to_json_object(watch));
        }

        auto json_file = File::open(StringType{UE4SSProgram::get_program().get_working_directory()} + std::format(STR("\\watches\\watches.meta.json")),
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
        if (m_listeners_set || !are_listeners_allowed()) { return; }
        m_listeners_set = true;
        UObjectArray::AddUObjectCreateListener(&FLiveViewCreateListener::LiveViewCreateListener);
        UObjectArray::AddUObjectDeleteListener(&FLiveViewDeleteListener::LiveViewDeleteListener);
    }

    auto LiveView::unset_listeners() -> void
    {
        if (!m_listeners_set || !are_listeners_allowed()) { return; }
        m_listeners_set = false;
        UObjectArray::RemoveUObjectCreateListener(&FLiveViewCreateListener::LiveViewCreateListener);
        UObjectArray::RemoveUObjectDeleteListener(&FLiveViewDeleteListener::LiveViewDeleteListener);
    }

    LiveView::Watch::Watch(StringType&& object_name, StringType&& property_name) : object_name(object_name), property_name(property_name)
    {
        auto& file_device = output.get_device<Output::FileDevice>();
        file_device.set_file_name_and_path(StringType{UE4SSProgram::get_program().get_working_directory()} + std::format(STR("\\watches\\ue4ss_watch_{}_{}.txt"), object_name, property_name));
        file_device.set_formatter([](File::StringViewType string) -> File::StringType {
            const auto when_as_string = std::format(STR("{:%Y-%m-%d %H:%M:%S}"), std::chrono::system_clock::now());
            return std::format(STR("[{}] {}"), when_as_string, string);
        });
    }

    auto LiveView::initialize() -> void
    {
        s_need_to_filter_out_properties = Version::IsBelow(4, 25);
        m_is_initialized = true;
    }

    struct PropertyFlagsStringifier
    {
        std::string flags_string{};
        std::vector<std::string> flag_parts{};

        PropertyFlagsStringifier(EPropertyFlags flags)
        {
            if ((flags & CPF_Edit) != 0) { flag_parts.emplace_back("CPF_Edit"); }
            if ((flags & CPF_ConstParm) != 0) { flag_parts.emplace_back("CPF_ConstParm"); }
            if ((flags & CPF_BlueprintVisible) != 0) { flag_parts.emplace_back("CPF_BlueprintVisible"); }
            if ((flags & CPF_ExportObject) != 0) { flag_parts.emplace_back("CPF_ExportObject"); }
            if ((flags & CPF_BlueprintReadOnly) != 0) { flag_parts.emplace_back("CPF_BlueprintReadOnly"); }
            if ((flags & CPF_Net) != 0) { flag_parts.emplace_back("CPF_Net"); }
            if ((flags & CPF_EditFixedSize) != 0) { flag_parts.emplace_back("CPF_EditFixedSize"); }
            if ((flags & CPF_Parm) != 0) { flag_parts.emplace_back("CPF_Parm"); }
            if ((flags & CPF_OutParm) != 0) { flag_parts.emplace_back("CPF_OutParm"); }
            if ((flags & CPF_ZeroConstructor) != 0) { flag_parts.emplace_back("CPF_ZeroConstructor"); }
            if ((flags & CPF_ReturnParm) != 0) { flag_parts.emplace_back("CPF_ReturnParm"); }
            if ((flags & CPF_DisableEditOnTemplate) != 0) { flag_parts.emplace_back("CPF_DisableEditOnTemplate"); }
            if ((flags & CPF_Transient) != 0) { flag_parts.emplace_back("CPF_Transient"); }
            if ((flags & CPF_Config) != 0) { flag_parts.emplace_back("CPF_Config"); }
            if ((flags & CPF_DisableEditOnInstance) != 0) { flag_parts.emplace_back("CPF_DisableEditOnInstance"); }
            if ((flags & CPF_EditConst) != 0) { flag_parts.emplace_back("CPF_EditConst"); }
            if ((flags & CPF_GlobalConfig) != 0) { flag_parts.emplace_back("CPF_GlobalConfig"); }
            if ((flags & CPF_InstancedReference) != 0) { flag_parts.emplace_back("CPF_InstancedReference"); }
            if ((flags & CPF_DuplicateTransient) != 0) { flag_parts.emplace_back("CPF_DuplicateTransient"); }
            if ((flags & CPF_SubobjectReference) != 0) { flag_parts.emplace_back("CPF_SubobjectReference"); }
            if ((flags & CPF_SaveGame) != 0) { flag_parts.emplace_back("CPF_SaveGame"); }
            if ((flags & CPF_NoClear) != 0) { flag_parts.emplace_back("CPF_NoClear"); }
            if ((flags & CPF_ReferenceParm) != 0) { flag_parts.emplace_back("CPF_ReferenceParm"); }
            if ((flags & CPF_BlueprintAssignable) != 0) { flag_parts.emplace_back("CPF_BlueprintAssignable"); }
            if ((flags & CPF_Deprecated) != 0) { flag_parts.emplace_back("CPF_Deprecated"); }
            if ((flags & CPF_IsPlainOldData) != 0) { flag_parts.emplace_back("CPF_IsPlainOldData"); }
            if ((flags & CPF_RepSkip) != 0) { flag_parts.emplace_back("CPF_RepSkip"); }
            if ((flags & CPF_RepNotify) != 0) { flag_parts.emplace_back("CPF_RepNotify"); }
            if ((flags & CPF_Interp) != 0) { flag_parts.emplace_back("CPF_Interp"); }
            if ((flags & CPF_NonTransactional) != 0) { flag_parts.emplace_back("CPF_NonTransactional"); }
            if ((flags & CPF_EditorOnly) != 0) { flag_parts.emplace_back("CPF_EditorOnly"); }
            if ((flags & CPF_NoDestructor) != 0) { flag_parts.emplace_back("CPF_NoDestructor"); }
            if ((flags & CPF_AutoWeak) != 0) { flag_parts.emplace_back("CPF_AutoWeak"); }
            if ((flags & CPF_ContainsInstancedReference) != 0) { flag_parts.emplace_back("CPF_ContainsInstancedReference"); }
            if ((flags & CPF_AssetRegistrySearchable) != 0) { flag_parts.emplace_back("CPF_AssetRegistrySearchable"); }
            if ((flags & CPF_SimpleDisplay) != 0) { flag_parts.emplace_back("CPF_SimpleDisplay"); }
            if ((flags & CPF_AdvancedDisplay) != 0) { flag_parts.emplace_back("CPF_AdvancedDisplay"); }
            if ((flags & CPF_Protected) != 0) { flag_parts.emplace_back("CPF_Protected"); }
            if ((flags & CPF_BlueprintCallable) != 0) { flag_parts.emplace_back("CPF_BlueprintCallable"); }
            if ((flags & CPF_BlueprintAuthorityOnly) != 0) { flag_parts.emplace_back("CPF_BlueprintAuthorityOnly"); }
            if ((flags & CPF_TextExportTransient) != 0) { flag_parts.emplace_back("CPF_TextExportTransient"); }
            if ((flags & CPF_NonPIEDuplicateTransient) != 0) { flag_parts.emplace_back("CPF_NonPIEDuplicateTransient"); }
            if ((flags & CPF_ExposeOnSpawn) != 0) { flag_parts.emplace_back("CPF_ExposeOnSpawn"); }
            if ((flags & CPF_PersistentInstance) != 0) { flag_parts.emplace_back("CPF_PersistentInstance"); }
            if ((flags & CPF_UObjectWrapper) != 0) { flag_parts.emplace_back("CPF_UObjectWrapper"); }
            if ((flags & CPF_HasGetValueTypeHash) != 0) { flag_parts.emplace_back("CPF_HasGetValueTypeHash"); }
            if ((flags & CPF_NativeAccessSpecifierPublic) != 0) { flag_parts.emplace_back("CPF_NativeAccessSpecifierPublic"); }
            if ((flags & CPF_NativeAccessSpecifierProtected) != 0) { flag_parts.emplace_back("CPF_NativeAccessSpecifierProtected"); }
            if ((flags & CPF_NativeAccessSpecifierPrivate) != 0) { flag_parts.emplace_back("CPF_NativeAccessSpecifierPrivate"); }
            if ((flags & CPF_SkipSerialization) != 0) { flag_parts.emplace_back("CPF_SkipSerialization"); }

            std::for_each(flag_parts.begin(), flag_parts.end(), [&](const std::string& flag_part) {
                if (!flags_string.empty())
                {
                    flags_string.append(", ");
                }
                flags_string.append(std::move(flag_part));
            });
        }
    };

    LiveView::LiveView()
    {
        m_search_by_name_buffer = new char[m_search_buffer_capacity];
        strncpy_s(m_search_by_name_buffer, m_default_search_buffer.size() + sizeof(char), m_default_search_buffer.data(), m_default_search_buffer.size() + sizeof(char));
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
            auto[history_object, did_emplace] = s_history_object_to_index.emplace(object, std::vector<size_t>{});
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
        if (!UnrealInitializer::StaticStorage::bIsInitialized) { return ""; }
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
        if (!UnrealInitializer::StaticStorage::bIsInitialized) { return ""; }
        if (auto it = s_object_ptr_to_full_name.find(object); it != s_object_ptr_to_full_name.end())
        {
            return it->second;
        }
        else
        {
            return s_object_ptr_to_full_name.emplace(object, to_string(object->GetFullName())).first->second;
        }
    }

    auto LiveView::guobjectarray_by_name_iterator([[maybe_unused]]int32_t int_data_1, [[maybe_unused]]int32_t int_data_2, const std::function<void(UObject*)>& callable) -> void
    {
        for (const auto& object : s_name_search_results)
        {
            callable(object);
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

    static auto ImGui_GetID(int int_id) -> ImGuiID
    {
        ImGuiWindow* window = GImGui->CurrentWindow;
        return window->GetID(int_id);
    }

    static auto ImGui_TreeNodeEx(const char* label, int int_id, ImGuiTreeNodeFlags flags = 0) -> bool
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        return ImGui::TreeNodeBehavior(window->GetID(int_id), flags, label, NULL);
    }

    static auto ImGui_TreeNodeEx(const char* label, void* ptr_id, ImGuiTreeNodeFlags flags = 0) -> bool
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        return ImGui::TreeNodeBehavior(window->GetID(ptr_id), flags, label, NULL);
    }

    static auto ImGui_TreeNodeEx(const char* label, const char* str_id, ImGuiTreeNodeFlags flags) -> bool
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        return ImGui::TreeNodeBehavior(window->GetID(str_id), flags, label, NULL);
    }

    static auto collapse_all_except(int except_id) -> void
    {
        for (int i = 0; i < UObjectArray::GetNumChunks(); ++i)
        {
            if (i + s_chunk_id_start == except_id) { continue; }
            ImGui::GetStateStorage()->SetInt(ImGui_GetID(i + s_chunk_id_start), 0);
        }
    }

    auto LiveView::collapse_all_except(void* except_id) -> void
    {
        // We don't need to do anything if we only have one node open.
        if (m_opened_tree_nodes.size() == 1) { return; }

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

    auto LiveView::render_object_sub_tree_hierarchy(UObject* object) -> void
    {
        if (!object) { return; }

        auto uclass = object->GetClassPrivate();
        ImGui::Text("%s", get_object_full_name(uclass));
        render_struct_sub_tree_hierarchy(uclass);
    }

    auto LiveView::render_struct_sub_tree_hierarchy(UStruct* ustruct) -> void
    {
        if (!ustruct) { return; }

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
            ustruct->ForEachProperty([&](FProperty* property) {
                ImGui::TreeNodeEx(to_string(property->GetFullName()).c_str(), ImGuiTreeNodeFlags_Leaf);
                if (ImGui::IsItemClicked())
                {
                    select_property(0, property, AffectsHistory::Yes);
                }
                ImGui::TreePop();

                return LoopAction::Continue;
            });
            ImGui::TreePop();
        }
        ImGui::Unindent();
    }

    auto LiveView::render_class(UClass* uclass) -> void
    {
        if (!uclass) { return; }
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
        uclass->ForEachProperty([](FProperty* property) {
            if (ImGui::TreeNode(to_string(property->GetFullName()).c_str()))
            {
                Output::send(STR("Show property: {}\n"), property->GetFullName());
            }

            return LoopAction::Continue;
        });
    }

    auto LiveView::render_super_struct(UStruct* ustruct) -> void
    {
        if (!ustruct) { return; }
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

    static auto render_property_value(FProperty* property, void* container, FProperty** last_property_in, bool* tried_to_open_nullptr_object, bool is_watchable = true, int32 first_offset = -1) -> std::variant<std::monostate, UObject*, FProperty*>
    {
        std::variant<std::monostate, UObject*, FProperty*> next_item_to_render{};
        auto property_offset = property->GetOffset_Internal();

        if (*last_property_in)
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

        auto render_property_value_context_menu = [&]() {
            if (ImGui::BeginPopupContextItem(property_name.c_str()))
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

                if (is_watchable)
                {
                    auto watch_id = LiveView::WatchIdentifier{container, property};
                    auto property_watcher_it = LiveView::s_watch_map.find(watch_id);
                    if (property_watcher_it == LiveView::s_watch_map.end())
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
            ImGui::Text("0x%X (0x%X) %s:", first_offset, property_offset, property_name.c_str());
        }
        if (auto struct_property = CastField<FStructProperty>(property); struct_property && struct_property->GetStruct()->GetFirstProperty())
        {
            is_watchable = false;
            ImGui::SameLine();
            auto tree_node_id = std::format("{}{}", static_cast<void*>(container_ptr), property_name);
            if (ImGui_TreeNodeEx(std::format("{}", to_string(property_text.GetCharArray())).c_str(), tree_node_id.c_str(), ImGuiTreeNodeFlags_NoAutoOpenOnLog))
            {
                render_property_value_context_menu();
                
                struct_property->GetStruct()->ForEachProperty([&](FProperty* inner_property) {
                    FString struct_prop_text_item{};
                    auto struct_prop_container_ptr = inner_property->ContainerPtrToValuePtr<void*>(container_ptr);
                    inner_property->ExportTextItem(struct_prop_text_item, struct_prop_container_ptr, struct_prop_container_ptr, static_cast<UObject*>(*container_ptr), NULL);
                    
                    ImGui::Indent();
                    FProperty* last_struct_prop{};
                    next_item_to_render = render_property_value(inner_property, container_ptr, &last_struct_prop, tried_to_open_nullptr_object, false, property_offset + inner_property->GetOffset_Internal());
                    ImGui::Unindent();
                    
                    if (std::holds_alternative<std::monostate>(next_item_to_render))
                    {
                        return LoopAction::Continue;
                    }
                    else
                    {
                        return LoopAction::Break;
                    }
                });
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::SameLine();
            ImGui::Text("%S", property_text.GetCharArray());
        }

        *last_property_in = property;

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%S", property->GetFullName().c_str());
            ImGui::Separator();
            ImGui::Text("Offset: 0x%X", property->GetOffset_Internal());
            ImGui::Text("Size: 0x%X", property->GetSize());
            ImGui::EndTooltip();
        }

        render_property_value_context_menu();

        return next_item_to_render;
    }

    auto LiveView::render_properties() -> void
    {
        const auto currently_selected_object = get_selected_object();
        if (!currently_selected_object.first || !currently_selected_object.second) { return; }

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
            auto next_item_variant = render_property_value(property, currently_selected_object.second, &last_property, &tried_to_open_nullptr_object);
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
            uclass->ForEachProperty([&](FProperty* property) {
                all_properties.emplace_back(OrderedProperty{property->GetOffset_Internal(), uclass, property});
                return LoopAction::Continue;
            });

            uclass->ForEachSuperStruct([&](UStruct* super_struct) {
                super_struct->ForEachProperty([&](FProperty* property) {
                    all_properties.emplace_back(OrderedProperty{property->GetOffset_Internal(), super_struct, property});
                    return LoopAction::Continue;
                });
                return LoopAction::Continue;
            });

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
                ImGui::SetClipboardText(std::format("{:016X}", std::bit_cast<uintptr_t>(object)).c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::Text("ClassPrivate: %s", to_string(object->GetClassPrivate()->GetName()).c_str());
        ImGui::Text("Path: %S", object->GetPathName().c_str());
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
                if (uclass == current_class) { break; }
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

        render_properties();
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
                ImGui::SetClipboardText(std::format("{:016X}", std::bit_cast<uintptr_t>(property)).c_str());
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
                ImGui::SetClipboardText(std::format("0x{:X}", static_cast<uint64_t>(property_flags)).c_str());
            }
            ImGui::EndPopup();
        }
        PropertyFlagsStringifier property_flags_stringifier{property_flags};
        size_t current_flag_line_count{};
        std::string current_flag_line{};
        std::string all_flags{};
        auto create_menu_for_copy_flags = [&](size_t menu_index) {
            if (ImGui::BeginPopupContextItem(std::format("property_flags_menu_{}", menu_index).c_str()))
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
            if (ImGui::BeginPopupContextItem(std::format("property_link_next_menu_{}", go_to_property_menu_count++).c_str()))
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

    auto LiveView::render_info_panel() -> void
    {
        ImGui::BeginChild("LiveView_InfoPanel", {-14.0f, m_bottom_size}, true, ImGuiWindowFlags_HorizontalScrollbar);

        bool should_select_object{};
        bool select_previous_object{};

        if (ImGui::Button("<<<"))
        {
            should_select_object = true;
            select_previous_object = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(">>>"))
        {
            should_select_object = true;
            select_previous_object = false;
        }

        ImGui::Separator();

        auto currently_selected_object = get_selected_object_or_property();
        if (currently_selected_object.is_object)
        {
            render_info_panel_as_object(currently_selected_object.object_item, currently_selected_object.object);
        }
        else
        {
            render_info_panel_as_property(currently_selected_object.property);
        }

        ImGui::EndChild();

        if (should_select_object)
        {
            if (select_previous_object)
            {
                if (s_currently_selected_object_index <= 0 || s_currently_selected_object_index - 1 <= 0) { return; }
                select_object(s_currently_selected_object_index - 1, nullptr, nullptr, AffectsHistory::No);
            }
            else
            {
                if (s_currently_selected_object_index + 1 >= s_object_view_history.size()) { return; }
                select_object(s_currently_selected_object_index + 1, nullptr, nullptr, AffectsHistory::No);
            }
        }
    }

    static auto search_field_always_callback(ImGuiInputTextCallbackData* data) -> int
    {
        auto typed_this = static_cast<LiveView*>(data->UserData);
        if (typed_this->was_search_field_clear_requested() && !typed_this->was_search_field_cleared())
        {
            strncpy_s(data->Buf, 1, "", 1);
            data->BufTextLen = 0;
            data->BufDirty = true;
            typed_this->set_search_field_cleared(true);
        }
        return 1;
    }

    auto LiveView::process_watches() -> void
    {
        for (auto& watch : s_watches)
        {
            if (!watch.enabled) { continue; }

            FString live_value_fstring{};
            watch.property->ExportTextItem(live_value_fstring, watch.property->ContainerPtrToValuePtr<void>(watch.container), nullptr, nullptr, 0);
            auto live_value_string = StringType{live_value_fstring.GetCharArray()};

            if (watch.property_value == live_value_string) { continue; }

            watch.property_value = std::move(live_value_string);

            const auto when_as_string = std::format(STR("{:%H:%M:%S}"), std::chrono::system_clock::now());
            watch.history.append(to_string(when_as_string + STR(" ") + watch.property_value + STR("\n")));

            if (watch.write_to_file)
            {
                watch.output.send(STR("{}\n"), watch.property_value);
            }
        }
    }

    auto LiveView::render() -> void
    {
        if (!UnrealInitializer::StaticStorage::bIsInitialized) { return; }
        if (!m_is_initialized) { return; }

        if (!s_watches_loaded_from_disk)
        {
            load_watches_from_disk();
            s_watches_loaded_from_disk = true;
        }

        bool listeners_allowed = are_listeners_allowed();
        if (!listeners_allowed) { ImGui::BeginDisabled(); }
        ImGui::PushItemWidth(-14.0f);
        bool push_inactive_text_color = !m_search_field_cleared;
        if (push_inactive_text_color) { ImGui::PushStyleColor(ImGuiCol_Text, g_imgui_text_inactive_color.Value); }
        if (ImGui::InputText("##Search by name", m_search_by_name_buffer, m_search_buffer_capacity, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways, &search_field_always_callback, this))
        {
            if (listeners_allowed)
            {
                std::string search_buffer{m_search_by_name_buffer};
                if (search_buffer.empty())
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
        if (push_inactive_text_color) { ImGui::PopStyleColor(); }
        ImGui::PopItemWidth();
        if (!listeners_allowed)
        {
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::BeginTooltip();
                ImGui::Text("Feature disabled due to 'General.UseUObjectArrayCache' being set to 0 in UE4SS-settings.ini.");
                ImGui::EndTooltip();
            }
        }

        if (!m_is_searching_by_name && m_search_field_clear_requested && !ImGui::IsItemActive())
        {
            strncpy_s(m_search_by_name_buffer, m_default_search_buffer.size() + sizeof(char), m_default_search_buffer.data(), m_default_search_buffer.size() + sizeof(char));
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

        m_bottom_size = (ImGui::GetContentRegionMaxAbs().y - m_top_size) - 94.0f;
        ImGui_Splitter(false, 4.0f, &m_top_size, &m_bottom_size, 32.0f, 32.0f, -14.0f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.156f, 0.156f, 0.156f, 1.0f});
        ImGui::BeginChild("LiveView_TreeView", {-14.0f, m_top_size}, true);

        auto do_iteration = [&](int32_t int_data_1 = 0, int32_t int_data_2 = 0) {
            ((*this).*((*this).m_object_iterator))(int_data_1, int_data_2, [&](UObject* object) {
                auto tree_node_name = std::string{get_object_full_name(object)};

                auto render_context_menu = [&]{
                    if (ImGui::BeginPopupContextItem(tree_node_name.c_str()))
                    {
                        if (ImGui::MenuItem("Copy Full Name"))
                        {
                            Output::send(STR("Copy Full Name: {}\n"), object->GetFullName());
                            ImGui::SetClipboardText(tree_node_name.c_str());
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
        if (m_is_searching_by_name)
        {
            do_iteration();
        }
        else
        {
            auto num_elements = UObjectArray::GetNumElements();
            int num_total_chunks = (num_elements / s_max_elements_per_chunk) + (num_elements % s_max_elements_per_chunk == 0 ? 0 : 1);
            for (int i = 0; i < num_total_chunks; ++i)
            {
                if (ImGui_TreeNodeEx(std::format("Chunk #{}", i).c_str(), i + s_chunk_id_start, ImGuiTreeNodeFlags_CollapsingHeader))
                {
                    ::RC::GUI::collapse_all_except(i + s_chunk_id_start);
                    auto start = s_max_elements_per_chunk * i;
                    auto end = start + s_max_elements_per_chunk;
                    do_iteration(start, end);
                }
            }
        }
        ImGui::PopStyleVar();

        ImGui::EndChild();
        ImGui::PopStyleColor();

        render_info_panel();
    }

    static auto toggle_all_watches(bool check) -> void
    {
        for (auto& watch : LiveView::s_watches)
        {
            watch.enabled = check;
        }
    }

    auto LiveView::render_watches() -> void
    {
        if (!s_watches_loaded_from_disk)
        {
            load_watches_from_disk();
            s_watches_loaded_from_disk = true;
        }

        ImGui::BeginChild("watch_render_frame", {-13.0f, -35.0f});

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
        if (ImGui::BeginTable("watch_table", num_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_NoPadOuterX))
        {
            ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Watch Identifier", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##watch-from-disk", ImGuiTableColumnFlags_WidthFixed, 21.0f);
            ImGui::TableHeadersRow();

            for (auto& watch : s_watches)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Checkbox(to_string(std::format(STR("##watch-on-off-{}"), watch.hash)).c_str(), &watch.enabled);
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("On/Off");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine(0.0f, 2.0f);
                ImGui::Checkbox(to_string(std::format(STR("##watch-write-to-file-{}"), watch.hash)).c_str(), &watch.write_to_file);
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Write to file");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine(0.0f, 2.0f);
                if (!watch.show_history)
                {
                    ImGui::PushID(std::format("button_open_history_{}", watch.hash).c_str());
                    if (ImGui::Button("+", {20.0f, 0.0f}))
                    {
                        watch.show_history = true;
                    }
                    ImGui::PopID();
                }
                else
                {
                    ImGui::PushID(std::format("button_close_history_{}", watch.hash).c_str());
                    if (ImGui::Button("-", {20.0f, 0.0f}))
                    {
                        watch.show_history = false;
                    }
                    ImGui::PopID();
                }
                ImGui::TableNextColumn();
                ImGui::Text("%S.%S", watch.object_name.c_str(), watch.property_name.c_str());
                if (watch.show_history)
                {
                    ImGui::PushID(std::format("history_{}", watch.hash).c_str());
                    ImGui::InputTextMultiline("##history", &watch.history, {-13.0f, 0.0f}, ImGuiInputTextFlags_ReadOnly);
                    ImGui_AutoScroll("##history", &watch.history_previous_max_scroll_y);
                    ImGui::PopID();
                }
                ImGui::TableNextColumn();
                if (ImGui::Checkbox(to_string(std::format(STR("##watch-from-disk-{}"), watch.hash)).c_str(), &watch.load_on_startup))
                {
                    save_watches_to_disk();
                }
                ImGui::SetNextWindowSize({690.0f, 0.0f});
                if (ImGui::BeginPopupContextItem(to_string(std::format(STR("##watch-from-disk-settings-popup-{}"), watch.hash)).c_str()))
                {
                    ImGui::Text("Acquisition Method");
                    ImGui::Text("This determines how the watch will be reacquired.");
                    ImGui::RadioButton("StaticFindObject", std::bit_cast<int*>(&watch.acquisition_method), static_cast<int>(Watch::AcquisitionMethod::StaticFindObject));
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

            ImGui::EndTable();
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
    }
}

