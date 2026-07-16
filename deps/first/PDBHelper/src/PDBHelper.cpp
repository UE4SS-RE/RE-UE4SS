#include <PDBHelper/PDBHelper.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#else
#error "PDBHelper is only supported on Windows"
#endif

#include <PDB.h>
#include <PDB_TPIStream.h>
#include <PDB_InfoStream.h>

namespace RC::PDB
{
    SymbolsHolder::SymbolsHolder() = default;

    SymbolsHolder::SymbolsHolder(const std::filesystem::path& pdb_file, const ScanTargetArray* modules_array)
        : m_pdb_file_handle(File::open(pdb_file)), m_pdb_memory_map(m_pdb_file_handle.memory_map()), m_modules_array(modules_array)
    {
        initialize();
    }

    auto SymbolsHolder::find_symbol_rva(const StringType& symbol_name) -> uint32_t
    {
        if (!is_valid())
        {
            initialize();
        }

        const auto symbol_name_as_ascii = to_string(symbol_name);
        const ::PDB::ArrayView<::PDB::HashRecord> hash_records = m_public_symbol_stream.GetRecords();
        for (const ::PDB::HashRecord& hash_record : hash_records)
        {
            const auto record = m_public_symbol_stream.GetRecord(m_symbol_record_stream, hash_record);
            if (record->header.kind != ::PDB::CodeView::DBI::SymbolRecordKind::S_PUB32)
            {
                continue;
            }

            const auto rva = m_image_section_stream.ConvertSectionOffsetToRVA(record->data.S_PUB32.section, record->data.S_PUB32.offset);
            if (rva == 0u)
            {
                continue;
            }

            const auto record_name = std::string_view{record->data.S_PUB32.name};
            if (record_name == symbol_name_as_ascii)
            {
                return rva;
            }
        }
        return 0;
    }

    auto SymbolsHolder::IsError(::PDB::ErrorCode error_code) -> bool
    {
        switch (error_code)
        {
        case ::PDB::ErrorCode::Success:
            return false;

        case ::PDB::ErrorCode::InvalidSuperBlock:
            Output::send(STR("Error: SymbolsHolder: Invalid Superblock\n"));
            return true;

        case ::PDB::ErrorCode::InvalidFreeBlockMap:
            Output::send(STR("Error: SymbolsHolder: Invalid free block map\n"));
            return true;

        case ::PDB::ErrorCode::InvalidStream:
            Output::send(STR("Error: SymbolsHolder: Invalid stream\n"));
            return true;

        case ::PDB::ErrorCode::InvalidSignature:
            Output::send(STR("Error: SymbolsHolder: Invalid stream signature\n"));
            return true;

        case ::PDB::ErrorCode::InvalidStreamIndex:
            Output::send(STR("Error: SymbolsHolder: Invalid stream index\n"));
            return true;

        case ::PDB::ErrorCode::UnknownVersion:
            Output::send(STR("Error: SymbolsHolder: Unknown version\n"));
            return true;
        }

        return true;
    }

    auto SymbolsHolder::HasValidDBIStreams() const -> bool
    {
        if (IsError(m_dbi_stream.HasValidPublicSymbolStream(*m_pdb_file_raw)))
        {
            return false;
        }

        if (IsError(m_dbi_stream.HasValidGlobalSymbolStream(*m_pdb_file_raw)))
        {
            return false;
        }

        if (IsError(m_dbi_stream.HasValidSectionContributionStream(*m_pdb_file_raw)))
        {
            return false;
        }

        if (IsError(m_dbi_stream.HasValidImageSectionStream(*m_pdb_file_raw)))
        {
            return false;
        }

        return true;
    }

