#pragma once

#include <File/Macros.hpp>
#include <GUI/GUI.hpp>

namespace RC::GUI
{
    class GUITab
    {
    friend class DebuggingGUI;

    protected:
        StringType TabName{};

    public:
        GUITab() = default;
        ~GUITab() = default;

        virtual auto render() -> void {}
    };
}


