#ifndef UE4SS_REWRITE_MAYBE_UE4SSPROGRAM_HPP
#define UE4SS_REWRITE_MAYBE_UE4SSPROGRAM_HPP

#include <thread>
#include <functional>
#include <string>
#include <string_view>
#include <mutex>

#include <Common.hpp>
#include <MProgram.hpp>
#include <SettingsManager.hpp>
#include <Unreal/UnrealVersion.hpp>
#include <Unreal/TArray.hpp>
#include <Input/Handler.hpp>
#include <Mod/Mod.hpp>
#include <Mod/LuaMod.hpp>
#include <LuaLibrary.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <GUI/GUI.hpp>
#include <GUI/GUITab.hpp>

// Used to set up ImGui context and allocator in DLL mods
#define UE4SS_ENABLE_IMGUI() \
{ \
    ImGui::SetCurrentContext(UE4SSProgram::get_current_imgui_context()); \
    ImGuiMemAllocFunc alloc_func{}; \
    ImGuiMemFreeFunc free_func{}; \
    void* user_data{}; \
    UE4SSProgram::get_current_imgui_allocator_functions(&alloc_func, &free_func, &user_data); \
    ImGui::SetAllocatorFunctions(alloc_func, free_func, user_data); \
}

namespace RC
{
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

    class UE4SSProgram : public MProgram
    {
    public:
        constexpr static wchar_t m_settings_file_name[] = L"UE4SS-settings.ini";
        constexpr static wchar_t m_log_file_name[] = L"UE4SS.log";
        constexpr static wchar_t m_object_dumper_file_name[] = L"UE4SS_ObjectDump.txt";

    public:
        RC_UE4SS_API static SettingsManager settings_manager;
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
        auto setup_mod_directory_path() -> void;
        auto create_simple_console() -> void;
        auto setup_unreal() -> void;
        auto load_unreal_offsets_from_file() -> void;
        auto share_lua_functions() -> void;
        auto on_program_start() -> void;
        auto setup_unreal_properties() -> void;

    protected:
        auto update() -> void;
        auto setup_cpp_mods() -> void;
        auto start_cpp_mods() -> void;
        auto setup_mods() -> void;
        auto start_lua_mods() -> void;
        auto uninstall_mods() -> void;
        auto fire_unreal_init_for_cpp_mods() -> void;
        auto fire_program_start_for_cpp_mods() -> void;

    public:
        auto reinstall_mods() -> void;
        auto get_object_dumper_output_directory() -> const File::StringType;
        RC_UE4SS_API auto get_module_directory() -> File::StringViewType;
        RC_UE4SS_API auto get_working_directory() -> File::StringViewType;
        RC_UE4SS_API auto get_mods_directory() -> File::StringViewType;
        RC_UE4SS_API auto generate_uht_compatible_headers() -> void;
        RC_UE4SS_API auto generate_cxx_headers(const std::filesystem::path& output_dir) -> void;
        RC_UE4SS_API auto generate_lua_types(const std::filesystem::path& output_dir) -> void;
        auto get_debugging_ui() -> GUI::DebuggingGUI&
        {
            return m_debugging_gui;
        };
        auto stop_render_thread() -> void;
        RC_UE4SS_API auto add_gui_tab(std::shared_ptr<GUI::GUITab> tab) -> void;
        RC_UE4SS_API auto remove_gui_tab(std::shared_ptr<GUI::GUITab> tab) -> void;
        RC_UE4SS_API static auto get_current_imgui_context() -> ImGuiContext*
        {
            return ImGui::GetCurrentContext();
        }
        RC_UE4SS_API static auto get_current_imgui_allocator_functions(ImGuiMemAllocFunc* alloc_func, ImGuiMemFreeFunc* free_func, void** user_data) -> void
        {
            return ImGui::GetAllocatorFunctions(alloc_func, free_func, user_data);
        }
        RC_UE4SS_API auto queue_event(EventCallable callable, void* data) -> void;
        RC_UE4SS_API auto is_queue_empty() -> bool;
        RC_UE4SS_API auto can_process_events() -> bool
        {
            return m_processing_events;
        }

    public:
        // API pass-through for use outside the private scope of UE4SSProgram
        RC_UE4SS_API auto register_keydown_event(Input::Key, const Input::EventCallbackCallable&, uint8_t custom_data = 0) -> void;
        RC_UE4SS_API auto register_keydown_event(Input::Key, const Input::Handler::ModifierKeyArray&, const Input::EventCallbackCallable&, uint8_t custom_data = 0) -> void;
        RC_UE4SS_API auto is_keydown_event_registered(Input::Key) -> bool;
        RC_UE4SS_API auto is_keydown_event_registered(Input::Key, const Input::Handler::ModifierKeyArray&) -> bool;

    private:
        static auto install_cpp_mods() -> void;
        static auto install_lua_mods() -> void;

    public:
        static auto dump_all_objects_and_properties(const File::StringType& output_path_and_file_name) -> void;
        RC_UE4SS_API static auto find_mod_by_name(std::wstring_view mod_name, IsInstalled = IsInstalled::No, IsStarted = IsStarted::No) -> Mod*;
        RC_UE4SS_API static auto find_mod_by_name(std::string_view mod_name, IsInstalled = IsInstalled::No, IsStarted = IsStarted::No) -> Mod*;
        RC_UE4SS_API static auto find_lua_mod_by_name(std::wstring_view mod_name, IsInstalled = IsInstalled::No, IsStarted = IsStarted::No) -> LuaMod*;
        RC_UE4SS_API static auto find_lua_mod_by_name(std::string_view mod_name, IsInstalled = IsInstalled::No, IsStarted = IsStarted::No) -> LuaMod*;
        static auto static_cleanup() -> void;
        RC_UE4SS_API static auto get_program() -> UE4SSProgram&
        {
            return *s_program;
        }
    };
}

#endif //UE4SS_REWRITE_MAYBE_UE4SSPROGRAM_HPP
