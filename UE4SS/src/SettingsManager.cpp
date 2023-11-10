#include <Helpers/String.hpp>
#include <IniParser/Ini.hpp>
#include <SettingsManager.hpp>

#define REGISTER_STRING_SETTING(member_var, section_name, key)                                                                                                 \
    try                                                                                                                                                        \
    {                                                                                                                                                          \
        (member_var) = parser.get_string(section_name, STR(#key));                                                                                             \
    }                                                                                                                                                          \
    catch (std::exception&)                                                                                                                                    \
    {                                                                                                                                                          \
    }

#define REGISTER_INT64_SETTING(member_var, section_name, key)                                                                                                  \
    try                                                                                                                                                        \
    {                                                                                                                                                          \
        (member_var) = parser.get_int64(section_name, STR(#key));                                                                                              \
    }                                                                                                                                                          \
    catch (std::exception&)                                                                                                                                    \
    {                                                                                                                                                          \
    }

#define REGISTER_BOOL_SETTING(member_var, section_name, key)                                                                                                   \
    try                                                                                                                                                        \
    {                                                                                                                                                          \
        (member_var) = parser.get_bool(section_name, STR(#key));                                                                                               \
    }                                                                                                                                                          \
    catch (std::exception&)                                                                                                                                    \
    {                                                                                                                                                          \
    }

#define REGISTER_FLOAT_SETTING(member_var, section_name, key)                                                                                                  \
    try                                                                                                                                                        \
    {                                                                                                                                                          \
        (member_var) = parser.get_float(section_name, STR(#key));                                                                                              \
    }                                                                                                                                                          \
    catch (std::exception&)                                                                                                                                    \
    {                                                                                                                                                          \
    }

namespace RC::Settings
{
    
    auto SettingsManager::read_settings() -> void
    {
        SettingsManager s{};
        std::string buffer{};
        Output::send<LogLevel::Error>(STR("Finding Settings.\n"));
        auto file = std::ofstream("./UE4SS-Settings.json", std::ios::in);
        if (file)
        {
            Output::send<LogLevel::Error>(STR("Settings found.\n"));
            glz::parse_error pe = glz::read_file_json(s, "./UE4SS-Settings.json", buffer);
            if (pe)
            {
                std::string descriptive_error = glz::format_error(pe, buffer);

                size_t count = descriptive_error.size() + 1;
                wchar_t* converted_method_name = new wchar_t[count];

                size_t num_of_char_converted = 0;
                mbstowcs_s(&num_of_char_converted, converted_method_name, count, descriptive_error.data(), count);

                auto converted = File::StringViewType(converted_method_name);

                delete[] converted_method_name;
                
                Output::send<LogLevel::Error>(STR("{}\n\nError parsing settings file, please fix the file or delete it to generate new settings.\nContinuing with default settings...\n"), converted);
            }
            Output::send<LogLevel::Default>(STR("Settings read.\n"));
        }
        else
        {
            glz::write<glz::opts{.prettify = true}>(s, buffer);
            glz::error_code ec = glz::buffer_to_file(buffer, "./UE4SS-Settings.json");
            if (ec != glz::error_code::none)
            {
                auto arr = glz::detail::make_enum_to_string_array<glz::error_code>();
                auto error_type_str = arr[static_cast<uint32_t>(ec)];
                
                size_t count = error_type_str.size() + 1;
                wchar_t* converted_method_name = new wchar_t[count];

                size_t num_of_char_converted = 0;
                mbstowcs_s(&num_of_char_converted, converted_method_name, count, error_type_str.data(), count);

                auto converted = File::StringViewType(converted_method_name);

                delete[] converted_method_name;
                
                Output::send<LogLevel::Error>(STR("\nError {} when writing new settings file.\nContinuing with default settings...\n"), converted);
            }
            Output::send<LogLevel::Default>(STR("Settings created.\n"));
        }   
    }
    
    auto SettingsManager::deserialize(std::filesystem::path& file_name) -> void
    {
        const auto SettingsManager = new RC::Settings::SettingsManager();
        SettingsManager->read_settings();

        auto file = File::open(file_name, File::OpenFor::Reading, File::OverwriteExistingFile::No, File::CreateIfNonExistent::Yes);
        Ini::Parser parser;
        parser.parse(file);
        file.close();

        constexpr static File::CharType section_overrides[] = STR("Overrides");
        REGISTER_STRING_SETTING(Overrides.ModsFolderPath, section_overrides, ModsFolderPath)

        constexpr static File::CharType section_general[] = STR("General");
        REGISTER_BOOL_SETTING(General.EnableHotReloadSystem.value, section_general, EnableHotReloadSystem)
        REGISTER_BOOL_SETTING(General.UseCache.value, section_general, UseCache)
        REGISTER_BOOL_SETTING(General.InvalidateCacheIfDLLDiffers.value, section_general, InvalidateCacheIfDLLDiffers)
        REGISTER_BOOL_SETTING(General.EnableDebugKeyBindings.value, section_general, EnableDebugKeyBindings)
        REGISTER_INT64_SETTING(General.SecondsToScanBeforeGivingUp.value, section_general, SecondsToScanBeforeGivingUp)
        REGISTER_BOOL_SETTING(General.UseUObjectArrayCache.value, section_general, bUseUObjectArrayCache)

        constexpr static File::CharType section_engine_version_override[] = STR("EngineVersionOverride");
        REGISTER_INT64_SETTING(EngineVersionOverride.MajorVersion, section_engine_version_override, MajorVersion)
        REGISTER_INT64_SETTING(EngineVersionOverride.MinorVersion, section_engine_version_override, MinorVersion)

        constexpr static File::CharType section_object_dumper[] = STR("ObjectDumper");
        REGISTER_BOOL_SETTING(ObjectDumper.LoadAllAssetsBeforeDumpingObjects, section_object_dumper, LoadAllAssetsBeforeDumpingObjects)

        constexpr static File::CharType section_cxx_header_generator[] = STR("CXXHeaderGenerator");
        REGISTER_BOOL_SETTING(CXXHeaderGenerator.DumpOffsetsAndSizes, section_cxx_header_generator, DumpOffsetsAndSizes)
        REGISTER_BOOL_SETTING(CXXHeaderGenerator.KeepMemoryLayout, section_cxx_header_generator, KeepMemoryLayout)
        REGISTER_BOOL_SETTING(CXXHeaderGenerator.LoadAllAssetsBeforeGeneratingCXXHeaders, section_cxx_header_generator, LoadAllAssetsBeforeGeneratingCXXHeaders)

        constexpr static File::CharType section_uht_header_generator[] = STR("UHTHeaderGenerator");
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.IgnoreAllCoreEngineModules, section_uht_header_generator, IgnoreAllCoreEngineModules)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.IgnoreEngineAndCoreUObject, section_uht_header_generator, IgnoreEngineAndCoreUObject)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.MakeAllFunctionsBlueprintCallable, section_uht_header_generator, MakeAllFunctionsBlueprintCallable)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.MakeAllPropertyBlueprintsReadWrite, section_uht_header_generator, MakeAllPropertyBlueprintsReadWrite)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.MakeEnumClassesBlueprintType, section_uht_header_generator, MakeEnumClassesBlueprintType)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.MakeAllConfigsEngineConfig, section_uht_header_generator, MakeAllConfigsEngineConfig)

        constexpr static File::CharType section_debug[] = STR("Debug");
        REGISTER_BOOL_SETTING(Debug.SimpleConsoleEnabled, section_debug, ConsoleEnabled)
        REGISTER_BOOL_SETTING(Debug.DebugConsoleEnabled, section_debug, GuiConsoleEnabled)
        REGISTER_BOOL_SETTING(Debug.DebugConsoleVisible, section_debug, GuiConsoleVisible)
        REGISTER_FLOAT_SETTING(Debug.DebugGUIFontScaling, section_debug, GuiConsoleFontScaling)
        StringType graphics_api_string{};
        REGISTER_STRING_SETTING(graphics_api_string, section_debug, GraphicsAPI)
        if (String::iequal(graphics_api_string, STR("DX11")) || String::iequal(graphics_api_string, STR("D3D11")))
        {
            Debug.GraphicsAPI = GUI::GfxBackend::DX11;
        }
        else if (String::iequal(graphics_api_string, STR("OpenGL")))
        {
            Debug.GraphicsAPI = GUI::GfxBackend::GLFW3_OpenGL3;
        }
        REGISTER_INT64_SETTING(Debug.LiveViewObjectsPerGroup, section_debug, LiveViewObjectsPerGroup);

        constexpr static File::CharType section_crash_dump[] = STR("CrashDump");
        REGISTER_BOOL_SETTING(CrashDump.EnableDumping, section_crash_dump, EnableDumping);
        REGISTER_BOOL_SETTING(CrashDump.FullMemoryDump, section_crash_dump, FullMemoryDump);

        constexpr static File::CharType section_threads[] = STR("Threads");
        REGISTER_INT64_SETTING(Threads.SigScannerNumThreads, section_threads, SigScannerNumThreads)
        REGISTER_INT64_SETTING(Threads.SigScannerMultithreadingModuleSizeThreshold, section_threads, SigScannerMultithreadingModuleSizeThreshold)

        constexpr static File::CharType section_memory[] = STR("Memory");
        REGISTER_INT64_SETTING(Memory.MaxMemoryUsageDuringAssetLoading, section_memory, MaxMemoryUsageDuringAssetLoading)

        constexpr static File::CharType section_hooks[] = STR("Hooks");
        REGISTER_BOOL_SETTING(Hooks.HookProcessInternal, section_hooks, HookProcessInternal)
        REGISTER_BOOL_SETTING(Hooks.HookProcessLocalScriptFunction, section_hooks, HookProcessLocalScriptFunction)
        REGISTER_BOOL_SETTING(Hooks.HookInitGameState, section_hooks, HookInitGameState)
        REGISTER_BOOL_SETTING(Hooks.HookCallFunctionByNameWithArguments, section_hooks, HookCallFunctionByNameWithArguments)
        REGISTER_BOOL_SETTING(Hooks.HookBeginPlay, section_hooks, HookBeginPlay)
        REGISTER_BOOL_SETTING(Hooks.HookLocalPlayerExec, section_hooks, HookLocalPlayerExec)
        REGISTER_INT64_SETTING(Hooks.FExecVTableOffsetInLocalPlayer, section_hooks, FExecVTableOffsetInLocalPlayer)

        constexpr static File::CharType section_experimental_features[] = STR("ExperimentalFeatures");
        REGISTER_BOOL_SETTING(Experimental.GUIUFunctionCaller, section_experimental_features, GUIUFunctionCaller)
    }
} // namespace RC