    auto SymbolsHolder::initialize() -> void
    {
        m_pdb_file_raw = std::make_unique<::PDB::RawFile>(::PDB::CreateRawFile(m_pdb_memory_map.data()));

        if (IsError(::PDB::ValidateFile(m_pdb_memory_map.data())))
        {
            Output::send(STR("Error: SymbolsHolder: Unable to open PDB file!\n"));
            return;
        }

        m_dbi_stream = ::PDB::CreateDBIStream(*m_pdb_file_raw);
        if (!HasValidDBIStreams())
        {
            Output::send(STR("Error: SymbolsHolder: Invalid DBI stream!\n"));
            return;
        }

        if (IsError(::PDB::HasValidTPIStream(*m_pdb_file_raw)))
        {
            Output::send(STR("Error: SymbolsHolder: Invalid TPI stream!\n"));
            return;
        }
        m_ipi_stream = ::PDB::CreateIPIStream(*m_pdb_file_raw);

        m_image_section_stream = m_dbi_stream.CreateImageSectionStream(*m_pdb_file_raw);
        m_symbol_record_stream = m_dbi_stream.CreateSymbolRecordStream(*m_pdb_file_raw);
        m_public_symbol_stream = m_dbi_stream.CreatePublicSymbolStream(*m_pdb_file_raw);

        m_is_valid = true;
    }

    auto SymbolsHolder::get_address_of_symbol(const StringType& symbol_name, ScanTarget scan_target) -> uintptr_t
    {
        if (!is_valid())
        {
            return reinterpret_cast<uintptr_t>(nullptr);
        }
        if (const auto rva = find_symbol_rva(symbol_name); rva)
        {
            const auto module_base = reinterpret_cast<uintptr_t>((*m_modules_array)[scan_target].lpBaseOfDll);
            return module_base + rva;
        }
        return reinterpret_cast<uintptr_t>(nullptr);
    }

    auto SymbolsHolder::for_each_symbol(const CallbackType& callback) -> void
    {
        if (!is_valid())
        {
            initialize();
        }

        const ::PDB::ArrayView<::PDB::HashRecord> hash_records = m_public_symbol_stream.GetRecords();
        for (const ::PDB::HashRecord& hash_record : hash_records)
        {
            const auto record = m_public_symbol_stream.GetRecord(m_symbol_record_stream, hash_record);
            if (record->header.kind != ::PDB::CodeView::DBI::SymbolRecordKind::S_PUB32)
            {
                continue;
            }

            const auto rva = m_image_section_stream.ConvertSectionOffsetToRVA(record->data.S_PUB32.section, record->data.S_PUB32.offset);
            if (rva == 0u)
            {
                continue;
            }


            const auto record_name = std::string_view{record->data.S_PUB32.name};
#if 1
            if (callback(record_name, rva) == LoopAction::Break)
            {
                return;
            }
#else
            if (record_name.contains("GUObjectArray"))
            {
                // TODO: GUObjectArray not found in binfold PDB, maybe we need to look for a reference ?
                // 5.3 confirmed, check other engine versions: ?GUObjectArray@@3VFUObjectArray@@A
                // 5.7 confirmed: same as 5.3
                Output::send(STR("GUObjectArray: '{}'\n"), ensure_str(record_name));
            }
            if (record_name.contains("FName") && record_name.contains("ToString"))
            {
                // 5.1 confirmed
                // 5.3 confirmed, check other engine versions: ?ToString@FName@@QEBAXAEAVFString@@@Z
                // 5.6 confirmed: same as 5.3
                // 5.7 confirmed: same as 5.3
                Output::send(STR("FName::ToString: '{}'\n"), ensure_str(record_name));
            }
            if (record_name.contains("FName") && record_name.contains("EFindName"))
            {
                // 5.3 confirmed, check other engine versions: ??0FName@@QEAA@PEB_WW4EFindName@@@Z
                Output::send(STR("FName::FName: '{}'\n"), ensure_str(record_name));
            }
            if (record_name.contains("GMalloc"))
            {
                // TODO: GMalloc not found in binfold PDB, maybe we need to look for FMemory::Free.
                // 5.3 confirmed: ?GMalloc@@3PEAVFMalloc@@EA
                // 5.6 confirmed: ?GMalloc@Private@UE@@3PEAVFMalloc@@EA
                // 5.7 confirmed: same as 5.6
                // check earlier engine versions
                Output::send(STR("GMalloc: '{}'\n"), ensure_str(record_name));
            }
            if (record_name.contains("Tick") && (record_name.contains("UEngine") || record_name.contains("UGameEngine")))
            {
                // TODO: Not found using binfold.
                // 5.6 confirmed: ?Tick@UGameEngine@@UEAAXM_N@Z
                // 5.7 confirmed: same as 5.6
                // check earlier engine versions
                Output::send(STR("GameEngineTick: '{}'\n"), ensure_str(record_name));
            }
#endif
        }
    }

