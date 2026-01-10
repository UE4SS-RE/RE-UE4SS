#include <Middleware.hpp>

#include <stdexcept>
#include <mutex>
#include <queue>
#include <atomic>
#include <unordered_set>

#include <Unreal/Hooks/Hooks.hpp>
#include <concurrentqueue.h>

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
    using moodycamel::ProducerToken;
    using moodycamel::ConsumerToken;

    class MiddlewareHooks : public IMiddleware
    {
    public:
        auto set_hook_target(const EMiddlewareHookTarget target) -> void final
        {
            assert_on_imgui_thread();
            if (m_hook_target == target) return;

            const bool should_resume = m_paused;
            stop();
            m_hook_target = target;

            if (should_resume) start();
        }

        [[nodiscard]] auto get_hook_target() const -> EMiddlewareHookTarget final
        {
            assert_on_imgui_thread();
            return m_hook_target;
        }

        auto stop() -> bool override
        {
            assert_on_imgui_thread();
            if (m_paused) return true;
            if (m_prehook_id && !UnregisterCallback(m_prehook_id)) return false;
            if (m_posthook_id && !UnregisterCallback(m_posthook_id)) return false;
            m_prehook_id = m_posthook_id = ERROR_ID;
            m_depth_reset_counter.fetch_add(1, std::memory_order_release);
            m_paused = true;
            return true;
        }

        [[nodiscard]] auto is_paused() const -> bool final
        {
            assert_on_imgui_thread();
            return m_paused;
        }

        auto start() -> bool override
        {
            assert_on_imgui_thread();
            if (!m_paused) return true;
            switch (m_hook_target)
            {
                case EMiddlewareHookTarget::ProcessEvent:
                {
                    m_prehook_id = RegisterProcessEventPreCallback(m_pe_pre, m_options);
                    m_posthook_id = RegisterProcessEventPostCallback(m_pe_post, m_options);
                    break;
                }
                case EMiddlewareHookTarget::ProcessInternal:
                {
                    m_prehook_id = RegisterProcessInternalPreCallback(m_pi_pre, m_options);
                    m_posthook_id = RegisterProcessInternalPostCallback(m_pi_post, m_options);
                    break;
                }
                default: throw std::runtime_error("Unknown hook target");
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

        auto assert_on_imgui_thread() const -> void
        {
            if (std::this_thread::get_id() != m_imgui_id) throw std::runtime_error("assert_on_imgui_thread failed!");
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
        }

        ~MiddlewareHooks() override
        {
            MiddlewareHooks::stop();
        }

    private:
        GlobalCallbackId m_prehook_id = ERROR_ID;
        GlobalCallbackId m_posthook_id = ERROR_ID;
        EMiddlewareHookTarget m_hook_target = EMiddlewareHookTarget::ProcessEvent;
        bool m_paused = true;
        std::thread::id m_imgui_id;

        const FCallbackOptions m_options = {false, true, STR("EventViewer"), STR("CallStackMonitor")};

        const ProcessEventCallbackWithData m_pe_pre = [this](auto&, UObject* context, UFunction* function, ...)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            thread_local uint64_t local_reset_counter = m_depth_reset_counter.load(std::memory_order_acquire);
            if (const auto current_counter = m_depth_reset_counter.load(std::memory_order_acquire); current_counter != local_reset_counter)
            {
                m_pe_depth = 0;
                local_reset_counter = current_counter;
            }
            return enqueue(context, function, m_pe_depth++, thread_id, m_tick_fns.contains(function));
        };

        const ProcessEventCallbackWithData m_pe_post = [](...)
        {
            m_pe_depth = m_pe_depth == 0 ? 0 : m_pe_depth - 1;
        };
// todo install post hook in prehook and return
        const ProcessInternalCallbackWithData m_pi_pre = [this](auto&, UObject* context, FFrame& stack, ...)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            thread_local uint64_t local_reset_counter = m_depth_reset_counter.load(std::memory_order_acquire);
            if (const auto current_counter = m_depth_reset_counter.load(std::memory_order_acquire); current_counter != local_reset_counter)
            {
                m_pe_depth = 0;
                local_reset_counter = current_counter;
            }
            const auto fn = stack.Node(); //TODO figure out if it should be stack.Node() or stack.CurrentNativeFunction()
            return enqueue(context, fn, m_pi_depth++, thread_id, m_tick_fns.contains(fn));
        };

        const ProcessInternalCallbackWithData m_pi_post = [](...)
        {
            m_pi_depth = m_pi_depth == 0 ? 0 : m_pi_depth - 1;
        };

        inline static std::once_flag m_get_tick_fns_flag;
        inline static std::unordered_set<UObject*> m_tick_fns;
        inline static thread_local uint32_t m_pe_depth = 0;
        inline static thread_local uint32_t m_pi_depth = 0;
        inline static std::atomic_uint64_t m_depth_reset_counter = 0;
    };

    class ConcurrentQueueMiddleware : public MiddlewareHooks
    {
    public:
        ConcurrentQueueMiddleware() = default;

        auto enqueue(Unreal::UObject* context, Unreal::UFunction* function, uint32_t depth, std::thread::id thread_id, bool is_tick) -> void override
        {
            thread_local ProducerToken producer_token { m_queue };
            m_queue.enqueue(producer_token, new CallStackEntry {
                context ? context->GetName() : STR("UnknownContext"),
                function ? function->GetName() : STR("UnknownFunction"),
                depth,
                thread_id,
                is_tick
            });
        }

        auto dequeue(uint16_t max_ms,
                     uint16_t max_count_per_iteration, const std::function<void(CallStackEntry*)>& on_dequeue) -> void override
        {
            assert_on_imgui_thread();
            const auto start = std::chrono::steady_clock::now();
            if (m_buffer.size() < max_count_per_iteration) m_buffer.resize(max_count_per_iteration);
            do
            {
                auto amount_dequeued = m_queue.try_dequeue_bulk(m_imgui_consumer_token, m_buffer.begin(), max_count_per_iteration);
                if (!amount_dequeued) break;
                for (size_t i = 0; i < amount_dequeued; ++i) // might run a little over time, but that's ok, the pointers need to be transferred to the imgui thread
                {
                    on_dequeue(m_buffer[i]); // transfers ownership
                }
            } while ((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < max_ms) && m_queue.size_approx());
        }

        auto stop() -> bool override
        {
            if (!MiddlewareHooks::stop()) return false;

            while (m_queue.size_approx())
            {
                const auto count = m_queue.try_dequeue_bulk(m_imgui_consumer_token, m_buffer.begin(), m_buffer.size());
                for (size_t i = 0; i < count; ++i)
                {
                    delete m_buffer[i];
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
        ConcurrentQueue<CallStackEntry*> m_queue;
        ConsumerToken m_imgui_consumer_token { m_queue };
        std::vector<CallStackEntry*> m_buffer {100};
    };

    class MutexMiddleware : public MiddlewareHooks
    {
    public:
        MutexMiddleware() = default;

        auto enqueue(Unreal::UObject* context, Unreal::UFunction* function, uint32_t depth, std::thread::id thread_id, const bool is_tick) -> void override
        {
            std::lock_guard lock(m_mutex);
            m_queue.push(new CallStackEntry(
                context ? context->GetName() : STR("UnknownContext"),
                function ? function->GetName() : STR("UnknownFunction"),
                depth,
                thread_id,
                is_tick
            ));
        }

        auto dequeue(uint16_t max_ms,
                     uint16_t max_count_per_iteration, const std::function<void(CallStackEntry*)>& on_dequeue) -> void override
        {
            assert_on_imgui_thread();
            const auto start = std::chrono::steady_clock::now();

            do
            {
                std::lock_guard lock(m_mutex);
                if (m_queue.empty()) return;
                for (size_t i = 0; i < max_count_per_iteration; ++i)
                {
                    on_dequeue(m_queue.front()); // transfers ownership of pointer
                    m_queue.pop();
                    if (m_queue.empty()) return;
                }
            } while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() < max_ms);
        }

        auto stop() -> bool override
        {
            if (!MiddlewareHooks::stop()) return false;
            std::lock_guard lock(m_mutex);
            while (!m_queue.empty())
            {
                delete m_queue.front();
                m_queue.pop();
            }
            return true;
        }

        [[nodiscard]] auto get_type() const -> EMiddlewareThreadScheme final
        {
            return EMiddlewareThreadScheme::Mutex;
        }

        ~MutexMiddleware() override = default;
    private:
        std::queue<CallStackEntry*> m_queue;
        std::mutex m_mutex;
    };

    auto GetNewMiddleware(const EMiddlewareThreadScheme type) -> std::unique_ptr<IMiddleware>
    {
        switch (type)
        {
            case EMiddlewareThreadScheme::ConcurrentQueue: return std::make_unique<ConcurrentQueueMiddleware>();
            case EMiddlewareThreadScheme::Mutex: return std::make_unique<MutexMiddleware>();
            default: throw std::runtime_error("Unknown middleware type");
        }
    }
}