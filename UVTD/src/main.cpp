#include <iostream>
#include <filesystem>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <UVTD/UVTD.hpp>

#define NOMINMAX
#include <Windows.h>

using namespace RC;

auto static get_user_selection() -> int32_t
{
    Output::send(STR("What would you like to do ?\n"));
    Output::send(STR("1. Generate VTable layouts\n"));
    Output::send(STR("2. Generate class/struct member variable layouts\n"));
    Output::send(STR("3. Generate sol bindings\n"));
    Output::send(STR("4. Everything\n"));
    Output::send(STR("0. Exit\n"));

    int32_t selection{};
    std::cin >> selection;
    if (!std::cin.good())
    {
        selection = 9;
        std::cin.clear();
        std::cin.ignore();
    }

    std::cin.get();
    return selection;
}

// We're outside DllMain here
auto thread_dll_start([[maybe_unused]]LPVOID thread_param) -> unsigned long
{

    std::filesystem::path module_path{};

    if (thread_param)
    {
        auto module_handle = reinterpret_cast<HMODULE>(thread_param);
        wchar_t module_filename_buffer[1024]{'\0'};
        GetModuleFileNameW(module_handle, module_filename_buffer, sizeof(module_filename_buffer) / sizeof(wchar_t));
        module_path = module_filename_buffer;
        module_path = module_path.parent_path();
    }

    Output::set_default_devices<Output::DebugConsoleDevice, Output::NewFileDevice>();
    auto& file_device = Output::get_device<Output::NewFileDevice>();
    file_device.set_file_name_and_path(module_path / "UVTD.log");

    try
    {
        Output::send(STR("Unreal Virtual Table Dumper -> START\n"));

        for (int32_t selection = get_user_selection(); selection != 1337; selection = get_user_selection())
        {
            if (selection == 0)
            {
                break;
            }
            else if (selection == 1)
            {
                Output::send(STR("Generating VTable layouts...\n"));
                UVTD::main(UVTD::DumpMode::VTable);
            }
            else if (selection == 2)
            {
                Output::send(STR("Generating class/struct member variable layouts...\n"));
                UVTD::main(UVTD::DumpMode::MemberVars);
            }
            else if (selection == 3)
            {
                Output::send(STR("Generating sol bindings...\n"));
                UVTD::main(UVTD::DumpMode::SolBindings);
            }
            else if (selection == 4)
            {
                Output::send(STR("Generating VTable layouts...\n"));
                UVTD::main(UVTD::DumpMode::VTable);

                Output::send(STR("Generating class/struct member variable layouts...\n"));
                UVTD::main(UVTD::DumpMode::MemberVars);
            }
            else if (selection == 9)
            {
                // Reserved for NOP, ask user again.
            }
        }
    }
    catch (std::exception& e)
    {
        Output::send(STR("Exception caught: {}\n"), to_wstring(e.what()));
    }

    return 0;
}

// We're still inside DllMain so be careful what you do here
auto dll_process_attached(HMODULE moduleHandle) -> void
{
    if (HANDLE handle = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(thread_dll_start), moduleHandle, 0, nullptr); handle)
    {
        CloseHandle(handle);
    }
}

auto main() -> int
{
    thread_dll_start(nullptr);
    return 0;
}

auto DllMain(HMODULE hModule, DWORD ul_reason_for_call, [[maybe_unused]] LPVOID lpReserved) -> BOOL
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            dll_process_attached(hModule);
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
