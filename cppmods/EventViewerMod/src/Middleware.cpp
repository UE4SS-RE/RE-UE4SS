#include <Middleware.hpp>

#include <QueueProfiler.hpp>
#include <StringPool.hpp>

#include <chrono>
#include <stdexcept>

#include <Unreal/CoreUObject/UObject/Class.hpp>

// if using fname index as a hash, games that implement name recycling can be problematic/inaccurate for context objects
// BUG layout issues
// FEATURES right click menus for adding to lists, copying, etc, lowercase filters, clear all should clear thread list, further cut down on memory usage by using text unformatted's end pointer
namespace RC::EventViewerMod
{
    using RC::Unreal::UFunction;
    using RC::Unreal::UObject;
    using RC::Unreal::FFrame;

    using RC::Unreal::Hook::GlobalCallbackId;
    using RC::Unreal::Hook::ERROR_ID;
    using RC::Unreal::Hook::UnregisterCallback;
    using RC::Unreal::Hook::RegisterProcessEventPreCallback;
    using RC::Unreal::Hook::RegisterProcessEventPostCallback;
    using RC::Unreal::Hook::RegisterProcessInternalPreCallback;
    using RC::Unreal::Hook::RegisterProcessInternalPostCallback;

    using moodycamel::ConsumerToken;
    using moodycamel::ProducerToken;

    auto Middleware::GetInstance() -> Middleware&
    {
        static Middleware s_instance;
        return s_instance;
    }

    Middleware::Middleware()
    {
        Unreal::UObjectGlobals::ForEachUObject([](UObject* object, ...) -> LoopAction {
            if (object && Unreal::Cast<UFunction>(object) && object->GetName().contains(STR("Tick")))
            {
                m_tick_fns.insert(object);
            }
            return LoopAction::Continue;
        });

        Output::send<LogLevel::Verbose>(L"Found {} engine tick functions!", m_tick_fns.size());

        // Hook callbacks
        m_pe_pre = [this](auto&, UObject* context, UFunction* function, void*)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            thread_local uint64_t local_reset_counter = m_depth_reset_counter.load(std::memory_order_acquire);

            const auto current_counter = m_depth_reset_counter.load(std::memory_order_acquire);
            if (current_counter != local_reset_counter)
            {
                m_pe_depth = 0;
                local_reset_counter = current_counter;
            }

            return enqueue(m_hook_target,
                    context,
                    function,
                    m_pe_depth++,
                    thread_id,
                    is_tick_fn(function));
        };

        m_pe_post = [](auto&, UObject*, UFunction*, void*)
        {
            m_pe_depth = (m_pe_depth == 0) ? 0 : (m_pe_depth - 1);
        };

        m_pi_pre = [this](auto&, UObject* context, FFrame& stack, void*)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            thread_local uint64_t local_reset_counter = m_depth_reset_counter.load(std::memory_order_acquire);

            const auto current_counter = m_depth_reset_counter.load(std::memory_order_acquire);
            if (current_counter != local_reset_counter)
            {
                m_pi_depth = 0;
                local_reset_counter = current_counter;
            }

            auto fn = stack.Node();
            if (!fn)
            {
                fn = stack.CurrentNativeFunction();
            }

