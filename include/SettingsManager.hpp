#ifndef UE4SS_SETTINGSMANAGER_HPP
#define UE4SS_SETTINGSMANAGER_HPP

#include <cstdint>
#include <filesystem>

#include <File/File.hpp>
#include <GUI/GUI.hpp>

namespace RC
{
    class SettingsManager
    {
    public:
        struct SectionOverrides
        {
            File::StringType ModsFolderPath{};
        } Overrides;

        struct SectionGeneral
        {
            bool EnableHotReloadSystem{};
            bool InvalidateCacheIfDLLDiffers{true};
            bool EnableDebugKeyBindings{false};
            int64_t MaxScanAttemptsNormal{20};
            int64_t MaxScanAttemptsModular{2000};
            bool UseUObjectArrayCache{true};
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
            GUI::GfxBackend GraphicsAPI{GUI::GfxBackend::GLFW3_OpenGL3};
            int64_t LiveViewObjectsPerGroup{64 * 1024 / 2};
        } Debug;

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
            bool HookCallFunctionByNameWithArguments{true};
            bool HookBeginPlay{true};
            bool HookLocalPlayerExec{true};
            int64_t FExecVTableOffsetInLocalPlayer{0x28};
        } Hooks;

    public:
        SettingsManager() = default;

    public:
        auto deserialize(std::filesystem::path& file_name) -> void;
    };
}

#endif //UE4SS_SETTINGSMANAGER_HPP
