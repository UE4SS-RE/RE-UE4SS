#include <Middleware.hpp>
#include <StringPool.hpp>
#include <QueueProfiler.hpp>

#include <atomic>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <Unreal/Hooks/Hooks.hpp>
#include <concurrentqueue.h>

// if using fname index as a hash, games that implement name recycling can be problematic/inaccurate for context objects
// BUG layout issues, using wide strings in imgui
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
    using RC::Unreal::Hook::ProcessEventCallbackWithData;
    using RC::Unreal::Hook::ProcessInternalCallbackWithData;
    using RC::Unreal::Hook::FCallbackOptions;

    using moodycamel::ConcurrentQueue;
    using moodycamel::ConsumerToken;
    using moodycamel::ProducerToken;

    class MiddlewareHooks : public IMiddleware
    {
    public:
        auto set_hook_target(const EMiddlewareHookTarget target) -> void final
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

        [[nodiscard]] auto get_hook_target() const -> EMiddlewareHookTarget final
        {
            assert_on_imgui_thread();
            return m_hook_target;
        }

        auto stop() -> bool override
        {
            return stop_impl(true);
        }

        [[nodiscard]] auto is_paused() const -> bool final
        {
            assert_on_imgui_thread();
            return m_paused;
        }

        auto start() -> bool override
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
                // Best-effort cleanup
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

        auto set_imgui_thread_id(std::thread::id id) -> void final
        {
            m_imgui_id = id;
        }

        [[nodiscard]] auto get_imgui_thread_id() const -> std::thread::id final
        {
            return m_imgui_id;
        }

        [[nodiscard]] auto get_average_enqueue_time() const -> double final
        {
            return QueueProfiler::GetEnqueueAverage();
        }

        [[nodiscard]] auto get_average_dequeue_time() const -> double final
        {
            return QueueProfiler::GetDequeueAverage();
        }

        ~MiddlewareHooks() override
        {
            // Don't throw from destructor.
            try
            {
                stop_impl(false);
            }
            catch (...)
            {
            }
        }

    protected:
        MiddlewareHooks()
        {
            std::call_once(m_get_tick_fns_flag, [] {
                Unreal::UObjectGlobals::ForEachUObject([](UObject* object, ...) -> LoopAction {
                    if (object && Unreal::Cast<UFunction>(object) && object->GetName().contains(STR("Tick")))
                    {
                        m_tick_fns.insert(object);
                    }
                    return LoopAction::Continue;
                });
            });

            QueueProfiler::Reset();
        }

        auto assert_on_imgui_thread() const -> void
        {
            if (std::this_thread::get_id() != m_imgui_id)
            {
                throw std::runtime_error("EventViewerMod middleware: must be called from ImGui thread");
            }
        }

        [[nodiscard]] auto is_tick_fn(const UFunction* fn) const -> bool
        {
            return fn && m_tick_fns.contains(const_cast<UFunction*>(fn));
        }

    private:
        auto stop_impl(const bool do_assert) -> bool
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

        GlobalCallbackId m_prehook_id = ERROR_ID;
        GlobalCallbackId m_posthook_id = ERROR_ID;

        EMiddlewareHookTarget m_hook_target = EMiddlewareHookTarget::ProcessEvent;
        bool m_paused = true;

        std::thread::id m_imgui_id{};

        const FCallbackOptions m_options = {false, true, STR("EventViewer"), STR("CallStackMonitor")};

        const ProcessEventCallbackWithData m_pe_pre = [this](auto&, UObject* context, UFunction* function, void*)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            thread_local uint64_t local_reset_counter = m_depth_reset_counter.load(std::memory_order_acquire);

            const auto current_counter = m_depth_reset_counter.load(std::memory_order_acquire);
            if (current_counter != local_reset_counter)
            {
                m_pe_depth = 0;
                local_reset_counter = current_counter;
            }

            enqueue(m_hook_target,
                    context,
                    function,
                    m_pe_depth++,
                    thread_id,
                    is_tick_fn(function));
        };

        const ProcessEventCallbackWithData m_pe_post = [](auto&, UObject*, UFunction*, void*)
        {
            m_pe_depth = (m_pe_depth == 0) ? 0 : (m_pe_depth - 1);
        };

        const ProcessInternalCallbackWithData m_pi_pre = [this](auto&, UObject* context, FFrame& stack, void*)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            thread_local uint64_t local_reset_counter = m_depth_reset_counter.load(std::memory_order_acquire);

            const auto current_counter = m_depth_reset_counter.load(std::memory_order_acquire);
            if (current_counter != local_reset_counter)
            {
                m_pi_depth = 0;
                local_reset_counter = current_counter;
            }

            auto fn = stack.Node(); // May be null for native frames.
            if (!fn)
            {
                fn = stack.CurrentNativeFunction();
            }

            enqueue(m_hook_target,
                    context,
                    fn,
                    m_pi_depth++,
                    thread_id,
                    is_tick_fn(fn));
        };

        const ProcessInternalCallbackWithData m_pi_post = [](auto&, UObject*, FFrame&, void*)
        {
            m_pi_depth = (m_pi_depth == 0) ? 0 : (m_pi_depth - 1);
        };

        inline static std::once_flag m_get_tick_fns_flag;
        inline static std::unordered_set<UObject*> m_tick_fns;

        inline static thread_local uint32_t m_pe_depth = 0;
        inline static thread_local uint32_t m_pi_depth = 0;
        inline static std::atomic_uint64_t m_depth_reset_counter = 0;
    };

    class ConcurrentQueueMiddleware final : public MiddlewareHooks
    {
    public:
        ConcurrentQueueMiddleware() = default;

        auto enqueue(const EMiddlewareHookTarget hook_target,
                     UObject* context,
                     UFunction* function,
                     const uint32_t depth,
                     const std::thread::id thread_id,
                     const bool is_tick) -> void override
        {
            // ProducerToken is a measurable win at high rates, but thread_local tokens become unsafe
            // if we swap middleware instances (queue address changes). We keep a per-thread token
            // per-queue address and rebuild it when the queue changes. Old tokens are intentionally
            // leaked to avoid destructor-order hazards with dead queues. This whole idea can be scrapped
            // if/when ConcurrentQueueMiddleware is the only schema
            struct TLSToken
            {
                ConcurrentQueue<CallStackEntry>* queue = nullptr;
                ProducerToken* token = nullptr;
            };
            thread_local TLSToken tls{};

            QueueProfiler::BeginEnqueue();

            if (tls.queue != &m_queue)
            {
                tls.queue = &m_queue;
                tls.token = new ProducerToken(m_queue);
            }
            auto strings = StringPool::GetInstance().get_strings(context, function);
            m_queue.enqueue(*tls.token, CallStackEntry{
                hook_target,
                strings.first,
                strings.second,
                depth,
                thread_id,
                is_tick
            });

            QueueProfiler::EndEnqueue();
        }

        auto dequeue(const uint16_t max_ms,
                     const uint16_t max_count_per_iteration,
                     const std::function<void(CallStackEntry&&)>& on_dequeue) -> void override
        {
            assert_on_imgui_thread();

            const auto start = std::chrono::steady_clock::now();
            if (m_buffer.size() < max_count_per_iteration)
            {
                m_buffer.resize(max_count_per_iteration);
            }

            while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < max_ms)
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

                // Avoid spinning if approx size says empty.
                if (m_queue.size_approx() == 0)
                {
                    break;
                }
            }
            QueueProfiler::AddPendingCount(m_queue.size_approx());
        }

        auto stop() -> bool override
        {
            if (!MiddlewareHooks::stop())
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

        [[nodiscard]] auto get_type() const -> EMiddlewareThreadScheme final
        {
            return EMiddlewareThreadScheme::ConcurrentQueue;
        }

        ~ConcurrentQueueMiddleware() override = default;

    private:
        ConcurrentQueue<CallStackEntry> m_queue;
        ConsumerToken m_imgui_consumer_token{m_queue};
        std::vector<CallStackEntry> m_buffer{};
    };

    class MutexMiddleware final : public MiddlewareHooks
    {
    public:
        MutexMiddleware() = default;

        auto enqueue(const EMiddlewareHookTarget hook_target,
                     UObject* context,
                     UFunction* function,
                     const uint32_t depth,
                     const std::thread::id thread_id,
                     const bool is_tick) -> void override
        {
            QueueProfiler::BeginEnqueue();
            std::lock_guard lock(m_mutex);
            auto strings = StringPool::GetInstance().get_strings(context, function);
            m_queue.emplace(
                hook_target,
                strings.first,
                strings.second,
                depth,
                thread_id,
                is_tick);
            QueueProfiler::EndEnqueue();
        }

        auto dequeue(const uint16_t max_ms,
                     const uint16_t max_count_per_iteration,
                     const std::function<void(CallStackEntry&&)>& on_dequeue) -> void override
        {
            assert_on_imgui_thread();

            const auto start = std::chrono::steady_clock::now();
            std::vector<CallStackEntry> local;
            local.reserve(max_count_per_iteration);

            while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < max_ms)
            {
                local.clear();

                {
                    QueueProfiler::BeginDequeue();
                    std::lock_guard lock(m_mutex);
                    if (m_queue.empty())
                    {
                        break;
                    }
                    for (size_t i = 0; i < max_count_per_iteration && !m_queue.empty(); ++i)
                    {
                        local.emplace_back(std::move(m_queue.front()));
                        m_queue.pop();
                    }
                    QueueProfiler::EndDequeue();
                }

                for (auto& entry : local)
                {
                    on_dequeue(std::move(entry));
                }

                if (local.size() < max_count_per_iteration)
                {
                    break; // likely empty; avoid relocking immediately
                }
            }
            QueueProfiler::AddPendingCount(m_queue.size());
        }

        auto stop() -> bool override
        {
            if (!MiddlewareHooks::stop())
            {
                return false;
            }

            std::lock_guard lock(m_mutex);
            while (!m_queue.empty())
            {
                m_queue.pop();
            }
            return true;
        }

        [[nodiscard]] auto get_type() const -> EMiddlewareThreadScheme  final
        {
            return EMiddlewareThreadScheme::Mutex;
        }

        ~MutexMiddleware() override = default;

    private:
        std::queue<CallStackEntry> m_queue;
        std::mutex m_mutex;
    };

    auto GetNewMiddleware(const EMiddlewareThreadScheme type) -> std::unique_ptr<IMiddleware>
    {
        switch (type)
        {
        case EMiddlewareThreadScheme::ConcurrentQueue:
            return std::make_unique<ConcurrentQueueMiddleware>();
        case EMiddlewareThreadScheme::Mutex:
            return std::make_unique<MutexMiddleware>();
        default:
            throw std::runtime_error("Unknown middleware type");
        }
    }
} // namespace RC::EventViewerMod
