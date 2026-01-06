#include <Middleware.hpp>

#include <stdexcept>
#include <mutex>
#include <queue>
#include <unordered_map>

#include <Unreal/Hooks/Hooks.hpp>
#include <concurrentqueue.h>

namespace RC::EventViewerMod
{
    using RC::Unreal::UFunction;
    using RC::Unreal::UObject;
    using RC::Unreal::FFrame;
    using RC::Unreal::Hook::GlobalCallbackId;
    using RC::Unreal::Hook::ERROR_ID;
    using RC::Unreal::Hook::UnregisterHook;
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
        auto set_hook_target(const EMiddlewareHookTarget target) -> void override
        {
            ensure_unhooked();
            clear();
            switch (target)
            {
                case EMiddlewareHookTarget::process_event:
                {
                    m_prehook_id = RegisterProcessEventPreCallback(m_pe_pre, m_options);
                    m_posthook_id = RegisterProcessEventPostCallback(m_pe_post, m_options);
                    break;
                }
                case EMiddlewareHookTarget::process_internal:
                {
                    m_prehook_id = RegisterProcessInternalPreCallback(m_pi_pre, m_options);
                    m_posthook_id = RegisterProcessInternalPostCallback(m_pi_post, m_options);
                    break;
                }
                default: throw std::runtime_error("Unknown hook target");
            }
        }
    protected:
        MiddlewareHooks() = default;
        ~MiddlewareHooks() override
        {
            ensure_unhooked();
        }

    private:
        auto ensure_unhooked() -> void
        {
            if (m_prehook_id) UnregisterHook(m_prehook_id);
            if (m_posthook_id) UnregisterHook(m_posthook_id);
            m_prehook_id = m_posthook_id = ERROR_ID;
            m_depth = 0;
        }

        GlobalCallbackId m_prehook_id = ERROR_ID;
        GlobalCallbackId m_posthook_id = ERROR_ID;
        uint32_t m_depth = 0;

        const FCallbackOptions m_options = {false, true, STR("EventViewer"), STR("PreHookListener")};

        const ProcessEventCallbackWithData m_pe_pre = [this](auto& data, UObject* context, UFunction* function, ...)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            return enqueue(context, function, m_depth++, thread_id);
        };

        const ProcessEventCallbackWithData m_pe_post = [this](...)
        {
            m_depth = m_depth == 0 ? 0 : m_depth - 1;
        };

        const ProcessInternalCallbackWithData m_pi_pre = [this](auto& data, UObject* context, FFrame& stack, ...)
        {
            thread_local std::thread::id thread_id = std::this_thread::get_id();
            return enqueue(context, stack.Node(), m_depth++, thread_id); //TODO figure out if it should be stack.Node() or stack.CurrentNativeFunction()
        };

        const ProcessInternalCallbackWithData m_pi_post = [this](...)
        {
            m_depth = m_depth == 0 ? 0 : m_depth - 1;
        };
    };

    class ConcurrentQueueMiddleware : public MiddlewareHooks
    {
    public:
        ConcurrentQueueMiddleware() = default;

        auto enqueue(Unreal::UObject* context, Unreal::UFunction* function, uint32_t depth, std::thread::id thread_id) -> void override
        {
            thread_local ProducerToken producer_token { m_queue };
            m_queue.enqueue(producer_token, new CallStackEntry {
                context ? context->GetName() : STR("UnknownContext"),
                function ? function->GetName() : STR("UnknownFunction"),
                depth,
                thread_id
            });
        }

        auto dequeue_for_max_time(const std::chrono::duration<std::chrono::milliseconds>& max_ms,
                std::function<void(CallStackEntry*)> on_dequeue) -> void override
        {
            thread_local ConsumerToken consumer_token { m_queue };

        }

        auto clear() -> void override
        {

        }

        ~ConcurrentQueueMiddleware() override = default;

    private:
        ConcurrentQueue<CallStackEntry*> m_queue;
        std::unordered_map<std::thread::id, ProducerToken> m_producer_tokens; //TODO might need to RCU this, or get rid of it
    };

    class MutexMiddleware : public MiddlewareHooks
    {
    public:
        MutexMiddleware() = default;

        auto enqueue(Unreal::UObject* context, Unreal::UFunction* function, uint32_t depth, std::thread::id thread_id) -> void override
        {

        }

        auto dequeue_for_max_time(const std::chrono::duration<std::chrono::milliseconds>& max_ms,
                std::function<void(CallStackEntry*)> on_dequeue) -> void override
        {

        }

        auto clear() -> void override
        {

        }

        ~MutexMiddleware() override = default;
    };

    auto GetNewMiddleware(const EMiddlewareThreadScheme type) -> std::unique_ptr<IMiddleware>
    {
        switch (type)
        {
            case EMiddlewareThreadScheme::cqueue: return std::make_unique<ConcurrentQueueMiddleware>();
            case EMiddlewareThreadScheme::mutex: return std::make_unique<MutexMiddleware>();
            default: throw std::runtime_error("Unknown middleware type");
        }
    }
}