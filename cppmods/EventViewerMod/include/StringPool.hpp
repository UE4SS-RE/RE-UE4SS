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
        // TODO null check caller/function
        auto get_strings(RC::Unreal::UObject* caller, RC::Unreal::UFunction* function) -> AllNameStringViews;

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
        std::unordered_map<uint64_t, StringInfo> m_pool;
        std::shared_mutex m_mutex;
    };
}