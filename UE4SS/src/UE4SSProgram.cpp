#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#ifdef TEXT
#undef TEXT
#endif

#include <algorithm>
#include <cwctype>
#include <format>
#include <fstream>
#include <limits>
#include <unordered_set>

#include <Profiler/Profiler.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <ExceptionHandling.hpp>
#ifdef HAS_UI
#include <GUI/ConsoleOutputDevice.hpp>
#include <GUI/GUI.hpp>
#include <GUI/LiveView.hpp>
#endif
#include <Helpers/ASM.hpp>
#include <Helpers/Integer.hpp>
#include <Helpers/String.hpp>
#include <IniParser/Ini.hpp>
#include <LuaLibrary.hpp>
#include <LuaType/LuaCustomProperty.hpp>
#include <LuaType/LuaUObject.hpp>
#include <Mod/CppMod.hpp>
#include <Mod/LuaMod.hpp>
#include <Mod/Mod.hpp>
#include <ObjectDumper/ObjectToString.hpp>
#include <SDKGenerator/Generator.hpp>
#include <SDKGenerator/UEHeaderGenerator.hpp>
#include <SigScanner/SinglePassSigScanner.hpp>
#include <Signatures.hpp>
#include <Timer/ScopedTimer.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/AGameMode.hpp>
#include <Unreal/AGameModeBase.hpp>
#include <Unreal/GameplayStatics.hpp>
#include <Unreal/Searcher/ObjectSearcher.hpp>
#include <Unreal/TPair.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/TypeChecker.hpp>
#include <Unreal/UActorComponent.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/UKismetSystemLibrary.hpp>
#include <Unreal/ULocalPlayer.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/World.hpp>
#include <UnrealDef.hpp>

#include <Platform.hpp>

namespace RC
{
    // Commented out because this system (turn off hotkeys when in-game console is open) it doesn't work properly.
    /*
    struct RC_UE_API FUEDeathListener : public Unreal::FUObjectCreateListener
    {
        static FUEDeathListener UEDeathListener;

        void NotifyUObjectCreated(const Unreal::UObjectBase* object, int32_t index) override {}
        void OnUObjectArrayShutdown() override
        {
            UE4SSProgram::unreal_is_shutting_down = true;
            Unreal::UObjectArray::RemoveUObjectCreateListener(this);
        }
    };
    FUEDeathListener FUEDeathListener::UEDeathListener{};

    auto get_player_controller() -> UObject*
    {
        std::vector<Unreal::UObject*> player_controllers{};
        UObjectGlobals::FindAllOf(STR("PlayerController"), player_controllers);
        if (!player_controllers.empty())
        {
            return player_controllers.back();
        }
        else
        {
            return nullptr;
        }
    }
    //*/

    SettingsManager UE4SSProgram::settings_manager{};

#define OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(StructName)                                                                                                           \
    for (const auto& [name, offset] : Unreal::StructName::MemberOffsets)                                                                                       \
    {                                                                                                                                                          \
        Output::send(SYSSTR(#StructName "::{} = 0x{:X}\n"), name, offset);                                                                                     \
    }

    auto output_all_member_offsets() -> void
    {
        Output::send(SYSSTR("\n##### MEMBER OFFSETS START #####\n\n"));
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UObjectBase);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UScriptStruct);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UScriptStruct::ICppStructOps);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FField);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FOutputDevice);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FEnumProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UStruct);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UFunction);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UField);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UWorld);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UClass);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UEnum);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FObjectPropertyBase);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FDelegateProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FMulticastDelegateProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FSetProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FStructProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FArrayProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FMapProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FBoolProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FByteProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FClassProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FSoftClassProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FInterfaceProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FFieldPathProperty);
        Output::send(SYSSTR("\n##### MEMBER OFFSETS END #####\n\n"));
    }

#ifdef WIN32
    void* HookedLoadLibraryA(const char* dll_name)
    {
        UE4SSProgram& program = UE4SSProgram::get_program();
        HMODULE lib = PLH::FnCast(program.m_hook_trampoline_load_library_a, &LoadLibraryA)(dll_name);
        program.fire_dll_load_for_cpp_mods(to_system(dll_name));
        return lib;
    }

    void* HookedLoadLibraryExA(const char* dll_name, void* file, int32_t flags)
    {
        UE4SSProgram& program = UE4SSProgram::get_program();
        HMODULE lib = PLH::FnCast(program.m_hook_trampoline_load_library_ex_a, &LoadLibraryExA)(dll_name, file, flags);
        program.fire_dll_load_for_cpp_mods(to_system(dll_name));
        return lib;
    }

    void* HookedLoadLibraryW(const wchar_t* dll_name)
    {
        UE4SSProgram& program = UE4SSProgram::get_program();
        HMODULE lib = PLH::FnCast(program.m_hook_trampoline_load_library_w, &LoadLibraryW)(dll_name);
        program.fire_dll_load_for_cpp_mods(dll_name);
        return lib;
    }

    void* HookedLoadLibraryExW(const wchar_t* dll_name, void* file, int32_t flags)
    {
        UE4SSProgram& program = UE4SSProgram::get_program();
        HMODULE lib = PLH::FnCast(program.m_hook_trampoline_load_library_ex_w, &LoadLibraryExW)(dll_name, file, flags);
        program.fire_dll_load_for_cpp_mods(dll_name);
        return lib;
    }
#endif

    UE4SSProgram::UE4SSProgram(const SystemStringType& moduleFilePath, std::initializer_list<BinaryOptions> options) : MProgram(options)
    {
        ProfilerScope();
        s_program = this;

        try
        {
            setup_paths(moduleFilePath);

            try
            {
                settings_manager.deserialize(m_settings_path_and_file);
            }
            catch (std::exception& e)
            {
                create_emergency_console_for_early_error(std::format(SYSSTR("The IniParser failed to parse: {}"), to_system(e.what())));
                return;
            }

#ifdef WIN32
            if (settings_manager.CrashDump.EnableDumping)
            {
                m_crash_dumper.enable();
            }

            m_crash_dumper.set_full_memory_dump(settings_manager.CrashDump.FullMemoryDump);
#endif

#ifdef HAS_UI
            m_debugging_gui.set_gfx_backend(settings_manager.Debug.GraphicsAPI);
#endif

#ifdef HAS_INPUT
            m_input_handler.init();
#endif

            // Setup the log file
            auto& file_device = Output::set_default_devices<Output::NewFileDevice>();
            file_device.set_file_name_and_path(to_system(m_log_directory / m_log_file_name));

            create_simple_console();

#ifdef HAS_UI
            if (settings_manager.Debug.DebugConsoleEnabled)
            {
                m_console_device = &Output::set_default_devices<Output::ConsoleDevice>();
                m_console_device->set_formatter([](SystemStringViewType string) -> SystemStringType {
                    return std::format(SYSSTR("[{}] {}"), std::format(SYSSTR("{:%X}"), std::chrono::system_clock::now()), string);
                });
                if (settings_manager.Debug.DebugConsoleVisible)
                {
                    m_render_thread = std::jthread{&GUI::gui_thread, &m_debugging_gui};
                }
            }
#endif
            // This is experimental code that's here only for future reference
            /*
            Unreal::UnrealInitializer::SetupUnrealModules();

            constexpr const wchar_t* str_to_find = STR("Allocator: %s");
            void* string_address = SinglePassScanner::string_scan(str_to_find, ScanTarget::Core);
            Output::send(SYSSTR("\n\nFound string '{}' at {}\n\n"), SystemStringViewType{str_to_find}, string_address);
            //*/

            Output::send(SYSSTR("Console created\n"));
            Output::send(SYSSTR("UE4SS - v{}.{}.{}{}{} - Git SHA #{}\n"),
                         UE4SS_LIB_VERSION_MAJOR,
                         UE4SS_LIB_VERSION_MINOR,
                         UE4SS_LIB_VERSION_HOTFIX,
                         std::format(SYSSTR("{}"),
                                     UE4SS_LIB_VERSION_PRERELEASE == 0 ? SYSSTR("") : std::format(SYSSTR(" PreRelease #{}"), UE4SS_LIB_VERSION_PRERELEASE)),
                         std::format(SYSSTR("{}"),
                                     UE4SS_LIB_BETA_STARTED == 0
                                             ? SYSSTR("")
                                             : (UE4SS_LIB_IS_BETA == 0 ? SYSSTR(" Beta #?") : std::format(SYSSTR(" Beta #{}"), UE4SS_LIB_VERSION_BETA))),
                         to_system(UE4SS_LIB_BUILD_GITSHA));

#ifdef __clang__
#define UE4SS_COMPILER SYSSTR("Clang")
#else
#define UE4SS_COMPILER SYSSTR("MSVC")
#endif

            Output::send(SYSSTR("UE4SS Build Configuration: {} ({})\n"), to_system(UE4SS_CONFIGURATION), UE4SS_COMPILER);
#ifdef WIN32
            m_load_library_a_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                   "LoadLibraryA",
                                                                   std::bit_cast<uint64_t>(&HookedLoadLibraryA),
                                                                   &m_hook_trampoline_load_library_a,
                                                                   SYSSTR(""));
            m_load_library_a_hook->hook();

            m_load_library_ex_a_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                      "LoadLibraryExA",
                                                                      std::bit_cast<uint64_t>(&HookedLoadLibraryExA),
                                                                      &m_hook_trampoline_load_library_ex_a,
                                                                      SYSSTR(""));
            m_load_library_ex_a_hook->hook();

            m_load_library_w_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                   "LoadLibraryW",
                                                                   std::bit_cast<uint64_t>(&HookedLoadLibraryW),
                                                                   &m_hook_trampoline_load_library_w,
                                                                   SYSSTR(""));
            m_load_library_w_hook->hook();

            m_load_library_ex_w_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                      "LoadLibraryExW",
                                                                      std::bit_cast<uint64_t>(&HookedLoadLibraryExW),
                                                                      &m_hook_trampoline_load_library_ex_w,
                                                                      SYSSTR(""));
            m_load_library_ex_w_hook->hook();
