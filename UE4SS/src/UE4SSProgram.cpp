#define NOMINMAX

#include <Windows.h>

#ifdef TEXT
#undef TEXT
#endif

#include <algorithm>
#include <cwctype>
#include <format>
#include <fstream>
#include <limits>
#include <unordered_set>
#include <fmt/chrono.h>
#include <Profiler/Profiler.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <ExceptionHandling.hpp>
#include <GUI/ConsoleOutputDevice.hpp>
#include <GUI/GUI.hpp>
#include <GUI/LiveView.hpp>
#include <Helpers/ASM.hpp>
#include <Helpers/Format.hpp>
#include <Helpers/Integer.hpp>
#include <Helpers/String.hpp>
#include <Helpers/Time.hpp>
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
#include <Unreal/Core/Templates/Tuple.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/TypeChecker.hpp>
#include <Unreal/UActorComponent.hpp>
#include <Unreal/UInterface.hpp>
#include <Unreal/UKismetSystemLibrary.hpp>
#include <Unreal/ULocalPlayer.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UPackage.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/UnrealInitializer.hpp>
#include <Unreal/World.hpp>
#include <Unreal/FWorldContext.hpp>
#include <Unreal/Engine/UDataTable.hpp>
#include <UnrealDef.hpp>

#include <polyhook2/PE/IatHook.hpp>

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
        Output::send(STR(#StructName "::{} = 0x{:X}\n"), name, offset);                                                                                        \
    }

    auto output_all_member_offsets() -> void
    {
        Output::send(STR("\n##### MEMBER OFFSETS START #####\n\n"));
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UObjectBase);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UScriptStruct::ICppStructOps);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UStruct);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UScriptStruct);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UClass);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UEnum);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UFunction);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(USparseDelegateFunction);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UField);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FField);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FNumericProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FObjectPropertyBase);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FStructProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FArrayProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FMapProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FSetProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FBoolProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FByteProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FEnumProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FClassProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FSoftClassProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FDelegateProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FMulticastDelegateProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FInterfaceProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FFieldPathProperty);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FWorldContext);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FOutputDevice);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FArchiveState);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(FArchive);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(AActor);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(AGameModeBase);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(AGameMode);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UEngine);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UGameViewportClient);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UPlayer);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(ULocalPlayer);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UWorld);
        OUTPUT_MEMBER_OFFSETS_FOR_STRUCT(UDataTable);
        Output::send(STR("\n##### MEMBER OFFSETS END #####\n\n"));
    }

    void* HookedLoadLibraryA(const char* dll_name)
    {
        UE4SSProgram& program = UE4SSProgram::get_program();
        HMODULE lib = PLH::FnCast(program.m_hook_trampoline_load_library_a, &LoadLibraryA)(dll_name);
        program.fire_dll_load_for_cpp_mods(ensure_str(dll_name));
        return lib;
    }

    void* HookedLoadLibraryExA(const char* dll_name, void* file, int32_t flags)
    {
        UE4SSProgram& program = UE4SSProgram::get_program();
        HMODULE lib = PLH::FnCast(program.m_hook_trampoline_load_library_ex_a, &LoadLibraryExA)(dll_name, file, flags);
        program.fire_dll_load_for_cpp_mods(ensure_str(dll_name));
        return lib;
    }

    void* HookedLoadLibraryW(const wchar_t* dll_name)
    {
        UE4SSProgram& program = UE4SSProgram::get_program();
        HMODULE lib = PLH::FnCast(program.m_hook_trampoline_load_library_w, &LoadLibraryW)(dll_name);
        program.fire_dll_load_for_cpp_mods(ToCharTypePtr(dll_name));
        return lib;
    }

    void* HookedLoadLibraryExW(const wchar_t* dll_name, void* file, int32_t flags)
    {
        UE4SSProgram& program = UE4SSProgram::get_program();
        HMODULE lib = PLH::FnCast(program.m_hook_trampoline_load_library_ex_w, &LoadLibraryExW)(dll_name, file, flags);
        program.fire_dll_load_for_cpp_mods(ToCharTypePtr(dll_name));
        return lib;
    }

    UE4SSProgram::UE4SSProgram(const std::filesystem::path& moduleFilePath, std::initializer_list<BinaryOptions> options) : MProgram(options)
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
                create_emergency_console_for_early_error(fmt::format(STR("The IniParser failed to parse: {}"), ensure_str(e.what())));
                return;
            }

            if (settings_manager.CrashDump.EnableDumping)
            {
                m_crash_dumper.enable();
            }

            m_crash_dumper.set_full_memory_dump(settings_manager.CrashDump.FullMemoryDump);

            m_debugging_gui.set_gfx_backend(settings_manager.Debug.GraphicsAPI);

            // Setup the log file
            auto& file_device = Output::set_default_devices<Output::NewFileDevice>();
            file_device.set_file_name_and_path(ensure_str((m_log_directory / m_log_file_name)));

            if (const auto ue4ss_mods_paths_var_raw = std::getenv("UE4SS_MODS_PATHS"); ue4ss_mods_paths_var_raw)
            {
                const auto ue4ss_mods_paths_var = ensure_str(ue4ss_mods_paths_var_raw);
                Output::send(STR("Environment variable 'UE4SS_MODS_PATHS' present, adding 'Mods' path overrides: {}\n"), ue4ss_mods_paths_var);
                const auto paths = parse_semicolon_separated_string(ue4ss_mods_paths_var);
                for (const auto& path : std::ranges::reverse_view(paths))
                {
                    add_mods_directory(std::filesystem::weakly_canonical(path));
                }
            }

            create_simple_console();

            if (settings_manager.Debug.DebugConsoleEnabled)
            {
                m_console_device = &Output::set_default_devices<Output::ConsoleDevice>();
                m_console_device->set_formatter([](File::StringViewType string) -> File::StringType {
                    return fmt::format(STR("[{}] {}"), get_now_as_string(STR("{:%X}")), string);
                });
                if (settings_manager.Debug.DebugConsoleVisible)
                {
                    switch (settings_manager.Debug.RenderMode)
                    {
                    case GUI::RenderMode::ExternalThread:
                        m_render_thread = std::jthread{&GUI::gui_thread, &m_debugging_gui};
                        break;
                    case GUI::RenderMode::EngineTick:
                    case GUI::RenderMode::GameViewportClientTick:
                        // The hooked game function will pick up on the window being "open", and start rendering.
                        get_debugging_ui().set_open(true);
                        break;
                    }
                }
            }

            // This is experimental code that's here only for future reference
            /*
            Unreal::UnrealInitializer::SetupUnrealModules();

            constexpr const wchar_t* str_to_find = STR("Allocator: %s");
            void* string_address = SinglePassScanner::string_scan(str_to_find, ScanTarget::Core);
            Output::send(STR("\n\nFound string '{}' at {}\n\n"), std::wstring_view{str_to_find}, string_address);
            //*/

            Output::send(STR("Console created\n"));
            Output::send(STR("UE4SS - v{}.{}.{}{}{} - Git SHA #{}\n"),
                         UE4SS_LIB_VERSION_MAJOR,
                         UE4SS_LIB_VERSION_MINOR,
                         UE4SS_LIB_VERSION_HOTFIX,
                         fmt::format(STR("{}"), UE4SS_LIB_VERSION_PRERELEASE == 0 ? STR("") : fmt::format(STR(" PreRelease #{}"), UE4SS_LIB_VERSION_PRERELEASE)),
                         fmt::format(STR("{}"),
                                     UE4SS_LIB_BETA_STARTED == 0
                                             ? STR("")
                                             : (UE4SS_LIB_IS_BETA == 0 ? STR(" Beta #?") : fmt::format(STR(" Beta #{}"), UE4SS_LIB_VERSION_BETA))),
                         ensure_str(UE4SS_LIB_BUILD_GITSHA));
            bool use_local_time = true;
#ifdef _WIN32
            if (auto module = GetModuleHandleW(L"ntdll.dll"); module && GetProcAddress(module, "wine_get_version"))
            {
                use_local_time = false;
            }
#endif
            if (use_local_time)
            {
                try
                {
                    Output::send(STR("Timezone: {}\n"), ensure_str(std::chrono::current_zone()->name()));
                }
                catch (std::runtime_error&)
                {
                    Output::send(STR("Timezone: UTC (local disabled due to lack of support (chrono::current_zone() failed))\n"));
                }
            }
            else
            {
                Output::send(STR("Timezone: UTC (local disabled due to wine)\n"));
            }

