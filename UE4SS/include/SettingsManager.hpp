#pragma once

#include <cstdint>
#include <filesystem>

#include <Common.hpp>
#include <File/File.hpp>

#ifdef HAS_GUI
#include <GUI/GUI.hpp>
#endif

namespace RC
{
    class RC_UE4SS_API SettingsManager
    {
      public:
        struct SectionOverrides
        {
            SystemStringType ModsFolderPath{};
        } Overrides;

        struct SectionGeneral
        {
            bool EnableHotReloadSystem{};
            bool UseCache{true};
            bool InvalidateCacheIfDLLDiffers{true};
            bool EnableDebugKeyBindings{false};
            int64_t SecondsToScanBeforeGivingUp{30};
            bool UseUObjectArrayCache{true};
            SystemStringType InputSource{SYSSTR("Default")};
        } General;

        struct SectionEngineVersionOverride
        {
            int64_t MajorVersion{-1};
            int64_t MinorVersion{-1};
        } EngineVersionOverride;

        struct SectionObjectDumper
        {
            bool LoadAllAssetsBeforeDumpingObjects{};
        } ObjectDumper;

        struct SectionCXXHeaderGenerator
        {
            bool DumpOffsetsAndSizes{};
            bool KeepMemoryLayout{};
            bool LoadAllAssetsBeforeGeneratingCXXHeaders{};
        } CXXHeaderGenerator;

        struct SectionUHTHeaderGenerator
        {
            bool IgnoreAllCoreEngineModules{};
            bool IgnoreEngineAndCoreUObject{true};
            bool MakeAllFunctionsBlueprintCallable{};
            bool MakeAllPropertyBlueprintsReadWrite{};
            bool MakeEnumClassesBlueprintType{};
            bool MakeAllConfigsEngineConfig{};
        } UHTHeaderGenerator;

        struct SectionDebug
        {
            bool SimpleConsoleEnabled{true};
            bool DebugConsoleEnabled{true};
            bool DebugConsoleVisible{true};
            float DebugGUIFontScaling{1.0};
#ifdef HAS_GUI
#ifdef WIN32
            GUI::GfxBackend GraphicsAPI{GUI::GfxBackend::GLFW3_OpenGL3};
#else
            GUI::GfxBackend GraphicsAPI{GUI::GfxBackend::TUI};
#endif
#endif
            int64_t LiveViewObjectsPerGroup{64 * 1024 / 2};
        } Debug;

        struct SectionCrashDump
        {
            bool EnableDumping{true};
            bool FullMemoryDump{false};
        } CrashDump;

        struct SectionThreads
        {
            int64_t SigScannerNumThreads{8};
            int64_t SigScannerMultithreadingModuleSizeThreshold{16777216};
        } Threads;

        struct SectionMemory
        {
            int64_t MaxMemoryUsageDuringAssetLoading{85};
        } Memory;

        struct SectionHooks
        {
            bool HookProcessInternal{true};
            bool HookProcessLocalScriptFunction{true};
            bool HookInitGameState{true};
            bool HookLoadMap{true};
            bool HookCallFunctionByNameWithArguments{true};
            bool HookBeginPlay{true};
            bool HookLocalPlayerExec{true};
            bool HookAActorTick{true};
            int64_t FExecVTableOffsetInLocalPlayer{0x28};
        } Hooks;

        struct ExperimentalFeatures
        {
            bool GUIUFunctionCaller{false};
        } Experimental;

#ifdef LINUX
        struct TUIFeatures
        {
            int ButtonLeft = 1;
            int ButtonRight = 3;
            int WheelUp = 4;
            int WheelDown = 5;
            bool TUIAsInputSource = true;
            bool TUINerdFont = true;
            /*
            SystemStringType TerminalCode = "f120";
            SystemStringType ArchiveCode = "f187";
            SystemStringType SyncCode = "f46a";
            SystemStringType FileCode = "f15b";
            SystemStringType EyeCode = "f06e";
            SystemStringType PuzzlePieceCode = "f12e";
            SystemStringType AngleLeftCode = "f100";
            SystemStringType AngleRightCode = "f101";
            SystemStringType BanCode = "f05e";
            SystemStringType CopyCode = "f0c5";
            SystemStringType SearchCode = "f002";*/
        } TUI;
#endif

      public:
        SettingsManager() = default;

      public:
        auto deserialize(std::filesystem::path& file_name) -> void;
    };
} // namespace RC
