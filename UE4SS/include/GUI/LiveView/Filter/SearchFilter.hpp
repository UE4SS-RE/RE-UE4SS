#pragma once

#include <unordered_set>

#include <Unreal/UObject.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UStruct.hpp>
#include <Unreal/FProperty.hpp>

// To create a new filter:
// 1. Create <FilterName>.hpp in the same directory as this file.
// 2. Create a class that has either a 'pre_eval' or 'post_eval' function that returns true.
//    'pre_eval' is executed before the object name has been processed if a search is currently being done.
//    'post_eval' is executed after the object name has been processed.
// 3. The 'pre_eval'/'post_eval' function must return true if the object is to be filtered out, and otherwise false.
// 4. Open GUI/LiveView.cpp and find the 'SearchFilters' variable and add your new type to the list.
// 5. In the same file, find the 'LiveView::render' function and find the 'if (ImGui::BeginPopupContextItem("##search-options"))' if-statement.
// 6. Add a checkbox (or whatever) to control your search filter inside the if-statement.

namespace RC::GUI::Filter
{
    using namespace Unreal;

    // Does not need to be deduplicated because properties in UE are already unique.
    // Note that std::vector is faster when adding new properties to be highlighted, and unordered_set is faster when checking if a property should be highlighted.
    // This container covers the entire search-set of objects, so it won't contain just properties for the one object, so it may contain 50k+ elements.
    // We could speed vector lookup by somehow making this a per-object vector.
    // We can't really do that without running the filters when you click on an object instead of just when you search or apply all filters for all objects.
    // For now, let's just use an unordered_set.
    static inline std::unordered_set<FProperty*> s_highlighted_properties{};
    static auto is_highlighted(FProperty* property) -> bool
    {
        return s_highlighted_properties.contains(property);
    }
    static auto highlight(FProperty* property) -> void
    {
        s_highlighted_properties.emplace(property);
    }

    template <typename...>
    struct Types
    {
    };

    template <typename T>
    concept CanPreEval = requires(T t) {
        { T::pre_eval(std::declval<UObject*>()) };
    };

    template <typename T>
    concept CanPostEval = requires(T t) {
        { T::post_eval(std::declval<UObject*>()) };
    };

    template <typename T>
    auto eval_pre_search_filters(T&, UObject* object) -> bool
    {
        if constexpr (CanPreEval<T>)
        {
            return T::pre_eval(object);
        }
        else
        {
            return false;
        }
    }

    template <typename T, typename... Ts>
    auto eval_pre_search_filters(Types<T, Ts...>&, UObject* object) -> bool
    {
        auto eval_next_filters = [&] {
            Types<Ts...> next_filters{};
            return eval_pre_search_filters(next_filters, object);
        };

        if constexpr (CanPreEval<T>)
        {
            if (T::pre_eval(object))
            {
                return true;
            }
            else
            {
                return eval_next_filters();
            }
        }
        else
        {
            return eval_next_filters();
        }
    }

    template <typename T>
    auto eval_post_search_filters(T&, UObject* object) -> bool
    {
        if constexpr (CanPostEval<T>)
        {
            return T::post_eval(object);
        }
        else
        {
            return false;
        }
    }

    template <typename T, typename... Ts>
    auto eval_post_search_filters(Types<T, Ts...>&, UObject* object) -> bool
    {
        auto eval_next_filters = [&] {
            Types<Ts...> next_filters{};
            return eval_post_search_filters(next_filters, object);
        };

        if constexpr (CanPostEval<T>)
        {
            if (T::post_eval(object))
            {
                return true;
            }
            else
            {
                return eval_next_filters();
            }
        }
        else
        {
            return eval_next_filters();
        }
    }

#define APPLY_PRE_SEARCH_FILTERS(Filters)                                                                                                                      \
    if (eval_pre_search_filters(Filters, object))                                                                                                              \
    {                                                                                                                                                          \
        return true;                                                                                                                                           \
    }

#define APPLY_POST_SEARCH_FILTERS(Filters)                                                                                                                     \
    if (eval_post_search_filters(Filters, object))                                                                                                             \
    {                                                                                                                                                          \
        return true;                                                                                                                                           \
    }

    static auto is_instance(UObject* object) -> bool
    {
        return !object->HasAnyFlags(static_cast<EObjectFlags>(RF_ClassDefaultObject | RF_ArchetypeObject)) && !object->IsA<UStruct>() &&
               !object->IsA<UField>() && !object->IsA<UPackage>();
    }
} // namespace RC::GUI::Filter
