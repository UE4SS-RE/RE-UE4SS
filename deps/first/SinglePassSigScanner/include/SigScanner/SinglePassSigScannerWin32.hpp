#pragma once

#include <array>
#include <functional>
#include <mutex>
#include <vector>

#include <SigScanner/Common.hpp>

#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
#define LO_NIBBLE(b) ((b)&0x0F)

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
        RC_SPSS_API auto operator=(MODULEINFO) -> WIN_MODULEINFO&;
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

    auto RC_SPSS_API ScanTargetToString(ScanTarget scan_target) -> std::string;
    auto RC_SPSS_API ScanTargetToString(size_t scan_target) -> std::string;

#ifdef WIN32
    class RC_SPSS_API ScanTargetArray
    {
      public:
        std::array<WIN_MODULEINFO, static_cast<size_t>(ScanTarget::Max)> array{};

        auto operator[](ScanTarget index) -> MODULEINFO&;
    };
#else
    class RC_SPSS_API ScanTargetArray
    {
      public:
        std::array<DLData, static_cast<size_t>(ScanTarget::Max)> array{};

        auto operator[](ScanTarget index) -> DLData&;
    };
#endif
    // Static storage to be used across all sig scanner types
    // At the moment the only scanner type that exists is SinglePassScanner
    // In the future there might be a multi-threaded version of SinglePassScanner
    class RC_SPSS_API SigScannerStaticData
    {
      public:
        // Store all of the MODULEINFO structs for all of the scan targets
        // Can a vector of something non-windows be stored here and then a static MODULEINFO can be created in the cpp file ?
        static ScanTargetArray m_modules_info;
        static bool m_is_modular;
    };

    struct RC_SPSS_API SignatureContainerLight
    {
        // The scanner will set this to whichever signature a match was found for
        size_t index_into_signatures{};

        // The scanner will set this to the address of the current match (if a match was found)
        // This is guaranteed to be non-null when 'callable' is called
        uint8_t* match_address{};
    };

    struct RC_SPSS_API SignatureData
    {
        std::string signature{};

        // TODO: Add 'ScanTarget' in here and remove 'ScanTarget' from the 'start_scan' function

        // The user-code can cast the custom data to their own enum type before using
        // It will be zero-defaulted in the event of no custom data being supplied
        int32_t custom_data{};

        // A mask that's used for the StdFind scanning method.
        std::string mask{};
    };

    class SignatureContainer
    {
      private:
        // Member variables in this class shouldn't be mutable outside of the scanner internals
        // They cannot simply be private because this class isn't part of the scanner
        // They cannot be const because they need to be mutated by the scanner
        // The solution is to make everything private and give the scanner access by using the 'friend' keyword
        friend class SinglePassScanner;

      private:
        std::vector<SignatureData> signatures;
        const std::function<bool(SignatureContainer&)> on_match_found;
        const std::function<void(SignatureContainer&)> on_scan_finished;

        // Whether to store the results and pass them to on_scan_completed
        const bool store_results{};

        // The scanner will store results in here if 'store_results' is true
        std::vector<SignatureContainerLight> result_store{};

        // True if the scan was successful, otherwise false
        bool did_succeed{false};

        // The scanner will use this to cancel all future calls to 'callable' if the 'callable' signaled
        bool ignore{};

        // The scanner will set this to whichever signature a match was found for
        size_t index_into_signatures{};

        // The scanner will set this to the address of the current match (if a match was found)
        // This is guaranteed to be non-null when 'callable' is called
        uint8_t* match_address{};

        // The scanner will set this to the size of the signature that was matched
        size_t match_signature_size{};

      public:
        template <typename OnMatchFound, typename OnScanFinished>
        SignatureContainer(std::vector<SignatureData> sig_param, OnMatchFound on_match_found_param, OnScanFinished on_scan_finished_param)
            : signatures(std::move(sig_param)), on_match_found(on_match_found_param), on_scan_finished(on_scan_finished_param)
        {
        }

        template <typename OnMatchFound, typename OnScanFinished>
        SignatureContainer(std::vector<SignatureData> sig_param, OnMatchFound on_match_found_param, OnScanFinished on_scan_finished_param, bool store_results_param)
            : signatures(std::move(sig_param)), on_match_found(on_match_found_param), on_scan_finished(on_scan_finished_param), store_results(store_results_param)
        {
        }

      public:
        [[nodiscard]] auto get_match_address() const -> uint8_t*
        {
            return match_address;
        }
        [[nodiscard]] auto get_index_into_signatures() const -> size_t
        {
            return index_into_signatures;
        }
        [[nodiscard]] auto get_did_succeed() -> bool&
        {
            return did_succeed;
        }
        [[nodiscard]] auto get_did_succeed() const -> bool
        {
            return did_succeed;
        }
        [[nodiscard]] auto get_signatures() const -> const std::vector<SignatureData>&
        {
            return signatures;
        }
        [[nodiscard]] auto get_result_store() const -> const std::vector<SignatureContainerLight>&
        {
            return result_store;
        }
        [[nodiscard]] auto get_match_signature_size() const -> size_t
        {
            return match_signature_size;
        }
    };

    class SinglePassScanner
    {
      private:
        static std::mutex m_scanner_mutex;

      public:
        enum class ScanMethod
        {
            Scalar,
            StdFind,
        };

      public:
        RC_SPSS_API static uint32_t m_num_threads;
        RC_SPSS_API static ScanMethod m_scan_method;

        // The minimum size a module has to be for multi-threading to be enabled
        // Smaller modules might increase the cost of scanning due to the cost of creating threads
        RC_SPSS_API static uint32_t m_multithreading_module_size_threshold;

      private:
        RC_SPSS_API auto static string_to_vector(std::string_view signature) -> std::vector<int>;
        RC_SPSS_API auto static string_to_vector(const std::vector<SignatureData>& signatures) -> std::vector<std::vector<int>>;
        RC_SPSS_API auto static format_aob_strings(std::vector<SignatureContainer>& signature_containers) -> void;

      public:
        RC_SPSS_API auto static scanner_work_thread(uint8_t* start_address,
                                                    uint8_t* end_address,
                                                    SYSTEM_INFO& info,
                                                    std::vector<SignatureContainer>& signature_containers) -> void;
        RC_SPSS_API auto static scanner_work_thread_scalar(uint8_t* start_address,
                                                           uint8_t* end_address,
                                                           SYSTEM_INFO& info,
                                                           std::vector<SignatureContainer>& signature_containers) -> void;
        RC_SPSS_API auto static scanner_work_thread_stdfind(uint8_t* start_address,
                                                            uint8_t* end_address,
                                                            SYSTEM_INFO& info,
                                                            std::vector<SignatureContainer>& signature_containers) -> void;

        using SignatureContainerMap = std::unordered_map<ScanTarget, std::vector<SignatureContainer>>;
        RC_SPSS_API auto static start_scan(SignatureContainerMap& signature_containers) -> void;
#ifdef WIN32
        RC_SPSS_API auto static string_scan(std::wstring_view string_to_scan_for, ScanTarget = ScanTarget::MainExe) -> void*;
#endif
    };
} // namespace RC
