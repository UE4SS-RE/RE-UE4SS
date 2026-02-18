// EventViewerMod: Windows DLL entry point (UE4SS loads this module).
//
// The actual mod logic lives in EventViewerMod (see EventViewer.cpp). This file exists to
// satisfy the DLL entry requirements on Windows.

#include <EventViewer.hpp>

extern "C"
{
    __declspec(dllexport) RC::CppUserModBase* start_mod()
    {
        return new EventViewerMod::EventViewerMod();
    }

    __declspec(dllexport) void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}