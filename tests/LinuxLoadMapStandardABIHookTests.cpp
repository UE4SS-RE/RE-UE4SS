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
    RC::Unreal::UPendingNetGame* s_expected_pending_game{};
    RC::Unreal::FString* s_expected_error{};
    bool s_original_called{};
} // namespace

__attribute__((noinline, used, visibility("default"))) bool standard_load_map_fixture(
        RC::Unreal::UEngine* engine,
        RC::Unreal::FWorldContext& world,
        RC::Unreal::FURL,
        RC::Unreal::UPendingNetGame* pending_game,
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
    return engine == s_expected_engine &&
           &world == s_expected_world &&
           pending_game == s_expected_pending_game &&
           &error == s_expected_error;
}

namespace
{
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
            reinterpret_cast<void*>(&standard_load_map_fixture));
    RC::Unreal::UnrealInitializer::StaticStorage::GlobalConfig.bHookLoadMap = true;

    alignas(void*) std::array<std::byte, 0x400> world_storage{};
    auto& world = *reinterpret_cast<RC::Unreal::FWorldContext*>(world_storage.data());
    auto* engine = reinterpret_cast<RC::Unreal::UEngine*>(0x12340000);
    auto* pending_game = reinterpret_cast<RC::Unreal::UPendingNetGame*>(0x56780000);
    RC::Unreal::FURL url{};
    RC::Unreal::FString error{};

    s_expected_engine = engine;
    s_expected_world = &world;
    s_expected_pending_game = pending_game;
    s_expected_error = &error;

    bool callback_called{};
    bool callback_arguments_valid{};
    const auto callback_id = RC::Unreal::Hook::RegisterLoadMapPreCallback(
            [&](auto&,
                RC::Unreal::UEngine* callback_engine,
                RC::Unreal::FWorldContext& callback_world,
                RC::Unreal::FURL,
                RC::Unreal::UPendingNetGame* callback_pending_game,
                RC::Unreal::FString& callback_error) {
                callback_called = true;
                callback_arguments_valid = callback_engine == engine &&
                                           &callback_world == &world &&
                                           callback_pending_game == pending_game &&
                                           &callback_error == &error;
            },
            {false, false, STR("LinuxLoadMapStandardABIHookTests"), STR("StandardABI")});
    if (callback_id == RC::Unreal::Hook::ERROR_ID)
    {
        return 1;
    }

    auto* public_entry = reinterpret_cast<PublicLoadMapFunction>(&standard_load_map_fixture);
    const bool result = invoke_public_load_map(public_entry, engine, world, url, pending_game, error);
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