    SymbolsHelper::SymbolsHelper(std::filesystem::path exe_path, const ScanTargetArray* modules_array, bool is_modular)
        : m_exe_path(std::move(exe_path)), m_modules_array(modules_array), m_is_modular(is_modular)
    {
    }

    auto SymbolsHelper::get_address_of_symbol(const StringType& symbol_name, ScanTarget scan_target) -> uintptr_t
    {
        const auto symbol_holder = get_symbol_holder(scan_target);
        if (!symbol_holder)
        {
            return reinterpret_cast<uintptr_t>(nullptr);
        }
        return symbol_holder->get_address_of_symbol(symbol_name, scan_target);
    }

    auto SymbolsHelper::find_all_symbols_containing_substr(const std::vector<std::string_view>& substrs) -> std::vector<std::pair<std::string, std::string>>
    {
        // Symbol name, and module name.
        std::vector<std::pair<std::string, std::string>> result{};
        if (m_is_modular)
        {
            // Iterate all modules.
            for (std::underlying_type_t<ScanTarget> scan_target = 0; scan_target < std::to_underlying(ScanTarget::Max); ++scan_target)
            {
                const auto symbol_holder = get_symbol_holder(static_cast<ScanTarget>(scan_target));
                if (!symbol_holder)
                {
                    continue;
                }
                symbol_holder->for_each_symbol([&](std::string_view symbol_name, [[maybe_unused]] uint32_t rva) {
                    //Output::send(STR("[{}]: {}\n"), ensure_str(ScanTargetToString(scan_target)), ensure_str(symbol_name));
                    bool all_substrings_found = true;
                    for (const auto& substr : substrs)
                    {
                        if (!symbol_name.contains(substr))
                        {
                            all_substrings_found = false;
                            break;
                        }
                    }
                    if (all_substrings_found)
                    {
                        result.emplace_back(symbol_name, ScanTargetToString(scan_target));
                    }
                    return LoopAction::Continue;
                });
            }
        }
        else
        {
            const auto symbol_holder = get_symbol_holder(ScanTarget::MainExe);
            if (!symbol_holder)
            {
                return {};
            }
            symbol_holder->for_each_symbol([&](std::string_view symbol_name, [[maybe_unused]] uint32_t rva) {
                bool all_substrings_found = true;
                for (const auto& substr : substrs)
                {
                    if (!symbol_name.contains(substr))
                    {
                        all_substrings_found = false;
                        break;
                    }
                }
                if (all_substrings_found)
                {
                    result.emplace_back(symbol_name, ScanTargetToString(ScanTarget::MainExe));
                }
                return LoopAction::Continue;
            });
            // Only main exe is needed.
        }
        return result;
    }

    auto SymbolsHelper::add_to_batch(std::string symbol_name, ScanTarget scan_target, const CallbackType& callback) -> void
    {
        if (!m_is_modular)
        {
            scan_target = ScanTarget::MainExe;
        }
        const auto it = m_callbacks.emplace(scan_target, std::vector<std::tuple<std::string, CallbackType, bool>>{{}});
        it.first->second.emplace_back(std::move(symbol_name), callback, false);
    }

    auto SymbolsHelper::start_batch_lookup() -> void
    {
        for (auto& [scan_target, callback_data] : m_callbacks)
        {
            const auto symbol_holder = get_symbol_holder(scan_target);
            if (!symbol_holder)
            {
                continue;
            }
            uint32_t matches{};
            symbol_holder->for_each_symbol([&](std::string_view symbol_name, uint32_t rva) {
                for (auto& [wanted_symbol_name, callback, already_found] : callback_data)
                {
                    if (!already_found && wanted_symbol_name == symbol_name)
                    {
                        const auto module_base = reinterpret_cast<uintptr_t>((*m_modules_array)[scan_target].lpBaseOfDll);
                        callback(module_base + rva);
                        already_found = true;
                        ++matches;
                    }
                }
                if (matches >= callback_data.size())
                {
                    return LoopAction::Break;
                }
                else
                {
                    return LoopAction::Continue;
                }
            });
        }
    }

