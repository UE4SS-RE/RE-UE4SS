#include <StringPool.hpp>

#include <Helpers/String.hpp>

#include <cctype>

namespace
{
    // ASCII-only lowercasing; Unreal function/object names are typically ASCII.
    // If this ever needs full Unicode casefolding, we'll revisit.
    auto to_lower_ascii_copy(std::string_view s) -> std::string
    {
        std::string out;
        out.reserve(s.size());
        for (const unsigned char ch : s)
        {
            out.push_back(static_cast<char>(std::tolower(ch)));
        }
        return out;
    }
}

auto RC::EventViewerMod::StringPool::get_strings(RC::Unreal::UObject* caller, RC::Unreal::UFunction* function) -> AllNameStringViews
{
    if (!caller || !function) return AllNameStringViews{};
    // StringPool combines the caller's ComparisonIndex and the function's ComparisonIndex
    // to generate a unique hash for the full name string.
    const uint32_t function_hash = function->GetNamePrivate().GetComparisonIndex();
    uint64_t hash = function_hash;
    hash = hash << 32;
    hash |= caller->GetNamePrivate().GetComparisonIndex();

    {
        std::shared_lock lock(m_mutex);
        auto string_info_it = m_main_pool.find(hash);
        if (string_info_it != m_main_pool.end())
        {
            auto& string_info = string_info_it->second;
            std::string_view full_name = string_info.full_name;
            std::string_view lower_full = string_info.lower_cased_full_name;

            std::string_view func_name = full_name;
            func_name.remove_prefix(string_info.function_begin);

            std::string_view lower_func_name = lower_full;
            lower_func_name.remove_prefix(string_info.function_begin);

            return AllNameStringViews{
                FunctionNameStringViews{function_hash, func_name, lower_func_name},
                full_name,
                lower_full,
                hash
            };
        }
    }

    const auto caller_str = RC::to_string(caller->GetName());
    const auto func_str = RC::to_string(function->GetName());

    // Build once, store both original-case and lower-cased versions.
    auto full = caller_str + "." + func_str;
    auto lower_full = to_lower_ascii_copy(full);
    auto path_str = RC::to_string(function->GetPathName());

    {
        std::unique_lock lock(m_mutex);
        auto& string_info = m_main_pool.emplace(hash, StringInfo{caller_str.size() + 1, std::move(full), std::move(lower_full)}).first->second;
        m_path_pool.emplace(function_hash, std::move(path_str));

        std::string_view full_name = string_info.full_name;
        std::string_view lower_full_name = string_info.lower_cased_full_name;

        std::string_view func_name = full_name;
        func_name.remove_prefix(string_info.function_begin);

        std::string_view lower_func_name = lower_full_name;
        lower_func_name.remove_prefix(string_info.function_begin);

        return AllNameStringViews{
            FunctionNameStringViews{function_hash, func_name, lower_func_name},
            full_name,
            lower_full_name,
            hash
        };
    }
}

auto RC::EventViewerMod::StringPool::get_path_name(const uint32_t function_hash) -> std::string_view
{
    std::shared_lock lock(m_mutex);
    auto path_it = m_path_pool.find(function_hash);
    if (path_it == m_path_pool.end()) return "";
    return path_it->second;
}

auto RC::EventViewerMod::StringPool::clear() -> void
{
    std::unique_lock lock(m_mutex);
    m_path_pool.clear();
}

auto RC::EventViewerMod::StringPool::GetInstance() -> StringPool&
{
    static StringPool string_pool;
    return string_pool;
}
