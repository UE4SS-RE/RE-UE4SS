#include <format>
#include <future>
#include <regex>
#include <cmath>
#include <algorithm>

#include <cstring>

#include <Profiler/Profiler.hpp>
#include <SigScanner/SinglePassSigScanner.hpp>

namespace RC
{
    ScanTargetArray SigScannerStaticData::m_modules_info;
    bool SigScannerStaticData::m_is_modular;

    uint32_t SinglePassScanner::m_num_threads = 8;
    SinglePassScanner::ScanMethod SinglePassScanner::m_scan_method = ScanMethod::Scalar;
    uint32_t SinglePassScanner::m_multithreading_module_size_threshold = 0x1000000;
    std::mutex SinglePassScanner::m_scanner_mutex{};

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

        throw std::runtime_error{std::format("Invalid param for ScanTargetToString, param: {}", static_cast<int32_t>(scan_target))};
    }

    auto ScanTargetToString(size_t scan_target) -> std::string
    {
        return ScanTargetToString(static_cast<ScanTarget>(scan_target));
    }

    static auto ConvertHexCharToInt(char ch) -> int
    {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
        if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
        return -1;
    }

    auto SinglePassScanner::string_to_vector(std::string_view signature) -> std::vector<int>
    {
        std::vector<int> bytes;
        char* const start = const_cast<char*>(signature.data());
        char* const end = const_cast<char*>(signature.data()) + strlen(signature.data());

        for (char* current = start; current < end; current++)
        {
            if (*current == '?')
            {
                bytes.push_back(-1);
            }
            else if (std::isxdigit(*current))
            {
                bytes.push_back(ConvertHexCharToInt(*current));
            }
        }

        return bytes;
    }

    auto SinglePassScanner::string_to_vector(const std::vector<SignatureData>& signatures) -> std::vector<std::vector<int>>
    {
        std::vector<std::vector<int>> vector_of_signatures;
        vector_of_signatures.reserve(signatures.size());

        for (const auto& signature_data : signatures)
        {
            vector_of_signatures.emplace_back(string_to_vector(signature_data.signature));
        }

        return vector_of_signatures;
    }

    struct PatternData
    {
        std::vector<uint8_t> pattern{};
        std::vector<uint8_t> mask{};
        SignatureContainer* signature_container{};
    };

    static auto CharToByte(char symbol) -> uint8_t
    {
        if (symbol >= 'a' && symbol <= 'z')
        {
            return symbol - 'a' + 0xA;
        }
        else if (symbol >= 'A' && symbol <= 'Z')
        {
            return symbol - 'A' + 0xA;
        }
        else if (symbol >= '0' && symbol <= '9')
        {
            return symbol - '0';
        }
        else
        {
            return 0;
        }
    }

    static auto make_mask(std::string_view pattern, SignatureContainer& signature_container, const size_t data_size) -> PatternData
    {
        PatternData pattern_data{};

        if (pattern.length() < 1 || pattern[0] == '?')
        {
            throw std::runtime_error{std::format("[make_mask] A pattern cannot start with a wildcard.\nPattern: {}", pattern)};
        }

        for (size_t i = 0; i < pattern.length(); i++)
        {
            char symbol = pattern[i];
            char next_symbol = ((i + 1) < pattern.length()) ? pattern[i + 1] : 0;
            if (symbol == ' ')
            {
                continue;
            }

            if (symbol == '?')
            {
                pattern_data.pattern.push_back(0x00);
                pattern_data.mask.push_back(0x00);

                if (next_symbol == '?')
                {
                    ++i;
                }
                continue;
            }

            uint8_t byte = CharToByte(symbol) << 4 | CharToByte(next_symbol);

            pattern_data.pattern.push_back(byte);
            pattern_data.mask.push_back(0xff);

            ++i;
        }

        static constexpr size_t Alignment = 32;
        size_t count = (size_t)std::ceil((float)data_size / Alignment);
        size_t padding_size = count * Alignment - data_size;

        for (size_t i = 0; i < padding_size; i++)
        {
            pattern_data.pattern.push_back(0x00);
            pattern_data.mask.push_back(0x00);
        }

        pattern_data.signature_container = &signature_container;
        return pattern_data;
    }

    auto SinglePassScanner::scanner_work_thread(uint8_t* start_address, uint8_t* end_address, SystemInfo *info, std::vector<SignatureContainer>& signature_containers)
            -> void
    {
        ProfilerSetThreadName("UE4SS-ScannerWorkThread");
        ProfilerScope();

        switch (m_scan_method)
        {
        case ScanMethod::Scalar:
            scanner_work_thread_scalar(start_address, end_address, info, signature_containers);
            break;
        case ScanMethod::StdFind:
            scanner_work_thread_stdfind(start_address, end_address, info, signature_containers);
            break;
        }
    }

    static auto format_aob_string(std::string& str) -> void
    {
        if (str.size() < 4)
        {
            return;
        }
        if (str[3] != '/')
        {
            return;
        }

        std::erase_if(str, [&](const char c) {
            return c == ' ';
        });

        std::ranges::transform(str, str.begin(), [&str](const char c) {
            return c == '/' ? ' ' : c;
        });
    }

    auto SinglePassScanner::format_aob_strings(std::vector<SignatureContainer>& signature_containers) -> void
    {
        std::lock_guard<std::mutex> safe_scope(m_scanner_mutex);
        for (auto& signature_container : signature_containers)
        {
            for (auto& signature : signature_container.signatures)
            {
                format_aob_string(signature.signature);
            }
        }
    }

    auto SinglePassScanner::scanner_work_thread_stdfind(uint8_t* start_address,
                                                        uint8_t* end_address,
                                                        SystemInfo* info,
                                                        std::vector<SignatureContainer>& signature_containers) -> void
    {
        ProfilerScope();

        if (!start_address)
        {
            start_address = Platform::get_start_address(info);
        }
        if (!end_address)
        {
            end_address = Platform::get_end_address(info);
        }

        format_aob_strings(signature_containers);

        std::vector<std::vector<PatternData>> pattern_datas{};
        for (auto& signature_container : signature_containers)
        {
            auto& pattern_data = pattern_datas.emplace_back();
            for (auto& signature : signature_container.signatures)
            {
                pattern_data.emplace_back(make_mask(signature.signature, signature_container, end_address - start_address));
            }
        }

        // Loop everything
        for (size_t container_index = 0; const auto& patterns : pattern_datas)
        {
            for (size_t signature_index = 0; const auto& pattern_data : patterns)
            {
                // If the container is refusing more calls then skip to the next container
                if (pattern_data.signature_container->ignore)
                {
                    break;
                }

                auto it = start_address;
                auto end = it + (end_address - start_address) - (pattern_data.pattern.size()) + 1;
                uint8_t needle = pattern_data.pattern[0];

                bool skip_to_next_container{};
                while (end != (it = std::find(it, end, needle)))
                {
                    bool found = true;
                    for (size_t pattern_offset = 0; pattern_offset < pattern_data.pattern.size(); ++pattern_offset)
                    {
                        if ((it[pattern_offset] & pattern_data.mask[pattern_offset]) != pattern_data.pattern[pattern_offset])
                        {
                            found = false;
                            break;
                        }
                    }

                    if (found)
                    {
                        {
                            std::lock_guard<std::mutex> safe_scope(m_scanner_mutex);

                            // Checking for the second time if the container is refusing more calls
                            // This is required when multi-threading is enabled
                            if (pattern_data.signature_container->ignore)
                            {
                                skip_to_next_container = true;
                                break;
                            }

                            // One of the signatures have found a full match so lets forward the details to the callable
                            pattern_data.signature_container->index_into_signatures = signature_index;
                            pattern_data.signature_container->match_address = it;
                            pattern_data.signature_container->match_signature_size = pattern_data.pattern.size();

                            skip_to_next_container = pattern_data.signature_container->on_match_found(*pattern_data.signature_container);
                            pattern_data.signature_container->ignore = skip_to_next_container;

                            // Store results if the container at the containers request
                            if (pattern_data.signature_container->store_results)
                            {
                                pattern_data.signature_container->result_store.emplace_back(
                                        SignatureContainerLight{.index_into_signatures = signature_index, .match_address = it});
                            }
                        }
                    }

                    it++;
                }

                if (skip_to_next_container)
                {
                    // A match was found and signaled to skip to the next container
                    break;
                }

                ++signature_index;
            }
            ++container_index;
        }
    }

    auto SinglePassScanner::start_scan(SignatureContainerMap& signature_containers) -> void
    {
        SystemInfo* info = Platform::get_system_info();

        // If not modular then the containers get merged into one scan target
        // That way there are no extra scans
        // If modular then loop the containers and retrieve the scan target for each and pass everything to the do_scan() lambda
        fprintf(stderr, "signature_containers.size() = %d\n", signature_containers.size());
        if (!SigScannerStaticData::m_is_modular)
        {
            ModuleOS* merged_module_info{};
            std::vector<SignatureContainer> merged_containers;

            for (const auto& [scan_target, outer_container] : signature_containers)
            {
                merged_module_info = std::bit_cast<ModuleOS*>(&SigScannerStaticData::m_modules_info[scan_target]);
                fprintf(stderr, "outer_container len = %d\n", outer_container.size());
                for (const auto& signature_container : outer_container)
                {
                    merged_containers.emplace_back(signature_container);
                }
            }

            if (merged_containers.empty())
            {
                fprintf(stderr, "No containers to merge\n");
                return ;
                //throw std::runtime_error{"[SinglePassScanner::start_scan] Could not merge containers. Either there were not containers to merge or there was "
                //                         "an internal error."};
            }

            uint8_t* module_start_address = Platform::get_module_base(merged_module_info);

            if (Platform::get_module_size(merged_module_info) >= m_multithreading_module_size_threshold)
            {
                // Module is large enough to make it overall faster to scan with multiple threads
                std::vector<std::future<void>> scan_threads;

                // Higher values are better for debugging
                // You will get a bigger diminishing returns the faster your computer is (especially in release mode)
                uint32_t range = Platform::get_module_size(merged_module_info) / m_num_threads;

                // Calculating the ranges for each thread to scan and starting the scan
                uint32_t last_range{};
                for (uint32_t thread_id = 0; thread_id < m_num_threads; ++thread_id)
                {
                    scan_threads.emplace_back(std::async(std::launch::async,
                                                         &scanner_work_thread,
                                                         module_start_address + last_range,
                                                         module_start_address + last_range + range,
                                                         info,
                                                         std::ref(merged_containers)));

                    last_range += range;
                }

                for (const auto& scan_thread : scan_threads)
                {
                    scan_thread.wait();
                }
            }
            else
            {
                // Module is too small to make it overall faster to scan with multiple threads
                uint8_t* module_end_address = static_cast<uint8_t*>(module_start_address + Platform::get_module_size(merged_module_info));
                scanner_work_thread(module_start_address, module_end_address, info, merged_containers);
            }

            for (auto& container : merged_containers)
            {
                container.on_scan_finished(container);
            }
        }
        else
        {
            // This ranged for loop is performing a copy of unordered_map<ScanTarget, vector<SignatureContainer>>
            // Is this required ? Would it be worth trying to avoid copying here ?
            // Right now it can't be auto& or const auto& because the do_scan function takes a non-const since it needs to mutate the values inside the vector
            auto info = Platform::get_system_info();
            for (auto& [scan_target, signature_container] : signature_containers)
            {
                uint8_t* module_start_address = static_cast<uint8_t*>(Platform::get_module_base(&SigScannerStaticData::m_modules_info[scan_target]));
                uint8_t* module_end_address = static_cast<uint8_t*>(module_start_address + Platform::get_module_size(&SigScannerStaticData::m_modules_info[scan_target]));

                scanner_work_thread(module_start_address, module_end_address, info, signature_container);

                for (auto& container : signature_container)
                {
                    container.on_scan_finished(container);
                }
            }
        }
    }
}; // namespace RC

