#pragma once
#include <Middleware.hpp>

namespace RC::EventViewerMod
{
    class Renderer
    {
    public:
        auto render() -> void;

        static auto GetInstance() -> Renderer*;
    private:
        Renderer() = default;

        std::unique_ptr<IMiddleware> m_middleware;
    };
}