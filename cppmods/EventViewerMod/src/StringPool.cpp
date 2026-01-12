#include <StringPool.hpp>
#include <stdexcept>
#include <Helpers/String.hpp>

auto RC::EventViewerMod::StringPool::get_strings(RC::Unreal::UObject* caller, RC::Unreal::UFunction* function) -> std::pair<std::string_view, std::string_view>
{
    uint64_t hash = function->GetNamePrivate().GetComparisonIndex();
    hash = hash << 32;
    hash |= caller->GetNamePrivate().GetComparisonIndex();

    {
        std::shared_lock lock(m_mutex);
        auto string_info_it = m_pool.find(hash);
        if (string_info_it != m_pool.end())
        {
            auto& string_info = string_info_it->second;
            std::string_view full_name = string_info.full_name;
            std::string_view func_name = full_name;
            func_name.remove_prefix(string_info.function_begin);
            return std::make_pair<std::string_view, std::string_view>(std::move(full_name), std::move(func_name));
        }
    }
    {
        const auto caller_str = to_string(caller->GetName());
        const auto func_str = to_string(function->GetName());
        {
            std::unique_lock lock(m_mutex);
            auto& string_info = m_pool.emplace(hash, std::move(StringInfo{ caller_str.size() + 1, caller_str + "." + func_str })).first->second;
            std::string_view full_name = string_info.full_name;
            std::string_view func_name = full_name;
            func_name.remove_prefix(string_info.function_begin);
            return std::make_pair<std::string_view, std::string_view>(std::move(full_name), std::move(func_name));
        }
    }
}

auto RC::EventViewerMod::StringPool::GetInstance() -> StringPool&
{
    static StringPool string_pool;
    return string_pool;
}