#pragma once

#include <cstdint>
#include <filesystem>

#include <Common.hpp>
#include <Constructs/Annotated.hpp>
#include <array>
#include <iostream>
#include <File/File.hpp>
#include <glaze/glaze.hpp>
#include <glaze/core/macros.hpp>
#include <GUI/GUI.hpp>

namespace RC::Settings
{
  
    
    struct SectionOverrides
    {
        File::StringType ModsFolderPath{};
        
        struct glaze
        {
            using T = SectionOverrides;
            [[maybe_unused]] static constexpr std::string_view name = "SectionOverrides";
            static constexpr auto value = glz::object("ModsFolderPath",
                                                      &T::ModsFolderPath, " Path to the 'Mods' folder. Default = [<dll_directory>/Mods] ");
        };
    };

    struct SectionGeneral
    {
        Annotated<Strings<"Allows mods to be hot-reloaded at runtime. Default: false">, bool> EnableHotReloadSystem{false};
        Annotated<Strings<"Whether the cache system for AOBs will be used.", "Default: true">, bool> UseCache{true};
        Annotated<Strings<"Whether caches will be invalidated if ue4ss.dll has changed.", "Default: true">, bool> InvalidateCacheIfDLLDiffers{true};
        Annotated<Strings<"Whether to enable keybinds for dumpers.", "Default: false">, bool> EnableDebugKeyBindings{false};
        Annotated<Strings<"The number of seconds the scanner will scan before giving up.", "Default: 30">, int64_t> SecondsToScanBeforeGivingUp{30};
        Annotated<Strings<"Whether to create UObject listeners in GUObjectArray to create a fast cache for use instead of iterating GUObjectArray.", "Setting this to false can help if you're experiencing a crash on startup.", "Default: true">, bool> UseUObjectArrayCache{true};

        struct glaze
        {
            using T = SectionGeneral;
            [[maybe_unused]] static constexpr std::string_view name = "SectionGeneral";
            static constexpr auto value = glz::object("EnableHotReloadSystem",
                                                       &T::EnableHotReloadSystem,
                                                       "UseCache",
                                                       &T::UseCache,
                                                       "InvalidateCacheIfDLLDiffers",
                                                       &T::InvalidateCacheIfDLLDiffers,
                                                       "EnableDebugKeyBindings",
                                                       &T::EnableDebugKeyBindings,
                                                       "SecondsToScanForFunctionsBeforeGivingUp",
                                                       &T::SecondsToScanBeforeGivingUp, " The number of seconds the scanner will scan for before giving up. Default = 30 ",
                                                       "UseUObjectArrayCache",
                                                       &T::UseUObjectArrayCache, " Whether to create UObject listeners in GUObjectArray to create a fast cache for use instead of iterating GUObjectArray. Default = true "
                                                       );
        };
    };

    struct SectionEngineVersionOverride
    {
        int64_t MajorVersion{-1};
        int64_t MinorVersion{-1};

        GLZ_LOCAL_META(SectionEngineVersionOverride,
        MajorVersion,
        MinorVersion
        );
    };

    struct SectionObjectDumper
    {
        bool LoadAllAssetsBeforeDumpingObjects{};

        GLZ_LOCAL_META(SectionObjectDumper,
        LoadAllAssetsBeforeDumpingObjects
        );
    };

    struct SectionCXXHeaderGenerator
    {
        bool DumpOffsetsAndSizes{};
        bool KeepMemoryLayout{};
        bool LoadAllAssetsBeforeGeneratingCXXHeaders{};

        GLZ_LOCAL_META(SectionCXXHeaderGenerator,
        DumpOffsetsAndSizes,
        KeepMemoryLayout,
        LoadAllAssetsBeforeGeneratingCXXHeaders
        );
    };

    struct SectionUHTHeaderGenerator
    {
        bool IgnoreAllCoreEngineModules{};
        bool IgnoreEngineAndCoreUObject{true};
        bool MakeAllFunctionsBlueprintCallable{};
        bool MakeAllPropertyBlueprintsReadWrite{};
        bool MakeEnumClassesBlueprintType{};
        bool MakeAllConfigsEngineConfig{};

