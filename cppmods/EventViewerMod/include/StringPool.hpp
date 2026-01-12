#pragma once
#include <string>
#include <string_view>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

#include <Unreal/CoreUObject/UObject/Class.hpp>

namespace RC::EventViewerMod
{
    class StringPool
    {
    public:
        // have to store/retrieve 2 separate strings since string_view.data() isn't null terminated, which won't work well with imgui
        auto get_strings(RC::Unreal::UObject* caller, RC::Unreal::UFunction* function) -> std::pair<std::string_view, std::string_view>; // <caller + function, function>

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
        };

        // TODO replace with better concurrent hash map solution, though not too important for now
        std::unordered_map<uint64_t, StringInfo> m_pool;
        std::shared_mutex m_mutex;
    };
}