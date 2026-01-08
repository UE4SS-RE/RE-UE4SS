#include <Middleware.hpp>

#include <stdexcept>
#include <mutex>
#include <queue>
#include <atomic>

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

        [[nodiscard]] auto get_imgui_thread_id() const -> std::thread::id
        {
            return m_imgui_id;
        }

        auto assert_on_imgui_thread() const -> void
        {
            if (std::this_thread::get_id() != m_imgui_id) throw std::runtime_error("assert_on_imgui_thread failed!");
        }

    protected:
        MiddlewareHooks() = default;
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
            return enqueue(context, function, m_pe_depth++, thread_id);
        };

        const ProcessEventCallbackWithData m_pe_post = [](...)
        {
            m_pe_depth = m_pe_depth == 0 ? 0 : m_pe_depth - 1;
        };

        const ProcessInternalCallbackWithData m_pi_pre = [this](auto&, UObject* context, FFrame& stack, ...)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            thread_local uint64_t local_reset_counter = m_depth_reset_counter.load(std::memory_order_acquire);
            if (const auto current_counter = m_depth_reset_counter.load(std::memory_order_acquire); current_counter != local_reset_counter)
            {
                m_pe_depth = 0;
                local_reset_counter = current_counter;
            }
            return enqueue(context, stack.Node(), m_pi_depth++, thread_id); //TODO figure out if it should be stack.Node() or stack.CurrentNativeFunction()
        };

        const ProcessInternalCallbackWithData m_pi_post = [](...)
        {
            m_pi_depth = m_pi_depth == 0 ? 0 : m_pi_depth - 1;
        };

        inline static thread_local uint32_t m_pe_depth = 0;
        inline static thread_local uint32_t m_pi_depth = 0;
        inline static std::atomic_uint64_t m_depth_reset_counter = 0;
    };

    class ConcurrentQueueMiddleware : public MiddlewareHooks
    {
    public:
        ConcurrentQueueMiddleware() = default;

        auto enqueue(Unreal::UObject* context, Unreal::UFunction* function, uint32_t depth, std::thread::id thread_id) -> void override
        {
            thread_local ProducerToken producer_token { m_queue };
            m_queue.enqueue(producer_token, new CallStackEntry {
                context ? std::move(context->GetName()) : STR("UnknownContext"),
                function ? std::move(function->GetName()) : STR("UnknownFunction"),
                depth,
                thread_id
            });
        }

        auto dequeue(const std::chrono::milliseconds& max_ms,
                     const size_t max_count_per_iteration, const std::function<void(CallStackEntry*)>& on_dequeue) -> void override
        {
            assert_on_imgui_thread();
            const auto start = std::chrono::steady_clock::now();
            std::vector<CallStackEntry*> entries{ max_count_per_iteration };
            do
            {
                auto amount_dequeued = m_queue.try_dequeue_bulk(m_imgui_consumer_token, entries.begin(), max_count_per_iteration);
                if (!amount_dequeued) break;
                for (size_t i = 0; i < amount_dequeued; ++i)
                {
                    on_dequeue(entries[i]); // transfers ownership
                }
            } while (((std::chrono::steady_clock::now() - start) < max_ms) && m_queue.size_approx());
        }

        auto stop() -> bool override
        {
            if (!MiddlewareHooks::stop()) return false;

            std::vector<CallStackEntry*> entries{ 100 };

            while (m_queue.size_approx())
            {
                const auto count = m_queue.try_dequeue_bulk(m_imgui_consumer_token, entries.begin(), 100);
                for (size_t i = 0; i < count; ++i)
                {
                    delete entries[i];
                }
            }

            return true;
        }

        ~ConcurrentQueueMiddleware() override = default;

    private:
        ConcurrentQueue<CallStackEntry*> m_queue;
        ConsumerToken m_imgui_consumer_token { m_queue };
    };

    class MutexMiddleware : public MiddlewareHooks
    {
    public:
        MutexMiddleware() = default;

        auto enqueue(Unreal::UObject* context, Unreal::UFunction* function, uint32_t depth, std::thread::id thread_id) -> void override
        {
            std::lock_guard lock(m_mutex);
            m_queue.push(new CallStackEntry {
                context ? std::move(context->GetName()) : STR("UnknownContext"),
                function ? std::move(function->GetName()) : STR("UnknownFunction"),
                depth,
                thread_id
            });
        }

        auto dequeue(const std::chrono::milliseconds& max_ms,
                     const size_t max_count_per_iteration, const std::function<void(CallStackEntry*)>& on_dequeue) -> void override
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
            } while ((std::chrono::steady_clock::now() - start) < max_ms);
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