#ifdef __clang__
#define UE4SS_COMPILER STR("Clang")
#else
#define UE4SS_COMPILER STR("MSVC")
#endif

            Output::send(STR("UE4SS Build Configuration: {} ({})\n"), ensure_str(UE4SS_CONFIGURATION), UE4SS_COMPILER);

            m_load_library_a_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                   "LoadLibraryA",
                                                                   std::bit_cast<uint64_t>(&HookedLoadLibraryA),
                                                                   &m_hook_trampoline_load_library_a,
                                                                   L"");
            m_load_library_a_hook->hook();

            m_load_library_ex_a_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                      "LoadLibraryExA",
                                                                      std::bit_cast<uint64_t>(&HookedLoadLibraryExA),
                                                                      &m_hook_trampoline_load_library_ex_a,
                                                                      L"");
            m_load_library_ex_a_hook->hook();

            m_load_library_w_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                   "LoadLibraryW",
                                                                   std::bit_cast<uint64_t>(&HookedLoadLibraryW),
                                                                   &m_hook_trampoline_load_library_w,
                                                                   L"");
            m_load_library_w_hook->hook();

            m_load_library_ex_w_hook = std::make_unique<PLH::IatHook>("kernel32.dll",
                                                                      "LoadLibraryExW",
                                                                      std::bit_cast<uint64_t>(&HookedLoadLibraryExW),
                                                                      &m_hook_trampoline_load_library_ex_w,
                                                                      L"");
            m_load_library_ex_w_hook->hook();

            Unreal::UnrealInitializer::SetupUnrealModules();

            setup_mod_directory_path();

            setup_mods();
            install_cpp_mods();
            start_cpp_mods(IsInitialStartup::Yes);

            if (m_has_game_specific_config)
            {
                Output::send(STR("Found configuration for game: {}\n"), ensure_str(m_working_directory.filename()));
            }
            else
            {
                Output::send(STR("No specific game configuration found, using default configuration file\n"));
            }

            Output::send(STR("Config: {}\n\n"), ensure_str(m_settings_path_and_file));
            Output::send(STR("root directory: {}\n"), ensure_str(m_root_directory));
            Output::send(STR("working directory: {}\n"), ensure_str(m_working_directory));
            Output::send(STR("game executable directory: {}\n"), ensure_str(m_game_executable_directory));
            Output::send(STR("game executable: {} ({} bytes)\n\n\n"), ensure_str(m_game_path_and_exe_name), std::filesystem::file_size(m_game_path_and_exe_name));
            Output::send(STR("mods directories: \n"));
            for (const auto& [index, mod_directory] : std::ranges::enumerate_view(m_mods_directories))
            {
                Output::send(STR("[{}] {}\n"), index, ensure_str(mod_directory));
            }
            Output::send(STR("\n"));
            Output::send(STR("log directory: {}\n"), ensure_str(m_log_directory));
            Output::send(STR("object dumper directory: {}\n\n\n"), ensure_str(m_object_dumper_output_directory));
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

            Output::send(STR("Unreal Engine modules ({}):\n"), SigScannerStaticData::m_is_modular ? STR("modular") : STR("non-modular"));
            auto& main_exe_ptr = SigScannerStaticData::m_modules_info.array[static_cast<size_t>(ScanTarget::MainExe)].lpBaseOfDll;
            for (size_t i = 0; i < static_cast<size_t>(ScanTarget::Max); ++i)
            {
                auto& module = SigScannerStaticData::m_modules_info.array[i];
                // only log modules with unique addresses (non-modular builds have everything in MainExe)
                if (i == static_cast<size_t>(ScanTarget::MainExe) || main_exe_ptr != module.lpBaseOfDll)
                {
                    auto module_name = ensure_str(ScanTargetToString(i));
                    Output::send(STR("{} @ {} size={:#x}\n"), module_name.c_str(), module.lpBaseOfDll, module.SizeOfImage);
                }
            }

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

    auto UE4SSProgram::setup_paths(const std::filesystem::path& moduleFilePath) -> void
    {
        ProfilerScope();
        m_root_directory = moduleFilePath.parent_path();
        m_module_file_path = moduleFilePath;

        // The default working directory is the root directory
        // Can be changed by creating a <GameName> directory in the root directory
        // At that point, the working directory will be "root/<GameName>"
        m_working_directory = m_root_directory;

        wchar_t exe_path_buffer[1024];
        GetModuleFileNameW(GetModuleHandle(nullptr), exe_path_buffer, 1023);
        std::filesystem::path game_exe_path = exe_path_buffer;
        std::filesystem::path game_directory_path = game_exe_path.parent_path();
        m_legacy_root_directory = game_directory_path;

        m_working_directory = m_root_directory;
        m_game_executable_directory = game_directory_path;
        m_settings_path_and_file = m_root_directory;
        m_game_path_and_exe_name = game_exe_path;
        m_object_dumper_output_directory = m_working_directory;

        // Allow loading of DLLs from the game directory
        AddDllDirectory(game_exe_path.c_str());

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
                m_settings_path_and_file = std::move(item.path());
                m_log_directory = m_working_directory;
                m_object_dumper_output_directory = m_working_directory;
                m_legacy_root_directory = m_legacy_root_directory / item.path();
                break;
            }
        }

        m_log_directory = m_working_directory;
        m_settings_path_and_file.append(m_settings_file_name);

        // Check for legacy locations and update paths accordingly
        if (std::filesystem::exists(m_legacy_root_directory / m_settings_file_name) && !std::filesystem::exists(m_settings_path_and_file))
        {
            m_settings_path_and_file = m_legacy_root_directory / m_settings_file_name;
        }
    }

    auto UE4SSProgram::create_emergency_console_for_early_error(File::StringViewType error_message) -> void
    {
        settings_manager.Debug.SimpleConsoleEnabled = true;
        create_simple_console();
        printf_s("%S\n", FromCharTypePtr<wchar_t>(error_message.data()));
    }

    auto UE4SSProgram::setup_mod_directory_path() -> void
    {
        std::filesystem::path default_mods_path{};
        if (!settings_manager.Overrides.ModsFolderPath.empty())
        {
            default_mods_path = settings_manager.Overrides.ModsFolderPath;
        }
        else
        {
            default_mods_path = m_working_directory / "Mods";
        }

        // If no paths were added, check legacy location for fallback
        if (std::filesystem::exists(m_legacy_root_directory / "Mods") && !std::filesystem::exists(default_mods_path))
        {
            default_mods_path = m_legacy_root_directory / "Mods";
        }

        insert_mods_directory(default_mods_path, 0);

        for (const auto& path : m_mods_directories_to_remove)
        {
            std::erase(m_mods_directories, path);
        }
    }

    auto UE4SSProgram::create_simple_console() -> void
    {
        if (settings_manager.Debug.SimpleConsoleEnabled)
        {
            m_debug_console_device = &Output::set_default_devices<Output::DebugConsoleDevice>();
            Output::set_default_log_level<LogLevel::Normal>();
            m_debug_console_device->set_formatter([](File::StringViewType string) -> File::StringType {
                return fmt::format(STR("[{}] {}"), get_now_as_string(STR("{:%X}")), string);
            });

            if (AllocConsole())
            {
                FILE* stdin_filename;
                FILE* stdout_filename;
                FILE* stderr_filename;
                freopen_s(&stdin_filename, "CONIN$", "r", stdin);
                freopen_s(&stdout_filename, "CONOUT$", "w", stdout);
                freopen_s(&stderr_filename, "CONOUT$", "w", stderr);
            }
        }
    }

    auto UE4SSProgram::load_unreal_offsets_from_file() -> void
    {
        std::filesystem::path file_path = m_working_directory / "MemberVariableLayout.ini";
        if (std::filesystem::exists(file_path))
        {
            auto file = File::open(file_path);
            if (auto file_contents = file.read_all(); !file_contents.empty())
            {
                Ini::Parser parser;
                parser.parse(file_contents);
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
        const StringType offset_overrides_section{STR("OffsetOverrides")};

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
            static File::StringType virtual_function_offset_override_file{ensure_str((m_working_directory / STR("VTableLayout.ini")))};
            if (std::filesystem::exists(virtual_function_offset_override_file))
            {
                auto file =
                        File::open(virtual_function_offset_override_file, File::OpenFor::Reading, File::OverwriteExistingFile::No, File::CreateIfNonExistent::No);
                Ini::Parser parser;
                parser.parse(file);

                Output::send<Color::Blue>(STR("Getting ordered lists from ini file\n"));

                auto calculate_virtual_function_offset = []<typename... BaseSizes>(uint32_t current_index, BaseSizes... base_sizes) -> uint32_t {
                    return current_index == 0 ? 0 : (current_index + (base_sizes + ...)) * 8;
                };

                auto retrieve_vtable_layout_from_ini = [&](const File::StringType& section_name, auto callable) -> uint32_t {
                    auto list = parser.get_ordered_list(section_name);
                    uint32_t vtable_size = list.size() - 1;
                    list.for_each([&](uint32_t index, File::StringType& item) {
                        callable(index, item);
                    });
                    return vtable_size;
                };

                Output::send<Color::Blue>(STR("UObjectBase\n"));
                uint32_t uobjectbase_size = retrieve_vtable_layout_from_ini(STR("UObjectBase"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, 0);
                    Output::send(STR("UObjectBase::{} = 0x{:X}\n"), item, offset);
                    Unreal::UObjectBase::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("UObjectBaseUtility\n"));
                uint32_t uobjectbaseutility_size = retrieve_vtable_layout_from_ini(STR("UObjectBaseUtility"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size);
                    Output::send(STR("UObjectBaseUtility::{} = 0x{:X}\n"), item, offset);
                    Unreal::UObjectBaseUtility::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("UObject\n"));
                uint32_t uobject_size = retrieve_vtable_layout_from_ini(STR("UObject"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size);
                    Output::send(STR("UObject::{} = 0x{:X}\n"), item, offset);
                    Unreal::UObject::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("UField\n"));
                uint32_t ufield_size = retrieve_vtable_layout_from_ini(STR("UField"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size);
                    Output::send(STR("UField::{} = 0x{:X}\n"), item, offset);
                    Unreal::UField::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("UEngine\n"));
                uint32_t uengine_size = retrieve_vtable_layout_from_ini(STR("UEngine"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size);
                    Output::send(STR("UEngine::{} = 0x{:X}\n"), item, offset);
                    Unreal::UEngine::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("UScriptStruct::ICppStructOps\n"));
                retrieve_vtable_layout_from_ini(STR("UScriptStruct::ICppStructOps"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, 0);
                    Output::send(STR("UScriptStruct::ICppStructOps::{} = 0x{:X}\n"), item, offset);
                    Unreal::UScriptStruct::ICppStructOps::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("FField\n"));
                uint32_t ffield_size = retrieve_vtable_layout_from_ini(STR("FField"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, 0);
                    Output::send(STR("FField::{} = 0x{:X}\n"), item, offset);
                    Unreal::FField::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("FProperty\n"));
                uint32_t fproperty_size = retrieve_vtable_layout_from_ini(STR("FProperty"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset{};
                    if (Unreal::Version::IsBelow(4, 25))
                    {
                        offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, ufield_size);
                    }
                    else
                    {
                        offset = calculate_virtual_function_offset(index, ffield_size);
                    }
                    Output::send(STR("FProperty::{} = 0x{:X}\n"), item, offset);
                    Unreal::FProperty::VTableLayoutMap.emplace(item, offset);
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

                Output::send<Color::Blue>(STR("FNumericProperty\n"));
                retrieve_vtable_layout_from_ini(STR("FNumericProperty"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, fproperty_size);
                    Output::send(STR("FNumericProperty::{} = 0x{:X}\n"), item, offset);
                    Unreal::FNumericProperty::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("FMulticastDelegateProperty\n"));
                retrieve_vtable_layout_from_ini(STR("FMulticastDelegateProperty"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, fproperty_size);
                    Output::send(STR("FMulticastDelegateProperty::{} = 0x{:X}\n"), item, offset);
                    Unreal::FMulticastDelegateProperty::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("FObjectPropertyBase\n"));
                retrieve_vtable_layout_from_ini(STR("FObjectPropertyBase"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, fproperty_size);
                    Output::send(STR("FObjectPropertyBase::{} = 0x{:X}\n"), item, offset);
                    Unreal::FObjectPropertyBase::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("UStruct\n"));
                retrieve_vtable_layout_from_ini(STR("UStruct"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, ufield_size);
                    Output::send(STR("UStruct::{} = 0x{:X}\n"), item, offset);
                    Unreal::UStruct::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("FOutputDevice\n"));
                retrieve_vtable_layout_from_ini(STR("FOutputDevice"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, 0);
                    Output::send(STR("FOutputDevice::{} = 0x{:X}\n"), item, offset);
                    Unreal::FOutputDevice::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("FMalloc\n"));
                retrieve_vtable_layout_from_ini(STR("FMalloc"), [&](uint32_t index, File::StringType& item) {
                    // We don't support FExec, so we're manually telling it the size.
                    static constexpr uint32_t fexec_size = 1;
                    uint32_t offset = calculate_virtual_function_offset(index, fexec_size);
                    Output::send(STR("FMalloc::{} = 0x{:X}\n"), item, offset);
                    Unreal::FMalloc::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("AActor\n"));
                uint32_t aactor_size = retrieve_vtable_layout_from_ini(STR("AActor"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size);
                    Output::send(STR("AActor::{} = 0x{:X}\n"), item, offset);
                    Unreal::AActor::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("AGameModeBase\n"));
                uint32_t agamemodebase_size = retrieve_vtable_layout_from_ini(STR("AGameModeBase"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, aactor_size);
                    Output::send(STR("AGameModeBase::{} = 0x{:X}\n"), item, offset);
                    Unreal::AGameModeBase::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("AGameMode\n"));
                retrieve_vtable_layout_from_ini(STR("AGameMode"), [&](uint32_t index, File::StringType& item) {
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
                    Output::send(STR("AGameMode::{} = 0x{:X}\n"), item, offset);
                    Unreal::AGameMode::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("UPlayer\n"));
                uint32_t uplayer_size = retrieve_vtable_layout_from_ini(STR("UPlayer"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size);
                    Output::send(STR("UPlayer::{} = 0x{:X}\n"), item, offset);
                    Unreal::UPlayer::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("ULocalPlayer\n"));
                retrieve_vtable_layout_from_ini(STR("ULocalPlayer"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, uplayer_size);
                    Output::send(STR("ULocalPlayer::{} = 0x{:X}\n"), item, offset);
                    Unreal::ULocalPlayer::VTableLayoutMap.emplace(item, offset);
                });

                Output::send<Color::Blue>(STR("UDataTable\n"));
                retrieve_vtable_layout_from_ini(STR("UDataTable"), [&](uint32_t index, File::StringType& item) {
                    uint32_t offset = calculate_virtual_function_offset(index, uobjectbase_size, uobjectbaseutility_size, uobject_size, uplayer_size);
                    Output::send(STR("UDataTable::{} = 0x{:X}\n"), item, offset);
                    Unreal::UDataTable::VTableLayoutMap.emplace(item, offset);
                });

                file.close();
            }
        });

        config.bHookProcessInternal = settings_manager.Hooks.HookProcessInternal;
        config.bHookProcessLocalScriptFunction = settings_manager.Hooks.HookProcessLocalScriptFunction;
        config.bHookLoadMap = settings_manager.Hooks.HookLoadMap;
        config.bHookInitGameState = settings_manager.Hooks.HookInitGameState;
        config.bHookCallFunctionByNameWithArguments = settings_manager.Hooks.HookCallFunctionByNameWithArguments;
        config.bHookBeginPlay = settings_manager.Hooks.HookBeginPlay;
        config.bHookEndPlay = settings_manager.Hooks.HookEndPlay;
        config.bHookLocalPlayerExec = settings_manager.Hooks.HookLocalPlayerExec;
        config.bHookAActorTick = settings_manager.Hooks.HookAActorTick;
        config.bHookEngineTick = settings_manager.Hooks.HookEngineTick;
        config.EngineTickResolveMethod = settings_manager.Hooks.EngineTickResolveMethod;
        config.bHookGameViewportClientTick = settings_manager.Hooks.HookGameViewportClientTick;
        config.bHookUObjectProcessEvent = settings_manager.Hooks.HookUObjectProcessEvent;
        config.bHookProcessConsoleExec = settings_manager.Hooks.HookProcessConsoleExec;
        config.bHookUStructLink = settings_manager.Hooks.HookUStructLink;
        config.FExecVTableOffsetInLocalPlayer = settings_manager.Hooks.FExecVTableOffsetInLocalPlayer;
        // Apply Debug Build setting from settings file only for now.
        Unreal::Version::DebugBuild = settings_manager.EngineVersionOverride.DebugBuild;
        Output::send<LogLevel::Warning>(STR("DebugGame Setting Enabled? {}\n"), Unreal::Version::DebugBuild);
        if (settings_manager.General.DoEarlyScan)
        {
            // Scan a single time while the game thread is locked after UE4SS is attached.
            Unreal::UnrealInitializer::PreInitialize(config);
            try
            {
                Unreal::UnrealInitializer::ScanGame();
            }
            catch (std::runtime_error&)
            {
                // No work to be done here. Error is non-fatal, just let the 'Initialize' function take it from here.
            }
        }
        cpp_mods_done_loading.store(true);
        cpp_mods_done_loading.notify_one();
        // Continuous scanning, and finish initializing after the game thread is unlocked.
        Unreal::UnrealInitializer::Initialize(config);

        bool can_create_custom_events{true};
        if (!UObject::ProcessLocalScriptFunctionInternal.is_ready() && Unreal::Version::IsAtLeast(4, 22))
        {
            can_create_custom_events = false;
            Output::send<LogLevel::Warning>(STR("ProcessLocalScriptFunction is not available, the following features will be unavailable:\n"));
        }
        else if (!UObject::ProcessInternalInternal.is_ready() && Unreal::Version::IsBelow(4, 22))
        {
            can_create_custom_events = false;
            Output::send<LogLevel::Warning>(STR("ProcessInternal is not available, the following features will be unavailable:\n"));
        }
        if (!can_create_custom_events)
        {
            Output::send<LogLevel::Warning>(STR("<Put function here responsible for creating custom UFunctions or events for BPs>\n"));
        }
        if (!Unreal::GNatives_Internal)
        {
            Output::send<LogLevel::Warning>(STR("GNatives not found, you will experience limited hooking functionality in certain scenarios.\n"));
        }
    }

    auto UE4SSProgram::share_lua_functions() -> void
    {
        m_shared_functions.set_script_variable_int32_function = &LuaLibrary::set_script_variable_int32;
        m_shared_functions.set_script_variable_default_data_function = &LuaLibrary::set_script_variable_default_data;
        m_shared_functions.call_script_function_function = &LuaLibrary::call_script_function;
        m_shared_functions.is_ue4ss_initialized_function = &LuaLibrary::is_ue4ss_initialized;
        Output::send(STR("m_shared_functions: {}\n"), static_cast<void*>(&m_shared_functions));
    }

    static bool s_gui_initialized_for_game_thread{};
    static bool s_gui_initializing_for_game_thread{};
    auto gui_render_thread_tick(Unreal::UObject*, float) -> void
    {
        if (UE4SSProgram::settings_manager.Debug.RenderMode == GUI::RenderMode::ExternalThread)
        {
            return;
        }
        std::lock_guard guard(UE4SSProgram::get_program().m_render_thread_mutex);
        if (UE4SSProgram::get_program().get_debugging_ui().exit_requested())
        {
            UE4SSProgram::get_program().get_debugging_ui().uninitialize();
            s_gui_initialized_for_game_thread = false;
            return;
        }
        if (!UE4SSProgram::get_program().get_debugging_ui().is_open())
        {
            return;
        }
        if (!s_gui_initialized_for_game_thread)
        {
            GUI::gui_thread(std::nullopt, &UE4SSProgram::get_program().get_debugging_ui());
            s_gui_initialized_for_game_thread = true;
        }
        if (s_gui_initializing_for_game_thread)
        {
            s_gui_initializing_for_game_thread = false;
        }
        UE4SSProgram::get_program().get_debugging_ui().main_loop_internal();
    }

    auto UE4SSProgram::on_program_start() -> void
    {
        ProfilerScope();
        using namespace Unreal;

        // Commented out because this system (turn off hotkeys when in-game console is open) it doesn't work properly.
        /*
        UObjectArray::AddUObjectCreateListener(&FUEDeathListener::UEDeathListener);
        //*/

        if (settings_manager.Debug.RenderMode == GUI::RenderMode::EngineTick)
        {
            Hook::RegisterEngineTickPostCallback(gui_render_thread_tick);
        }
        else if (settings_manager.Debug.RenderMode == GUI::RenderMode::GameViewportClientTick)
        {
            Hook::RegisterGameViewportClientTickPostCallback(gui_render_thread_tick);
        }

        if (settings_manager.Debug.DebugConsoleEnabled)
        {
            if (settings_manager.General.UseUObjectArrayCache)
            {
                m_debugging_gui.get_live_view().set_listeners_allowed(true);
            }
            else
            {
                m_debugging_gui.get_live_view().set_listeners_allowed(false);
            }
            register_keydown_event(Input::Key::O, {Input::ModifierKey::CONTROL}, [&]() {
                TRY([&] {
                    std::lock_guard guard(m_render_thread_mutex);
                    if (s_gui_initializing_for_game_thread)
                    {
                        Output::send<LogLevel::Verbose>(STR("Cancelled GUI toggle during GUI initialization.\n"));
                        return;
                    }
                    auto was_gui_open = get_debugging_ui().is_open();
                    stop_render_thread();
                    if (!was_gui_open)
                    {
                        switch (settings_manager.Debug.RenderMode)
                        {
                        case GUI::RenderMode::ExternalThread:
                            m_render_thread = std::jthread{&GUI::gui_thread, &m_debugging_gui};
                            break;
                        case GUI::RenderMode::EngineTick:
                        case GUI::RenderMode::GameViewportClientTick:
                            // The hooked game function will pick up on the window being "open", and start rendering.
                            s_gui_initialized_for_game_thread = false;
                            s_gui_initializing_for_game_thread = true;
                            get_debugging_ui().set_open(true);
                            break;
                        }
                        fire_ui_init_for_cpp_mods();
                    }
                });
            });
        }

#ifdef TIME_FUNCTION_MACRO_ENABLED
        register_keydown_event(Input::Key::Y, {Input::ModifierKey::CONTROL}, [&]() {
            if (FunctionTimerFrame::s_timer_enabled)
            {
                FunctionTimerFrame::stop_profiling();
                FunctionTimerFrame::dump_profile();
                Output::send(STR("Profiler stopped & dumped\n"));
            }
            else
            {
                FunctionTimerFrame::start_profiling();
                Output::send(STR("Profiler started\n"));
            }
        });
#endif

        TRY([&] {
            ObjectDumper::init();
            if (settings_manager.General.EnableHotReloadSystem)
            {
                register_keydown_event(settings_manager.General.HotReloadKey, {Input::ModifierKey::CONTROL}, [&]() {
                    TRY([&] {
                        queue_reinstall_mods();
                    });
                });
            }
            if ((settings_manager.ObjectDumper.LoadAllAssetsBeforeDumpingObjects || settings_manager.CXXHeaderGenerator.LoadAllAssetsBeforeGeneratingCXXHeaders) &&
                Unreal::Version::IsBelow(4, 17))
            {
                Output::send<LogLevel::Warning>(
                        STR("FAssetData not available in <4.17, ignoring 'LoadAllAssetsBeforeDumpingObjects' & 'LoadAllAssetsBeforeGeneratingCXXHeaders'."));
            }
            else if (!bFAssetDataAvailable)
            {
                Output::send<LogLevel::Warning>(
                        STR("FAssetData not available, ignoring 'LoadAllAssetsBeforeDumpingObjects' & 'LoadAllAssetsBeforeGeneratingCXXHeaders'."));
            }

#ifdef HAS_INPUT
            m_input_handler.init();
            if (!settings_manager.General.InputSource.empty())
            {
                if (m_input_handler.set_input_source(to_string(settings_manager.General.InputSource)))
                {
                    Output::send(STR("Input source set to: {}\n"), to_generic_string(m_input_handler.get_current_input_source()));
                }
                else
                {
                    Output::send<LogLevel::Error>(STR("Failed to set input source to: {}\n"), settings_manager.General.InputSource);
                }
            }
#endif

            // Set default ExecuteInGameThread method from settings
            LuaMod::m_default_game_thread_method = settings_manager.General.DefaultExecuteInGameThreadMethod;

            install_lua_mods();
            LuaMod::on_program_start();
            fire_program_start_for_cpp_mods();
            start_lua_mods();
        });

        if (settings_manager.General.EnableDebugKeyBindings)
        {
            register_keydown_event(Input::Key::NUM_NINE, {Input::ModifierKey::CONTROL}, [&]() {
                generate_uht_compatible_headers();
            });
        }
    }

    auto UE4SSProgram::update() -> void
    {
        ProfilerSetThreadName("UE4SS-UpdateThread");
        m_event_loop_thread_id = std::this_thread::get_id();

        on_program_start();

        Output::send(STR("Event loop start\n"));
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
                                                     [&](EventCallable& event) -> bool {
                                                         if (num_events_executed >= max_events_executed_per_frame)
                                                         {
                                                             return false;
                                                         }
                                                         ++num_events_executed;
                                                         event();
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
        Output::send(STR("Event loop end\n"));
    }

    auto UE4SSProgram::setup_unreal_properties() -> void
    {
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("ObjectProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_objectproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("ClassProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_classproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("Int8Property"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_int8property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("Int16Property"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_int16property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("IntProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_intproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("Int64Property"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_int64property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("ByteProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_byteproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("UInt16Property"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_uint16property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("UInt32Property"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_uint32property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("UInt64Property"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_uint64property);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("StructProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_structproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("ArrayProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_arrayproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("SetProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_setproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("MapProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_mapproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("FloatProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_floatproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("DoubleProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_doubleproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("BoolProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_boolproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("EnumProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_enumproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("WeakObjectProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_weakobjectproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("NameProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_nameproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("TextProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_textproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("StrProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_strproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("SoftObjectProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_softobjectproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("SoftClassProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_softobjectproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("InterfaceProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_interfaceproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("DelegateProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_delegateproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("MulticastDelegateProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_multicastdelegateproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("MulticastInlineDelegateProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_multicastdelegateproperty);
        LuaType::StaticState::m_property_value_pushers.emplace(FName(STR("MulticastSparseDelegateProperty"), Unreal::FNAME_Find).GetComparisonIndex(), &LuaType::push_multicastsparsedelegateproperty);
    }

    auto UE4SSProgram::setup_mods() -> void
    {
        ProfilerScope();

        Output::send(STR("Setting up mods...\n"));

        for (const auto& mods_directory : std::ranges::reverse_view(m_mods_directories))
        {
            if (!std::filesystem::exists(mods_directory))
            {
                Output::send<LogLevel::Warning>(STR("Mods directory doesn't exist, skipping: {}\n"), ensure_str(mods_directory));
                continue;
            }

            Output::send(STR("Loading mods from: {}\n"), ensure_str(mods_directory));

            for (const auto& sub_directory : std::filesystem::directory_iterator(mods_directory))
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

                StringType directory_lowercase = ensure_str(sub_directory.path().stem());
                std::transform(directory_lowercase.begin(), directory_lowercase.end(), directory_lowercase.begin(), std::towlower);

                if (directory_lowercase == STR("shared"))
                {
                    // Do stuff when shared libraries have been implemented
                }
                else
                {
                    auto mod_name = ensure_str(sub_directory.path().stem());
                    // Create the mod but don't install it yet
                    if (!find_mod_by_name<LuaMod>(mod_name) && std::filesystem::exists(sub_directory.path() / "scripts"))
                        m_mods.emplace_back(std::make_unique<LuaMod>(*this, std::move(mod_name), ensure_str(sub_directory.path())));
                    if (!find_mod_by_name<CppMod>(mod_name) && std::filesystem::exists(sub_directory.path() / "dlls"))
                        m_mods.emplace_back(std::make_unique<CppMod>(*this, std::move(mod_name), ensure_str(sub_directory.path())));
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
                Output::send<LogLevel::Warning>(STR("Mod name '{}' is already in use.\n"), mod->get_name());
                continue;
            }

            if (mod->is_installed())
            {
                Output::send<LogLevel::Warning>(STR("Tried to install a mod that was already installed, Mod: '{}'\n"), mod->get_name());
                continue;
            }

            if (!mod->is_installable())
            {
                Output::send<LogLevel::Warning>(STR("Was unable to install mod '{}' for unknown reasons. Mod is not installable.\n"), mod->get_name());
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

    auto UE4SSProgram::fire_ui_init_for_cpp_mods() -> void
    {
        ProfilerScope();
        for (const auto& mod : m_mods)
        {
            if (!dynamic_cast<CppMod*>(mod.get()))
            {
                continue;
            }
            mod->fire_ui_init();
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

    auto UE4SSProgram::fire_dll_load_for_cpp_mods(StringViewType dll_name) -> void
    {
        for (const auto& mod : m_mods)
        {
            if (auto cpp_mod = dynamic_cast<CppMod*>(mod.get()); cpp_mod)
            {
                cpp_mod->fire_dll_load(dll_name);
            }
        }
    }

    auto UE4SSProgram::fire_on_cpp_mods_loaded_for_cpp_mods() -> void
    {
        for (const auto& mod : m_mods)
        {
            if (auto cpp_mod = dynamic_cast<CppMod*>(mod.get()); cpp_mod)
            {
                cpp_mod->fire_on_cpp_mods_loaded();
            }
        }
    }

    template <typename ModType>
    auto start_mods() -> std::string
    {
        ProfilerScope();

        // Determine which mods.txt file(s) to parse
        std::vector<std::filesystem::path> mods_txt_files_to_parse;

        if (!UE4SSProgram::settings_manager.Overrides.ControllingModsTxt.empty())
        {
            // If a controlling mods.txt is specified, only use that one
            auto controlling_path = UE4SSProgram::get_program().make_compatible_path(UE4SSProgram::settings_manager.Overrides.ControllingModsTxt);
            if (std::filesystem::exists(controlling_path))
            {
                mods_txt_files_to_parse.push_back(controlling_path);
                Output::send(STR("Using controlling mods.txt from: {}\n"), ensure_str(controlling_path));
            }
            else
            {
                Output::send(STR("Warning: Controlling mods.txt not found at: {}\n"), ensure_str(controlling_path));
            }
        }
        else
        {
            // Parse mods.txt from all directories
            for (const auto& mods_directory : std::ranges::reverse_view(UE4SSProgram::get_program().get_mods_directories()))
            {
                if (!std::filesystem::exists(mods_directory))
                {
                    continue;
                }

                auto mods_txt_path = mods_directory / "mods.txt";
                if (std::filesystem::exists(mods_txt_path))
                {
                    mods_txt_files_to_parse.push_back(mods_txt_path);
                }
            }
        }

        // Process each mods.txt file
        for (const auto& enabled_mods_file : mods_txt_files_to_parse)
        {
            // Part #1: Start all mods that are enabled in mods.txt.
            if (!std::filesystem::exists(enabled_mods_file))
            {
                Output::send(STR("No mods.txt file found...\n"));
            }
            else
            {
                // 'mods.txt' exists, lets parse it
                Output::send(STR("Starting mods (from mods.txt ({}) load order)...\n"), ensure_str(enabled_mods_file));

                // First, check for BOM using a byte stream
                std::ifstream bom_check(enabled_mods_file, std::ios::binary);
                char bom[3] = {0};
                bom_check.read(bom, 3);
                bool has_bom = (bom[0] == '\xEF' && bom[1] == '\xBB' && bom[2] == '\xBF');
                bom_check.close();

                // Now open the actual stream
                StreamIType mods_stream{enabled_mods_file};

                // If BOM was detected, skip the first "character" (which will be the BOM interpreted as a wide char)
                if (has_bom)
                {
                    wchar_t discard;
                    mods_stream.get(discard);
                }

                StringType current_line;
                while (std::getline(mods_stream, current_line))
                {
                    // Don't parse any lines with ';'
                    if (current_line.find(STR(";")) != current_line.npos)
                    {
                        continue;
                    }

                    // Don't parse if the line is impossibly short (empty lines for example)
                    if (current_line.size() <= 4)
                    {
                        continue;
                    }

                    // Remove all spaces
                    auto end = std::remove(current_line.begin(), current_line.end(), STR(' '));
                    current_line.erase(end, current_line.end());

                    // Parse the line into something that can be converted into proper data
                    StringType mod_name = explode_by_occurrence(current_line, STR(':'), 1);
                    StringType mod_enabled = explode_by_occurrence(current_line, STR(':'), ExplodeType::FromEnd);

                    auto mod = UE4SSProgram::find_mod_by_name<ModType>(mod_name, UE4SSProgram::IsInstalled::Yes);
                    if (!mod || !dynamic_cast<ModType*>(mod) || mod->is_started())
                    {
                        continue;
                    }

                    if (!mod_enabled.empty() && mod_enabled[0] == STR('1'))
                    {
                        Output::send(STR("Starting {} mod '{}'\n"), std::is_same_v<ModType, LuaMod> ? STR("Lua") : STR("C++"), mod->get_name().data());
                        mod->start_mod();
                    }
                    else
                    {
                        Output::send(STR("Mod '{}' disabled in mods.txt.\n"), mod_name);
                    }
                }
            }
        }

        // Part #2: Start all mods that have enabled.txt present in the mod directory.
        for (const auto& mods_directory : UE4SSProgram::get_program().get_mods_directories())
        {
            if (!std::filesystem::exists(mods_directory))
            {
                continue;
            }

            Output::send(STR("Starting mods (from enabled.txt ({}), no defined load order)...\n"), ensure_str(mods_directory));

            for (const auto& mod_directory : std::filesystem::directory_iterator(mods_directory))
            {
                std::error_code ec{};

                if (!mod_directory.is_directory(ec))
                {
                    continue;
                }
                if (ec.value() != 0)
                {
                    return fmt::format("is_directory ran into error {}", ec.value());
                }

                if (!std::filesystem::exists(mod_directory.path() / "enabled.txt", ec))
                {
                    continue;
                }
                if (ec.value() != 0)
                {
                    return fmt::format("exists ran into error {}", ec.value());
                }

                auto mod = UE4SSProgram::find_mod_by_name<ModType>(ensure_str(mod_directory.path().stem()), UE4SSProgram::IsInstalled::Yes);
                if (!dynamic_cast<ModType*>(mod))
                {
                    continue;
                }
                if (!mod)
                {
                    Output::send<LogLevel::Warning>(STR("Found a mod with enabled.txt but mod has not been installed properly.\n"));
                    continue;
                }

                if (mod->is_started())
                {
                    continue;
                }

                Output::send(STR("Mod '{}' has enabled.txt, starting mod.\n"), mod->get_name().data());
                mod->start_mod();
            }
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

    auto UE4SSProgram::start_cpp_mods(IsInitialStartup is_initial_startup) -> void
    {
        ProfilerScope();
        auto error_message = start_mods<CppMod>();
        if (!error_message.empty())
        {
            set_error(error_message.c_str());
        }
        // If this is the initial startup, notify mods that the UI has initialized.
        // This isn't completely accurate since the UI will usually have started a while ago.
        // However, we can't immediately notify mods of this because no mods have been started at that point.
        // We only need to do this for the initial start of UE4SS because after that, more accurate notifications will happen when the UI is closed an reopened.
        if (is_initial_startup == IsInitialStartup::Yes && m_render_thread.get_id() != std::this_thread::get_id())
        {
            fire_ui_init_for_cpp_mods();
        }
        fire_on_cpp_mods_loaded_for_cpp_mods();
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
            if (!mod->is_installable())
            {
                continue;
            }
            mod->uninstall();
        }

        m_mods.clear();
        LuaMod::global_uninstall();
    }

    auto UE4SSProgram::delete_mod(Mod* mod) -> void
    {
        for (auto it = m_mods.begin(); it != m_mods.end();)
        {
            if (it->get() == mod)
            {
                it = m_mods.erase(it);
                break;
            }
            else
            {
                ++it;
            }
        }
    }

    auto UE4SSProgram::is_program_started() -> bool
    {
        return m_is_program_started;
    }

    auto UE4SSProgram::find_mod_by_id(ModId mod_id) -> Mod*
    {
        if (mod_id == InvalidModId)
        {
            return nullptr;
        }
        for (auto& mod : m_mods)
        {
            if (mod->get_id() == mod_id)
            {
                return mod.get();
            }
        }
        return nullptr;
    }

    auto UE4SSProgram::find_lua_mod_by_id(ModId mod_id) -> LuaMod*
    {
        return dynamic_cast<LuaMod*>(find_mod_by_id(mod_id));
    }

    auto UE4SSProgram::queue_reinstall_mods() -> void
    {
        if (!is_event_loop_thread())
        {
            queue_event([this]() { queue_reinstall_mods(); });
            return;
        }

        ProfilerScope();
        Output::send(STR("Re-installing all mods\n"));

        // Stop processing events while stuff isn't properly setup
        m_pause_events_processing = true;

        uninstall_mods();

// Remove key binds that were set from Lua scripts
#ifdef HAS_INPUT
        m_input_handler.get_events_safe([&](auto& key_set) {
            std::erase_if(key_set.key_data, [&](auto& item) -> bool {
                auto& [_, key_data] = item;
                bool were_all_events_registered_from_lua = true;
                std::erase_if(key_data, [&](Input::KeyData& key_data) -> bool {
                    // custom_data == 1: Bind came from Lua, and custom_data2 is a pointer to LuaMod.
                    // custom_data == 2: Bind came from C++, and custom_data2 is a pointer to KeyDownEventData. Must free it.
                    if (key_data.custom_data == 1)
                    {
                        return true;
                    }
                    else
                    {
                        were_all_events_registered_from_lua = false;
                        return false;
                    }
                });

                return were_all_events_registered_from_lua;
            });
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

        Output::send(STR("All mods re-installed\n"));
    }

    auto UE4SSProgram::queue_reinstall_mod(LuaMod* mod) -> void
    {
        if (!mod)
        {
            return;
        }

        if (!is_event_loop_thread())
        {
            queue_event([this, mod]() { queue_reinstall_mod(mod); });
            return;
        }

        // Save mod info before uninstalling
        StringType mod_name = StringType(mod->get_name());
        std::filesystem::path mod_path = mod->get_path();

        Output::send(STR("Reinstalling mod: {}\n"), mod_name);

        // Pause event processing for safety
        m_pause_events_processing = true;

        mod->uninstall();

        // Remove key binds registered by this specific mod
#ifdef HAS_INPUT
        m_input_handler.get_events_safe([&](auto& key_set) {
            std::erase_if(key_set.key_data, [&](auto& item) -> bool {
                auto& [_, key_data] = item;
                std::erase_if(key_data, [&](Input::KeyData& kd) -> bool {
                    // custom_data == 1: Bind came from Lua, custom_data2 is pointer to LuaMod
                    return kd.custom_data == 1 && kd.custom_data2 == mod;
                });
                return key_data.empty();
            });
        });
#endif

        // Remove the old mod from the list
        delete_mod(mod);
        mod = nullptr;

        // Resume event processing before starting the new mod
        m_pause_events_processing = false;

        // Create a new LuaMod for this mod (same as setup_mods does)
        auto new_mod = std::make_unique<LuaMod>(*this, std::move(mod_name), std::move(mod_path));
        LuaMod* new_mod_ptr = new_mod.get();
        m_mods.emplace_back(std::move(new_mod));

        new_mod_ptr->start_mod();

        Output::send(STR("Mod '{}' reinstalled\n"), new_mod_ptr->get_name());
    }

    auto UE4SSProgram::queue_uninstall_mod(LuaMod* mod) -> void
    {
        if (!mod)
        {
            return;
        }

        if (!is_event_loop_thread())
        {
            queue_event([this, mod]() { queue_uninstall_mod(mod); });
            return;
        }

        StringType mod_name = StringType(mod->get_name());
        Output::send(STR("Uninstalling mod: {}\n"), mod_name);

        // Pause event processing for safety
        m_pause_events_processing = true;

        mod->uninstall();

        // Remove key binds registered by this specific mod
#ifdef HAS_INPUT
        m_input_handler.get_events_safe([&](auto& key_set) {
            std::erase_if(key_set.key_data, [&](auto& item) -> bool {
                auto& [_, key_data] = item;
                std::erase_if(key_data, [&](Input::KeyData& kd) -> bool {
                    // custom_data == 1: Bind came from Lua, custom_data2 is pointer to LuaMod
                    return kd.custom_data == 1 && kd.custom_data2 == mod;
                });
                return key_data.empty();
            });
        });
#endif

        delete_mod(mod);

        // Resume event processing
        m_pause_events_processing = false;

        Output::send(STR("Mod '{}' uninstalled\n"), mod_name);
    }

    auto UE4SSProgram::queue_reinstall_mod(ModId mod_id) -> void
    {
        if (mod_id == InvalidModId)
        {
            return;
        }

        if (!is_event_loop_thread())
        {
            queue_event([this, mod_id]() { queue_reinstall_mod(mod_id); });
            return;
        }

        // Look up the mod by ID at execution time (safe for queued events)
        if (auto* lua_mod = find_lua_mod_by_id(mod_id))
        {
            queue_reinstall_mod(lua_mod);
        }
        else
        {
            Output::send<LogLevel::Warning>(STR("Could not find mod to reinstall with ID: {}\n"), mod_id);
        }
    }

    auto UE4SSProgram::queue_uninstall_mod(ModId mod_id) -> void
    {
        if (mod_id == InvalidModId)
        {
            return;
        }

        if (!is_event_loop_thread())
        {
            queue_event([this, mod_id]() { queue_uninstall_mod(mod_id); });
            return;
        }

        // Look up the mod by ID at execution time (safe for queued events)
        if (auto* lua_mod = find_lua_mod_by_id(mod_id))
        {
            queue_uninstall_mod(lua_mod);
        }
        else
        {
            Output::send<LogLevel::Warning>(STR("Could not find mod to uninstall with ID: {}\n"), mod_id);
        }
    }

    auto UE4SSProgram::queue_reinstall_mod_by_name(const std::string& mod_name) -> void
    {
        if (!is_event_loop_thread())
        {
            queue_event([this, mod_name]() { queue_reinstall_mod_by_name(mod_name); });
            return;
        }

        // Find the mod by name at execution time (safe for queued events)
        for (auto& mod : m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod && to_string(lua_mod->get_name()) == mod_name)
            {
                queue_reinstall_mod(lua_mod);
                return;
            }
        }
        Output::send<LogLevel::Warning>(STR("Could not find mod to reinstall: {}\n"), ensure_str(mod_name));
    }

    auto UE4SSProgram::queue_reinstall_mod_by_name(std::string_view mod_name) -> void
    {
        queue_reinstall_mod_by_name(std::string{mod_name});
    }

    auto UE4SSProgram::queue_uninstall_mod_by_name(const std::string& mod_name) -> void
    {
        if (!is_event_loop_thread())
        {
            queue_event([this, mod_name]() { queue_uninstall_mod_by_name(mod_name); });
            return;
        }

        // Find the mod by name at execution time (safe for queued events)
        for (auto& mod : m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod && to_string(lua_mod->get_name()) == mod_name)
            {
                queue_uninstall_mod(lua_mod);
                return;
            }
        }
        Output::send<LogLevel::Warning>(STR("Could not find mod to uninstall: {}\n"), ensure_str(mod_name));
    }

    auto UE4SSProgram::queue_uninstall_mod_by_name(std::string_view mod_name) -> void
    {
        queue_uninstall_mod_by_name(std::string{mod_name});
    }

    auto UE4SSProgram::queue_start_lua_mod_by_path(const std::filesystem::path& mod_path) -> void
    {
        if (!is_event_loop_thread())
        {
            queue_event([this, mod_path]() { queue_start_lua_mod_by_path(mod_path); });
            return;
        }

        std::string mod_name_str = mod_path.stem().string();

        // Check if mod already exists in m_mods
        for (auto& mod : m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod && to_string(lua_mod->get_name()) == mod_name_str)
            {
                if (lua_mod->is_started())
                {
                    Output::send<LogLevel::Warning>(STR("Mod '{}' is already running\n"), ensure_str(mod_name_str));
                    return;
                }
                else
                {
                    // Mod exists but is not started - remove it first (its Lua state is invalid)
                    // Then we'll create a fresh one below
                    delete_mod(lua_mod);
                    break;
                }
            }
        }

        // Verify the mod path exists and has a main.lua
        std::filesystem::path scripts_path = mod_path / STR("Scripts");
        std::filesystem::path main_lua = scripts_path / STR("main.lua");
        if (!std::filesystem::exists(main_lua))
        {
            Output::send<LogLevel::Error>(STR("Cannot start mod '{}': main.lua not found\n"), ensure_str(mod_name_str));
            return;
        }

        Output::send(STR("Starting mod: {}\n"), ensure_str(mod_name_str));

        StringType mod_name = ensure_str(mod_name_str);

        auto new_mod = std::make_unique<LuaMod>(*this, std::move(mod_name), std::filesystem::path(mod_path));
        LuaMod* new_mod_ptr = new_mod.get();
        m_mods.emplace_back(std::move(new_mod));

        new_mod_ptr->start_mod();

        Output::send(STR("Mod '{}' started\n"), new_mod_ptr->get_name());
    }

    auto UE4SSProgram::get_module_directory() -> File::StringType
    {
        return ensure_str(m_module_file_path);
    }

    auto UE4SSProgram::get_game_executable_directory() -> File::StringType
    {
        return ensure_str(m_game_executable_directory);
    }

    auto UE4SSProgram::get_working_directory() -> File::StringType
    {
        return ensure_str(m_working_directory);
    }

    auto UE4SSProgram::get_mods_directory() -> File::StringType
    {
        // Return the first (primary) mods directory for backwards compatibility
        return m_mods_directories.empty() ? STR("") : ensure_str(m_mods_directories[0]);
    }

    auto UE4SSProgram::get_mods_directories() -> std::vector<std::filesystem::path>&
    {
        return m_mods_directories;
    }

    auto UE4SSProgram::get_mods_txt_entries() -> std::unordered_map<std::string, bool>
    {
        std::unordered_map<std::string, bool> result;

        std::vector<std::filesystem::path> mods_txt_files;

        if (!settings_manager.Overrides.ControllingModsTxt.empty())
        {
            auto controlling_path = make_compatible_path(settings_manager.Overrides.ControllingModsTxt);
            if (std::filesystem::exists(controlling_path))
            {
                mods_txt_files.push_back(controlling_path);
            }
        }
        else
        {
            for (const auto& mods_directory : std::ranges::reverse_view(m_mods_directories))
            {
                if (!std::filesystem::exists(mods_directory))
                {
                    continue;
                }

                auto mods_txt_path = mods_directory / "mods.txt";
                if (std::filesystem::exists(mods_txt_path))
                {
                    mods_txt_files.push_back(mods_txt_path);
                }
            }
        }

        for (const auto& mods_txt_path : mods_txt_files)
        {
            std::ifstream bom_check(mods_txt_path, std::ios::binary);
            char bom[3] = {0};
            bom_check.read(bom, 3);
            bool has_bom = (bom[0] == '\xEF' && bom[1] == '\xBB' && bom[2] == '\xBF');
            bom_check.close();

            StreamIType mods_stream{mods_txt_path};

            if (has_bom)
            {
                wchar_t discard;
                mods_stream.get(discard);
            }

            StringType current_line;
            while (std::getline(mods_stream, current_line))
            {
                if (current_line.find(STR(";")) != current_line.npos)
                {
                    continue;
                }

                if (current_line.size() <= 4)
                {
                    continue;
                }

                auto end = std::remove(current_line.begin(), current_line.end(), STR(' '));
                current_line.erase(end, current_line.end());

                StringType mod_name = explode_by_occurrence(current_line, STR(':'), 1);
                StringType mod_enabled = explode_by_occurrence(current_line, STR(':'), ExplodeType::FromEnd);

                std::string mod_name_str = to_string(mod_name);
                bool enabled = !mod_enabled.empty() && mod_enabled[0] == STR('1');

                if (result.find(mod_name_str) == result.end())
                {
                    result[mod_name_str] = enabled;
                }
            }
        }

        return result;
    }

    auto UE4SSProgram::make_compatible_path(const std::filesystem::path& in_path) const -> std::filesystem::path
    {
        auto path = in_path;
        if (path.is_relative())
        {
            path = m_working_directory / path;
        }
        path = path.lexically_normal().make_preferred();
        path = std::filesystem::weakly_canonical(path);
        return path;
    }

    auto UE4SSProgram::insert_mods_directory(const std::filesystem::path& path, int64_t index) -> void
    {
        m_mods_directories.insert(m_mods_directories.begin() + index, path);
    }

    auto UE4SSProgram::add_mods_directory(const std::filesystem::path& in_path) -> void
    {
        auto path = make_compatible_path(in_path);
        if (const auto it = std::ranges::find(m_mods_directories, path); it != m_mods_directories.end())
        {
            m_mods_directories.erase(it);
        }
        m_mods_directories.emplace_back(std::forward<decltype(path)>(path));
    }

    auto UE4SSProgram::remove_mods_directory(const std::filesystem::path& in_path) -> void
    {
        auto path = make_compatible_path(in_path);
        m_mods_directories_to_remove.emplace_back(std::forward<decltype(path)>(path));
    }

    auto UE4SSProgram::get_legacy_root_directory() -> File::StringType
    {
        return ensure_str(m_legacy_root_directory);
    }

    auto UE4SSProgram::generate_uht_compatible_headers() -> void
    {
        ProfilerScope();
        Output::send(STR("Generating UHT compatible headers...\n"));

        double generator_duration{};
        {
            ScopedTimer generator_timer{&generator_duration};

            const std::filesystem::path DumpRootDirectory = m_working_directory / "UHTHeaderDump";
            UEGenerator::UEHeaderGenerator HeaderGenerator = UEGenerator::UEHeaderGenerator(DumpRootDirectory);
            HeaderGenerator.dump_native_packages();
        }

        Output::send(STR("Generating UHT compatible headers took {} seconds\n"), generator_duration);
    }

    auto UE4SSProgram::generate_cxx_headers(const std::filesystem::path& output_dir) -> void
    {
        ProfilerScope();
        if (settings_manager.CXXHeaderGenerator.LoadAllAssetsBeforeGeneratingCXXHeaders)
        {
            Output::send(STR("Loading all assets...\n"));
            double asset_loading_duration{};
            {
                ProfilerScopeNamed("loading all assets");
                ScopedTimer loading_timer{&asset_loading_duration};

                UAssetRegistry::LoadAllAssets();
            }
            Output::send(STR("Loading all assets took {} seconds\n"), asset_loading_duration);
        }

        double generator_duration;
        {
            ProfilerScopeNamed("unloading all force-loaded assets");
            ScopedTimer generator_timer{&generator_duration};

            UEGenerator::generate_cxx_headers(output_dir);

            Output::send(STR("Unloading all forcefully loaded assets\n"));
        }

        UAssetRegistry::FreeAllForcefullyLoadedAssets();
        Output::send(STR("SDK generated in {} seconds.\n"), generator_duration);
    }

    auto UE4SSProgram::generate_lua_types(const std::filesystem::path& output_dir) -> void
    {
        ProfilerScope();
        if (settings_manager.CXXHeaderGenerator.LoadAllAssetsBeforeGeneratingCXXHeaders)
        {
            Output::send(STR("Loading all assets...\n"));
            double asset_loading_duration{};
            {
                ProfilerScopeNamed("loading all assets");
                ScopedTimer loading_timer{&asset_loading_duration};

                UAssetRegistry::LoadAllAssets();
            }
            Output::send(STR("Loading all assets took {} seconds\n"), asset_loading_duration);
        }

        double generator_duration;
        {
            ProfilerScopeNamed("unloading all force-loaded assets");
            ScopedTimer generator_timer{&generator_duration};

            UEGenerator::generate_lua_types(output_dir);

            Output::send(STR("Unloading all forcefully loaded assets\n"));
        }

        UAssetRegistry::FreeAllForcefullyLoadedAssets();
        Output::send(STR("SDK generated in {} seconds.\n"), generator_duration);
    }

    auto UE4SSProgram::stop_render_thread() -> void
    {
        if (!get_debugging_ui().is_open())
        {
            return;
        }
        if (settings_manager.Debug.RenderMode == GUI::RenderMode::ExternalThread && m_render_thread.joinable())
        {
            m_render_thread.request_stop();
            m_render_thread.join();
        }
        else
        {
            get_debugging_ui().request_exit();
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

    auto UE4SSProgram::queue_event(EventCallable callable) -> void
    {
        if (!can_process_events())
        {
            return;
        }
        std::lock_guard<std::mutex> guard(m_event_queue_mutex);
        m_queued_events.emplace_back(std::move(callable));
    }

    auto UE4SSProgram::queue_event(LegacyEventCallable callable, void* data) -> void
    {
        queue_event([callable, data]() { callable(data); });
    }

    auto UE4SSProgram::is_queue_empty() -> bool
    {
        // Not locking here because if the worst that could happen as far as I know is that the event loop processes the event slightly late.
        return m_queued_events.empty();
    }

    auto UE4SSProgram::register_keydown_event(Input::Key key, const Input::EventCallbackCallable& callback, uint8_t custom_data, void* custom_data2) -> void
    {
#ifdef HAS_INPUT
        m_input_handler.register_keydown_event(key, callback, custom_data, custom_data2);
#endif
    }

    auto UE4SSProgram::register_keydown_event(Input::Key key,
                                              const Input::Handler::ModifierKeyArray& modifier_keys,
                                              const Input::EventCallbackCallable& callback,
                                              uint8_t custom_data,
                                              void* custom_data2) -> void
    {
#ifdef HAS_INPUT
        m_input_handler.register_keydown_event(key, modifier_keys, callback, custom_data, custom_data2);
#endif
    }

    auto UE4SSProgram::is_keydown_event_registered(Input::Key key) -> bool
    {
#ifdef HAS_INPUT
        return m_input_handler.is_keydown_event_registered(key);
#else
        return false;
#endif
    }

    auto UE4SSProgram::is_keydown_event_registered(Input::Key key, const Input::Handler::ModifierKeyArray& modifier_keys) -> bool
    {
#ifdef HAS_INPUT
        return m_input_handler.is_keydown_event_registered(key, modifier_keys);
#else
        return false;
#endif
    }

    auto UE4SSProgram::get_all_input_events(std::function<void(Input::KeySet&)> callback) -> void
    {
#ifdef HAS_INPUT
        m_input_handler.get_events_safe(callback);
#endif
    }

    auto UE4SSProgram::find_mod_by_name_internal(StringViewType mod_name, IsInstalled is_installed, IsStarted is_started, FMBNI_ExtraPredicate extra_predicate)
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

    auto UE4SSProgram::find_lua_mod_by_name(std::string_view mod_name, UE4SSProgram::IsInstalled installed_only, IsStarted is_started) -> LuaMod*
    {
        return static_cast<LuaMod*>(find_mod_by_name<LuaMod>(mod_name, installed_only, is_started));
    }

    auto UE4SSProgram::find_lua_mod_by_name(StringViewType mod_name, UE4SSProgram::IsInstalled installed_only, IsStarted is_started) -> LuaMod*
    {
        return static_cast<LuaMod*>(find_mod_by_name<LuaMod>(mod_name, installed_only, is_started));
    }

    auto UE4SSProgram::get_object_dumper_output_directory() -> const File::StringType
    {
        return ensure_str(m_object_dumper_output_directory);
    }

    auto UE4SSProgram::dump_uobject(UObject* object,
                                    std::unordered_set<FField*>* in_dumped_fields,
                                    StringType& out_line,
                                    bool is_below_425,
                                    std::unordered_set<UFunction*>* in_dumped_functions) -> void
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

        static auto delegate_function_class = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/CoreUObject.DelegateFunction"));
        static auto linker_placeholder_function_class =
                UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/CoreUObject.LinkerPlaceholderFunction"));

        bool is_property = is_below_425 && Unreal::TypeChecker::is_property(typed_obj) &&
                           !typed_obj->HasAnyFlags(static_cast<EObjectFlags>(EObjectFlags::RF_DefaultSubObject | EObjectFlags::RF_ArchetypeObject));
        if (!is_property && (!typed_obj->IsA<UFunction>() || typed_obj->IsA(delegate_function_class) || typed_obj->IsA(linker_placeholder_function_class)))
        {
            if (in_dumped_functions && typed_obj->IsA<UFunction>())
            {
                if (in_dumped_functions->contains(static_cast<UFunction*>(typed_obj)))
                {
                    return;
                }
                else
                {
                    in_dumped_functions->emplace(static_cast<UFunction*>(typed_obj));
                }
            }
            auto typed_class = typed_obj->GetClassPrivate()->HashObject();
            if (ObjectDumper::to_string_exists(typed_class))
            {
                // Call type-specific implementation to dump UObject
                // The type is determined at runtime

                // Dump UObject
                ObjectDumper::get_to_string(typed_class)(object, out_line);
                out_line.append(STR("\n"));

                if (ObjectDumper::to_string_complex_exists(typed_class))
                {
                    // Dump all properties that are directly owned by this UObject (not its UClass)
                    ObjectDumper::get_to_string_complex(typed_class)(object, out_line, [&](void* prop) {
                        if (dumped_fields.contains(static_cast<FField*>(prop)))
                        {
                            return;
                        }

                        ObjectDumper::dump_xproperty(static_cast<FProperty*>(prop), out_line);
                        dumped_fields.emplace(static_cast<FField*>(prop));
                    });
                }
            }
            else
            {
                // A type-specific implementation does not exist so lets call the default implementation for UObjects instead
                ObjectDumper::object_to_string(object, out_line);
                out_line.append(STR("\n"));
            }

            // If the UClass of the UObject has any properties then dump them
            if (typed_obj->IsA<UStruct>())
            {
                for (FProperty* prop : TFieldRange<FProperty>(static_cast<UClass*>(typed_obj), Unreal::EFieldIterationFlags::IncludeDeprecated))
                {
                    if (dumped_fields.contains(prop))
                    {
                        continue;
                    }

                    ObjectDumper::dump_xproperty(prop, out_line);
                    dumped_fields.emplace(prop);
                }
            }

            if (typed_obj->IsA<UStruct>())
            {
                for (UFunction* func : TFieldRange<UFunction>(static_cast<UStruct*>(typed_obj), Unreal::EFieldIterationFlags::None))
                {
                    ObjectDumper::function_to_string(func, out_line, in_dumped_functions);
                }
            }
        }

        if (owns_dumped_fields)
        {
            delete dumped_fields_ptr;
        }
    }

    auto UE4SSProgram::dump_all_objects_and_properties(const File::StringType& output_path_and_file_name) -> void
    {
        /*
        Output::send(STR("Test msg with no fmt args, and no optional arg\n"));
        Output::send(STR("Test msg with no fmt args, and one optional arg [Normal]\n"), LogLevel::Normal);
        Output::send(STR("Test msg with no fmt args, and one optional arg [Verbose]\n"), LogLevel::Verbose);
        Output::send(STR("Test msg with one fmt arg [{}], and one optional arg [Warning]\n"), LogLevel::Warning, 33);
        Output::send(STR("Test msg with two fmt args [{}, {}], and one optional arg [Error]\n"), LogLevel::Error, 33, 44);
        //*/

        // Object & Property Dumper -> START
        if (settings_manager.ObjectDumper.LoadAllAssetsBeforeDumpingObjects)
        {
            Output::send(STR("Loading all assets...\n"));
            double asset_loading_duration{};
            {
                ScopedTimer loading_timer{&asset_loading_duration};

                UAssetRegistry::LoadAllAssets();
            }
            Output::send(STR("Loading all assets took {} seconds\n"), asset_loading_duration);
        }

        double dumper_duration{};
        {
            ScopedTimer dumper_timer{&dumper_duration};

            std::unordered_set<FField*> dumped_fields;
            // There will be tons of dumped fields so lets just reserve tons in order to speed things up a bit
            dumped_fields.reserve(100000);

            // Some delegate functions belong to a class, and are dumped as part of the class.
            // Others are not, and must be dumped as part of GUObjectArray.
            // Both are part of GUObjectArray.
            // We must maintain a list of already dumped functions to avoid dumping the same function multiple times.
            // We can't just use GUObjectArray even though they all exist in there because that would destroy the order in which objects get dumped.
            std::unordered_set<UFunction*> dumped_functions;
            dumped_fields.reserve(10000);

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
            file_device.set_formatter([](File::StringViewType string) -> File::StringType {
                return File::StringType{string};
            });

            // Make string & reserve massive amounts of space to hopefully not reach the end of the string and require more
            // dynamic allocations
            StringType out_line;
            out_line.reserve(200000000);

            Output::send(STR("Dumping all objects & properties in GUObjectArray\n"));
            UObjectGlobals::ForEachUObject([&](void* object, [[maybe_unused]] int32_t chunk_index, [[maybe_unused]] int32_t object_index) {
                dump_uobject(static_cast<UObject*>(object), &dumped_fields, out_line, is_below_425, &dumped_functions);
                return LoopAction::Continue;
            });

            // Save to file
            scoped_dumper_out.send(out_line);

            // Reset the dumped_fields set, otherwise no fields will be dumped in subsequent dumps
            dumped_fields.clear();
            Output::send(STR("Done iterating GUObjectArray\n"));
        }

        UAssetRegistry::FreeAllForcefullyLoadedAssets();
        Output::send(STR("Dumping GUObjectArray took {} seconds\n"), dumper_duration);
        // Object & Property Dumper -> END
    }

    auto UE4SSProgram::static_cleanup() -> void
    {
        delete &get_program();

        // Do cleanup of static objects here
        // This function is called right before the DLL detaches from the game
        // Including when the player hits the 'X' button to exit the game
    }

    auto UE4SSProgram::parse_semicolon_separated_string(const StringType& string) -> std::vector<StringType>
    {
        std::vector<StringType> strings{};
        if (auto end = string.find(STR(';')); end == string.npos)
        {
            // No colon, but we still have content in the variable, so we'll assume this a single path.
            strings.emplace_back(string);
        }
        else
        {
            size_t start = 0;
            while (end != string.npos)
            {
                strings.emplace_back(string.substr(start, end - start));
                start = end + 1; // Adding 1 to skip the colon.
                end = string.find(STR(';'), start);
                if (end == string.npos)
                {
                    // No more colons, but we still content so let's assume that's another path.
                    strings.emplace_back(string.substr(start));
                }
            }
        }
        return strings;
    }
} // namespace RC
