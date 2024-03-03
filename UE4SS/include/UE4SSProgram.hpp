#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

#include <Common.hpp>
#ifdef WIN32
#include <CrashDumper.hpp>
#endif
#include <DynamicOutput/DynamicOutput.hpp>

#ifdef HAS_UI
#include <imgui.h>
#include <GUI/GUI.hpp>
#include <GUI/GUITab.hpp>
#endif

#ifdef HAS_INPUT
#include <Input/Handler.hpp>
#endif

#include <LuaLibrary.hpp>
#include <MProgram.hpp>
#include <Mod/CppMod.hpp>
#include <Mod/LuaMod.hpp>
#include <Mod/Mod.hpp>
#include <SettingsManager.hpp>
#include <Unreal/TArray.hpp>
#include <Unreal/UnrealVersion.hpp>

// Used to set up ImGui context and allocator in DLL mods
#define UE4SS_ENABLE_IMGUI()                                                                                                                                   \
    {                                                                                                                                                          \
        ImGui::SetCurrentContext(UE4SSProgram::get_current_imgui_context());                                                                                   \
        ImGuiMemAllocFunc alloc_func{};                                                                                                                        \
        ImGuiMemFreeFunc free_func{};                                                                                                                          \
        void* user_data{};                                                                                                                                     \
        UE4SSProgram::get_current_imgui_allocator_functions(&alloc_func, &free_func, &user_data);                                                              \
        ImGui::SetAllocatorFunctions(alloc_func, free_func, user_data);                                                                                        \
    }

namespace RC
{
    namespace Unreal
    {
        class UObject;
        class UObjectBase;
        class UObjectBaseUtility;
        class UWorld;
    } // namespace Unreal

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
        constexpr static SystemCharType m_settings_file_name[] = SYSSTR("UE4SS-settings.ini");
        constexpr static SystemCharType m_log_file_name[] = SYSSTR("UE4SS.log");
        constexpr static SystemCharType m_object_dumper_file_name[] = SYSSTR("UE4SS_ObjectDump.txt");

      public:
        RC_UE4SS_API static SettingsManager settings_manager;
        static inline bool unreal_is_shutting_down{};

      public:
        bool m_is_program_started;

      protected:
#ifdef HAS_INPUT
        Input::Handler m_input_handler;
#endif
        std::jthread m_event_loop;
#ifdef HAS_UI
        std::jthread m_render_thread;
#endif
      private:
#ifdef WIN32
        CrashDumper m_crash_dumper{};
#endif
      private:
        std::filesystem::path m_game_path_and_exe_name;
        std::filesystem::path m_root_directory;
        std::filesystem::path m_module_file_path;
        std::filesystem::path m_working_directory;
        std::filesystem::path m_mods_directory;

        SystemStringType m_module_file_path_str;
        SystemStringType m_working_directory_str;
        SystemStringType m_game_executable_str;
        SystemStringType m_mods_directory_str;

        std::filesystem::path m_game_executable_directory;
        std::filesystem::path m_log_directory;
        std::filesystem::path m_object_dumper_output_directory;
        std::filesystem::path m_default_settings_path_and_file;
        std::filesystem::path m_settings_path_and_file;
        Output::DebugConsoleDevice* m_debug_console_device{};
        Output::ConsoleDevice* m_console_device{};

#ifdef HAS_UI
        GUI::DebuggingGUI m_debugging_gui{};
#endif

        using EventCallable = void (*)(void* data);
        struct Event
        {
            EventCallable callable{};
            void* data{};
        };
        std::vector<Event> m_queued_events{};
        std::mutex m_event_queue_mutex{};

      private:
#ifdef WIN32
        std::unique_ptr<PLH::IatHook> m_load_library_a_hook;
        uint64_t m_hook_trampoline_load_library_a;

        std::unique_ptr<PLH::IatHook> m_load_library_ex_a_hook;
        uint64_t m_hook_trampoline_load_library_ex_a;

        std::unique_ptr<PLH::IatHook> m_load_library_w_hook;
        uint64_t m_hook_trampoline_load_library_w;

        std::unique_ptr<PLH::IatHook> m_load_library_ex_w_hook;
        uint64_t m_hook_trampoline_load_library_ex_w;
#endif

      public:
        std::vector<std::unique_ptr<Mod>> m_mods;

        RecognizableStruct m_shared_functions{};

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
        UE4SSProgram(const SystemStringType& ModuleFilePath, std::initializer_list<BinaryOptions> options);
        ~UE4SSProgram();
        UE4SSProgram(const UE4SSProgram&) = delete;
        UE4SSProgram(UE4SSProgram&&) = delete;

      private:
        auto setup_paths(const SystemStringType& moduleFilePath) -> void;
        enum class FunctionStatus
        {
            Success,
            Failure,
        };
        auto create_emergency_console_for_early_error(SystemStringViewType error_message) -> void;
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
        auto fire_dll_load_for_cpp_mods(SystemStringViewType dll_name) -> void;

