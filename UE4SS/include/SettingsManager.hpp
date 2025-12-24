#pragma once

#include <cstdint>
#include <filesystem>

#include <Common.hpp>
#include <File/File.hpp>
#include <GUI/GUI.hpp>
#include <Input/KeyDef.hpp>
#include <Unreal/UnrealInitializer.hpp>

namespace RC
{
    // Method for executing callbacks in game thread
    enum class GameThreadExecutionMethod
    {
        ProcessEvent, // Use ProcessEvent hook
        EngineTick    // Use Engine Tick hook (once per frame)
    };

    class RC_UE4SS_API SettingsManager
    {
      public:
        struct SectionOverrides
        {
            File::StringType ModsFolderPath{};
            File::StringType ControllingModsTxt{};
        } Overrides;

        struct SectionGeneral
        {
            bool EnableHotReloadSystem{};
            Input::Key HotReloadKey{Input::Key::R};
            bool UseCache{true};
            bool InvalidateCacheIfDLLDiffers{true};
            bool EnableDebugKeyBindings{false};
            int64_t SecondsToScanBeforeGivingUp{30};
            bool UseUObjectArrayCache{true};
            StringType InputSource{STR("Default")};
            bool DoEarlyScan{false};
            bool SearchByAddress{false};
            GameThreadExecutionMethod DefaultExecuteInGameThreadMethod{GameThreadExecutionMethod::EngineTick};
        } General;

        struct SectionEngineVersionOverride
        {
            int64_t MajorVersion{-1};
            int64_t MinorVersion{-1};
            bool DebugBuild{false};
        } EngineVersionOverride;

        struct SectionObjectDumper
        {
            bool LoadAllAssetsBeforeDumpingObjects{};
            bool UseModuleOffsets{};
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
            GUI::GfxBackend GraphicsAPI{GUI::GfxBackend::GLFW3_OpenGL3};
            GUI::RenderMode RenderMode{GUI::RenderMode::ExternalThread};
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
            bool HookEndPlay{true};
            bool HookLocalPlayerExec{true};
            bool HookAActorTick{true};
            bool HookEngineTick{true};
            Unreal::UnrealInitializer::FunctionResolveMethod EngineTickResolveMethod{Unreal::UnrealInitializer::FunctionResolveMethod::Scan};
            bool HookGameViewportClientTick{true};
            bool HookUObjectProcessEvent{true};
            bool HookProcessConsoleExec{true};
            bool HookUStructLink{true};
            int64_t FExecVTableOffsetInLocalPlayer{0x28};
        } Hooks;

        struct ExperimentalFeatures
        {
        } Experimental;

      public:
        SettingsManager() = default;

      public:
        auto deserialize(std::filesystem::path& file_name) -> void;
    };
} // namespace RC
