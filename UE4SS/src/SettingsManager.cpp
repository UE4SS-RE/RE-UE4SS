#include <Helpers/String.hpp>
#include <IniParser/Ini.hpp>
#include <SettingsManager.hpp>

#define REGISTER_STRING_SETTING(member_var, section_name, key)                                                                                                 \
    try                                                                                                                                                        \
    {                                                                                                                                                          \
        (member_var) = parser.get_string(section_name, SYSSTR(#key));                                                                                          \
    }                                                                                                                                                          \
    catch (std::exception&)                                                                                                                                    \
    {                                                                                                                                                          \
    }

#define REGISTER_INT64_SETTING(member_var, section_name, key)                                                                                                  \
    try                                                                                                                                                        \
    {                                                                                                                                                          \
        (member_var) = parser.get_int64(section_name, SYSSTR(#key));                                                                                           \
    }                                                                                                                                                          \
    catch (std::exception&)                                                                                                                                    \
    {                                                                                                                                                          \
    }

#define REGISTER_BOOL_SETTING(member_var, section_name, key)                                                                                                   \
    try                                                                                                                                                        \
    {                                                                                                                                                          \
        (member_var) = parser.get_bool(section_name, SYSSTR(#key));                                                                                            \
    }                                                                                                                                                          \
    catch (std::exception&)                                                                                                                                    \
    {                                                                                                                                                          \
    }

#define REGISTER_FLOAT_SETTING(member_var, section_name, key)                                                                                                  \
    try                                                                                                                                                        \
    {                                                                                                                                                          \
        (member_var) = parser.get_float(section_name, SYSSTR(#key));                                                                                           \
    }                                                                                                                                                          \
    catch (std::exception&)                                                                                                                                    \
    {                                                                                                                                                          \
    }

namespace RC
{
    auto SettingsManager::deserialize(std::filesystem::path& file_name) -> void
    {
        auto file = File::open(file_name, File::OpenFor::Reading, File::OverwriteExistingFile::No, File::CreateIfNonExistent::Yes);
        Ini::Parser parser;
        parser.parse(file);
        file.close();
        constexpr static SystemCharType section_overrides[] = SYSSTR("Overrides");
        REGISTER_STRING_SETTING(Overrides.ModsFolderPath, section_overrides, ModsFolderPath)

        constexpr static SystemCharType section_general[] = SYSSTR("General");
        REGISTER_BOOL_SETTING(General.EnableHotReloadSystem, section_general, EnableHotReloadSystem)
        REGISTER_BOOL_SETTING(General.UseCache, section_general, UseCache)
        REGISTER_BOOL_SETTING(General.InvalidateCacheIfDLLDiffers, section_general, InvalidateCacheIfDLLDiffers)
        REGISTER_BOOL_SETTING(General.EnableDebugKeyBindings, section_general, EnableDebugKeyBindings)
        REGISTER_INT64_SETTING(General.SecondsToScanBeforeGivingUp, section_general, SecondsToScanBeforeGivingUp)
        REGISTER_BOOL_SETTING(General.UseUObjectArrayCache, section_general, bUseUObjectArrayCache)
        REGISTER_STRING_SETTING(General.InputSource, section_general, InputSource)

        constexpr static SystemCharType section_engine_version_override[] = SYSSTR("EngineVersionOverride");
        REGISTER_INT64_SETTING(EngineVersionOverride.MajorVersion, section_engine_version_override, MajorVersion)
        REGISTER_INT64_SETTING(EngineVersionOverride.MinorVersion, section_engine_version_override, MinorVersion)

        constexpr static SystemCharType section_object_dumper[] = SYSSTR("ObjectDumper");
        REGISTER_BOOL_SETTING(ObjectDumper.LoadAllAssetsBeforeDumpingObjects, section_object_dumper, LoadAllAssetsBeforeDumpingObjects)

        constexpr static SystemCharType section_cxx_header_generator[] = SYSSTR("CXXHeaderGenerator");
        REGISTER_BOOL_SETTING(CXXHeaderGenerator.DumpOffsetsAndSizes, section_cxx_header_generator, DumpOffsetsAndSizes)
        REGISTER_BOOL_SETTING(CXXHeaderGenerator.KeepMemoryLayout, section_cxx_header_generator, KeepMemoryLayout)
        REGISTER_BOOL_SETTING(CXXHeaderGenerator.LoadAllAssetsBeforeGeneratingCXXHeaders, section_cxx_header_generator, LoadAllAssetsBeforeGeneratingCXXHeaders)

        constexpr static SystemCharType section_uht_header_generator[] = SYSSTR("UHTHeaderGenerator");
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.IgnoreAllCoreEngineModules, section_uht_header_generator, IgnoreAllCoreEngineModules)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.IgnoreEngineAndCoreUObject, section_uht_header_generator, IgnoreEngineAndCoreUObject)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.MakeAllFunctionsBlueprintCallable, section_uht_header_generator, MakeAllFunctionsBlueprintCallable)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.MakeAllPropertyBlueprintsReadWrite, section_uht_header_generator, MakeAllPropertyBlueprintsReadWrite)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.MakeEnumClassesBlueprintType, section_uht_header_generator, MakeEnumClassesBlueprintType)
        REGISTER_BOOL_SETTING(UHTHeaderGenerator.MakeAllConfigsEngineConfig, section_uht_header_generator, MakeAllConfigsEngineConfig)

        constexpr static SystemCharType section_debug[] = SYSSTR("Debug");
        REGISTER_BOOL_SETTING(Debug.SimpleConsoleEnabled, section_debug, ConsoleEnabled)
        REGISTER_BOOL_SETTING(Debug.DebugConsoleEnabled, section_debug, GuiConsoleEnabled)
        REGISTER_BOOL_SETTING(Debug.DebugConsoleVisible, section_debug, GuiConsoleVisible)
        REGISTER_FLOAT_SETTING(Debug.DebugGUIFontScaling, section_debug, GuiConsoleFontScaling)

#ifdef HAS_GUI
        SystemStringType graphics_api_string{};
        REGISTER_STRING_SETTING(graphics_api_string, section_debug, GraphicsAPI)
#ifdef WIN32
        if (String::iequal(graphics_api_string, SYSSTR("DX11")) || String::iequal(graphics_api_string, SYSSTR("D3D11")))
        {
            Debug.GraphicsAPI = GUI::GfxBackend::DX11;
        }
        else if (String::iequal(graphics_api_string, SYSSTR("OpenGL")))
        {
            Debug.GraphicsAPI = GUI::GfxBackend::GLFW3_OpenGL3;
        }
#else
#ifdef HAS_GLFW
        Debug.GraphicsAPI = GUI::GfxBackend::GLFW3_OpenGL3;
#else
        Debug.GraphicsAPI = GUI::GfxBackend::TUI;
#endif
#endif
#endif
        REGISTER_INT64_SETTING(Debug.LiveViewObjectsPerGroup, section_debug, LiveViewObjectsPerGroup);

        /// constexpr static SystemCharType section_crash_dump[] = SSTR("CrashDump");
        /// REGISTER_BOOL_SETTING(CrashDump.EnableDumping, section_crash_dump, EnableDumping);
        /// REGISTER_BOOL_SETTING(CrashDump.FullMemoryDump, section_crash_dump, FullMemoryDump);

        constexpr static SystemCharType section_threads[] = SYSSTR("Threads");
        REGISTER_INT64_SETTING(Threads.SigScannerNumThreads, section_threads, SigScannerNumThreads)
        REGISTER_INT64_SETTING(Threads.SigScannerMultithreadingModuleSizeThreshold, section_threads, SigScannerMultithreadingModuleSizeThreshold)

        constexpr static SystemCharType section_memory[] = SYSSTR("Memory");
        REGISTER_INT64_SETTING(Memory.MaxMemoryUsageDuringAssetLoading, section_memory, MaxMemoryUsageDuringAssetLoading)

        constexpr static SystemCharType section_hooks[] = SYSSTR("Hooks");
        REGISTER_BOOL_SETTING(Hooks.HookProcessInternal, section_hooks, HookProcessInternal)
        REGISTER_BOOL_SETTING(Hooks.HookProcessLocalScriptFunction, section_hooks, HookProcessLocalScriptFunction)
        REGISTER_BOOL_SETTING(Hooks.HookLoadMap, section_hooks, HookLoadMap)
        REGISTER_BOOL_SETTING(Hooks.HookInitGameState, section_hooks, HookInitGameState)
        REGISTER_BOOL_SETTING(Hooks.HookCallFunctionByNameWithArguments, section_hooks, HookCallFunctionByNameWithArguments)
        REGISTER_BOOL_SETTING(Hooks.HookBeginPlay, section_hooks, HookBeginPlay)
        REGISTER_BOOL_SETTING(Hooks.HookLocalPlayerExec, section_hooks, HookLocalPlayerExec)
        REGISTER_BOOL_SETTING(Hooks.HookAActorTick, section_hooks, HookAActorTick)
        REGISTER_INT64_SETTING(Hooks.FExecVTableOffsetInLocalPlayer, section_hooks, FExecVTableOffsetInLocalPlayer)

        constexpr static SystemCharType section_experimental_features[] = SYSSTR("ExperimentalFeatures");
        REGISTER_BOOL_SETTING(Experimental.GUIUFunctionCaller, section_experimental_features, GUIUFunctionCaller)

#ifdef LINUX
        constexpr static SystemCharType section_tui_features[] = SYSSTR("TUI");
        REGISTER_INT64_SETTING(TUI.ButtonLeft, section_tui_features, ButtonLeft)
        REGISTER_INT64_SETTING(TUI.ButtonRight, section_tui_features, ButtonRight)
        REGISTER_INT64_SETTING(TUI.WheelUp, section_tui_features, WheelUp)
        REGISTER_INT64_SETTING(TUI.WheelDown, section_tui_features, WheelDown)
        REGISTER_BOOL_SETTING(TUI.TUIAsInputSource, section_tui_features, TUIAsInputSource)
        REGISTER_BOOL_SETTING(TUI.TUINerdFont, section_tui_features, TUINerdFont)
        /*
        REGISTER_STRING_SETTING(TUI.TerminalCode, section_tui_features, TerminalCode)
        REGISTER_STRING_SETTING(TUI.ArchiveCode, section_tui_features, ArchiveCode)
        REGISTER_STRING_SETTING(TUI.SyncCode, section_tui_features, SyncCode)
        REGISTER_STRING_SETTING(TUI.FileCode, section_tui_features, FileCode)
        REGISTER_STRING_SETTING(TUI.EyeCode, section_tui_features, EyeCode)
        REGISTER_STRING_SETTING(TUI.PuzzlePieceCode, section_tui_features, PuzzlePieceCode)
        REGISTER_STRING_SETTING(TUI.AngleLeftCode, section_tui_features, AngleLeftCode)
        REGISTER_STRING_SETTING(TUI.AngleRightCode, section_tui_features, AngleRightCode)
        REGISTER_STRING_SETTING(TUI.BanCode, section_tui_features, BanCode)
        REGISTER_STRING_SETTING(TUI.CopyCode, section_tui_features, CopyCode)
        REGISTER_STRING_SETTING(TUI.SearchCode, section_tui_features, SearchCode)
        */
#endif
    }
} // namespace RC
