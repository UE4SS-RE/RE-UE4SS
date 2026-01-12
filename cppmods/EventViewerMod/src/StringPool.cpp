#include <StringPool.hpp>
#include <stdexcept>
#include <Helpers/String.hpp>

auto RC::EventViewerMod::StringPool::get_strings(RC::Unreal::UObject* caller, RC::Unreal::UFunction* function) -> std::pair<std::string_view, std::string_view>
{
    uint64_t function_name_hash = function->GetNamePrivate().GetComparisonIndex();
    function_name_hash = function_name_hash << 32;
    uint64_t full_hash = function_name_hash;
    full_hash |= caller->GetNamePrivate().GetComparisonIndex();

    {
        std::shared_lock lock(m_mutex);
        if (const auto full_hash_it = m_pool.find(full_hash); full_hash_it != m_pool.end())
        {
            if (const auto function_hash_it = m_pool.find(function_name_hash); function_hash_it != m_pool.end())
            {
                return std::make_pair<std::string_view, std::string_view>(full_hash_it->second, function_hash_it->second);
            }
            throw std::runtime_error("Invariant violation in EventViewerMod::StringPool: for every full name added, there is a function name added, but failed to find function name!");
        }
    }
    {
        std::unique_lock lock(m_mutex);
        return std::make_pair<std::string_view, std::string_view>(
            m_pool.emplace(full_hash, to_string(caller->GetName()) + "." + to_string(function->GetName())).first->second, //TODO log when fails
            m_pool.emplace(function_name_hash, to_string(function->GetName())).first->second);
    }
}

auto RC::EventViewerMod::StringPool::GetInstance() -> StringPool&
{
    static StringPool string_pool;
    return string_pool;
}