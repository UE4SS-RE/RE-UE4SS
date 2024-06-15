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

      public:
        using RenderFunctionType = void (*)(CppUserModBase*);

      private:
        RenderFunctionType render_function{};
        CppUserModBase* owner{};
        UEStringType tab_name{};

      public:
        GUITab() = delete;
        GUITab(UEStringViewType name, RenderFunctionType render_function) : tab_name(name), render_function(render_function){};
        GUITab(UEStringViewType name, RenderFunctionType render_function, CppUserModBase* owner)
            : tab_name(name), render_function(render_function), owner(owner){};
        ~GUITab() = default;

      private:
        auto get_owner() -> CppUserModBase*
        {
            return owner;
        }
    };
} // namespace RC::GUI
