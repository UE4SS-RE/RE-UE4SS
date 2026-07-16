#include <Helpers/UETargetModules.hpp>

#include <stdexcept>

#include <fmt/format.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#else
#error "UETargetModules is only supported on Windows"
#endif

namespace RC
{
    auto WIN_MODULEINFO::operator=(MODULEINFO other) -> WIN_MODULEINFO&
    {
        lpBaseOfDll = other.lpBaseOfDll;
        SizeOfImage = other.SizeOfImage;
        EntryPoint = other.EntryPoint;
        return *this;
    }

    auto ScanTargetArray::operator[](ScanTarget index) -> MODULEINFO&
    {
        return *std::bit_cast<MODULEINFO*>(&array[static_cast<size_t>(index)]);
    }

    auto ScanTargetArray::operator[](ScanTarget index) const -> MODULEINFO&
    {
        return *std::bit_cast<MODULEINFO*>(&array[static_cast<size_t>(index)]);
    }

    auto ScanTargetToString(ScanTarget scan_target) -> std::string
    {
        switch (scan_target)
        {
        case ScanTarget::MainExe:
            return {"MainExe"};
        case ScanTarget::AIModule:
            return {"AIModule"};
        case ScanTarget::Analytics:
            return {"Analytics"};
        case ScanTarget::AnalyticsET:
            return {"AnalyticsET"};
        case ScanTarget::AnimationCore:
            return {"AnimationCore"};
        case ScanTarget::AnimGraphRuntime:
            return {"AnimGraphRuntime"};
        case ScanTarget::AppFramework:
            return {"AppFramework"};
        case ScanTarget::ApplicationCore:
            return {"ApplicationCore"};
        case ScanTarget::AssetRegistry:
            return {"AssetRegistry"};
        case ScanTarget::AudioCaptureCore:
            return {"AudioCaptureCore"};
        case ScanTarget::AudioCaptureRtAudio:
            return {"AudioCaptureRtAudio"};
        case ScanTarget::AudioExtensions:
            return {"AudioExtensions"};
        case ScanTarget::AudioMixer:
            return {"AudioMixer"};
        case ScanTarget::AudioMixerCore:
            return {"AudioMixerCore"};
        case ScanTarget::AudioMixerXAudio2:
            return {"AudioMixerXAudio2"};
        case ScanTarget::AudioPlatformConfiguration:
            return {"AudioPlatformConfiguration"};
        case ScanTarget::AugmentedReality:
            return {"AugmentedReality"};
        case ScanTarget::AVEncoder:
            return {"AVEncoder"};
        case ScanTarget::AVIWriter:
            return {"AVIWriter"};
        case ScanTarget::BuildPatchServices:
            return {"BuildPatchServices"};
        case ScanTarget::BuildSettings:
            return {"BuildSettings"};
        case ScanTarget::Cbor:
            return {"Cbor"};
        case ScanTarget::CEF3Utils:
            return {"CEF3Utils"};
        case ScanTarget::Chaos:
            return {"Chaos"};
        case ScanTarget::ChaosCore:
            return {"ChaosCore"};
        case ScanTarget::ChaosSolverEngine:
            return {"ChaosSolverEngine"};
        case ScanTarget::ChaosSolvers:
            return {"ChaosSolvers"};
        case ScanTarget::CinematicCamera:
            return {"CinematicCamera"};
        case ScanTarget::ClothingSystemRuntimeCommon:
            return {"ClothingSystemRuntimeCommon"};
        case ScanTarget::ClothingSystemRuntimeInterface:
            return {"ClothingSystemRuntimeInterface"};
        case ScanTarget::ClothingSystemRuntimeNv:
            return {"ClothingSystemRuntimeNv"};
        case ScanTarget::Core:
            return {"Core"};
        case ScanTarget::CoreUObject:
            return {"CoreUObject"};
        case ScanTarget::CrunchCompression:
            return {"CrunchCompression"};
        case ScanTarget::D3D11RHI:
            return {"D3D11RHI"};
        case ScanTarget::D3D12RHI:
            return {"D3D12RHI"};
        case ScanTarget::Engine:
            return {"Engine"};
        case ScanTarget::EngineMessages:
            return {"EngineMessages"};
        case ScanTarget::EngineSettings:
            return {"EngineSettings"};
        case ScanTarget::EyeTracker:
            return {"EyeTracker"};
        case ScanTarget::FieldSystemCore:
            return {"FieldSystemCore"};
        case ScanTarget::FieldSystemEngine:
            return {"FieldSystemEngine"};
        case ScanTarget::FieldSystemSimulationCore:
            return {"FieldSystemSimulationCore"};
        case ScanTarget::Foliage:
            return {"Foliage"};
        case ScanTarget::GameplayMediaEncoder:
            return {"GameplayMediaEncoder"};
        case ScanTarget::GameplayTags:
            return {"GameplayTags"};
        case ScanTarget::GameplayTasks:
            return {"GameplayTasks"};
        case ScanTarget::GeometryCollectionCore:
            return {"GeometryCollectionCore"};
        case ScanTarget::GeometryCollectionEngine:
            return {"GeometryCollectionEngine"};
        case ScanTarget::GeometryCollectionSimulationCore:
            return {"GeometryCollectionSimulationCore"};
        case ScanTarget::HeadMountedDisplay:
            return {"HeadMountedDisplay"};
        case ScanTarget::HTTP:
            return {"HTTP"};
        case ScanTarget::HttpNetworkReplayStreaming:
            return {"HttpNetworkReplayStreaming"};
        case ScanTarget::HTTPServer:
            return {"HTTPServer"};
        case ScanTarget::Icmp:
            return {"Icmp"};
        case ScanTarget::ImageCore:
            return {"ImageCore"};
        case ScanTarget::ImageWrapper:
            return {"ImageWrapper"};
        case ScanTarget::ImageWriteQueue:
            return {"ImageWriteQueue"};
        case ScanTarget::InputCore:
            return {"InputCore"};
        case ScanTarget::InputDevice:
            return {"InputDevice"};
        case ScanTarget::InstallBundleManager:
            return {"InstallBundleManager"};
        case ScanTarget::InstancedSplines:
            return {"InstancedSplines"};
        case ScanTarget::InteractiveToolsFramework:
            return {"InteractiveToolsFramework"};
        case ScanTarget::Json:
            return {"Json"};
        case ScanTarget::JsonUtilities:
            return {"JsonUtilities"};
        case ScanTarget::Landscape:
            return {"Landscape"};
        case ScanTarget::LauncherCheck:
            return {"LauncherCheck"};
        case ScanTarget::LauncherPlatform:
            return {"LauncherPlatform"};
        case ScanTarget::LevelSequence:
            return {"LevelSequence"};
        case ScanTarget::LocalFileNetworkReplayStreaming:
            return {"LocalFileNetworkReplayStreaming"};
        case ScanTarget::MaterialShaderQualitySettings:
            return {"MaterialShaderQualitySettings"};
        case ScanTarget::Media:
            return {"Media"};
        case ScanTarget::MediaAssets:
            return {"MediaAssets"};
        case ScanTarget::MediaUtils:
            return {"MediaUtils"};
        case ScanTarget::MeshDescription:
            return {"MeshDescription"};
        case ScanTarget::MeshUtilitiesCommon:
            return {"MeshUtilitiesCommon"};
        case ScanTarget::Messaging:
            return {"Messaging"};
        case ScanTarget::MessagingCommon:
            return {"MessagingCommon"};
        case ScanTarget::MoviePlayer:
            return {"MoviePlayer"};
        case ScanTarget::MovieScene:
            return {"MovieScene"};
        case ScanTarget::MovieSceneCapture:
            return {"MovieSceneCapture"};
        case ScanTarget::MovieSceneTracks:
            return {"MovieSceneTracks"};
        case ScanTarget::MRMesh:
            return {"MRMesh"};
        case ScanTarget::NavigationSystem:
            return {"NavigationSystem"};
        case ScanTarget::Navmesh:
            return {"Navmesh"};
        case ScanTarget::NetCore:
            return {"NetCore"};
        case ScanTarget::Networking:
            return {"Networking"};
        case ScanTarget::NetworkReplayStreaming:
            return {"NetworkReplayStreaming"};
        case ScanTarget::NonRealtimeAudioRenderer:
            return {"NonRealtimeAudioRenderer"};
        case ScanTarget::NullDrv:
            return {"NullDrv"};
        case ScanTarget::NullNetworkReplayStreaming:
            return {"NullNetworkReplayStreaming"};
        case ScanTarget::OpenGLDrv:
            return {"OpenGLDrv"};
        case ScanTarget::Overlay:
            return {"Overlay"};
        case ScanTarget::PacketHandler:
            return {"PacketHandler"};
        case ScanTarget::PakFile:
            return {"PakFile"};
        case ScanTarget::PerfCounters:
            return {"PerfCounters"};
        case ScanTarget::PhysicsCore:
            return {"PhysicsCore"};
        case ScanTarget::PhysicsSQ:
            return {"PhysicsSQ"};
        case ScanTarget::PhysXCooking:
            return {"PhysXCooking"};
        case ScanTarget::PreLoadScreen:
            return {"PreLoadScreen"};
        case ScanTarget::Projects:
            return {"Projects"};
        case ScanTarget::PropertyPath:
            return {"PropertyPath"};
        case ScanTarget::RawMesh:
            return {"RawMesh"};
        case ScanTarget::ReliabilityHandlerComponent:
            return {"ReliabilityHandlerComponent"};
        case ScanTarget::RenderCore:
            return {"RenderCore"};
        case ScanTarget::Renderer:
            return {"Renderer"};
        case ScanTarget::RHI:
            return {"RHI"};
        case ScanTarget::RSA:
            return {"RSA"};
        case ScanTarget::SandboxFile:
            return {"SandboxFile"};
        case ScanTarget::Serialization:
            return {"Serialization"};
        case ScanTarget::SessionMessages:
            return {"SessionMessages"};
        case ScanTarget::SessionServices:
            return {"SessionServices"};
        case ScanTarget::SignalProcessing:
            return {"SignalProcessing"};
        case ScanTarget::Slate:
            return {"Slate"};
        case ScanTarget::SlateCore:
            return {"SlateCore"};
        case ScanTarget::SlateNullRenderer:
            return {"SlateNullRenderer"};
        case ScanTarget::SlateRHIRenderer:
            return {"SlateRHIRenderer"};
        case ScanTarget::Sockets:
            return {"Sockets"};
        case ScanTarget::SoundFieldRendering:
            return {"SoundFieldRendering"};
        case ScanTarget::SSL:
            return {"SSL"};
        case ScanTarget::StaticMeshDescription:
            return {"StaticMeshDescription"};
        case ScanTarget::StreamingPauseRendering:
            return {"StreamingPauseRendering"};
        case ScanTarget::SynthBenchmark:
            return {"SynthBenchmark"};
        case ScanTarget::TimeManagement:
            return {"TimeManagement"};
        case ScanTarget::TraceLog:
            return {"TraceLog"};
        case ScanTarget::UELibSampleRate:
            return {"UELibSampleRate"};
        case ScanTarget::UMG:
            return {"UMG"};
        case ScanTarget::VectorVM:
            return {"VectorVM"};
        case ScanTarget::Voice:
            return {"Voice"};
        case ScanTarget::Voronoi:
            return {"Voronoi"};
        case ScanTarget::VulkanRHI:
            return {"VulkanRHI"};
        case ScanTarget::WebBrowser:
            return {"WebBrowser"};
        case ScanTarget::WindowsPlatformFeatures:
            return {"WindowsPlatformFeatures"};
        case ScanTarget::XAudio2:
            return {"XAudio2"};
        case ScanTarget::Max:
            return {"Max"};
        }

        throw std::runtime_error{fmt::format("Invalid param for ScanTargetToString, param: {}", static_cast<int32_t>(scan_target))};
    }

    auto ScanTargetToString(size_t scan_target) -> std::string
    {
        return ScanTargetToString(static_cast<ScanTarget>(scan_target));
    }
}
