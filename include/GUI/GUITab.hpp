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
        StringType tab_name{};

    public:
        GUITab() = delete;
        explicit GUITab(StringViewType name, CppUserModBase* owner) : tab_name(name), owner(owner) {};
        ~GUITab() = default;

    protected:
        auto get_owner() -> CppUserModBase* { return owner; }
    };
}