      public:
        auto init() -> void;
        auto is_program_started() -> bool;
        auto reinstall_mods() -> void;
        auto get_object_dumper_output_directory() -> const SystemStringType;
        RC_UE4SS_API auto get_module_directory() -> SystemStringViewType;
        RC_UE4SS_API auto get_working_directory() -> SystemStringViewType;
        RC_UE4SS_API auto get_mods_directory() -> SystemStringViewType;
        RC_UE4SS_API auto get_game_directory() -> SystemStringViewType;
        RC_UE4SS_API auto generate_uht_compatible_headers() -> void;
        RC_UE4SS_API auto generate_cxx_headers(const std::filesystem::path& output_dir) -> void;
        RC_UE4SS_API auto generate_lua_types(const std::filesystem::path& output_dir) -> void;
#ifdef HAS_INPUT
        auto get_input_handler() -> Input::Handler&
        {
            return m_input_handler;
        }
#endif
#ifdef HAS_UI
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
#ifdef WIN32
        RC_UE4SS_API static auto get_current_imgui_allocator_functions(ImGuiMemAllocFunc* alloc_func, ImGuiMemFreeFunc* free_func, void** user_data) -> void
        {
            return ImGui::GetAllocatorFunctions(alloc_func, free_func, user_data);
        }
#endif
#endif
        RC_UE4SS_API auto queue_event(EventCallable callable, void* data) -> void;
        RC_UE4SS_API auto is_queue_empty() -> bool;
        RC_UE4SS_API auto can_process_events() -> bool
        {
            return m_processing_events;
        }

      public:
        // API pass-through for use outside the private scope of UE4SSProgram
#ifdef HAS_INPUT
        RC_UE4SS_API auto register_keydown_event(Input::Key, const Input::EventCallbackCallable&, uint8_t custom_data = 0) -> void;
        RC_UE4SS_API auto register_keydown_event(Input::Key, const Input::Handler::ModifierKeyArray&, const Input::EventCallbackCallable&, uint8_t custom_data = 0)
                -> void;
        RC_UE4SS_API auto is_keydown_event_registered(Input::Key) -> bool;
        RC_UE4SS_API auto is_keydown_event_registered(Input::Key, const Input::Handler::ModifierKeyArray&) -> bool;
#endif

      private:
        static auto install_cpp_mods() -> void;
        static auto install_lua_mods() -> void;

        using FMBNI_ExtraPredicate = std::function<bool(Mod*)>;
        static auto find_mod_by_name_internal(SystemStringViewType mod_name,
                                              IsInstalled = IsInstalled::No,
                                              IsStarted = IsStarted::No,
                                              FMBNI_ExtraPredicate extra_predicate = {}) -> Mod*;

      public:
        RC_UE4SS_API static auto dump_uobject(Unreal::UObject* object, std::unordered_set<Unreal::FField*>* dumped_fields, SystemStringType& out_line, bool is_below_425)
                -> void;
        RC_UE4SS_API static auto dump_xproperty(Unreal::FProperty* property, SystemStringType& out_line) -> void;
        RC_UE4SS_API static auto dump_all_objects_and_properties(const SystemStringType& output_path_and_file_name) -> void;

        template <typename T>
        static auto find_mod_by_name(SystemStringType mod_name, IsInstalled is_installed = IsInstalled::No, IsStarted is_started = IsStarted::No) -> T*
        {
            return static_cast<T*>(find_mod_by_name_internal(mod_name, is_installed, is_started, [](auto elem) -> bool {
                return dynamic_cast<T*>(elem);
            }));
        };

        template <typename T>
        static auto find_mod_by_name(SystemStringViewType mod_name, IsInstalled is_installed = IsInstalled::No, IsStarted is_started = IsStarted::No) -> T*
        {
            return find_mod_by_name<T>(to_system_string(mod_name), is_installed, is_started);
        };

        template <typename S>
        RC_UE4SS_API static auto find_lua_mod_by_name(S mod_name, IsInstalled is_installed = IsInstalled::No, IsStarted is_started = IsStarted::No) -> LuaMod*
        {
            auto mod_name_str = to_system_string(mod_name);
            return static_cast<LuaMod*>(find_mod_by_name<LuaMod>(mod_name_str, is_installed, is_started));
        }
        static auto static_cleanup() -> void;
        RC_UE4SS_API static auto get_program() -> UE4SSProgram&
        {
            return *s_program;
        }

      private:
#ifdef WIN32
        friend void* HookedLoadLibraryA(const char* dll_name);
        friend void* HookedLoadLibraryExA(const char* dll_name, void* file, int32_t flags);
        friend void* HookedLoadLibraryW(const wchar_t* dll_name);
        friend void* HookedLoadLibraryExW(const wchar_t* dll_name, void* file, int32_t flags);
#endif
    };
} // namespace RC