            return enqueue(m_hook_target,
                    context,
                    fn,
                    m_pi_depth++,
                    thread_id,
                    is_tick_fn(fn));
        };

        m_pi_post = [](auto&, UObject*, FFrame&, void*)
        {
            m_pi_depth = (m_pi_depth == 0) ? 0 : (m_pi_depth - 1);
        };

        QueueProfiler::Reset();
    }

    Middleware::~Middleware()
    {
        stop_impl(false);
    }

    auto Middleware::assert_on_imgui_thread() const -> void
    {
        if (std::this_thread::get_id() != m_imgui_id)
        {
            throw std::runtime_error("EventViewerMod middleware: must be called from ImGui thread");
        }
    }

    auto Middleware::is_tick_fn(const UFunction* fn) const -> bool
    {
        return fn && m_tick_fns.contains(const_cast<UFunction*>(fn));
    }

    auto Middleware::set_imgui_thread_id(std::thread::id id) -> void
    {
        m_imgui_id = id;
    }

    auto Middleware::get_imgui_thread_id() const -> std::thread::id
    {
        return m_imgui_id;
    }

    auto Middleware::get_average_enqueue_time() const -> double
    {
        return QueueProfiler::GetEnqueueAverage();
    }

    auto Middleware::get_average_dequeue_time() const -> double
    {
        return QueueProfiler::GetDequeueAverage();
    }

    auto Middleware::set_hook_target(const EMiddlewareHookTarget target) -> void
    {
        assert_on_imgui_thread();
        if (m_hook_target == target)
        {
            return;
        }

        const bool should_resume = !m_paused;
        stop();
        m_hook_target = target;
        if (should_resume)
        {
            start();
        }
    }

    auto Middleware::get_hook_target() const -> EMiddlewareHookTarget
    {
        assert_on_imgui_thread();
        return m_hook_target;
    }

    auto Middleware::stop_impl(const bool do_assert) -> bool
    {
        if (do_assert)
        {
            assert_on_imgui_thread();
        }
        if (m_paused)
        {
            return true;
        }

        if (m_prehook_id != ERROR_ID && !UnregisterCallback(m_prehook_id))
        {
            return false;
        }
        if (m_posthook_id != ERROR_ID && !UnregisterCallback(m_posthook_id))
        {
            return false;
        }

        m_prehook_id = m_posthook_id = ERROR_ID;

        // Causes all thread_local depths to be reset the next time the prehook runs.
        m_depth_reset_counter.fetch_add(1, std::memory_order_release);

        m_paused = true;
        return true;
    }

    auto Middleware::stop() -> bool
    {
        if (!stop_impl(true))
        {
            return false;
        }

        // Drain remaining items (discard).
        if (m_buffer.empty())
        {
            m_buffer.resize(256);
        }

        for (;;)
        {
            const auto count = m_queue.try_dequeue_bulk(m_imgui_consumer_token, m_buffer.data(), m_buffer.size());
            if (count == 0)
            {
                break;
            }
        }

        return true;
    }

    auto Middleware::is_paused() const -> bool
    {
        assert_on_imgui_thread();
        return m_paused;
    }

    auto Middleware::start() -> bool
    {
        assert_on_imgui_thread();
        if (!m_paused)
        {
            return true;
        }

        switch (m_hook_target)
        {
        case EMiddlewareHookTarget::ProcessEvent:
            // Install post first; UE4SS historically had edge cases where pre wouldn't run unless post exists.
            m_posthook_id = RegisterProcessEventPostCallback(m_pe_post, m_options);
            m_prehook_id = RegisterProcessEventPreCallback(m_pe_pre, m_options);
            break;
        case EMiddlewareHookTarget::ProcessInternal:
            m_posthook_id = RegisterProcessInternalPostCallback(m_pi_post, m_options);
            m_prehook_id = RegisterProcessInternalPreCallback(m_pi_pre, m_options);
            break;
        default:
            throw std::runtime_error("Unknown hook target");
        }

        if (m_prehook_id == ERROR_ID || m_posthook_id == ERROR_ID)
        {
            if (m_prehook_id != ERROR_ID)
            {
                UnregisterCallback(m_prehook_id);
            }
            if (m_posthook_id != ERROR_ID)
            {
                UnregisterCallback(m_posthook_id);
            }
            m_prehook_id = m_posthook_id = ERROR_ID;
            m_paused = true;
            return false;
        }

        m_paused = false;
        return true;
    }

    auto Middleware::enqueue(const EMiddlewareHookTarget hook_target,
                             UObject* context,
                             UFunction* function,
                             const uint32_t depth,
                             const std::thread::id thread_id,
                             const bool is_tick) -> void
    {
        QueueProfiler::BeginEnqueue();

        // Middleware is a singleton and lives for the mod lifetime, so a simple thread_local token is safe here.
        thread_local ProducerToken tls_token{m_queue};

        auto strings = StringPool::GetInstance().get_strings(context, function);
        m_queue.enqueue(tls_token, CallStackEntry{hook_target, strings, depth, thread_id, is_tick});

        QueueProfiler::EndEnqueue();
    }

    auto Middleware::dequeue(const uint16_t max_ms,
                             const uint16_t max_count_per_iteration,
                             const std::function<void(CallStackEntry&&)>& on_dequeue) -> void
    {
        assert_on_imgui_thread();

        const auto start_time = std::chrono::steady_clock::now();
        if (m_buffer.size() < max_count_per_iteration)
        {
            m_buffer.resize(max_count_per_iteration);
        }

        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < max_ms)
        {
            QueueProfiler::BeginDequeue();
            const auto amount = m_queue.try_dequeue_bulk(m_imgui_consumer_token, m_buffer.data(), max_count_per_iteration);
            QueueProfiler::EndDequeue();

            if (amount == 0)
            {
                break;
            }

            for (size_t i = 0; i < amount; ++i)
            {
                on_dequeue(std::move(m_buffer[i]));
            }

            if (m_queue.size_approx() == 0)
            {
                break;
            }
        }

        QueueProfiler::AddPendingCount(m_queue.size_approx());
    }

} // namespace RC::EventViewerMod