        GLZ_LOCAL_META(SectionUHTHeaderGenerator,
        IgnoreAllCoreEngineModules,
        IgnoreEngineAndCoreUObject,
        MakeAllFunctionsBlueprintCallable,
        MakeAllPropertyBlueprintsReadWrite,
        MakeEnumClassesBlueprintType,
        MakeAllConfigsEngineConfig
        );
    };

    struct SectionDebug
    {
        bool SimpleConsoleEnabled{true};
        bool DebugConsoleEnabled{true};
        bool DebugConsoleVisible{true};
        float DebugGUIFontScaling{1.0};
        GUI::GfxBackend GraphicsAPI{ GUI::GfxBackend::GLFW3_OpenGL3 };
        int64_t LiveViewObjectsPerGroup{64 * 1024 / 2};

        GLZ_LOCAL_META(SectionDebug,
        SimpleConsoleEnabled,
        DebugConsoleEnabled,
        DebugConsoleVisible,
        DebugGUIFontScaling,
        GraphicsAPI,
        LiveViewObjectsPerGroup
        );
    };

    struct SectionCrashDump
    {
        bool EnableDumping{true};
        bool FullMemoryDump{false};

        GLZ_LOCAL_META(SectionCrashDump,
        EnableDumping,
        FullMemoryDump
        );
    };

    struct SectionThreads
    {
        int64_t SigScannerNumThreads{8};
        int64_t SigScannerMultithreadingModuleSizeThreshold{16777216};
        
        GLZ_LOCAL_META(SectionThreads,
        SigScannerNumThreads,
        SigScannerMultithreadingModuleSizeThreshold
        );
    };

    struct SectionMemory
    {
        int64_t MaxMemoryUsageDuringAssetLoading{85};

        GLZ_LOCAL_META(SectionMemory,
        MaxMemoryUsageDuringAssetLoading
        );
    };

    struct SectionHooks
    {
        bool HookProcessInternal{true};
        bool HookProcessLocalScriptFunction{true};
        bool HookInitGameState{true};
        bool HookCallFunctionByNameWithArguments{true};
        bool HookBeginPlay{true};
        bool HookLocalPlayerExec{true};
        int64_t FExecVTableOffsetInLocalPlayer{0x28};

        GLZ_LOCAL_META(SectionHooks,
        HookProcessInternal,
        HookProcessLocalScriptFunction,
        HookInitGameState,
        HookCallFunctionByNameWithArguments,
        HookBeginPlay,
        HookLocalPlayerExec,
        FExecVTableOffsetInLocalPlayer
        );
    };

    struct ExperimentalFeatures
    {
        bool GUIUFunctionCaller{false};

        GLZ_LOCAL_META(ExperimentalFeatures,
        GUIUFunctionCaller
        );
    };
    
    class RC_UE4SS_API SettingsManager
    {
    public:

            SectionOverrides Overrides;
            SectionGeneral General;
            SectionEngineVersionOverride EngineVersionOverride;
            SectionObjectDumper ObjectDumper;
            SectionCXXHeaderGenerator CXXHeaderGenerator;
            SectionUHTHeaderGenerator UHTHeaderGenerator;
            SectionDebug Debug;
            SectionCrashDump CrashDump;
            SectionThreads Threads;
            SectionMemory Memory;
            SectionHooks Hooks;
            ExperimentalFeatures Experimental;

            GLZ_LOCAL_META(RC::Settings::SettingsManager,
            Overrides,
            General,
            EngineVersionOverride,
            ObjectDumper,
            CXXHeaderGenerator,
            UHTHeaderGenerator,
            Debug,
            CrashDump,
            Threads,
            Memory,
            Hooks,
            Experimental
            );

        

    public:
        SettingsManager() = default;

    public:
        auto deserialize(std::filesystem::path& file_name) -> void;

        auto read_settings() -> void;
    };
    
}


// Template to serialize Gfx renderer enum to string in JSON
template <>
struct glz::meta<GUI::GfxBackend> {
using enum GUI::GfxBackend;
static constexpr auto value = glz::enumerate("DX11", DX11,
                                             "OpenGL3", GLFW3_OpenGL3
                                             );
};