#include <GUI/UFunctionCallerWidget.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/FOutputDevice.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FBoolProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FClassProperty.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace RC::GUI
{
    UFunctionCallerWidget::UFunctionCallerWidget() = default;

    auto UFunctionCallerWidget::is_widget_open() -> bool&
    {
        return m_render_functions_for_instance_window;
    }
    
    auto UFunctionCallerWidget::open_widget_deferred() -> void
    {
        m_render_functions_for_instance_window = true;
    }

    auto UFunctionCallerWidget::cache_instance(UObject* instance) -> void
    {
        m_current_instance = instance;
        if (!m_current_instance) { return; }
        instance->GetClassPrivate()->ForEachFunctionInChain([&](UFunction* function) {
            bool should_cache_function{};
            if (m_searcher.was_search_requested())
            {
                auto function_name = to_string(function->GetName());
                std::transform(function_name.begin(), function_name.end(), function_name.begin(), [](char c) {
                    return std::tolower(c);
                });
                
                auto search_for_name = m_searcher.get_search_value();
                std::transform(search_for_name.begin(), search_for_name.end(), search_for_name.begin(), [](char c) {
                    return std::tolower(c);
                });

                if (function_name.find(search_for_name) != function_name.npos)
                {
                    should_cache_function = true;
                }
            }
            else
            {
                should_cache_function = true;
            }

            if (!should_cache_function) { return LoopAction::Continue; }
            auto& cached_function = m_callable_functions.emplace_back(CallableUFunction{function});

            cached_function.function->ForEachProperty([&](FProperty* param) {
                if (param->HasAllPropertyFlags(CPF_OutParm)) { cached_function.has_out_params = true; }
                return LoopAction::Continue;
            });

            cached_function.cached_name = std::format("{}{}", to_string(function->GetName()), cached_function.has_out_params ? " | CPF_OutParm" : "");

            return LoopAction::Continue;
        });
        m_is_cache_valid = true;
    }

    auto ufunction_caller_search_mode_changed(void* userdata, SearcherWidget::SearchMode) -> void
    {
        auto widget = static_cast<UFunctionCallerWidget*>(userdata);
        widget->invalidate_cache();
    }

    auto ufunction_caller_all_iterator(void* userdata) -> void
    {
        //auto widget = static_cast<UFunctionCallerWidget*>(userdata);
    }
    
    auto ufunction_caller_search_iterator(void* userdata) -> void
    {
        //auto widget = static_cast<UFunctionCallerWidget*>(userdata);
    }

    auto UFunctionCallerWidget::invalidate_cache() -> void
    {
        if (!m_is_cache_valid) { return; }
        m_is_cache_valid = false;
        m_callable_functions.clear();
        m_params_for_selected_function.clear();
        m_currently_selected_function = nullptr;
    }

    auto UFunctionCallerWidget::deselect_all_functions() -> void
    {
        for (auto& callable_function : m_callable_functions)
        {
            callable_function.is_selected = false;
        }

        m_currently_selected_function = nullptr;
        m_params_for_selected_function.clear();
    }

    auto UFunctionCallerWidget::select_function(CallableUFunction& selectable_function) -> void
    {
        selectable_function.is_selected = true;
        m_currently_selected_function = &selectable_function;
        m_currently_selected_function->function->ForEachProperty([&](FProperty* param) {
            if (param->HasAllPropertyFlags(CPF_ReturnParm)) { return LoopAction::Continue; }
            m_params_for_selected_function.emplace_back(UFunctionParam{{}, to_string(param->GetName()).c_str(), param});
            return LoopAction::Continue;
        });
    }

    static bool s_do_call{};
    static UObject* s_instance{};
    static StringType s_cmd{};
    static FOutputDevice s_ar{};
    static UFunction* s_function{};
    auto call_process_console_exec(UObject*, UFunction*, void*) -> void
    {
        if (s_do_call)
        {
            s_do_call = false;
            auto& function_flags = s_function->GetFunctionFlags();
            function_flags |= FUNC_Exec;
            bool call_succeeded = s_instance->ProcessConsoleExec(s_cmd.c_str(), s_ar, s_instance);
            Output::send(STR("call_succeeded: {}\n"), call_succeeded);
            function_flags &= ~FUNC_Exec;
        }
    }
    auto UFunctionCallerWidget::call_selected_function(UObject* instance) -> void
    {
        if (!m_currently_selected_function || !m_currently_selected_function->function) { return; }
        auto function = m_currently_selected_function->function;

        auto cmd = std::format(STR("{}"), function->GetName());
        for (const auto& param : m_params_for_selected_function)
        {
            cmd.append(std::format(STR(" {}"), to_wstring(param.value_from_ui)));
        }

        s_cmd = cmd;
        s_instance = instance;
        s_function = function;
        static bool s_is_hooked{};
        if (!s_is_hooked)
        {
            s_is_hooked = true;
            Hook::RegisterProcessEventPostCallback(call_process_console_exec);
        }
        s_do_call = true;
    }

    static auto value_from_ui_callback(ImGuiInputTextCallbackData* data) -> int
    {
        auto param = static_cast<UFunctionParam*>(data->UserData);
        if (data->CursorPos < 0) { return 0; }
        param->current_cursor_position_in_ui = static_cast<size_t>(data->CursorPos);
        return 0;
    }

    auto UFunctionCallerWidget::render_param_example(UFunctionParam& param) -> void
    {
        if (auto as_struct_property = CastField<FStructProperty>(param.unreal_param); as_struct_property)
        {
            ImGui::Text("e.g.: (Prop1=Val1,Prop2=Val2,Prop3=Val3)");
        }
        else if (auto as_array_property = CastField<FArrayProperty>(param.unreal_param); as_array_property)
        {
            if (auto arr_inner_as_struct_property = CastField<FStructProperty>(as_array_property->GetInner()); arr_inner_as_struct_property)
            {
                ImGui::Text("e.g.: ((Prop1=Val1,Prop2=Val2,Prop3=Val3),(Prop1=Val1,Prop2=Val2,Prop3=Val3))");
            }
            else
            {
                ImGui::Text("e.g.: (Elem1,Elem2,Elem3)");
            }
        }
        else if (auto as_bool_property = CastField<FBoolProperty>(param.unreal_param); as_bool_property)
        {
            ImGui::Text("e.g.: true");
        }
    }

    auto UFunctionCallerWidget::render_param_type(UFunctionParam& param) -> void
    {
        if (auto as_struct_property = CastField<FStructProperty>(param.unreal_param); as_struct_property)
        {
            ImGui::Text("%S (%S)", param.unreal_param->GetClass().GetName().c_str(), as_struct_property->GetStruct()->GetName().c_str());
        }
        else if (auto as_array_property = CastField<FArrayProperty>(param.unreal_param); as_array_property)
        {
            ImGui::Text("%S (%S)", param.unreal_param->GetClass().GetName().c_str(), as_array_property->GetInner()->GetName().c_str());
        }
        else if (auto as_object_property = CastField<FObjectProperty>(param.unreal_param); as_object_property)
        {
            ImGui::Text("%S (%S)", param.unreal_param->GetClass().GetName().c_str(), as_object_property->GetPropertyClass()->GetName().c_str());
        }
        else if (auto as_class_property = CastField<FClassProperty>(param.unreal_param); as_class_property)
        {
            ImGui::Text("%S (%S)", param.unreal_param->GetClass().GetName().c_str(), as_class_property->GetMetaClass()->GetName().c_str());
        }
        else
        {
            ImGui::Text("%S", param.unreal_param->GetClass().GetName().c_str());
        }
    }

    auto UFunctionCallerWidget::render(UObject* instance) -> void
    {
        m_prev_instance = instance;

        auto popup_modal_id = to_string(std::format(STR("##functions-for-{}"), instance->HashObject()));
        auto& is_open = is_widget_open();
        if (is_open)
        {
            ImGui::OpenPopup(popup_modal_id.c_str());
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {925.0f, 300.0f});
        if (ImGui::BeginPopupModal(popup_modal_id.c_str(), &is_open))
        {
            ImGui::PushItemWidth(-1.0f);
            m_searcher.render();
            ImGui::PopItemWidth();

            static int num_columns = 2;
            if (ImGui::BeginTable("function_list_table", num_columns, ImGuiTableFlags_Borders | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_SizingStretchSame))
            {
                ImGui::TableSetupColumn("Select a function to call");
                ImGui::TableSetupColumn("Set parameters");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::BeginListBox("##selectable-functions-for-current-instance", {-1.0f, -1.0f}))
                {
                    if (!is_cache_valid()) { cache_instance(instance); }

                    for (auto& callable_function : m_callable_functions)
                    {
                        if (ImGui::Selectable(callable_function.cached_name.c_str(), callable_function.is_selected))
                        {
                            deselect_all_functions();
                            select_function(callable_function);
                        }
                    }

                    ImGui::EndListBox();
                }

                ImGui::TableNextColumn();
                if (is_function_selected() && selected_function().has_out_params)
                {
                    //ImGui::Text("Function cannot be called because:\nWe don't yet support params taken by reference (aka out params).");
                    //ImGui::BeginDisabled(true);
                }
                for (auto& param : m_params_for_selected_function)
                {
                    ImGui::Text("%s", param.cached_name.c_str());
                    ImGui::SameLine();
                    render_param_example(param);
                    ImGui::SameLine();
                    ImGui::Text("[?]");
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        render_param_type(param);
                        ImGui::EndTooltip();
                    }
                    ImGui::SetNextItemWidth(-2.0f);
                    ImGui::InputText(std::format("##param-input-{}", param.cached_name).c_str(), &param.value_from_ui, ImGuiInputTextFlags_CallbackAlways, &value_from_ui_callback, &param);
                    if (ImGui::BeginPopupContextItem(param.cached_name.c_str()))
                    {
                        if (ImGui::MenuItem("Insert Zero'd FVector"))
                        {
                            param.value_from_ui.insert(param.current_cursor_position_in_ui, "(X=0.0,Y=0.0,Z=0.0)");
                        }
                        if (ImGui::MenuItem("Insert Zero'd Quat"))
                        {
                            param.value_from_ui.insert(param.current_cursor_position_in_ui, "(X=0.0,Y=0.0,Z=0.0,W=0.0)");
                        }
                        if (ImGui::MenuItem("Insert Zero'd Rotator"))
                        {
                            param.value_from_ui.insert(param.current_cursor_position_in_ui, "(Pitch=0.0,Yaw=0.0,Roll=0.0)");
                        }
                        ImGui::EndPopup();
                    }
                }

                if (ImGui::Button("Call"))
                {
                    call_selected_function(instance);
                }
                //if (is_function_selected() && selected_function().has_out_params) { ImGui::EndDisabled(); }

                ImGui::EndTable();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        if (m_current_instance != m_prev_instance) { invalidate_cache(); }
    }
}
