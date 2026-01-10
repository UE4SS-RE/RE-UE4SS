#include <Structs.h>
#include <Helpers/String.hpp>

#include <Unreal/UnrealInitializer.hpp>
#include <utility>
#include <sstream>

#include "LuaType/LuaThreadId.hpp"

auto make_tab_string(uint32_t count) -> std::string
{
    std::string out;
    out.reserve(count);
    while (count--) out += '\t';
    return out;
}

namespace RC::EventViewerMod {
    EntryBase::EntryBase(std::string text, const bool is_tick): text(std::move(text)),
                                                          is_tick(is_tick)
    {
    }

    CallStackEntry::CallStackEntry(const StringType& context_name, const StringType& function_name, const uint32_t depth, const std::thread::id thread_id, const bool is_tick) :
        EntryBase(std::move(make_tab_string(depth) + RC::to_string(context_name + STR(".") + function_name)), is_tick),
        depth(depth),
        thread_id(thread_id)
    {
    }

    CallFrequencyEntry::CallFrequencyEntry(std::string text, const bool is_tick) : EntryBase(std::move(text), is_tick)
    {
    }

    ThreadInfo::ThreadInfo(const std::thread::id thread_id):    thread_id(thread_id),
                                                                is_game_thread(RC::Unreal::GetGameThreadId() == thread_id)
    {
    }

    auto ThreadInfo::operator<=>(const ThreadInfo& other) const noexcept
    {
        return other.thread_id <=> thread_id;
    }

    auto ThreadInfo::operator<=>(const std::thread::id& other) const noexcept
    {
        return other <=> thread_id;
    }

    auto ThreadInfo::id_string() -> const char*
    {
        if (m_id_string.empty())
        {
            m_id_string = (std::stringstream() << thread_id).str();
            if (is_game_thread) m_id_string += " (Game)";
        }
        return m_id_string.c_str();
    }

    auto ThreadInfo::clear() -> void
    {
        call_frequencies.clear();
        call_stack.clear();
    }

    auto TargetInfo::clear() -> void
    {
        threads.clear();
    }
}