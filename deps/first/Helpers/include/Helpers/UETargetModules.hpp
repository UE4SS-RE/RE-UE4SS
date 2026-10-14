#pragma once

#include <Helpers/Common.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#ifdef _WIN32
// Windows.h forward declarations
struct _SYSTEM_INFO;
typedef _SYSTEM_INFO SYSTEM_INFO;
struct _MODULEINFO;
typedef _MODULEINFO MODULEINFO;
#elifdef __linux__
// No platform declarations required here.
#else
#error "UETargetModules is not supported on this platform"
#endif

namespace RC
{
// MODULEINFO and SYSTEM_INFO are Windows API types and are not defined on Linux.
#ifdef _WIN32
    // Windows structs, to prevent the need to include Windows.h in this header
    struct RC_HELPERS_API WIN_MODULEINFO
    {
        void* lpBaseOfDll;
        unsigned long SizeOfImage;
        void* EntryPoint;

        auto operator=(MODULEINFO) -> WIN_MODULEINFO&;
    };

    using OS_MODULEINFO = WIN_MODULEINFO;
#elifdef __linux__
	// Provide a Linux equivalent.
    struct RC_HELPERS_API LINUX_MODULEINFO
    {
        void* lpBaseOfDll{};
        unsigned long SizeOfImage{};
		
		// Absolute start, size, and ELF p_flags for each readable PT_LOAD segment.
        std::vector<std::tuple<uint8_t*, size_t, int>> readable_segments{};
    };

    using OS_MODULEINFO = LINUX_MODULEINFO;
#else
#error "UETargetModules is not supported on this platform"
#endif

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

    RC_HELPERS_API auto ScanTargetToString(ScanTarget scan_target) -> std::string;
    RC_HELPERS_API auto ScanTargetToString(size_t scan_target) -> std::string;

    class ScanTargetArray
    {
    public:
        std::array<OS_MODULEINFO, static_cast<size_t>(ScanTarget::Max)> array{};

#ifdef _WIN32
        RC_HELPERS_API auto operator[](ScanTarget index) -> MODULEINFO&;
        RC_HELPERS_API auto operator[](ScanTarget index) const -> MODULEINFO&;
#elifdef __linux__
        RC_HELPERS_API auto operator[](ScanTarget index) -> LINUX_MODULEINFO&;
        RC_HELPERS_API auto operator[](ScanTarget index) const -> LINUX_MODULEINFO&;
#else
#error "UETargetModules is not supported on this platform"
#endif
    };
}