#endif
            Unreal::UnrealInitializer::Platform::SetupUnrealModules();

            setup_mods();
            install_cpp_mods();
            start_cpp_mods();

            setup_mod_directory_path();

            if (m_has_game_specific_config)
            {
                Output::send(SYSSTR("Found configuration for game: {}\n"), m_mods_directory.parent_path().filename().c_str());
            }
            else
            {
                Output::send(SYSSTR("No specific game configuration found, using default configuration file\n"));
            }

            Output::send(SYSSTR("Config: {}\n\n"), m_settings_path_and_file.c_str());
            Output::send(SYSSTR("root directory: {}\n"), m_root_directory.c_str());
            Output::send(SYSSTR("working directory: {}\n"), m_working_directory.c_str());
            Output::send(SYSSTR("game executable directory: {}\n"), m_game_executable_directory.c_str());
            Output::send(SYSSTR("game executable: {} ({} bytes)\n\n\n"), m_game_path_and_exe_name.c_str(), std::filesystem::file_size(m_game_path_and_exe_name));
            Output::send(SYSSTR("mods directory: {}\n"), m_mods_directory.c_str());
            Output::send(SYSSTR("log directory: {}\n"), m_log_directory.c_str());
            Output::send(SYSSTR("object dumper directory: {}\n\n\n"), m_object_dumper_output_directory.c_str());
        }
        catch (std::runtime_error& e)
        {
            // Returns to main from here which checks, displays & handles whether to close the program or not
            // If has_error() returns false that means that set_error was not called
            // In that case we need to copy the exception message to the error buffer before we return to main
            if (!m_error_object->has_error())
            {
                copy_error_into_message(e.what());
            }
            return;
        }
    }

    UE4SSProgram::~UE4SSProgram()
    {
        // Shut down the event loop
        m_processing_events = false;

        // It's possible that main() will destroy the default devices (they are static)
        // However it's also possible that this program object is constructed in a context where main() is not gonna immediately exit
        // Because of that and because the default devices are created in the constructor, it's preferred to explicitly close all default devices in the destructor
        Output::close_all_default_devices();
    }

    auto UE4SSProgram::init() -> void
    {
        ProfilerSetThreadName("UE4SS-InitThread");
        ProfilerScope();

        try
        {
            setup_unreal();

            Output::send(SYSSTR("Unreal Engine modules ({}):\n"), SigScannerStaticData::m_is_modular ? SYSSTR("modular") : SYSSTR("non-modular"));
#ifdef WIN32
            auto& main_exe_ptr = SigScannerStaticData::m_modules_info.array[static_cast<size_t>(ScanTarget::MainExe)].lpBaseOfDll;
            for (size_t i = 0; i < static_cast<size_t>(ScanTarget::Max); ++i)
            {
                auto& module = SigScannerStaticData::m_modules_info.array[i];
                // only log modules with unique addresses (non-modular builds have everything in MainExe)
                if (i == static_cast<size_t>(ScanTarget::MainExe) || main_exe_ptr != module.lpBaseOfDll)
                {
                    auto module_name = to_system(ScanTargetToString(i));
                    Output::send(SYSSTR("{} @ {} size={:#x}\n"), module_name.c_str(), module.lpBaseOfDll, module.SizeOfImage);
                }
            }
#elif defined(LINUX)
            auto& main_exe_ptr = SigScannerStaticData::m_modules_info.array[static_cast<size_t>(ScanTarget::MainExe)].base_address;
            for (size_t i = 0; i < static_cast<size_t>(ScanTarget::Max); ++i)
            {
                auto& module = SigScannerStaticData::m_modules_info.array[i];
                // only log modules with unique addresses (non-modular builds have everything in MainExe)
                if (i == static_cast<size_t>(ScanTarget::MainExe) || main_exe_ptr != module.base_address)
                {
                    auto module_name = to_system(ScanTargetToString(i));
                    // FIXME: FIX Why this won't WORK?
                    // Output::send(SYSSTR("{} @ {} size={:#x}\n"), module_name, module.base_address, module.size);
                }
            }
#else

#endif
            fire_unreal_init_for_cpp_mods();
            setup_unreal_properties();
            UAssetRegistry::SetMaxMemoryUsageDuringAssetLoading(settings_manager.Memory.MaxMemoryUsageDuringAssetLoading);

            output_all_member_offsets();

            share_lua_functions();

            // Only deal with the event loop thread here if the 'Test' constructor doesn't need to be called
#ifndef RUN_TESTS
            // Program is now fully setup
            // Start event loop
            m_event_loop = std::jthread{&UE4SSProgram::update, this};

            // Wait for thread
            // There's a loop inside the thread that only exits when you hit the 'End' key on the keyboard
            // As long as you don't do that the thread will stay open and accept further inputs
            m_event_loop.join();
#endif
        }
        catch (std::runtime_error& e)
        {
            // Returns to main from here which checks, displays & handles whether to close the program or not
            // If has_error() returns false that means that set_error was not called
            // In that case we need to copy the exception message to the error buffer before we return to main
            if (!m_error_object->has_error())
            {
                copy_error_into_message(e.what());
            }
            return;
        }
    }

    auto UE4SSProgram::setup_paths(const SystemStringType& moduleFilePathString) -> void
    {
        ProfilerScope();
        const std::filesystem::path moduleFilePath = std::filesystem::path(moduleFilePathString);
        m_root_directory = to_system(moduleFilePath.parent_path());
        m_module_file_path = to_system(moduleFilePath);

        // The default working directory is the root directory
        // Can be changed by creating a <GameName> directory in the root directory
        // At that point, the working directory will be "root/<GameName>"
        m_working_directory = m_root_directory;

        // Default file to open if there is no game specific config
        m_default_settings_path_and_file = m_root_directory / m_settings_file_name;

        std::filesystem::path game_exe_path = get_executable_path();
        std::filesystem::path game_directory_path = game_exe_path.parent_path();
        m_working_directory = m_root_directory;
        m_mods_directory = m_working_directory / "Mods";
        m_game_executable_directory = game_directory_path /*game_exe_path.parent_path()*/;
        m_settings_path_and_file = m_root_directory;
        m_game_path_and_exe_name = game_exe_path;
        m_object_dumper_output_directory = m_game_executable_directory;

        // Allow loading of DLLs from mod folders
        add_dlsearch_folder(game_exe_path);

        for (const auto& item : std::filesystem::directory_iterator(m_root_directory))
        {
            if (!item.is_directory())
            {
                continue;
            }

            if (item.path().filename() == game_directory_path.parent_path().parent_path().parent_path().filename())
            {
                m_has_game_specific_config = true;
                m_working_directory = item.path();
                m_mods_directory = item.path() / SYSSTR("Mods");
                m_settings_path_and_file = std::move(item.path());
                m_log_directory = m_working_directory;
                m_object_dumper_output_directory = m_working_directory;
                break;
            }
        }

        m_log_directory = m_working_directory;
        auto resolaved_settings_file = File::get_path_if_exists(m_settings_path_and_file, m_settings_file_name);
        if (resolaved_settings_file)
        {
            m_settings_path_and_file = *resolaved_settings_file;
        }
        else
        {
            throw std::runtime_error{"UE4SS-Settings.ini file not found"};
        }
    }

    auto UE4SSProgram::create_emergency_console_for_early_error(SystemStringViewType error_message) -> void
    {
        settings_manager.Debug.SimpleConsoleEnabled = true;
        create_simple_console();
        printf_s(SystemStringPrint "\n", error_message.data());
    }

    auto UE4SSProgram::setup_mod_directory_path() -> void
    {
        // Mods folder path, typically '<m_working_directory>\Mods'
        // Can be customized via UE4SS-settings.ini
        if (settings_manager.Overrides.ModsFolderPath.empty())
        {
            m_mods_directory = m_mods_directory.empty() ? m_working_directory / "Mods" : m_mods_directory;
        }
        else
        {
            m_mods_directory = settings_manager.Overrides.ModsFolderPath;
        }
    }

    auto UE4SSProgram::create_simple_console() -> void
    {
        if (settings_manager.Debug.SimpleConsoleEnabled)
        {
            m_debug_console_device = &Output::set_default_devices<Output::DebugConsoleDevice>();
            Output::set_default_log_level<LogLevel::Normal>();
            m_debug_console_device->set_formatter([](SystemStringViewType string) -> SystemStringType {
                return std::format(SYSSTR("[{}] {}"), std::format(SYSSTR("{:%X}"), std::chrono::system_clock::now()), string);
            });
#ifdef WIN32
            if (AllocConsole())
            {
                FILE* stdin_filename;
                FILE* stdout_filename;
                FILE* stderr_filename;
                freopen_s(&stdin_filename, "CONIN$", "r", stdin);
                freopen_s(&stdout_filename, "CONOUT$", "w", stdout);
                freopen_s(&stderr_filename, "CONOUT$", "w", stderr);
            }
#endif
        }
    }

    auto UE4SSProgram::load_unreal_offsets_from_file() -> void
    {
        std::filesystem::path file_path = m_working_directory / "MemberVariableLayout.ini";
        if (std::filesystem::exists(file_path))
        {
            auto file = File::open(file_path);
            if (auto file_contents = file.read_file_all(); !file_contents.empty())
            {
                Ini::Parser parser;
                auto content = to_system_string(file_contents);
                parser.parse(content);
                file.close();

                // The following code is auto-generated.
#include <MacroSetter.hpp>
            }
        }
    }

    auto UE4SSProgram::setup_unreal() -> void
    {
        ProfilerScope();
        // Retrieve offsets from the config file
        const SystemStringType offset_overrides_section{SYSSTR("OffsetOverrides")};

        load_unreal_offsets_from_file();

        Unreal::UnrealInitializer::Config config;
        config.CachePath = m_root_directory / "cache";
        config.bInvalidateCacheIfSelfChanged = settings_manager.General.InvalidateCacheIfDLLDiffers;
        config.bEnableCache = settings_manager.General.UseCache;
        config.SecondsToScanBeforeGivingUp = settings_manager.General.SecondsToScanBeforeGivingUp;
        config.bUseUObjectArrayCache = settings_manager.General.UseUObjectArrayCache;

        // Retrieve from the config file the number of threads to be used for aob scanning
        {
            // The config system only directly supports signed 64-bit integers
            // I'm using '-1' for the default and then only proceeding with using the value from the config file if it's within the
            // range of an unsigned 32-bit integer (which is what the SinglePassScanner uses)
            // The variables for these settings are default initialized with valid values so no need to set them if the config value
            // was either missing or invalid
            int64_t num_threads_for_scanner_from_config = settings_manager.Threads.SigScannerNumThreads;

            // The scanner is expecting a uint32_t so lets make sure we can safely convert to a uint32_t
            if (num_threads_for_scanner_from_config <= std::numeric_limits<uint32_t>::max() && num_threads_for_scanner_from_config >= 1)
            {
                config.NumScanThreads = static_cast<uint32_t>(num_threads_for_scanner_from_config);
            }
        }

        {
            int64_t multithreading_module_size_threshold_from_config = settings_manager.Threads.SigScannerMultithreadingModuleSizeThreshold;

            if (multithreading_module_size_threshold_from_config <= std::numeric_limits<uint32_t>::max() &&
                multithreading_module_size_threshold_from_config >= std::numeric_limits<uint32_t>::min())
            {
                config.MultithreadingModuleSizeThreshold = static_cast<uint32_t>(multithreading_module_size_threshold_from_config);
            }
        }

        // Version override from ini file
        {
            int64_t major_version = settings_manager.EngineVersionOverride.MajorVersion;
            int64_t minor_version = settings_manager.EngineVersionOverride.MinorVersion;

            if (major_version != -1 && minor_version != -1)
            {
                // clang-format off
                if (major_version < std::numeric_limits<uint32_t>::min() ||
                    major_version > std::numeric_limits<uint32_t>::max() ||
                    minor_version < std::numeric_limits<uint32_t>::min() ||
                    minor_version > std::numeric_limits<uint32_t>::max())
                {
                    throw std::runtime_error{
                            "Was unable to override engine version from ini file; The number in the ini file must be in range of a uint32"};
                }
                // clang-format on

                Unreal::Version::Major = static_cast<uint32_t>(major_version);
                Unreal::Version::Minor = static_cast<uint32_t>(minor_version);

                config.ScanOverrides.version_finder = [&]([[maybe_unused]] auto&, Unreal::Signatures::ScanResult&) {};
            }
        }

        // If any Lua scripts are found, add overrides so that the Lua script can perform the aob scan instead of the Unreal API itself
        setup_lua_scan_overrides(m_working_directory, config);

        // Virtual function offset overrides
        TRY([&]() {
            ProfilerScopeNamed("loading virtual function offset overrides");
            fprintf(stderr, "Loading offset\n");
            static auto virtual_function_offset_override_file{m_working_directory / SYSSTR("VTableLayout.ini")};
            if (std::filesystem::exists(virtual_function_offset_override_file))
            {
                auto file =
                        File::open(virtual_function_offset_override_file, File::OpenFor::Reading, File::OverwriteExistingFile::No, File::CreateIfNonExistent::No);
                Ini::Parser parser;
                parser.parse(file);

                Output::send<Color::Blue>(SYSSTR("Getting ordered lists from ini file\n"));

                auto calculate_virtual_function_offset = []<typename... BaseSizes>(uint32_t current_index, BaseSizes... base_sizes) -> uint32_t {
                    return current_index == 0 ? 0 : (current_index + (base_sizes + ...)) * 8;
                };

                auto retrieve_vtable_layout_from_ini = [&](const SystemStringType& section_name, auto callable) -> uint32_t {
                    auto list = parser.get_ordered_list(section_name);
                    uint32_t vtable_size = list.size() - 1;
                    list.for_each([&](uint32_t index, SystemStringType& item) {
                        auto ue_str = to_ue(item);
                        callable(index, item, ue_str);
                    });
                    return vtable_size;
                };

                Output::send<Color::Blue>(SYSSTR("UObjectBase\n"));
                uint32_t uobjectbase_size =
                        retrieve_vtable_layout_from_ini(SYSSTR("UObjectBase"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                            uint32_t offset = calculate_virtual_function_offset(index, 0);
                            Output::send(SYSSTR("UObjectBase::{} = 0x{:X}\n"), item, offset);
                            Unreal::UObjectBase::VTableLayoutMap.emplace(item_ue, offset);
                        });

                Output::send<Color::Blue>(SYSSTR("UObjectBaseUtility\n"));
                uint32_t uobjectbaseutility_size =
                        retrieve_vtable_layout_from_ini(SYSSTR("UObjectBaseUtility"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                            uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size);
                            Output::send(SYSSTR("UObjectBaseUtility::{} = 0x{:X}\n"), item, offset);
                            Unreal::UObjectBaseUtility::VTableLayoutMap.emplace(item_ue, offset);
                        });

                Output::send<Color::Blue>(SYSSTR("UObject\n"));
                uint32_t uobject_size = retrieve_vtable_layout_from_ini(SYSSTR("UObject"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size);
                    Output::send(SYSSTR("UObject::{} = 0x{:X}\n"), item, offset);
                    Unreal::UObject::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("UField\n"));
                uint32_t ufield_size = retrieve_vtable_layout_from_ini(SYSSTR("UField"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size);
                    Output::send(SYSSTR("UField::{} = 0x{:X}\n"), item, offset);
                    Unreal::UField::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("UEngine\n"));
                uint32_t uengine_size = retrieve_vtable_layout_from_ini(SYSSTR("UEngine"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size);
                    Output::send(SYSSTR("UEngine::{} = 0x{:X}\n"), item, offset);
                    Unreal::UEngine::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("UScriptStruct::ICppStructOps\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("UScriptStruct::ICppStructOps"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, 0);
                    Output::send(SYSSTR("UScriptStruct::ICppStructOps::{} = 0x{:X}\n"), item, offset);
                    Unreal::UScriptStruct::ICppStructOps::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("FField\n"));
                uint32_t ffield_size = retrieve_vtable_layout_from_ini(SYSSTR("FField"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, 0);
                    Output::send(SYSSTR("FField::{} = 0x{:X}\n"), item, offset);
                    Unreal::FField::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("FProperty\n"));
                uint32_t fproperty_size = retrieve_vtable_layout_from_ini(SYSSTR("FProperty"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset{};
                    if (Unreal::Version::IsBelow(4, 25))
                    {
                        offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, ufield_size);
                    }
                    else
                    {
                        offset = calculate_virtual_function_offset(index, ffield_size);
                    }
                    Output::send(SYSSTR("FProperty::{} = 0x{:X}\n"), item, offset);
                    Unreal::FProperty::VTableLayoutMap.emplace(item_ue, offset);
                });

                // If the engine version is <4.25 then the inheritance is different and we must take that into consideration.
                if (Unreal::Version::IsBelow(4, 25))
                {
                    fproperty_size = uobjectbase_size + uobjectbaseutility_size + uobject_size + ufield_size + fproperty_size;
                }
                else
                {
                    fproperty_size = ffield_size + fproperty_size;
                }

                Output::send<Color::Blue>(SYSSTR("FNumericProperty\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("FNumericProperty"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, fproperty_size);
                    Output::send(SYSSTR("FNumericProperty::{} = 0x{:X}\n"), item, offset);
                    Unreal::FNumericProperty::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("FMulticastDelegateProperty\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("FMulticastDelegateProperty"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, fproperty_size);
                    Output::send(SYSSTR("FMulticastDelegateProperty::{} = 0x{:X}\n"), item, offset);
                    Unreal::FMulticastDelegateProperty::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("FObjectPropertyBase\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("FObjectPropertyBase"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, fproperty_size);
                    Output::send(SYSSTR("FObjectPropertyBase::{} = 0x{:X}\n"), item, offset);
                    Unreal::FObjectPropertyBase::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("UStruct\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("UStruct"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, ufield_size);
                    Output::send(SYSSTR("UStruct::{} = 0x{:X}\n"), item, offset);
                    Unreal::UStruct::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("FOutputDevice\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("FOutputDevice"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, 0);
                    Output::send(SYSSTR("FOutputDevice::{} = 0x{:X}\n"), item, offset);
                    Unreal::FOutputDevice::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("FMalloc\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("FMalloc"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    // We don't support FExec, so we're manually telling it the size.
                    static constexpr uint32_t fexec_size = 1;
                    uint32_t offset = calculate_virtual_function_offset(index, fexec_size);
                    Output::send(SYSSTR("FMalloc::{} = 0x{:X}\n"), item, offset);
                    Unreal::FMalloc::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("AActor\n"));
                uint32_t aactor_size = retrieve_vtable_layout_from_ini(SYSSTR("AActor"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size);
                    Output::send(SYSSTR("AActor::{} = 0x{:X}\n"), item, offset);
                    Unreal::AActor::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("AGameModeBase\n"));
                uint32_t agamemodebase_size =
                        retrieve_vtable_layout_from_ini(SYSSTR("AGameModeBase"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                            uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, aactor_size);
                            Output::send(SYSSTR("AGameModeBase::{} = 0x{:X}\n"), item, offset);
                            Unreal::AGameModeBase::VTableLayoutMap.emplace(item_ue, offset);
                        });

                Output::send<Color::Blue>(SYSSTR("AGameMode\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("AGameMode"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index,
                                                                        Unreal::Version::IsAtLeast(4, 14)
                                                                        ? uobjectbase_size,
                                                                        uobjectbaseutility_size,
                                                                        uobject_size,
                                                                        aactor_size,
                                                                        agamemodebase_size
                                                                        : uobjectbase_size,
                                                                        uobjectbaseutility_size,
                                                                        uobject_size,
                                                                        aactor_size);
                    Output::send(SYSSTR("AGameMode::{} = 0x{:X}\n"), item, offset);
                    Unreal::AGameMode::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("UPlayer\n"));
                uint32_t uplayer_size = retrieve_vtable_layout_from_ini(SYSSTR("UPlayer"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size);
                    Output::send(SYSSTR("UPlayer::{} = 0x{:X}\n"), item, offset);
                    Unreal::UPlayer::VTableLayoutMap.emplace(item_ue, offset);
                });

                Output::send<Color::Blue>(SYSSTR("ULocalPlayer\n"));
                retrieve_vtable_layout_from_ini(SYSSTR("ULocalPlayer"), [&](uint32_t index, SystemStringType& item, UEStringType& item_ue) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, uplayer_size);
                    Output::send(SYSSTR("ULocalPlayer::{} = 0x{:X}\n"), item, offset);
                    Unreal::ULocalPlayer::VTableLayoutMap.emplace(item_ue, offset);
                });

                file.close();
            }

            fprintf(stderr, "Loading offset done\n");
        });

        fprintf(stderr, "Setting hooks\n");
        config.bHookProcessInternal = settings_manager.Hooks.HookProcessInternal;
        config.bHookProcessLocalScriptFunction = settings_manager.Hooks.HookProcessLocalScriptFunction;
        config.bHookLoadMap = settings_manager.Hooks.HookLoadMap;
        config.bHookInitGameState = settings_manager.Hooks.HookInitGameState;
        config.bHookCallFunctionByNameWithArguments = settings_manager.Hooks.HookCallFunctionByNameWithArguments;
        config.bHookBeginPlay = settings_manager.Hooks.HookBeginPlay;
        config.bHookLocalPlayerExec = settings_manager.Hooks.HookLocalPlayerExec;
        config.FExecVTableOffsetInLocalPlayer = settings_manager.Hooks.FExecVTableOffsetInLocalPlayer;
        fprintf(stderr, "Before UnrealInitializer::Initialize\n");
        Unreal::UnrealInitializer::Initialize(config);

        bool can_create_custom_events{true};
        if (!UObject::ProcessLocalScriptFunctionInternal.is_ready() && Unreal::Version::IsAtLeast(4, 22))
        {
            can_create_custom_events = false;
            Output::send<LogLevel::Warning>(SYSSTR("ProcessLocalScriptFunction is not available, the following features will be unavailable:\n"));
        }
        else if (!UObject::ProcessInternalInternal.is_ready() && Unreal::Version::IsBelow(4, 22))
        {
            can_create_custom_events = false;
            Output::send<LogLevel::Warning>(SYSSTR("ProcessInternal is not available, the following features will be unavailable:\n"));
        }
        if (!can_create_custom_events)
        {
            Output::send<LogLevel::Warning>(SYSSTR("<Put function here responsible for creating custom UFunctions or events for BPs>\n"));
        }
    }

    auto UE4SSProgram::share_lua_functions() -> void
    {
        m_shared_functions.set_script_variable_int32_function = &LuaLibrary::set_script_variable_int32;
        m_shared_functions.set_script_variable_default_data_function = &LuaLibrary::set_script_variable_default_data;
        m_shared_functions.call_script_function_function = &LuaLibrary::call_script_function;
        m_shared_functions.is_ue4ss_initialized_function = &LuaLibrary::is_ue4ss_initialized;
        Output::send(SYSSTR("m_shared_functions: {}\n"), static_cast<void*>(&m_shared_functions));
    }

    auto UE4SSProgram::on_program_start() -> void
    {
        ProfilerScope();
        using namespace Unreal;

        // Commented out because this system (turn off hotkeys when in-game console is open) it doesn't work properly.
        /*
        UObjectArray::AddUObjectCreateListener(&FUEDeathListener::UEDeathListener);
        //*/
#ifdef HAS_UI
        if (settings_manager.Debug.DebugConsoleEnabled)
        {
            if (settings_manager.General.UseUObjectArrayCache)
            {
                m_debugging_gui.get_live_view().set_listeners_allowed(true);
                m_debugging_gui.get_live_view().set_listeners();
            }
            else
            {
                m_debugging_gui.get_live_view().set_listeners_allowed(false);
            }
#ifdef HAS_INPUT
            m_input_handler.register_keydown_event(Input::Key::O, {Input::ModifierKey::CONTROL}, [&]() {
                TRY([&] {
                    auto was_gui_open = get_debugging_ui().is_open();
                    stop_render_thread();
                    if (!was_gui_open)
                    {
                        m_render_thread = std::jthread{&GUI::gui_thread, &m_debugging_gui};
                    }
                });
            });
#endif
        }
#endif

#ifdef TIME_FUNCTION_MACRO_ENABLED
        m_input_handler.register_keydown_event(Input::Key::Y, {Input::ModifierKey::CONTROL}, [&]() {
            if (FunctionTimerFrame::s_timer_enabled)
            {
                FunctionTimerFrame::stop_profiling();
                FunctionTimerFrame::dump_profile();
                Output::send(SYSSTR("Profiler stopped & dumped\n"));
            }
            else
            {
                FunctionTimerFrame::start_profiling();
                Output::send(SYSSTR("Profiler started\n"));
            }
        });
#endif

        TRY([&] {
            ObjectDumper::init();
#ifdef HAS_INPUT
            if (settings_manager.General.EnableHotReloadSystem)
            {
                m_input_handler.register_keydown_event(Input::Key::R, {Input::ModifierKey::CONTROL}, [&]() {
                    TRY([&] {
                        reinstall_mods();
                    });
                });
            }
#endif
            if ((settings_manager.ObjectDumper.LoadAllAssetsBeforeDumpingObjects || settings_manager.CXXHeaderGenerator.LoadAllAssetsBeforeGeneratingCXXHeaders) &&
                Unreal::Version::IsBelow(4, 17))
            {
                Output::send<LogLevel::Warning>(
                        SYSSTR("FAssetData not available in <4.17, ignoring 'LoadAllAssetsBeforeDumpingObjects' & 'LoadAllAssetsBeforeGeneratingCXXHeaders'."));
            }

#ifdef HAS_INPUT
            if (!settings_manager.General.InputSource.empty())
            {
                if (m_input_handler.set_input_source(to_string(settings_manager.General.InputSource)))
                {
                    Output::send(SYSSTR("Input source set to: {}\n"), m_input_handler.get_current_input_source());
                }
                else
                {
                    Output::send<LogLevel::Error>(SYSSTR("Failed to set input source to: {}\n"), settings_manager.General.InputSource);
                }
            }
#endif

            install_lua_mods();
            LuaMod::on_program_start();
            fire_program_start_for_cpp_mods();
            start_lua_mods();
        });

#ifdef HAS_INPUT
        if (settings_manager.General.EnableDebugKeyBindings)
        {
            m_input_handler.register_keydown_event(Input::Key::NUM_NINE, {Input::ModifierKey::CONTROL}, [&]() {
                generate_uht_compatible_headers();
            });
        }
#endif
    }

    auto UE4SSProgram::update() -> void
    {
        ProfilerSetThreadName("UE4SS-UpdateThread");

        on_program_start();

        Output::send(SYSSTR("Event loop start\n"));
        for (m_processing_events = true; m_processing_events;)
        {
            if (m_pause_events_processing || UE4SSProgram::unreal_is_shutting_down)
            {
                continue;
            }

            if (!is_queue_empty())
            {
                ProfilerScopeNamed("event processing");

                static constexpr size_t max_events_executed_per_frame = 5;
                size_t num_events_executed{};
                std::lock_guard<std::mutex> guard(m_event_queue_mutex);
                m_queued_events.erase(std::remove_if(m_queued_events.begin(),
                                                     m_queued_events.end(),
                                                     [&](Event& event) -> bool {
                                                         if (num_events_executed >= max_events_executed_per_frame)
                                                         {
                                                             return false;
                                                         }
                                                         ++num_events_executed;
                                                         event.callable(event.data);
                                                         return true;
                                                     }),
                                      m_queued_events.end());
            }

            // Commented out because this system (turn off hotkeys when in-game console is open) it doesn't work properly.
            /*
            auto* player_controller = get_player_controller();
            if (player_controller)
            {
                auto** player = player_controller->GetValuePtrByPropertyName<UObject*>(STR("Player"));
                if (player && *player)
                {
                    auto** viewportclient = (*player)->GetValuePtrByPropertyName<UObject*>(STR("ViewportClient"));
                    if (viewportclient && *viewportclient)
                    {
                        auto** console = (*viewportclient)->GetValuePtrByPropertyName<UObject*>(STR("ViewportConsole"));
                        if (console && *console)
                        {
                            auto* console_state = std::bit_cast<FName*>(static_cast<uint8_t*>((*console)->GetValuePtrByPropertyNameInChain(STR("HistoryBuffer"))) + 0x70);
                            m_input_handler.set_allow_input(console_state && *console_state == Unreal::NAME_None);
                        }
                    }
                }
            }
            //*/
#ifdef HAS_INPUT
            m_input_handler.process_event();
#endif
            {
                ProfilerScopeNamed("mod update processing");

                for (auto& mod : m_mods)
                {
                    if (mod->is_started())
                    {
                        mod->fire_update();
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            ProfilerFrameMark();
        }
        Output::send(SYSSTR("Event loop end\n"));
    }

    auto UE4SSProgram::setup_unreal_properties() -> void
    {
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("ObjectProperty")).GetComparisonIndex(), &LuaType::push_objectproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("ClassProperty")).GetComparisonIndex(), &LuaType::push_classproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("Int8Property")).GetComparisonIndex(), &LuaType::push_int8property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("Int16Property")).GetComparisonIndex(), &LuaType::push_int16property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("IntProperty")).GetComparisonIndex(), &LuaType::push_intproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("Int64Property")).GetComparisonIndex(), &LuaType::push_int64property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("ByteProperty")).GetComparisonIndex(), &LuaType::push_byteproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("UInt16Property")).GetComparisonIndex(), &LuaType::push_uint16property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("UInt32Property")).GetComparisonIndex(), &LuaType::push_uint32property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("UInt64Property")).GetComparisonIndex(), &LuaType::push_uint64property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("StructProperty")).GetComparisonIndex(), &LuaType::push_structproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("ArrayProperty")).GetComparisonIndex(), &LuaType::push_arrayproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("FloatProperty")).GetComparisonIndex(), &LuaType::push_floatproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("DoubleProperty")).GetComparisonIndex(), &LuaType::push_doubleproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("BoolProperty")).GetComparisonIndex(), &LuaType::push_boolproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("EnumProperty")).GetComparisonIndex(), &LuaType::push_enumproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("WeakObjectProperty")).GetComparisonIndex(), &LuaType::push_weakobjectproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("NameProperty")).GetComparisonIndex(), &LuaType::push_nameproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("TextProperty")).GetComparisonIndex(), &LuaType::push_textproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("StrProperty")).GetComparisonIndex(), &LuaType::push_strproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("SoftClassProperty")).GetComparisonIndex(), &LuaType::push_softclassproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("InterfaceProperty")).GetComparisonIndex(), &LuaType::push_interfaceproperty);
    }

    auto UE4SSProgram::setup_mods() -> void
    {
        ProfilerScope();

        Output::send(SYSSTR("Setting up mods...\n"));

        if (!std::filesystem::exists(m_mods_directory))
        {
            set_error("Mods directory doesn't exist, please create it: <%S>", m_mods_directory.c_str());
        }

        for (const auto& sub_directory : std::filesystem::directory_iterator(m_mods_directory))
        {
            std::error_code ec;

            // Ignore all non-directories
            if (!sub_directory.is_directory())
            {
                continue;
            }
            if (ec.value() != 0)
            {
                set_error("is_directory ran into error %d", ec.value());
            }

            SystemStringType directory_lowercase = to_system_string(sub_directory.path().stem());
            std::transform(directory_lowercase.begin(), directory_lowercase.end(), directory_lowercase.begin(), std::towlower);

            if (directory_lowercase == SYSSTR("shared"))
            {
                // Do stuff when shared libraries have been implemented
            }
            else
            {
                // Create the mod but don't install it yet
                auto scripts_directory = File::get_path_if_exists(sub_directory.path(), "Scripts");
                auto dlls_directory = File::get_path_if_exists(sub_directory.path(), "dlls");
                if (scripts_directory)
                {
                    m_mods.emplace_back(std::make_unique<LuaMod>(*this, to_system_string(sub_directory.path().stem()), to_system_string(sub_directory.path())));
                }
                if (dlls_directory)
                {
                    m_mods.emplace_back(std::make_unique<CppMod>(*this, to_system_string(sub_directory.path().stem()), to_system_string(sub_directory.path())));
                }
            }
        }
    }

    template <typename ModType>
    auto install_mods(std::vector<std::unique_ptr<Mod>>& mods) -> void
    {
        ProfilerScope();
        for (auto& mod : mods)
        {
            if (!dynamic_cast<ModType*>(mod.get()))
            {
                continue;
            }

            bool mod_name_is_taken = std::find_if(mods.begin(), mods.end(), [&](auto& elem) {
                                         return elem->get_name() == mod->get_name();
                                     }) == mods.end();

            if (mod_name_is_taken)
            {
                mod->set_installable(false);
                Output::send(SYSSTR("Mod name '{}' is already in use.\n"), mod->get_name());
                continue;
            }

            if (mod->is_installed())
            {
                Output::send(SYSSTR("Tried to install a mod that was already installed, Mod: '{}'\n"), mod->get_name());
                continue;
            }

            if (!mod->is_installable())
            {
                Output::send(SYSSTR("Was unable to install mod '{}' for unknown reasons. Mod is not installable.\n"), mod->get_name());
                continue;
            }

            mod->set_installed(true);
        }
    }

    auto UE4SSProgram::install_cpp_mods() -> void
    {
        install_mods<CppMod>(get_program().m_mods);
    }

    auto UE4SSProgram::install_lua_mods() -> void
    {
        install_mods<LuaMod>(get_program().m_mods);
    }

    auto UE4SSProgram::fire_unreal_init_for_cpp_mods() -> void
    {
        ProfilerScope();
        for (const auto& mod : m_mods)
        {
            if (!dynamic_cast<CppMod*>(mod.get()))
            {
                continue;
            }
            mod->fire_unreal_init();
        }
    }

    auto UE4SSProgram::fire_program_start_for_cpp_mods() -> void
    {
        ProfilerScope();
        for (const auto& mod : m_mods)
        {
            if (!dynamic_cast<CppMod*>(mod.get()))
            {
                continue;
            }
            mod->fire_program_start();
        }
    }

    auto UE4SSProgram::fire_dll_load_for_cpp_mods(SystemStringViewType dll_name) -> void
    {
        for (const auto& mod : m_mods)
        {
            if (auto cpp_mod = dynamic_cast<CppMod*>(mod.get()); cpp_mod)
            {
                cpp_mod->fire_dll_load(dll_name);
            }
        }
    }

    template <typename ModType>
    auto start_mods() -> std::string
    {
        ProfilerScope();
        // Part #1: Start all mods that are enabled in mods.txt.
        Output::send(SYSSTR("Starting mods (from mods.txt load order)...\n"));

        std::filesystem::path mods_directory = UE4SSProgram::get_program().get_mods_directory();
        auto enabled_mods_file = File::get_path_if_exists(mods_directory, "mods.txt");
        if (!enabled_mods_file.has_value())
        {
            Output::send(SYSSTR("No mods.txt file found...\n"));
        }
        else
        {
            // 'mods.txt' exists, lets parse it
            SystemStreamType mods_stream{*enabled_mods_file};

            SystemStringType current_line;
            while (std::getline(mods_stream, current_line))
            {
                // Don't parse any lines with ';'
                if (current_line.find(SYSSTR(";")) != current_line.npos)
                {
                    continue;
                }

                // Don't parse if the line is impossibly short (empty lines for example)
                if (current_line.size() <= 4)
                {
                    continue;
                }

                // Remove all spaces
                auto end = std::remove(current_line.begin(), current_line.end(), SYSSTR(' '));
                current_line.erase(end, current_line.end());

                // Parse the line into something that can be converted into proper data
                SystemStringType mod_name = explode_by_occurrence(current_line, SYSSTR(':'), 1);
                SystemStringType mod_enabled = explode_by_occurrence(current_line, SYSSTR(':'), ExplodeType::FromEnd);

                auto mod = UE4SSProgram::find_mod_by_name<ModType>(mod_name, UE4SSProgram::IsInstalled::Yes);
                if (!mod || !dynamic_cast<ModType*>(mod))
                {
                    continue;
                }

                if (!mod_enabled.empty() && mod_enabled[0] == SYSSTR('1'))
                {
                    Output::send(SYSSTR("Starting {} mod '{}'\n"), std::is_same_v<ModType, LuaMod> ? SYSSTR("Lua") : SYSSTR("C++"), mod->get_name().data());
                    mod->start_mod();
                }
                else
                {
                    Output::send(SYSSTR("Mod '{}' disabled in mods.txt.\n"), mod_name);
                }
            }
        }

        // Part #2: Start all mods that have enabled.txt present in the mod directory.
        Output::send(SYSSTR("Starting mods (from enabled.txt, no defined load order)...\n"));

        for (const auto& mod_directory : std::filesystem::directory_iterator(mods_directory))
        {
            std::error_code ec{};

            if (!mod_directory.is_directory(ec))
            {
                continue;
            }
            if (ec.value() != 0)
            {
                return std::format("is_directory ran into error {}", ec.value());
            }

            auto enabled = File::get_path_if_exists(mod_directory.path(), "enabled.txt");

            if (!enabled.has_value())
            {
                continue;
            }

            auto mod = UE4SSProgram::find_mod_by_name<ModType>(to_system_string(mod_directory.path().stem()), UE4SSProgram::IsInstalled::Yes);
            if (!dynamic_cast<ModType*>(mod))
            {
                continue;
            }
            if (!mod)
            {
                Output::send<LogLevel::Warning>(SYSSTR("Found a mod with enabled.txt but mod has not been installed properly.\n"));
                continue;
            }

            if (mod->is_started())
            {
                continue;
            }

            Output::send(SYSSTR("Mod '{}' has enabled.txt, starting mod.\n"), mod->get_name().data());
            mod->start_mod();
        }

        return {};
    }

    auto UE4SSProgram::start_lua_mods() -> void
    {
        ProfilerScope();
        auto error_message = start_mods<LuaMod>();
        if (!error_message.empty())
        {
            set_error(error_message.c_str());
        }
    }

    auto UE4SSProgram::start_cpp_mods() -> void
    {
        ProfilerScope();
        auto error_message = start_mods<CppMod>();
        if (!error_message.empty())
        {
            set_error(error_message.c_str());
        }
    }

    auto UE4SSProgram::uninstall_mods() -> void
    {
        ProfilerScope();

        std::vector<CppMod*> cpp_mods{};
        std::vector<LuaMod*> lua_mods{};
        for (auto& mod : m_mods)
        {
            if (auto cpp_mod = dynamic_cast<CppMod*>(mod.get()); cpp_mod)
            {
                cpp_mods.emplace_back(cpp_mod);
            }
            else if (auto lua_mod = dynamic_cast<LuaMod*>(mod.get()); lua_mod)
            {
                lua_mods.emplace_back(lua_mod);
            }
        }

        for (auto& mod : lua_mods)
        {
            // Remove any actions, or we'll get an internal error as the lua ref won't be valid
            mod->uninstall();
        }

        for (auto& mod : cpp_mods)
        {
            mod->uninstall();
        }
        m_mods.clear();
        LuaMod::global_uninstall();
    }

    auto UE4SSProgram::is_program_started() -> bool
    {
        return m_is_program_started;
    }

    auto UE4SSProgram::reinstall_mods() -> void
    {
        ProfilerScope();
        Output::send(SYSSTR("Re-installing all mods\n"));

        // Stop processing events while stuff isn't properly setup
        m_pause_events_processing = true;

        uninstall_mods();

// Remove key binds that were set from Lua scripts
#ifdef HAS_INPUT
        m_input_handler.get_events_safe([&](Input::KeySet& input_event) -> void {
            for (auto& [key, vector_of_key_data] : input_event.key_data)
            {
                std::erase_if(vector_of_key_data, [&](Input::KeyData& key_data) -> bool {
                    return key_data.custom_data == 1;
                });

                if (vector_of_key_data.empty())
                {
                    m_input_handler.clear_subscribed_key(key);
                }
            }
        });
#endif

        // Remove all custom properties
        // Uncomment when custom properties are working
        LuaType::LuaCustomProperty::StaticStorage::property_list.clear();

        // Reset the Lua callbacks for the global Lua function 'NotifyOnNewObject'
        LuaMod::m_static_construct_object_lua_callbacks.clear();

        // Start processing events again as everything is now properly setup
        // Do this before mods are started or else you won't be able to use the hot-reload key bind if there's an error from Lua
        m_pause_events_processing = false;

        setup_mods();
        start_cpp_mods();
        start_lua_mods();

        if (Unreal::UnrealInitializer::StaticStorage::bIsInitialized)
        {
            fire_unreal_init_for_cpp_mods();
        }

        if (is_program_started())
        {
            fire_program_start_for_cpp_mods();
        }

        Output::send(SYSSTR("All mods re-installed\n"));
    }

    auto UE4SSProgram::get_module_directory() -> SystemStringViewType
    {
        m_module_file_path_str = to_system_string(m_module_file_path);
        return m_module_file_path_str;
    }

    auto UE4SSProgram::get_working_directory() -> SystemStringViewType
    {
        m_working_directory_str = to_system_string(m_working_directory);
        return m_working_directory_str;
    }

    auto UE4SSProgram::get_mods_directory() -> SystemStringViewType
    {
        m_mods_directory_str = to_system_string(m_mods_directory);
        return m_mods_directory_str;
    }

    auto UE4SSProgram::get_game_directory() -> SystemStringViewType
    {
        m_game_executable_str = to_system_string(m_game_executable_directory);
        return m_game_executable_str;
    }

    auto UE4SSProgram::generate_uht_compatible_headers() -> void
    {
        ProfilerScope();
        Output::send(SYSSTR("Generating UHT compatible headers...\n"));

        double generator_duration{};
        {
            ScopedTimer generator_timer{&generator_duration};

            const std::filesystem::path DumpRootDirectory = m_working_directory / "UHTHeaderDump";
            UEGenerator::UEHeaderGenerator HeaderGenerator = UEGenerator::UEHeaderGenerator(DumpRootDirectory);
            HeaderGenerator.dump_native_packages();
        }

        Output::send(SYSSTR("Generating UHT compatible headers took {} seconds\n"), generator_duration);
    }

    auto UE4SSProgram::generate_cxx_headers(const std::filesystem::path& output_dir) -> void
    {
        ProfilerScope();
        if (settings_manager.CXXHeaderGenerator.LoadAllAssetsBeforeGeneratingCXXHeaders)
        {
            Output::send(SYSSTR("Loading all assets...\n"));
            double asset_loading_duration{};
            {
                ProfilerScopeNamed("loading all assets");
                ScopedTimer loading_timer{&asset_loading_duration};

                UAssetRegistry::LoadAllAssets();
            }
            Output::send(SYSSTR("Loading all assets took {} seconds\n"), asset_loading_duration);
        }

        double generator_duration;
        {
            ProfilerScopeNamed("unloading all force-loaded assets");
            ScopedTimer generator_timer{&generator_duration};

            UEGenerator::generate_cxx_headers(output_dir);

            Output::send(SYSSTR("Unloading all forcefully loaded assets\n"));
        }

        UAssetRegistry::FreeAllForcefullyLoadedAssets();
        Output::send(SYSSTR("SDK generated in {} seconds.\n"), generator_duration);
    }

    auto UE4SSProgram::generate_lua_types(const std::filesystem::path& output_dir) -> void
    {
        ProfilerScope();
        if (settings_manager.CXXHeaderGenerator.LoadAllAssetsBeforeGeneratingCXXHeaders)
        {
            Output::send(SYSSTR("Loading all assets...\n"));
            double asset_loading_duration{};
            {
                ProfilerScopeNamed("loading all assets");
                ScopedTimer loading_timer{&asset_loading_duration};

                UAssetRegistry::LoadAllAssets();
            }
            Output::send(SYSSTR("Loading all assets took {} seconds\n"), asset_loading_duration);
        }

        double generator_duration;
        {
            ProfilerScopeNamed("unloading all force-loaded assets");
            ScopedTimer generator_timer{&generator_duration};

            UEGenerator::generate_lua_types(output_dir);

            Output::send(SYSSTR("Unloading all forcefully loaded assets\n"));
        }

        UAssetRegistry::FreeAllForcefullyLoadedAssets();
        Output::send(SYSSTR("SDK generated in {} seconds.\n"), generator_duration);
    }
