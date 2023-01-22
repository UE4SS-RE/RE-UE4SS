#ifndef UE4SS_REWRITE_MAYBE_UE4SSPROGRAM_HPP
#define UE4SS_REWRITE_MAYBE_UE4SSPROGRAM_HPP

#include <thread>
#include <functional>
#include <string>
#include <string_view>
#include <mutex>

#include <MProgram.hpp>
#include <SettingsManager.hpp>
#include <Unreal/UnrealVersion.hpp>
#include <Unreal/TArray.hpp>
#include <Input/Handler.hpp>
#include <Mod.hpp>
#include <LuaLibrary.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <GUI/GUI.hpp>

class GlobalClassTest
{
public:
    auto DoSomething() -> void { printf_s("GlobalClassTest\n"); }
};

auto GlobalFunctionTest() -> GlobalClassTest;

namespace RC2
{
    class MyFirstClass
    {
    public:
        auto DoSomething() -> void { printf_s("RC2::MyFirstClass\n"); }
    };

    namespace RC3::RC4
    {
        class MyFirstClass
        {
        public:
            auto DoSomething() -> void { printf_s("RC2::RC3::RC4::MyFirstClass\n"); }
        };
    }
}

namespace RC
{
    class MyFirstClass
    {
    public:
        auto DoSomething() -> void { printf_s("RC::MyFirstClass\n"); }
    };

    namespace Unreal
    {
        class UObject;
        class UObjectBase;
        class UObjectBaseUtility;
        class UWorld;
    }

    namespace Output
    {
        class ConsoleDevice;
    }

    struct RecognizableStruct
    {
        // Sha1 hash with no salt: "RecognizableString"
        char recognizable_string[41]{"81acd41b7490f7b70ec6455657855733e21d7c0e"};

        LuaLibrary::SetScriptVariableInt32Signature set_script_variable_int32_function;
        LuaLibrary::SetScriptVariableDefaultDataSignature set_script_variable_default_data_function;
        LuaLibrary::CallScriptFunctionSignature call_script_function_function;
        LuaLibrary::IsUE4SSInitializedSignature is_ue4ss_initialized_function;

        // Ensure that we have zero-initialized memory at the end of the struct
        // This means that we can reliably check whether a function exists externally
        uint64_t safety_padding[8]{0};
    };

    auto GetPlayerControllerTest2() -> Unreal::UObjectBase*;
    auto GetPlayerControllerTest3() -> Unreal::UObjectBaseUtility*;
    auto GetMyFirstClass() -> MyFirstClass;
    auto GetMyFirstClass2() -> ::RC2::MyFirstClass;
    auto GetMyFirstClass3() -> ::RC2::RC3::RC4::MyFirstClass;

    class UE4SSProgram : public MProgram
    {
    public:
        constexpr static wchar_t m_settings_file_name[] = L"UE4SS-settings.ini";
        constexpr static wchar_t m_log_file_name[] = L"UE4SS.log";
        constexpr static wchar_t m_object_dumper_file_name[] = L"UE4SS_ObjectDump.txt";

    public:
        static SettingsManager settings_manager;
        static inline bool unreal_is_shutting_down{};

    protected:
        Input::Handler m_input_handler{L"ConsoleWindowClass", L"UnrealWindow"};
        std::jthread m_event_loop;
        std::jthread m_render_thread;

    private:
        std::filesystem::path m_game_path_and_exe_name;
        std::filesystem::path m_root_directory;
        std::filesystem::path m_module_file_path;
        std::filesystem::path m_working_directory;
        std::filesystem::path m_mods_directory;
        std::filesystem::path m_game_executable_directory;
        std::filesystem::path m_log_directory;
        std::filesystem::path m_object_dumper_output_directory;
        std::filesystem::path m_default_settings_path_and_file;
        std::filesystem::path m_settings_path_and_file;
        Output::DebugConsoleDevice* m_debug_console_device{};
        Output::ConsoleDevice* m_console_device{};
        GUI::DebuggingGUI m_debugging_gui{};

        using EventCallable = void(*)(void* data);
        struct Event
        {
            EventCallable callable{};
            void* data{};
        };
        std::vector<Event> m_queued_events{};
        std::mutex m_event_queue_mutex{};

    public:
        static inline std::vector<std::unique_ptr<Mod>> m_mods;

        static inline RecognizableStruct m_shared_functions{};

        static inline UE4SSProgram* s_program{};

        bool m_has_game_specific_config{};
        bool m_processing_events{};
        bool m_pause_events_processing{};
        bool m_simple_console_enabled{};
        bool m_debug_console_enabled{};
        bool m_debug_console_visible{};

    public:
        enum class IsInstalled
        {
            Yes,
            No
        };

        enum class IsStarted
        {
            Yes,
            No
        };

    public:
        UE4SSProgram(const std::wstring& ModuleFilePath, std::initializer_list<BinaryOptions> options);
        ~UE4SSProgram();
        UE4SSProgram(const UE4SSProgram&) = delete;
        UE4SSProgram(UE4SSProgram&&) = delete;

    private:
        auto setup_paths(const std::wstring& moduleFilePath) -> void;
        enum class FunctionStatus
        {
            Success,
            Failure,
        };
        auto create_emergency_console_for_early_error(File::StringViewType error_message) -> void;
        auto setup_output_devices() -> void;
        auto setup_mod_directory_path() -> void;
        auto create_debug_console() -> void;
        auto setup_unreal() -> void;
        auto load_unreal_offsets_from_file() -> void;
        auto share_lua_functions() -> void;
        auto on_program_start() -> void;
        auto setup_unreal_properties() -> void;

    protected:
        auto update() -> void;
        auto setup_mods() -> void;
        auto start_mods() -> void;
        auto uninstall_mods() -> void;

    public:
        auto reinstall_mods() -> void;
        auto get_object_dumper_output_directory() -> const File::StringType;
        auto get_working_directory() -> File::StringViewType;
        auto get_mods_directory() -> File::StringViewType;
        auto generate_uht_compatible_headers() -> void;
        auto generate_cxx_headers(const std::filesystem::path& output_dir) -> void;
        auto get_debugging_ui() -> GUI::DebuggingGUI& { return m_debugging_gui; };
        auto stop_render_thread() -> void;
        auto queue_event(EventCallable callable, void* data) -> void;
        auto is_queue_empty() -> bool;
        auto can_process_events() -> bool { return m_processing_events; }

    public:
        // API pass-through for use outside the private scope of UE4SSProgram
        auto register_keydown_event(Input::Key, const Input::EventCallbackCallable&, uint8_t custom_data = 0) -> void;
        auto register_keydown_event(Input::Key, const Input::Handler::ModifierKeyArray&, const Input::EventCallbackCallable&, uint8_t custom_data = 0) -> void;
        auto is_keydown_event_registered(Input::Key) -> bool;
        auto is_keydown_event_registered(Input::Key, const Input::Handler::ModifierKeyArray&) -> bool;

    private:
        static auto install_mods() -> void;

    public:
        static auto dump_all_objects_and_properties(const File::StringType& output_path_and_file_name) -> void;
        static auto find_mod_by_name(std::wstring_view mod_name, IsInstalled = IsInstalled::No, IsStarted = IsStarted::No) -> Mod*;
        static auto find_mod_by_name(std::string_view mod_name, IsInstalled = IsInstalled::No, IsStarted = IsStarted::No) -> Mod*;
        static auto static_cleanup() -> void;
        static auto get_program() -> UE4SSProgram& { return *s_program; }
    };
}

#endif //UE4SS_REWRITE_MAYBE_UE4SSPROGRAM_HPP
