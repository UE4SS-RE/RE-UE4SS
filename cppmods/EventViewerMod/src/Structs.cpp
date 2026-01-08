#include <Structs.h>
#include <Helpers/String.hpp>
#include <cstring>

namespace RC::EventViewerMod {
    auto CallStackEntry::context_name_as_string() -> const char*
    {
        if (!m_context_name)
        {
            auto utf8 = to_string(context_name);
            m_context_name = new char[utf8.length() + 1];
            m_context_name[utf8.length()] = '\0';
            strncpy_s(m_context_name, utf8.length() + 1, utf8.c_str(), utf8.length());
        }
        return m_context_name;
    }

    auto CallStackEntry::function_name_as_string() -> const char*
    {
        if (!m_function_name)
        {
            auto utf8 = to_string(function_name);
            m_function_name = new char[utf8.length() + 1];
            m_function_name[utf8.length()] = '\0';
            strncpy_s(m_function_name, utf8.length() + 1, utf8.c_str(), utf8.length());
        }
        return m_function_name;
    }

    CallStackEntry::~CallStackEntry()
    {
        delete[] m_context_name;
        delete[] m_function_name;
    }
}