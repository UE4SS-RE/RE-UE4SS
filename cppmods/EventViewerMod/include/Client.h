#pragma once
#include <Middleware.hpp>
#include <Structs.h>

namespace RC::EventViewerMod
{
    class Client
    {
    public:
        // [Thread-ImGui]
        auto render() -> void;

        // [Thread-Any] Saves state on the next frame
        auto request_save_state() -> void;

        // [Thread-Any]
        static auto GetInstance() -> Client&;
    private:
        Client() = default;

        auto render_cfg() -> void;
        auto render_perf_opts() -> void;
        auto render_view() -> void;

        auto save_state() -> void;
        auto load_state() -> void;
        auto check_save_request() -> bool;

        auto apply_filters_to_history(bool whitelist_changed, bool blacklist_changed, bool tick_changed) -> void;
        auto dequeue() -> void;

        auto passes_filters(const std::string& test_str) const -> bool;

        UIState m_state;
        std::unique_ptr<IMiddleware> m_middleware = GetNewMiddleware(EMiddlewareThreadScheme::ConcurrentQueue);
    };
}