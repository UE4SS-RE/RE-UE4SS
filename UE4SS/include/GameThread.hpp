#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <memory>
#include <utility>

#include <DynamicOutput/DynamicOutput.hpp>
#include <UE4SSProgram.hpp>
#include <UE4SSRuntime.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UnrealInitializer.hpp>

namespace RC
{
    // The game thread id comes from the EngineTick hook, so a ProcessEvent callback can't be
    // checked against it. This marks code running from either hook instead.
    inline thread_local bool t_in_game_thread_hook{};

    inline auto is_on_game_thread() -> bool
    {
        return t_in_game_thread_hook || Unreal::IsInGameThreadRaw();
    }

    // Blueprint types can be freed while a dump reads them unless it runs on the game thread.
    inline auto allow_blueprint_types(bool requested) -> bool
    {
        if (requested && !is_on_game_thread())
        {
            Output::send<LogLevel::Warning>(STR("Not on the game thread, dumping native types only\n"));
            return false;
        }
        return requested;
    }

    // Preserve synchronous dumper APIs while keeping reflection and UObject lifetimes on
    // the engine thread. Exceptions are returned to the caller, not thrown through Tick.
    // Tries EngineTick, then ProcessEvent, then runs on the calling thread.
    template <typename Callable>
    auto execute_on_game_thread_and_wait(Callable task) -> void
    {
        if (is_on_game_thread())
        {
            task();
            return;
        }

        struct State
        {
            Callable task;
            std::promise<void> completion{};
            std::atomic_bool claimed{};
        };
        auto state = std::make_shared<State>(std::move(task));
        auto result = state->completion.get_future();

        auto run_in_hook = [state](Unreal::Hook::TCallbackIterationData<void>&, auto...) {
            if (state->claimed.exchange(true))
            {
                return;
            }
            t_in_game_thread_hook = true;
            try
            {
                state->task();
                state->completion.set_value();
            }
            catch (...)
            {
                state->completion.set_exception(std::current_exception());
            }
            t_in_game_thread_hook = false;
        };
        const Unreal::Hook::FCallbackOptions options{true, true, STR("UE4SS"), STR("ReflectionDump")};

        auto registered = UE4SSRuntime::IsEngineTickAvailable() && Unreal::Hook::RegisterEngineTickPostCallback(run_in_hook, options) != Unreal::Hook::ERROR_ID;
        if (!registered && UE4SSRuntime::IsProcessEventAvailable())
        {
            registered = Unreal::Hook::RegisterProcessEventPostCallback(run_in_hook, options) != Unreal::Hook::ERROR_ID;
            if (registered)
            {
                Output::send<LogLevel::Warning>(STR("EngineTick hook unavailable, dumping from ProcessEvent\n"));
            }
        }

        if (registered)
        {
            // The hook may not fire, for example during a startup movie or when the caller is
            // blocking the game thread. A late callback finds the task claimed and does nothing.
            const std::chrono::seconds wait{UE4SSProgram::settings_manager.General.SecondsToWaitForGameThread};
            if (wait <= std::chrono::seconds::zero() || result.wait_for(wait) == std::future_status::ready || state->claimed.exchange(true))
            {
                result.get();
                return;
            }
        }

        Output::send<LogLevel::Warning>(STR("Could not reach the game thread, dumping from this thread\n"));
        state->task();
    }
}
