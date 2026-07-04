#pragma once

#include <Helpers/Common.hpp>

#include <array>
#include <string>

// Windows.h forward declarations
struct _SYSTEM_INFO;
typedef _SYSTEM_INFO SYSTEM_INFO;
struct _MODULEINFO;
typedef _MODULEINFO MODULEINFO;

namespace RC
{
    // Windows structs, to prevent the need to include Windows.h in this header
    struct WIN_MODULEINFO
    {
        void* lpBaseOfDll;
        unsigned long SizeOfImage;
        void* EntryPoint;

        RC_HELPERS_API auto operator=(MODULEINFO) -> WIN_MODULEINFO&;
    };

    enum class ScanTarget
    {
        MainExe,

        AIModule,
        Analytics,
        AnalyticsET,
        AnimationCore,
        AnimGraphRuntime,
        AppFramework,
        ApplicationCore,
        AssetRegistry,
        AudioCaptureCore,
        AudioCaptureRtAudio,
        AudioExtensions,
        AudioMixer,
        AudioMixerCore,
        AudioMixerXAudio2,
        AudioPlatformConfiguration,
        AugmentedReality,
        AVEncoder,
        AVIWriter,
        BuildPatchServices,
        BuildSettings,
        Cbor,
        CEF3Utils,
        Chaos,
        ChaosCore,
        ChaosSolverEngine,
        ChaosSolvers,
        CinematicCamera,
        ClothingSystemRuntimeCommon,
        ClothingSystemRuntimeInterface,
        ClothingSystemRuntimeNv,
        Core,
        CoreUObject,
        CrunchCompression,
        D3D11RHI,
        D3D12RHI,
        Engine,
        EngineMessages,
        EngineSettings,
        EyeTracker,
        FieldSystemCore,
        FieldSystemEngine,
        FieldSystemSimulationCore,
        Foliage,
        GameplayMediaEncoder,
        GameplayTags,
        GameplayTasks,
        GeometryCollectionCore,
        GeometryCollectionEngine,
        GeometryCollectionSimulationCore,
        HeadMountedDisplay,
        HTTP,
        HttpNetworkReplayStreaming,
        HTTPServer,
        Icmp,
        ImageCore,
        ImageWrapper,
        ImageWriteQueue,
        InputCore,
        InputDevice,
        InstallBundleManager,
        InstancedSplines,
        InteractiveToolsFramework,
        Json,
        JsonUtilities,
        Landscape,
        LauncherCheck,
        LauncherPlatform,
        LevelSequence,
        LocalFileNetworkReplayStreaming,
        MaterialShaderQualitySettings,
        Media,
        MediaAssets,
        MediaUtils,
        MeshDescription,
        MeshUtilitiesCommon,
        Messaging,
        MessagingCommon,
        MoviePlayer,
        MovieScene,
        MovieSceneCapture,
        MovieSceneTracks,
        MRMesh,
        NavigationSystem,
        Navmesh,
        NetCore,
        Networking,
        NetworkReplayStreaming,
        NonRealtimeAudioRenderer,
        NullDrv,
        NullNetworkReplayStreaming,
        OpenGLDrv,
        Overlay,
        PacketHandler,
        PakFile,
        PerfCounters,
        PhysicsCore,
        PhysicsSQ,
        PhysXCooking,
        PreLoadScreen,
        Projects,
        PropertyPath,
        RawMesh,
        ReliabilityHandlerComponent,
        RenderCore,
        Renderer,
        RHI,
        RSA,
        SandboxFile,
        Serialization,
        SessionMessages,
        SessionServices,
        SignalProcessing,
        Slate,
        SlateCore,
        SlateNullRenderer,
        SlateRHIRenderer,
        Sockets,
        SoundFieldRendering,
        SSL,
        StaticMeshDescription,
        StreamingPauseRendering,
        SynthBenchmark,
        TimeManagement,
        TraceLog,
        UELibSampleRate,
        UMG,
        VectorVM,
        Voice,
        Voronoi,
        VulkanRHI,
        WebBrowser,
        WindowsPlatformFeatures,
        XAudio2,

        Max,
    };

    auto ScanTargetToString(ScanTarget scan_target) -> std::string;
    auto ScanTargetToString(size_t scan_target) -> std::string;

    class ScanTargetArray
    {
    public:
        std::array<WIN_MODULEINFO, static_cast<size_t>(ScanTarget::Max)> array{};

        auto operator[](ScanTarget index) -> MODULEINFO&;
        auto operator[](ScanTarget index) const -> MODULEINFO&;
    };
}
