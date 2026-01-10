#pragma once
#include <fstream>
#include <filesystem>
#include <Middleware.hpp>
#include <Structs.h>

#include "UE4SSProgram.hpp"

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
        Client();

        auto render_cfg() -> void;
        auto render_perf_opts() -> void;
        auto render_view() -> void;

        auto save_state() -> void;
        auto load_state() -> void;
        auto check_save_request() -> bool;

        auto apply_filters_to_history(bool whitelist_changed, bool blacklist_changed, bool tick_changed) -> void;
        auto dequeue() -> void;

        auto passes_filters(const std::string& test_str) const -> bool;

        enum class ESaveMode { none, current, all };
        auto save(ESaveMode mode) -> void;
        auto serialize_view(ThreadInfo& info, EMode mode, std::ofstream& out) const -> void;
        auto serialize_all_views(std::ofstream& out) -> void;

        UIState m_state;
        std::unique_ptr<IMiddleware> m_middleware = GetNewMiddleware(EMiddlewareThreadScheme::ConcurrentQueue);

        const std::filesystem::path m_cfg_path = StringType{UE4SSProgram::get_program().get_working_directory()} + fmt::format(STR("\\Mods\\EventViewerMod\\config\\settings.json"));
        const std::filesystem::path m_dump_path = StringType{UE4SSProgram::get_program().get_working_directory()};
    };
}