#ifdef HAS_UI
    auto UE4SSProgram::stop_render_thread() -> void
    {
        if (m_render_thread.joinable())
        {
            m_render_thread.request_stop();
            m_render_thread.join();
        }
    }

    auto UE4SSProgram::add_gui_tab(std::shared_ptr<GUI::GUITab> tab) -> void
    {
        m_debugging_gui.add_tab(tab);
    }

    auto UE4SSProgram::remove_gui_tab(std::shared_ptr<GUI::GUITab> tab) -> void
    {
        m_debugging_gui.remove_tab(tab);
    }
#endif

    auto UE4SSProgram::queue_event(EventCallable callable, void* data) -> void
    {
        if (!can_process_events())
        {
            return;
        }
        std::lock_guard<std::mutex> guard(m_event_queue_mutex);
        m_queued_events.emplace_back(Event{callable, data});
    }

    auto UE4SSProgram::is_queue_empty() -> bool
    {
        // Not locking here because if the worst that could happen as far as I know is that the event loop processes the event slightly late.
        return m_queued_events.empty();
    }
#ifdef HAS_INPUT
    auto UE4SSProgram::register_keydown_event(Input::Key key, const Input::EventCallbackCallable& callback, uint8_t custom_data) -> void
    {
        m_input_handler.register_keydown_event(key, callback, custom_data);
    }

    auto UE4SSProgram::register_keydown_event(Input::Key key,
                                              const Input::Handler::ModifierKeyArray& modifier_keys,
                                              const Input::EventCallbackCallable& callback,
                                              uint8_t custom_data) -> void
    {
        m_input_handler.register_keydown_event(key, modifier_keys, callback, custom_data);
    }

    auto UE4SSProgram::is_keydown_event_registered(Input::Key key) -> bool
    {
        return m_input_handler.is_keydown_event_registered(key);
    }

    auto UE4SSProgram::is_keydown_event_registered(Input::Key key, const Input::Handler::ModifierKeyArray& modifier_keys) -> bool
    {
        return m_input_handler.is_keydown_event_registered(key, modifier_keys);
    }