    auto SymbolsHelper::get_symbol_holder(ScanTarget scan_target) -> SymbolsHolder*
    {
        if (!m_is_modular)
        {
            scan_target = ScanTarget::MainExe;
        }
        auto symbol = &m_symbol_holders[static_cast<size_t>(scan_target)];
        if (!symbol->is_valid())
        {
            // TODO: Figure out the PDB location.
            //       We're already selecting the correct PDB module name (i.e. Core, CoreUObject, Engine, etc), but we  still need to two things:
            //       1. Select the path, trying both Game/Game/Win64, and Game/Engine/Win64 directories.
            //       2. Select the game name part of the pdb file name, i.e. FactoryGameEGS.
            //       For modular builds, PDB names are made up of the following: <GameExeWithoutExtension>-<ModuleName>-<Plat>-<BuildMode>.pdb
            //       For example: FactoryGameEGS-CoreUObject-Win64-Shipping.pdb
            //       For non-modular builds, there's only one PDB, and it's <GameExeWithoutExtension>-<Plat>-<BuildMode>.pdb
            //       For example: FactoryGameEGS-Win64-Shipping.pdb
            //       We're currently hard-coding to a modular Satisfactory game, so always FactoryGameEGS-<ModuleName>-Win64-Shipping.pdb.
            //       Update: All done.
            // TODO: Add an ini option for setting the pdb location.
            //       I like keeping modular PDBs (i.e. Satisfactory) out of the exe directory so that debugging tools (i.e. x64dbg/VS)
            //       don't pick up on them automatically because these tools load PDBs extremely slowly and I never need them all loaded.
            //       Update: Still need to add an ini option for the pdb location.
            //               To keep this simple, just add it on the Unreal end first, then merge it into main, update the submodule in UE4SS, and
            //               then add the ini option, make the PR, and wait for approval and merge.
            std::filesystem::path pdb_file{m_exe_path};
            if (m_is_modular)
            {
                // For modular builds, PDB names are made up of the following: <GameExeWithoutExtension>-<ModuleName>-<Plat>-<BuildMode>.pdb
                // For example: FactoryGameEGS-CoreUObject-Win64-Shipping.pdb
                pdb_file.remove_filename();
                // Temporary until ini option for pdb location has been implemented.
                if (ensure_str(m_exe_path).contains(STR("Factory")))
                {
                    pdb_file /= "bk";
                }
                pdb_file /= get_pdb_file_name_for_target(scan_target);
            }
            else
            {
                // For non-modular builds, there's only one PDB, and it's <GameExeWithoutExtension>-<Plat>-<BuildMode>.pdb
                // For example: Palworld-Win64-Shipping.pdb
                pdb_file.replace_extension(".pdb");
            }
            if (!std::filesystem::exists(pdb_file))
            {
                StringType error_message{STR("PDB file not found, cannot lookup symbol address\n")};
                error_message.append(fmt::format(STR("Looked at: '{}'"), ensure_str(pdb_file)));
                Output::send<LogLevel::Warning>(STR("Warning: PDBLookupHelper::get_address_of_symbol: {}\n"), error_message);
                return nullptr;
            }
            *symbol = SymbolsHolder(pdb_file, m_modules_array);
        }
        return symbol;
    }

