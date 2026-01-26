#pragma once

// EventViewerMod: String interning + hashing.
//
// Unreal provides stable numeric identifiers for names (ComparisonIndex). StringPool uses those to:
// - Deduplicate storage for function/caller strings.
// - Provide fast comparisons via hashes (avoid expensive string comparisons in hot paths).
// - Cache both original and lowercased strings for case-insensitive filtering.
//
// The pool is designed for “grow-only” lifetime during a session; string_views returned from the pool
// stay valid as long as the underlying storage isn't cleared.

#include <string>
#include <string_view>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include <Structs.hpp>

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

        // Clears the string pool. Currently unused, but may be useful if games ever start recycling FName indices.
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
        std::unordered_map<uint64_t, StringInfo> m_main_pool;  // hashed by function->GetComparisonIndex and caller->GetComparisonIndex
        std::unordered_map<uint32_t, std::string> m_path_pool; // hashed by function->GetComparisonIndex
        std::shared_mutex m_mutex;
    };
} // namespace RC::EventViewerMod