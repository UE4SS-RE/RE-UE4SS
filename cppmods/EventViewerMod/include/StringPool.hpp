#pragma once
#include <string>
#include <string_view>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include <Structs.h>

#include <Unreal/CoreUObject/UObject/Class.hpp>

namespace RC::EventViewerMod
{
    class StringPool
    {
    public:
        // Returns string_views owned by the pool:
        //  - full_name / function_name for display
        //  - lower_cased_* for case-insensitive filtering
        //  - function_hash for fast equality on function identity
        auto get_strings(RC::Unreal::UObject* caller, RC::Unreal::UFunction* function) -> AllNameStringViews;

        // Returns path name of a function, same as calling GetPathName but with a string view instead.
        auto get_path_name(uint32_t function_hash) -> std::string_view;

        // Clears the string pool. It should be done when new maps/games are loaded since FName comparison indices might change in some games.
        // Note that this will invalidate any string_views acquired through the getters, so the ImGui views should be cleared before doing this
        // in the same frame.
        auto clear() -> void;

        static auto GetInstance() -> StringPool&;

        StringPool(const StringPool& Other) = delete;
        StringPool(StringPool&& Other) noexcept = delete;
        StringPool& operator=(const StringPool& Other) = delete;
        StringPool& operator=(StringPool&& Other) noexcept = delete;
    private:
        StringPool() = default;

        struct StringInfo
        {
            size_t function_begin;
            std::string full_name;
            std::string lower_cased_full_name;
        };

        // TODO replace with better concurrent hash map solution, though not too important for now
        std::unordered_map<uint64_t, StringInfo> m_main_pool; // hashed by function->GetComparisonIndex and caller->GetComparisonIndex
        std::unordered_map<uint32_t, std::string> m_path_pool; // hashed by function->GetComparisonIndex
        std::shared_mutex m_mutex;
    };
}