    auto SymbolsHelper::get_pdb_file_name_for_target(ScanTarget scan_target) -> std::filesystem::path
    {
        switch (scan_target)
        {
        case ScanTarget::MainExe:
            return "FactoryGameEGS-Win64-Shipping.pdb";
        case ScanTarget::AIModule:
            return "FactoryGameEGS-AIModule-Win64-Shipping.pdb";
        case ScanTarget::Analytics:
            return "FactoryGameEGS-Analytics-Win64-Shipping.pdb";
        case ScanTarget::AnalyticsET:
            return "FactoryGameEGS-AnalyticsET-Win64-Shipping.pdb";
        case ScanTarget::AnimationCore:
            return "FactoryGameEGS-AnimationCore-Win64-Shipping.pdb";
        case ScanTarget::AnimGraphRuntime:
            return "FactoryGameEGS-AnimGraphRuntime-Win64-Shipping.pdb";
        case ScanTarget::AppFramework:
            return "FactoryGameEGS-AppFramework-Win64-Shipping.pdb";
        case ScanTarget::ApplicationCore:
            return "FactoryGameEGS-ApplicationCore-Win64-Shipping.pdb";
        case ScanTarget::AssetRegistry:
            return "FactoryGameEGS-AssetRegistry-Win64-Shipping.pdb";
        case ScanTarget::AudioCaptureCore:
            return "FactoryGameEGS-AudioCaptureCore-Win64-Shipping.pdb";
        case ScanTarget::AudioCaptureRtAudio:
            return "FactoryGameEGS-AudioCaptureRtAudio-Win64-Shipping.pdb";
        case ScanTarget::AudioExtensions:
            return "FactoryGameEGS-AudioExtensions-Win64-Shipping.pdb";
        case ScanTarget::AudioMixer:
            return "FactoryGameEGS-AudioMixer-Win64-Shipping.pdb";
        case ScanTarget::AudioMixerCore:
            return "FactoryGameEGS-AudioMixerCore-Win64-Shipping.pdb";
        case ScanTarget::AudioMixerXAudio2:
            return "FactoryGameEGS-AudioMixerXAudio2-Win64-Shipping.pdb";
        case ScanTarget::AudioPlatformConfiguration:
            return "FactoryGameEGS-AudioPlatformConfiguration-Win64-Shipping.pdb";
        case ScanTarget::AugmentedReality:
            return "FactoryGameEGS-AugmentedReality-Win64-Shipping.pdb";
        case ScanTarget::AVEncoder:
            return "FactoryGameEGS-AVEncoder-Win64-Shipping.pdb";
        case ScanTarget::AVIWriter:
            return "FactoryGameEGS-AVIWriter-Win64-Shipping.pdb";
        case ScanTarget::BuildPatchServices:
            return "FactoryGameEGS-BuildPatchServices-Win64-Shipping.pdb";
        case ScanTarget::BuildSettings:
            return "FactoryGameEGS-BuildSettings-Win64-Shipping.pdb";
        case ScanTarget::Cbor:
            return "FactoryGameEGS-Cbor-Win64-Shipping.pdb";
        case ScanTarget::CEF3Utils:
            return "FactoryGameEGS-CEF3Utils-Win64-Shipping.pdb";
        case ScanTarget::Chaos:
            return "FactoryGameEGS-Chaos-Win64-Shipping.pdb";
        case ScanTarget::ChaosCore:
            return "FactoryGameEGS-ChaosCore-Win64-Shipping.pdb";
        case ScanTarget::ChaosSolverEngine:
            return "FactoryGameEGS-ChaosSolverEngine-Win64-Shipping.pdb";
        case ScanTarget::ChaosSolvers:
            return "FactoryGameEGS-ChaosSolvers-Win64-Shipping.pdb";
        case ScanTarget::CinematicCamera:
            return "FactoryGameEGS-CinematicCamera-Win64-Shipping.pdb";
        case ScanTarget::ClothingSystemRuntimeCommon:
            return "FactoryGameEGS-ClothingSystemRuntimeCommon-Win64-Shipping.pdb";
        case ScanTarget::ClothingSystemRuntimeInterface:
            return "FactoryGameEGS-ClothingSystemRuntimeInterface-Win64-Shipping.pdb";
        case ScanTarget::ClothingSystemRuntimeNv:
            return "FactoryGameEGS-ClothingSystemRuntimeNv-Win64-Shipping.pdb";
        case ScanTarget::Core:
            return "FactoryGameEGS-Core-Win64-Shipping.pdb";
        case ScanTarget::CoreUObject:
            return "FactoryGameEGS-CoreUObject-Win64-Shipping.pdb";
        case ScanTarget::CrunchCompression:
            return "FactoryGameEGS-CrunchCompression-Win64-Shipping.pdb";
        case ScanTarget::D3D11RHI:
            return "FactoryGameEGS-D3D11RHI-Win64-Shipping.pdb";
        case ScanTarget::D3D12RHI:
            return "FactoryGameEGS-D3D12RHI-Win64-Shipping.pdb";
        case ScanTarget::Engine:
            return "FactoryGameEGS-Engine-Win64-Shipping.pdb";
        case ScanTarget::EngineMessages:
            return "FactoryGameEGS-EngineMessages-Win64-Shipping.pdb";
        case ScanTarget::EngineSettings:
            return "FactoryGameEGS-EngineSettings-Win64-Shipping.pdb";
        case ScanTarget::EyeTracker:
            return "FactoryGameEGS-EyeTracker-Win64-Shipping.pdb";
        case ScanTarget::FieldSystemCore:
            return "FactoryGameEGS-FieldSystemCore-Win64-Shipping.pdb";
        case ScanTarget::FieldSystemEngine:
            return "FactoryGameEGS-FieldSystemEngine-Win64-Shipping.pdb";
        case ScanTarget::FieldSystemSimulationCore:
            return "FactoryGameEGS-FieldSystemSimulationCore-Win64-Shipping.pdb";
        case ScanTarget::Foliage:
            return "FactoryGameEGS-Foliage-Win64-Shipping.pdb";
        case ScanTarget::GameplayMediaEncoder:
            return "FactoryGameEGS-GameplayMediaEncoder-Win64-Shipping.pdb";
        case ScanTarget::GameplayTags:
            return "FactoryGameEGS-GameplayTags-Win64-Shipping.pdb";
        case ScanTarget::GameplayTasks:
            return "FactoryGameEGS-GameplayTasks-Win64-Shipping.pdb";
        case ScanTarget::GeometryCollectionCore:
            return "FactoryGameEGS-GeometryCollectionCore-Win64-Shipping.pdb";
        case ScanTarget::GeometryCollectionEngine:
            return "FactoryGameEGS-GeometryCollectionEngine-Win64-Shipping.pdb";
        case ScanTarget::GeometryCollectionSimulationCore:
            return "FactoryGameEGS-GeometryCollectionSimulationCore-Win64-Shipping.pdb";
        case ScanTarget::HeadMountedDisplay:
            return "FactoryGameEGS-HeadMountedDisplay-Win64-Shipping.pdb";
        case ScanTarget::HTTP:
            return "FactoryGameEGS-HTTP-Win64-Shipping.pdb";
        case ScanTarget::HttpNetworkReplayStreaming:
            return "FactoryGameEGS-HttpNetworkReplayStreaming-Win64-Shipping.pdb";
        case ScanTarget::HTTPServer:
            return "FactoryGameEGS-HTTPServer-Win64-Shipping.pdb";
        case ScanTarget::Icmp:
            return "FactoryGameEGS-Icmp-Win64-Shipping.pdb";
        case ScanTarget::ImageCore:
            return "FactoryGameEGS-ImageCore-Win64-Shipping.pdb";
        case ScanTarget::ImageWrapper:
            return "FactoryGameEGS-ImageWrapper-Win64-Shipping.pdb";
        case ScanTarget::ImageWriteQueue:
            return "FactoryGameEGS-ImageWriteQueue-Win64-Shipping.pdb";
        case ScanTarget::InputCore:
            return "FactoryGameEGS-InputCore-Win64-Shipping.pdb";
        case ScanTarget::InputDevice:
            return "FactoryGameEGS-InputDevice-Win64-Shipping.pdb";
        case ScanTarget::InstallBundleManager:
            return "FactoryGameEGS-InstallBundleManager-Win64-Shipping.pdb";
        case ScanTarget::InstancedSplines:
            return "FactoryGameEGS-InstancedSplines-Win64-Shipping.pdb";
        case ScanTarget::InteractiveToolsFramework:
            return "FactoryGameEGS-InteractiveToolsFramework-Win64-Shipping.pdb";
        case ScanTarget::Json:
            return "FactoryGameEGS-Json-Win64-Shipping.pdb";
        case ScanTarget::JsonUtilities:
            return "FactoryGameEGS-JsonUtilities-Win64-Shipping.pdb";
        case ScanTarget::Landscape:
            return "FactoryGameEGS-Landscape-Win64-Shipping.pdb";
        case ScanTarget::LauncherCheck:
            return "FactoryGameEGS-LauncherCheck-Win64-Shipping.pdb";
        case ScanTarget::LauncherPlatform:
            return "FactoryGameEGS-LauncherPlatform-Win64-Shipping.pdb";
        case ScanTarget::LevelSequence:
            return "FactoryGameEGS-LevelSequence-Win64-Shipping.pdb";
        case ScanTarget::LocalFileNetworkReplayStreaming:
            return "FactoryGameEGS-LocalFileNetworkReplayStreaming-Win64-Shipping.pdb";
        case ScanTarget::MaterialShaderQualitySettings:
            return "FactoryGameEGS-MaterialShaderQualitySettings-Win64-Shipping.pdb";
        case ScanTarget::Media:
            return "FactoryGameEGS-Media-Win64-Shipping.pdb";
        case ScanTarget::MediaAssets:
            return "FactoryGameEGS-MediaAssets-Win64-Shipping.pdb";
        case ScanTarget::MediaUtils:
            return "FactoryGameEGS-MediaUtils-Win64-Shipping.pdb";
        case ScanTarget::MeshDescription:
            return "FactoryGameEGS-MeshDescription-Win64-Shipping.pdb";
        case ScanTarget::MeshUtilitiesCommon:
            return "FactoryGameEGS-MeshUtilitiesCommon-Win64-Shipping.pdb";
        case ScanTarget::Messaging:
            return "FactoryGameEGS-Messaging-Win64-Shipping.pdb";
        case ScanTarget::MessagingCommon:
            return "FactoryGameEGS-MessagingCommon-Win64-Shipping.pdb";
        case ScanTarget::MoviePlayer:
            return "FactoryGameEGS-MoviePlayer-Win64-Shipping.pdb";
        case ScanTarget::MovieScene:
            return "FactoryGameEGS-MovieScene-Win64-Shipping.pdb";
        case ScanTarget::MovieSceneCapture:
            return "FactoryGameEGS-MovieSceneCapture-Win64-Shipping.pdb";
        case ScanTarget::MovieSceneTracks:
            return "FactoryGameEGS-MovieSceneTracks-Win64-Shipping.pdb";
        case ScanTarget::MRMesh:
            return "FactoryGameEGS-MRMesh-Win64-Shipping.pdb";
        case ScanTarget::NavigationSystem:
            return "FactoryGameEGS-NavigationSystem-Win64-Shipping.pdb";
        case ScanTarget::Navmesh:
            return "FactoryGameEGS-Navmesh-Win64-Shipping.pdb";
        case ScanTarget::NetCore:
            return "FactoryGameEGS-NetCore-Win64-Shipping.pdb";
        case ScanTarget::Networking:
            return "FactoryGameEGS-Networking-Win64-Shipping.pdb";
        case ScanTarget::NetworkReplayStreaming:
            return "FactoryGameEGS-NetworkReplayStreaming-Win64-Shipping.pdb";
        case ScanTarget::NonRealtimeAudioRenderer:
            return "FactoryGameEGS-NonRealtimeAudioRenderer-Win64-Shipping.pdb";
        case ScanTarget::NullDrv:
            return "FactoryGameEGS-NullDrv-Win64-Shipping.pdb";
        case ScanTarget::NullNetworkReplayStreaming:
            return "FactoryGameEGS-NullNetworkReplayStreaming-Win64-Shipping.pdb";
        case ScanTarget::OpenGLDrv:
            return "FactoryGameEGS-OpenGLDrv-Win64-Shipping.pdb";
        case ScanTarget::Overlay:
            return "FactoryGameEGS-Overlay-Win64-Shipping.pdb";
        case ScanTarget::PacketHandler:
            return "FactoryGameEGS-PacketHandler-Win64-Shipping.pdb";
        case ScanTarget::PakFile:
            return "FactoryGameEGS-PakFile-Win64-Shipping.pdb";
        case ScanTarget::PerfCounters:
            return "FactoryGameEGS-PerfCounters-Win64-Shipping.pdb";
        case ScanTarget::PhysicsCore:
            return "FactoryGameEGS-PhysicsCore-Win64-Shipping.pdb";
        case ScanTarget::PhysicsSQ:
            return "FactoryGameEGS-PhysicsSQ-Win64-Shipping.pdb";
        case ScanTarget::PhysXCooking:
            return "FactoryGameEGS-PhysXCooking-Win64-Shipping.pdb";
        case ScanTarget::PreLoadScreen:
            return "FactoryGameEGS-PreLoadScreen-Win64-Shipping.pdb";
        case ScanTarget::Projects:
            return "FactoryGameEGS-Projects-Win64-Shipping.pdb";
        case ScanTarget::PropertyPath:
            return "FactoryGameEGS-PropertyPath-Win64-Shipping.pdb";
        case ScanTarget::RawMesh:
            return "FactoryGameEGS-RawMesh-Win64-Shipping.pdb";
        case ScanTarget::ReliabilityHandlerComponent:
            return "FactoryGameEGS-ReliabilityHandlerComponent-Win64-Shipping.pdb";
        case ScanTarget::RenderCore:
            return "FactoryGameEGS-RenderCore-Win64-Shipping.pdb";
        case ScanTarget::Renderer:
            return "FactoryGameEGS-Renderer-Win64-Shipping.pdb";
        case ScanTarget::RHI:
            return "FactoryGameEGS-RHI-Win64-Shipping.pdb";
        case ScanTarget::RSA:
            return "FactoryGameEGS-RSA-Win64-Shipping.pdb";
        case ScanTarget::SandboxFile:
            return "FactoryGameEGS-SandboxFile-Win64-Shipping.pdb";
        case ScanTarget::Serialization:
            return "FactoryGameEGS-Serialization-Win64-Shipping.pdb";
        case ScanTarget::SessionMessages:
            return "FactoryGameEGS-SessionMessages-Win64-Shipping.pdb";
        case ScanTarget::SessionServices:
            return "FactoryGameEGS-SessionServices-Win64-Shipping.pdb";
        case ScanTarget::SignalProcessing:
            return "FactoryGameEGS-SignalProcessing-Win64-Shipping.pdb";
        case ScanTarget::Slate:
            return "FactoryGameEGS-Slate-Win64-Shipping.pdb";
        case ScanTarget::SlateCore:
            return "FactoryGameEGS-SlateCore-Win64-Shipping.pdb";
        case ScanTarget::SlateNullRenderer:
            return "FactoryGameEGS-SlateNullRenderer-Win64-Shipping.pdb";
        case ScanTarget::SlateRHIRenderer:
            return "FactoryGameEGS-SlateRHIRenderer-Win64-Shipping.pdb";
        case ScanTarget::Sockets:
            return "FactoryGameEGS-Sockets-Win64-Shipping.pdb";
        case ScanTarget::SoundFieldRendering:
            return "FactoryGameEGS-SoundFieldRendering-Win64-Shipping.pdb";
        case ScanTarget::SSL:
            return "FactoryGameEGS-SSL-Win64-Shipping.pdb";
        case ScanTarget::StaticMeshDescription:
            return "FactoryGameEGS-StaticMeshDescription-Win64-Shipping.pdb";
        case ScanTarget::StreamingPauseRendering:
            return "FactoryGameEGS-StreamingPauseRendering-Win64-Shipping.pdb";
        case ScanTarget::SynthBenchmark:
            return "FactoryGameEGS-SynthBenchmark-Win64-Shipping.pdb";
        case ScanTarget::TimeManagement:
            return "FactoryGameEGS-TimeManagement-Win64-Shipping.pdb";
        case ScanTarget::TraceLog:
            return "FactoryGameEGS-TraceLog-Win64-Shipping.pdb";
        case ScanTarget::UELibSampleRate:
            return "FactoryGameEGS-UELibSampleRate-Win64-Shipping.pdb";
        case ScanTarget::UMG:
            return "FactoryGameEGS-UMG-Win64-Shipping.pdb";
        case ScanTarget::VectorVM:
            return "FactoryGameEGS-VectorVM-Win64-Shipping.pdb";
        case ScanTarget::Voice:
            return "FactoryGameEGS-Voice-Win64-Shipping.pdb";
        case ScanTarget::Voronoi:
            return "FactoryGameEGS-Voronoi-Win64-Shipping.pdb";
        case ScanTarget::VulkanRHI:
            return "FactoryGameEGS-VulkanRHI-Win64-Shipping.pdb";
        case ScanTarget::WebBrowser:
            return "FactoryGameEGS-WebBrowser-Win64-Shipping.pdb";
        case ScanTarget::WindowsPlatformFeatures:
            return "FactoryGameEGS-WindowsPlatformFeatures-Win64-Shipping.pdb";
        case ScanTarget::XAudio2:
            return "FactoryGameEGS-XAudio2-Win64-Shipping.pdb";
        case ScanTarget::Max: {
            Output::send<LogLevel::Warning>(STR("Warning: PDBLookupHelper: Unknown target: {}, using main exe target instead\n"), static_cast<size_t>(scan_target));
            return "FactoryGameEGS-Win64-Shipping.pdb";
        }
        }
        Output::send<LogLevel::Warning>(STR("Warning: PDBLookupHelper: Unknown target: {}, using main exe target instead\n"), static_cast<size_t>(scan_target));
        return "FactoryGameEGS-Win64-Shipping.pdb";
    }
} // namespace RC::PDB