#endif
    auto UE4SSProgram::find_mod_by_name_internal(SystemStringViewType mod_name, IsInstalled is_installed, IsStarted is_started, FMBNI_ExtraPredicate extra_predicate)
            -> Mod*
    {
        auto mod_exists_with_name = std::find_if(get_program().m_mods.begin(), get_program().m_mods.end(), [&](auto& elem) -> bool {
            bool found = true;

            if (!extra_predicate(elem.get()))
            {
                found = false;
            }
            if (mod_name != elem->get_name())
            {
                found = false;
            }
            if (is_installed == IsInstalled::Yes && !elem->is_installable())
            {
                found = false;
            }
            if (is_started == IsStarted::Yes && !elem->is_started())
            {
                found = false;
            }

            return found;
        });

        // clang-format off
        if (mod_exists_with_name == get_program().m_mods.end())
        {
            return nullptr;
        }
        // clang-format on
        else
        {
            return mod_exists_with_name->get();
        }
    }

    auto UE4SSProgram::get_object_dumper_output_directory() -> const SystemStringType
    {
        return to_system_string(m_object_dumper_output_directory);
    }

    auto UE4SSProgram::dump_uobject(UObject* object, std::unordered_set<FField*>* in_dumped_fields, SystemStringType& out_line, bool is_below_425) -> void
    {
        bool owns_dumped_fields{};
        auto dumped_fields_ptr = [&] {
            if (in_dumped_fields)
            {
                return in_dumped_fields;
            }
            else
            {
                owns_dumped_fields = true;
                return new std::unordered_set<FField*>{};
            }
        }();
        auto& dumped_fields = *dumped_fields_ptr;

        UObject* typed_obj = static_cast<UObject*>(object);

        if (is_below_425 && Unreal::TypeChecker::is_property(typed_obj) &&
            !typed_obj->HasAnyFlags(static_cast<EObjectFlags>(EObjectFlags::RF_DefaultSubObject | EObjectFlags::RF_ArchetypeObject)))
        {
            // We've verified that we're in <4.25 so this cast is safe but should be abstracted at some point
            dump_xproperty(std::bit_cast<FProperty*>(typed_obj), out_line);
        }
        else
        {
            auto typed_class = typed_obj->GetClassPrivate()->HashObject();
            if (ObjectDumper::to_string_exists(typed_class))
            {
                // Call type-specific implementation to dump UObject
                // The type is determined at runtime

                // Dump UObject
                ObjectDumper::get_to_string(typed_class)(object, out_line);
                out_line.append(SYSSTR("\n"));

                if (!is_below_425 && ObjectDumper::to_string_complex_exists(typed_class))
                {
                    // Dump all properties that are directly owned by this UObject (not its UClass)
                    // UE 4.25+ (properties are part of GUObjectArray in earlier versions)
                    ObjectDumper::get_to_string_complex(typed_class)(object, out_line, [&](void* prop) {
                        if (dumped_fields.contains(static_cast<FField*>(prop)))
                        {
                            return;
                        }

                        dump_xproperty(static_cast<FProperty*>(prop), out_line);
                        dumped_fields.emplace(static_cast<FField*>(prop));
                    });
                }
            }
            else
            {
                // A type-specific implementation does not exist so lets call the default implementation for UObjects instead
                ObjectDumper::object_to_string(object, out_line);
                out_line.append(SYSSTR("\n"));
            }

            // If the UClass of the UObject has any properties then dump them
            // UE 4.25+ (properties are part of GUObjectArray in earlier versions)
            if (!is_below_425)
            {
                if (typed_obj->IsA<UStruct>())
                {
                    for (FProperty* prop : static_cast<UClass*>(typed_obj)->ForEachProperty())
                    {
                        if (dumped_fields.contains(prop))
                        {
                            continue;
                        }

                        dump_xproperty(prop, out_line);
                        dumped_fields.emplace(prop);
                    }
                }
            }
        }

        if (owns_dumped_fields)
        {
            delete dumped_fields_ptr;
        }
    }

    auto UE4SSProgram::dump_xproperty(FProperty* property, SystemStringType& out_line) -> void
    {
        auto typed_prop_class = property->GetClass().HashObject();

        if (ObjectDumper::to_string_exists(typed_prop_class))
        {
            ObjectDumper::get_to_string(typed_prop_class)(property, out_line);
            out_line.append(SYSSTR("\n"));

            if (ObjectDumper::to_string_complex_exists(typed_prop_class))
            {
                ObjectDumper::get_to_string_complex(typed_prop_class)(property, out_line, [&]([[maybe_unused]] void* prop) {
                    out_line.append(SYSSTR("\n"));
                });
            }
        }
        else
        {
            ObjectDumper::property_to_string(property, out_line);
            out_line.append(SYSSTR("\n"));
        }
    }

    auto UE4SSProgram::dump_all_objects_and_properties(const SystemStringType& output_path_and_file_name) -> void
    {
        /*
        Output::send(SYSSTR("Test msg with no fmt args, and no optional arg\n"));
        Output::send(SYSSTR("Test msg with no fmt args, and one optional arg [Normal]\n"), LogLevel::Normal);
        Output::send(SYSSTR("Test msg with no fmt args, and one optional arg [Verbose]\n"), LogLevel::Verbose);
        Output::send(SYSSTR("Test msg with one fmt arg [{}], and one optional arg [Warning]\n"), LogLevel::Warning, 33);
        Output::send(SYSSTR("Test msg with two fmt args [{}, {}], and one optional arg [Error]\n"), LogLevel::Error, 33, 44);
        //*/

        // Object & Property Dumper -> START
        if (settings_manager.ObjectDumper.LoadAllAssetsBeforeDumpingObjects)
        {
            Output::send(SYSSTR("Loading all assets...\n"));
            double asset_loading_duration{};
            {
                ScopedTimer loading_timer{&asset_loading_duration};

                UAssetRegistry::LoadAllAssets();
            }
            Output::send(SYSSTR("Loading all assets took {} seconds\n"), asset_loading_duration);
        }

        double dumper_duration{};
        {
            ScopedTimer dumper_timer{&dumper_duration};

            std::unordered_set<FField*> dumped_fields;
            // There will be tons of dumped fields so lets just reserve tons in order to speed things up a bit
            dumped_fields.reserve(100000);

            bool is_below_425 = Unreal::Version::IsBelow(4, 25);

            // The final outputted string shouldn't need be reformatted just to put a new line at the end
            // Instead the object/property implementations should add a new line in the last format that they do
            //
            // Optimizations done:
            // 1. The entire code-base has been changed to use 'wchar_t' instead of 'char'.
            // The effect of this is that there is no need to ever convert between types.
            // There's also no thinking about which type should be used since 'wchar_t' is now the standard for UE4SS.
            // The downside with wchar_t is that all files that get output to will be doubled in size.

            using ObjectDumperOutputDevice = Output::NewFileDevice;
            Output::Targets<ObjectDumperOutputDevice> scoped_dumper_out;
            auto& file_device = scoped_dumper_out.get_device<ObjectDumperOutputDevice>();
            file_device.set_file_name_and_path(output_path_and_file_name);
            file_device.set_formatter([](SystemStringViewType string) -> SystemStringType {
                return SystemStringType{string};
            });

            // Make string & reserve massive amounts of space to hopefully not reach the end of the string and require more
            // dynamic allocations
            SystemStringType out_line;
            out_line.reserve(200000000);

            Output::send(SYSSTR("Dumping all objects & properties in GUObjectArray\n"));
            UObjectGlobals::ForEachUObject([&](void* object, [[maybe_unused]] int32_t chunk_index, [[maybe_unused]] int32_t object_index) {
                dump_uobject(static_cast<UObject*>(object), &dumped_fields, out_line, is_below_425);
                return LoopAction::Continue;
            });

            // Save to file
            scoped_dumper_out.send(out_line);

            // Reset the dumped_fields set, otherwise no fields will be dumped in subsequent dumps
            dumped_fields.clear();
            Output::send(SYSSTR("Done iterating GUObjectArray\n"));
        }

        UAssetRegistry::FreeAllForcefullyLoadedAssets();
        Output::send(SYSSTR("Dumping GUObjectArray took {} seconds\n"), dumper_duration);
        // Object & Property Dumper -> END
    }

    auto UE4SSProgram::static_cleanup() -> void
    {
        delete &get_program();

        // Do cleanup of static objects here
        // This function is called right before the DLL detaches from the game
        // Including when the player hits the 'X' button to exit the game
    }
} // namespace RC
