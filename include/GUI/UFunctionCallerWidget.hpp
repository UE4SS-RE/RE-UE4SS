#ifndef UE4SS_GUI_UFUNCTION_CALLER_WIDGET_HPP
#define UE4SS_GUI_UFUNCTION_CALLER_WIDGET_HPP

#include <vector>
#include <string>

#include <GUI/SearcherWidget.hpp>

namespace RC::Unreal
{
    class UObject;
    class UFunction;
    class FProperty;
}

namespace RC::GUI
{
    using namespace RC::Unreal;

    struct CallableUFunction
    {
        UFunction* function{};
        std::string cached_name{};
        bool is_selected{};
        bool has_out_params{};
        bool context_is_implicit{true};
    };

    struct UFunctionParam
    {
        std::string value_from_ui{};
        std::string cached_name{};
        FProperty* unreal_param{};
        size_t current_cursor_position_in_ui{};
    };

    auto ufunction_caller_search_mode_changed(void* userdata, SearcherWidget::SearchMode) -> void;
    auto ufunction_caller_all_iterator(void* userdata) -> void;
    auto ufunction_caller_search_iterator(void* userdata) -> void;

    class UFunctionCallerWidget
    {
    public:
        UFunctionCallerWidget();

    private:
        SearcherWidget m_searcher{&ufunction_caller_search_mode_changed, &ufunction_caller_all_iterator, &ufunction_caller_search_iterator, this};

    private:
        std::vector<CallableUFunction> m_callable_functions{};
        std::vector<UFunctionParam> m_params_for_selected_function{};
        CallableUFunction* m_currently_selected_function{};
        UObject* m_current_instance{};
        UObject* m_prev_instance{};
        bool m_render_functions_for_instance_window{};
        bool m_function_for_instance_is_selected{};
        bool m_is_cache_valid{};

    private:
        auto is_widget_open() -> bool&;
        auto is_cache_valid() -> bool { return m_is_cache_valid; }
        auto cache_instance(UObject* instance) -> void;
        auto deselect_all_functions() -> void;
        auto select_function(CallableUFunction&) -> void;
        auto is_function_selected() -> bool { return m_currently_selected_function; }
        auto selected_function() -> CallableUFunction& { return *m_currently_selected_function; }
        auto call_selected_function(UObject* instance) -> void;
        auto render_param_example(UFunctionParam& param) -> void;
        auto render_param_type(UFunctionParam& param) -> void;

    public:
        auto open_widget_deferred() -> void;
        auto invalidate_cache() -> void;

    public:
        auto render(UObject* instance) -> void;
    };
}

#endif // UE4SS_GUI_UFUNCTION_CALLER_WIDGET_HPP
