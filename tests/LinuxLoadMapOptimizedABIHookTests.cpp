#include <array>
#include <cstddef>

#include <Unreal/FWorldContext.hpp>
#include <Unreal/Hooks/Hooks.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UnrealInitializer.hpp>

namespace
{
    using PublicLoadMapFunction = bool (*)(
            RC::Unreal::UEngine*,
            RC::Unreal::FWorldContext&,
            RC::Unreal::FURL,
            RC::Unreal::UPendingNetGame*,
            RC::Unreal::FString&);

    RC::Unreal::UEngine* s_expected_engine{};
    RC::Unreal::FWorldContext* s_expected_world{};
    RC::Unreal::FString* s_expected_error{};
    bool s_original_called{};

    __attribute__((noinline)) bool optimized_load_map_fixture(
            RC::Unreal::UEngine* engine,
            RC::Unreal::FWorldContext& world,
            RC::Unreal::FURL,
            RC::Unreal::FString& error)
    {
        asm volatile("nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop");

        s_original_called = true;
        return engine == s_expected_engine && &world == s_expected_world && &error == s_expected_error;
    }

    __attribute__((noinline, optnone)) bool invoke_public_load_map(
            PublicLoadMapFunction entry,
            RC::Unreal::UEngine* engine,
            RC::Unreal::FWorldContext& world,
            RC::Unreal::FURL& url,
            RC::Unreal::UPendingNetGame* pending_game,
            RC::Unreal::FString& error)
    {
        return entry(engine, world, url, pending_game, error);
    }
} // namespace

int main()
{
    RC::Unreal::UEngine::LoadMapInternal.assign_address(
            reinterpret_cast<void*>(&optimized_load_map_fixture));
    RC::Unreal::UnrealInitializer::StaticStorage::GlobalConfig.bHookLoadMap = true;

    alignas(void*) std::array<std::byte, 0x400> world_storage{};
    auto& world = *reinterpret_cast<RC::Unreal::FWorldContext*>(world_storage.data());
    auto* engine = reinterpret_cast<RC::Unreal::UEngine*>(0x12340000);
    RC::Unreal::FURL url{};
    RC::Unreal::FString real_error{};
    RC::Unreal::FString dummy_error{};

    s_expected_engine = engine;
    s_expected_world = &world;
    s_expected_error = &real_error;

    bool callback_called{};
    bool callback_arguments_valid{};
    const auto callback_id = RC::Unreal::Hook::RegisterLoadMapPreCallback(
            [&](auto&,
                RC::Unreal::UEngine* callback_engine,
                RC::Unreal::FWorldContext& callback_world,
                RC::Unreal::FURL,
                RC::Unreal::UPendingNetGame* pending_game,
                RC::Unreal::FString& callback_error) {
                callback_called = true;
                callback_arguments_valid = callback_engine == engine &&
                                           &callback_world == &world &&
                                           pending_game == nullptr &&
                                           &callback_error == &real_error;
            },
            {false, false, STR("LinuxLoadMapOptimizedABIHookTests"), STR("OptimizedServerABI")});
    if (callback_id == RC::Unreal::Hook::ERROR_ID)
    {
        return 1;
    }

    // Call through the public five-argument type so the current generic detour has
    // valid storage in both rcx and r8. The native fixture deliberately consumes
    // rcx as FString&, matching the optimized Palworld server ABI.
    auto* public_entry = reinterpret_cast<PublicLoadMapFunction>(&optimized_load_map_fixture);
    auto* rcx_error_storage = reinterpret_cast<RC::Unreal::UPendingNetGame*>(&real_error);
    const bool result = invoke_public_load_map(public_entry, engine, world, url, rcx_error_storage, dummy_error);

    if (!s_original_called || !result)
    {
        return 2;
    }
    if (!callback_called)
    {
        return 3;
    }
    return callback_arguments_valid ? 0 : 4;
}
