#include <Middleware.hpp>

#include <QueueProfiler.hpp>
#include <StringPool.hpp>

#include <chrono>
#include <stdexcept>

#include <Unreal/CoreUObject/UObject/Class.hpp>

// note if using fname index as a hash, games that implement name recycling can be problematic/inaccurate for context objects, and right clicking entries
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
        Unreal::UObjectGlobals::ForEachUObject([this](UObject* object, ...) -> LoopAction {
            if (object && Unreal::Cast<UFunction>(object) && object->GetName().contains(STR("Tick")))
            {
                m_tick_fns.insert(object);
            }
            return LoopAction::Continue;
        });

        Output::send<LogLevel::Verbose>(L"Found {} engine tick functions!", m_tick_fns.size());

        m_pe_controller = {
            .register_prehook_fn = &RC::Unreal::Hook::RegisterProcessEventPreCallback,
            .register_posthook_fn = &RC::Unreal::Hook::RegisterProcessEventPostCallback,
            .m_pre_callback = [this](auto&, UObject* context, UFunction* function, void*) {
                return enqueue(EMiddlewareHookTarget::ProcessEvent, context, function);
            },
            .m_post_callback = [](auto&, UObject*, UFunction*, void*) {
                m_depth = (m_depth == 0) ? 0 : (m_depth - 1);
            }
        };

        m_pi_controller = {
            .register_prehook_fn = &RC::Unreal::Hook::RegisterProcessInternalPreCallback,
            .register_posthook_fn = &RC::Unreal::Hook::RegisterProcessInternalPostCallback,
            .m_pre_callback = [this](auto&, UObject* context, FFrame& stack, void*) {
                auto fn = stack.Node();
                if (!fn) fn = stack.CurrentNativeFunction();
                return enqueue(EMiddlewareHookTarget::ProcessInternal, context, fn);
            },
            .m_post_callback = [](auto&, UObject*, FFrame&, void*) {
                m_depth = (m_depth == 0) ? 0 : (m_depth - 1);
            }
        };

        m_plsf_controller = {
            .register_prehook_fn = &RC::Unreal::Hook::RegisterProcessLocalScriptFunctionPreCallback,
            .register_posthook_fn = &RC::Unreal::Hook::RegisterProcessLocalScriptFunctionPostCallback,
            .m_pre_callback = [this](auto&, UObject* context, FFrame& stack, void*) {
                auto fn = stack.Node();
                if (!fn) fn = stack.CurrentNativeFunction();
                return enqueue(EMiddlewareHookTarget::ProcessLocalScriptFunction, context, fn);
            },
            .m_post_callback = [](auto&, UObject*, FFrame&, void*) {
                m_depth = (m_depth == 0) ? 0 : (m_depth - 1);
            }
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

        m_pe_controller.unhook();
        m_pi_controller.unhook();
        m_plsf_controller.unhook();

        // Causes all thread_local depths to be reset the next time the prehook runs.
        m_depth_reset_counter.fetch_add(1, std::memory_order_release);
        m_allow_queue.clear(std::memory_order_release);
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

        if (!(m_plsf_controller.install_posthook() &&
        m_pi_controller.install_posthook() &&
        m_pe_controller.install_posthook() &&
        m_plsf_controller.install_prehook() &&
        m_pi_controller.install_prehook() &&
        m_pe_controller.install_prehook())) {
            m_pe_controller.unhook();
            m_pi_controller.unhook();
            m_plsf_controller.unhook();
            return false;
        }

        m_paused = false;
        m_allow_queue.test_and_set(std::memory_order_acq_rel);
        return true;
    }

    auto Middleware::enqueue(const EMiddlewareHookTarget hook_target,
                             UObject* context,
                             UFunction* function) -> void
    {
        thread_local std::thread::id thread_id = std::this_thread::get_id();
        thread_local uint64_t local_reset_counter = m_depth_reset_counter.load(std::memory_order_acquire);
        if (!m_allow_queue.test(std::memory_order_acquire)) return;
        const auto current_counter = m_depth_reset_counter.load(std::memory_order_acquire);
        if (current_counter != local_reset_counter)
        {
            m_depth = 0;
            local_reset_counter = current_counter;
        }

        const auto is_tick = is_tick_fn(function);

        // Middleware is a singleton and lives for the mod lifetime, so a simple thread_local token is safe here.
        thread_local ProducerToken tls_token{m_queue};

        auto strings = StringPool::GetInstance().get_strings(context, function);

        QueueProfiler::BeginEnqueue();
        m_queue.enqueue(tls_token, CallStackEntry{hook_target, strings, m_depth++, thread_id, is_tick});
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
        auto now = std::chrono::steady_clock::now();
        for (; std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count() < max_ms; now = std::chrono::steady_clock::now())
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
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count() >= max_ms)
        {
            QueueProfiler::AddTimeExceededCount();
        }
    }

} // namespace RC::EventViewerMod
