#pragma once

#include <utility>

#include <File/Macros.hpp>
#include <GUI/GUI.hpp>

namespace RC
{
    class CppUserModBase;
}

namespace RC::GUI
{
    class GUITab
    {
    friend class DebuggingGUI;

    protected:
        CppUserModBase* owner{};
        StringType TabName{};

    public:
        GUITab() = delete;
        explicit GUITab(StringViewType Name, CppUserModBase* owner) : TabName(Name), owner(owner) {};
        ~GUITab() = default;

    protected:
        auto get_owner() -> CppUserModBase* { return owner; }
    };
}


