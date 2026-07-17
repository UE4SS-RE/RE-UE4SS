#include "game_thread_scheduler.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <string>
#include <thread>

namespace
{
    std::atomic<int> g_original_calls{};

    void fake_tick(void*, float, bool)
    {
        ++g_original_calls;
    }
}

int main()
{
    using namespace std::chrono_literals;
    using ue4ss::linux::core::GameThreadScheduler;

    void* slot = reinterpret_cast<void*>(&fake_tick);
    GameThreadScheduler scheduler;
    std::string error;
    assert(scheduler.start_on_slot(&slot, &fake_tick, error));
    assert(scheduler.is_ready());

    std::atomic<int> immediate{};
    assert(scheduler.enqueue([&] {
        ++immediate;
        return true;
    }) != 0u);

    std::atomic<int> loop{};
    assert(scheduler.enqueue(
                   [&] {
                       return ++loop >= 2;
                   },
                   1ms,
                   1ms) != 0u);

    const auto hooked_tick = reinterpret_cast<GameThreadScheduler::TickFunction>(slot);
    assert(!scheduler.is_game_thread());
    hooked_tick(nullptr, 0.016f, false);
    assert(scheduler.wait_for_first_tick(10ms));
    assert(immediate == 1);
    assert(g_original_calls == 1);

    std::this_thread::sleep_for(2ms);
    hooked_tick(nullptr, 0.016f, false);
    std::this_thread::sleep_for(2ms);
    hooked_tick(nullptr, 0.016f, false);
    assert(loop == 2);
    assert(g_original_calls == 3);

    std::atomic<int> retried_one_shot{};
    const auto retried_handle = scheduler.enqueue([&] {
        return ++retried_one_shot >= 2;
    });
    assert(retried_handle != 0u);
    hooked_tick(nullptr, 0.016f, false);
    assert(retried_one_shot == 1 && scheduler.is_valid_handle(retried_handle));
    hooked_tick(nullptr, 0.016f, false);
    assert(retried_one_shot == 2 && !scheduler.is_valid_handle(retried_handle));

    bool synchronous_result{};
    std::atomic<bool> synchronous_done{};
    std::string synchronous_error;
    std::thread synchronous_worker{[&] {
        synchronous_result = scheduler.execute_sync(
                [&](std::string&) {
                    assert(scheduler.is_game_thread());
                    return true;
                },
                100ms,
                synchronous_error);
        synchronous_done = true;
    }};
    for (int attempt = 0; attempt < 100 && !synchronous_done.load(); ++attempt)
    {
        hooked_tick(nullptr, 0.016f, false);
        std::this_thread::sleep_for(1ms);
    }
    synchronous_worker.join();
    assert(synchronous_result && synchronous_error.empty());

    bool direct_sync_called{};
    assert(scheduler.execute_sync(
            [&](std::string&) {
                direct_sync_called = scheduler.is_game_thread();
                return true;
            },
            100ms,
            synchronous_error));
    assert(direct_sync_called);

    std::atomic<bool> timed_operation_called{};
    bool timed_result{true};
    std::string timed_error;
    std::thread timed_worker{[&] {
        timed_result = scheduler.execute_sync(
                [&](std::string&) {
                    timed_operation_called = true;
                    return true;
                },
                2ms,
                timed_error);
    }};
    timed_worker.join();
    assert(!timed_result);
    assert(timed_error.find("cancelled") != std::string::npos ||
           timed_error.find("timed out") != std::string::npos);
    hooked_tick(nullptr, 0.016f, false);
    assert(!timed_operation_called);

    constexpr GameThreadScheduler::OwnerId owner_one = 0x101u;
    constexpr GameThreadScheduler::OwnerId owner_two = 0x202u;
    std::atomic<int> cleanup_calls{};
    const auto reserved = scheduler.reserve_handle();
    assert(reserved != 0u);
    const auto controlled = scheduler.enqueue_owned(
            owner_one,
            [] { return true; },
            100ms,
            {},
            [&] { ++cleanup_calls; },
            reserved);
    assert(controlled == reserved);
    assert(scheduler.is_valid_handle(controlled));
    assert(scheduler.enqueue_owned(owner_two, [] { return true; }, 1ms, {}, {}, controlled) == 0u);
    auto controlled_status = scheduler.action_snapshot(controlled);
    assert(controlled_status.has_value());
    assert(controlled_status->owner == owner_one);
    assert(controlled_status->state == GameThreadScheduler::ActionState::active);
    assert(!controlled_status->frame_based);
    assert(controlled_status->rate == 100);
    assert(!scheduler.pause_owned(owner_two, controlled));
    assert(scheduler.pause_owned(owner_one, controlled));
    controlled_status = scheduler.action_snapshot(controlled);
    assert(controlled_status->state == GameThreadScheduler::ActionState::paused);
    assert(!scheduler.set_rate_owned(owner_two, controlled, 25));
    assert(scheduler.set_rate_owned(owner_one, controlled, 25));
    controlled_status = scheduler.action_snapshot(controlled);
    assert(controlled_status->state == GameThreadScheduler::ActionState::active);
    assert(controlled_status->rate == 25);
    assert(scheduler.reset_owned(owner_one, controlled));
    assert(!scheduler.cancel_owned(owner_two, controlled));
    assert(scheduler.cancel_owned(owner_one, controlled));
    assert(!scheduler.is_valid_handle(controlled));
    assert(cleanup_calls == 1);

    std::atomic<int> frame_once{};
    const auto frame_handle = scheduler.enqueue_frames_owned(
            owner_one,
            [&] {
                ++frame_once;
                return true;
            },
            3,
            false,
            [&] { ++cleanup_calls; });
    assert(frame_handle != 0u);
    auto frame_status = scheduler.action_snapshot(frame_handle);
    assert(frame_status.has_value() && frame_status->frame_based && frame_status->rate == 3);
    hooked_tick(nullptr, 0.016f, false);
    frame_status = scheduler.action_snapshot(frame_handle);
    assert(frame_once == 0 && frame_status->remaining == 2 && frame_status->elapsed == 1);
    hooked_tick(nullptr, 0.016f, false);
    assert(frame_once == 0);
    hooked_tick(nullptr, 0.016f, false);
    assert(frame_once == 1);
    assert(!scheduler.is_valid_handle(frame_handle));
    assert(cleanup_calls == 2);

    std::atomic<int> frame_loop{};
    std::uint64_t frame_loop_handle{};
    frame_loop_handle = scheduler.enqueue_frames_owned(
            owner_one,
            [&] {
                const int call = ++frame_loop;
                if (call == 1)
                {
                    assert(scheduler.pause_owned(owner_one, frame_loop_handle));
                    return false;
                }
                return true;
            },
            2,
            true,
            [&] { ++cleanup_calls; });
    assert(frame_loop_handle != 0u);
    hooked_tick(nullptr, 0.016f, false);
    hooked_tick(nullptr, 0.016f, false);
    assert(frame_loop == 1);
    frame_status = scheduler.action_snapshot(frame_loop_handle);
    assert(frame_status.has_value());
    assert(frame_status->state == GameThreadScheduler::ActionState::paused);
    assert(frame_status->remaining == 2);
    hooked_tick(nullptr, 0.016f, false);
    assert(frame_loop == 1);
    assert(scheduler.unpause_owned(owner_one, frame_loop_handle));
    hooked_tick(nullptr, 0.016f, false);
    hooked_tick(nullptr, 0.016f, false);
    assert(frame_loop == 2);
    assert(!scheduler.is_valid_handle(frame_loop_handle));
    assert(cleanup_calls == 3);

    const auto owner_one_action = scheduler.enqueue_owned(
            owner_one, [] { return true; }, 1s, {}, [&] { ++cleanup_calls; });
    const auto owner_two_action = scheduler.enqueue_owned(
            owner_two, [] { return true; }, 1s, {}, [&] { ++cleanup_calls; });
    assert(owner_one_action != 0u && owner_two_action != 0u);
    assert(scheduler.clear_owner(owner_one) == 1u);
    assert(!scheduler.is_valid_handle(owner_one_action));
    assert(scheduler.is_valid_handle(owner_two_action));
    assert(cleanup_calls == 4);
    assert(scheduler.cancel_owned(owner_two, owner_two_action));
    assert(cleanup_calls == 5);

    std::atomic<bool> callback_entered{};
    std::atomic<bool> callback_release{};
    assert(scheduler.enqueue([&] {
        assert(scheduler.is_game_thread());
        callback_entered = true;
        while (!callback_release.load())
        {
            std::this_thread::yield();
        }
        return true;
    }) != 0u);
    std::atomic<bool> stop_returned{};
    std::thread shutdown_controller{[&] {
        while (!callback_entered.load())
        {
            std::this_thread::yield();
        }
        std::thread stop_thread{[&] {
            scheduler.stop();
            stop_returned = true;
        }};
        std::this_thread::sleep_for(2ms);
        assert(!stop_returned.load());
        callback_release = true;
        stop_thread.join();
    }};
    hooked_tick(nullptr, 0.016f, false);
    shutdown_controller.join();

    assert(slot == reinterpret_cast<void*>(&fake_tick));
    assert(!scheduler.is_ready());
    const int original_calls_before_direct_tick = g_original_calls.load();
    reinterpret_cast<GameThreadScheduler::TickFunction>(slot)(nullptr, 0.016f, false);
    assert(g_original_calls == original_calls_before_direct_tick + 1);
    return 0;
}
