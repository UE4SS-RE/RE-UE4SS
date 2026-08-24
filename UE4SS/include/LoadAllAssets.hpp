#pragma once

#include <DynamicOutput/DynamicOutput.hpp>
#include <Timer/ScopedTimer.hpp>
#include <Unreal/FAssetData.hpp>
#include <Unreal/UAssetRegistry.hpp>
#include <Unreal/UnrealVersion.hpp>

namespace RC
{
    // Blueprint classes, structs and enums only exist in memory once their asset has been loaded, so a
    // dump taken at the main menu sees almost none of them. Force-loading everything first makes the
    // dump complete at the cost of a lot of memory and stability -- see the warning below -- so it is
    // opt-in per dump rather than a setting.
    //
    // Runs 'dump' with every asset loaded, then releases them again. The release runs even if the dump
    // throws, because leaving the game holding every asset is far worse than losing the dump.
    template <typename Callable>
    auto run_with_all_assets_loaded(Callable&& dump) -> void
    {
        if (Unreal::Version::IsBelow(4, 17) || !Unreal::bFAssetDataAvailable)
        {
            Output::send<LogLevel::Warning>(STR("FAssetData is not available in this game, dumping without force-loading assets.\n"));
            dump();
            return;
        }

        Output::send(STR("Loading all assets...\n"));
        double asset_loading_duration{};
        {
            ScopedTimer loading_timer{&asset_loading_duration};
            Unreal::UAssetRegistry::LoadAllAssets();
        }
        Output::send(STR("Loading all assets took {} seconds\n"), asset_loading_duration);

        // Anything that force-loads assets leaves the game in a state it was never meant to be in;
        // the game is likely to crash if play continues past this point.
        struct AssetReleaseGuard
        {
            ~AssetReleaseGuard()
            {
                Output::send(STR("Unloading all forcefully loaded assets\n"));
                Unreal::UAssetRegistry::FreeAllForcefullyLoadedAssets();
            }
        } release_guard{};

        dump();
    }
} // namespace RC
