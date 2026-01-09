#include <Structs.h>
#include <Helpers/String.hpp>

#include <Unreal/UnrealInitializer.hpp>

#include "LuaType/LuaThreadId.hpp"

namespace RC::EventViewerMod {
    CallStackEntry::CallStackEntry(StringType context_name, StringType function_name, const uint32_t depth, const std::thread::id thread_id, const bool is_tick) :
        context_name(std::move(context_name)),
        function_name(std::move(function_name)),
        depth(depth),
        thread_id(thread_id),
        is_tick(is_tick),
        visual_depth(depth)
    {
    }

    auto CallStackEntry::entry_as_string() -> const char*
    {
        if (m_print_name.empty())
        {
            m_print_name = RC::to_string(context_name) + "." + RC::to_string(function_name);
        }
        return m_print_name.c_str();
    }

    auto CallStackEntry::entry_as_std_string() -> const std::string&
    {
        if (m_print_name.empty())
        {
            m_print_name = RC::to_string(context_name) + "." + RC::to_string(function_name);
        }
        return m_print_name;
    }

    ThreadInfo::ThreadInfo(const std::thread::id thread_id):    thread_id(thread_id),
                                                                is_game_thread(RC::Unreal::GetGameThreadId() == thread_id)
    {
    }

    auto ThreadInfo::id_string() -> const char*
    {
        if (m_id_string.empty())
        {
            m_id_string = std::to_string(thread_id._Get_underlying_id()); //TODO use stringstream for cross platform
            if (is_game_thread) m_id_string += " (Game)";
        }
        return m_id_string.c_str();
    }
}