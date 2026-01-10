#include "../include/Client.h"

#include <execution>
#include <ranges>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <string_view>
#include <unordered_map>

#include <UE4SSProgram.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <glaze/glaze.hpp>

std::vector<std::string_view> split_string_by_comma(const std::string& string)
{
    std::vector<std::string_view> result;

    if (string.empty()) return result;

    auto split = std::views::split(string, ',');

    for (auto substring : split)
    {
        auto view = std::string_view(substring);

        // trim leading whitespace
        const auto leading = view.find_first_not_of(" \t\n\r\f\v");
        if (leading == std::string_view::npos) continue; // blank string
        view = view.substr(leading);

        //trim trailing whitespace
        const auto trailing = view.find_last_not_of(" \t\n\r\f\v");
        if (trailing == std::string_view::npos) continue; //blank string
        view = view.substr(0, trailing + 1);

        result.emplace_back(view);
    }

    return result;
}

namespace RC::EventViewerMod
{
    using namespace std::literals::string_literals;

    Client::Client()
    {
        load_state();
    }

    auto Client::render() -> void
    {
        UE4SS_ENABLE_IMGUI();

        const auto saved = check_save_request();

        if (ImGui::Checkbox("Enable", &m_state.enabled))
        {
            // disabled -> enabled
            if (m_state.enabled)
            {
                load_state();
                m_middleware->set_imgui_thread_id(std::this_thread::get_id());
            }

            // enabled -> disabled, any non-savables that need to reset should be reset
            else
            {
                m_middleware->stop();
                m_state.started = false;
                m_state.needs_save.clear();
                for (auto & target : m_state.targets) target.clear();
                if (!saved) save_state();
                return;
            }
        }

        render_cfg();

        if (ImGui::TreeNode("Performance Options")) render_perf_opts();

        dequeue();

        render_view();
    }

    auto Client::render_cfg() -> void
    {
        if (ImGui::Combo("Target", reinterpret_cast<int*>(&m_state.hook_target), EMiddlewareHookTarget_NameArray, EMiddlewareHookTarget_Size))
        {
            m_middleware->set_hook_target(m_state.hook_target);
        }

        ImGui::SameLine();

        ImGui::Combo("Mode", reinterpret_cast<int*>(&m_state.mode), EMode_NameArray, EMode_Size);

        bool whitelist_changed = false;
        bool blacklist_changed = false;
        bool tick_changed = false;

        // whitelist
        const auto _wl_input_changed = ImGui::InputText("Whitelist", &m_state.whitelist, ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (_wl_input_changed || ImGui::Button("Apply"))
        {
            whitelist_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            m_state.whitelist.clear();
            whitelist_changed = true;
        }

        // blacklist
        const auto _bl_input_changed = ImGui::InputText("Blacklist", &m_state.blacklist, ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (_bl_input_changed || ImGui::Button("Apply"))
        {
            blacklist_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            m_state.blacklist.clear();
            blacklist_changed = true;
        }

        //thread selector
        auto& target = m_state.targets[static_cast<int>(m_state.hook_target)];
        auto& threads = target.threads;
        const bool has_threads = !threads.empty();
        if (has_threads)
        {
            if (ImGui::BeginCombo("Thread", threads[target.current_thread].id_string(), ImGuiComboFlags_WidthFitPreview))
            {
                for (size_t idx = 0; idx < threads.size(); ++idx)
                {
                    const bool selected = idx == target.current_thread;
                    if (ImGui::Selectable(threads[idx].id_string(), selected))
                    {
                        target.current_thread = idx;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        auto save_mode = ESaveMode::none;

        // controls
        if (ImGui::Button(m_state.started ? "Stop" : "Start"))
        {
            m_state.started = !m_state.started;
            m_state.started ? m_middleware->start() : m_middleware->stop();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear") && has_threads)
        {
            auto& thread = threads[target.current_thread];
            thread.call_frequencies.clear();
            thread.call_stack.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All") && has_threads)
        {
            for (auto& thread : threads)
            {
                thread.call_stack.clear();
                thread.call_frequencies.clear();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            save_mode = ESaveMode::current;
        }
        ImGui::SameLine();
        if (ImGui::Button("Save All"))
        {
            save_mode = ESaveMode::all;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Show Tick Functions", &m_state.show_tick))
        {
            tick_changed = true;
        }

        apply_filters_to_history(whitelist_changed, blacklist_changed, tick_changed);
        save(save_mode);
    }

    auto Client::render_perf_opts() -> void
    {
        if (ImGui::Combo("Thread Scheme", reinterpret_cast<int*>(&m_state.thread_scheme), EMiddlewareThreadScheme_NameArray, EMiddlewareThreadScheme_Size))
        {
            if (m_middleware->get_type() != m_state.thread_scheme)
            {
                const bool should_restart = !m_middleware->is_paused();
                m_middleware->stop();
                m_middleware = GetNewMiddleware(m_state.thread_scheme);
                m_middleware->set_hook_target(m_state.hook_target);
                if (should_restart) m_middleware->start();
            }
        }

        ImGui::SameLine();
        static uint16_t step = 1;
        if (ImGui::InputScalar("Max MS Read Time", ImGuiDataType_U16, &m_state.dequeue_max_ms, &step))
        {
            if (!m_state.dequeue_max_ms) m_state.dequeue_max_ms = 1;
        }

        ImGui::SameLine();
        if (ImGui::InputScalar("Max Count Per Iteration", ImGuiDataType_U16, &m_state.dequeue_max_count, &step))
        {
            if (!m_state.dequeue_max_count) m_state.dequeue_max_count = 1;
        }
    }

    auto Client::render_view() -> void
    {
        auto& target = m_state.targets[static_cast<int>(m_state.hook_target)];
        if (target.threads.empty()) return;
        auto& thread = target.threads[target.current_thread];

        ImGui::BeginChild("##view");

        if (m_state.mode == EMode::Stack)
        {
            for (const auto& entry : thread.call_stack)
            {
                ImGui::Text(entry->text.c_str());
            }
        }
        else
        {
            if (ImGui::BeginTable("Frequency", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV))
            {
                for (const auto& entry : thread.call_frequencies)
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(entry->text.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(std::to_string(entry->frequency).c_str());
                }

                ImGui::EndTable();
            }
        }

        ImGui::EndChild();
    }

    auto Client::save_state() -> void
    {
        std::unordered_map<std::string, std::string> state_map;
        state_map.emplace("Enabled", std::to_string(m_state.enabled));
        state_map.emplace("ShowTick", std::to_string(m_state.show_tick));
        state_map.emplace("HookTarget", std::to_string(static_cast<int>(m_state.hook_target)));
        state_map.emplace("ThreadScheme", std::to_string(static_cast<int>(m_state.thread_scheme)));
        state_map.emplace("Mode", std::to_string(static_cast<int>(m_state.mode)));
        state_map.emplace("DequeueMaxMs", std::to_string(m_state.dequeue_max_ms));
        state_map.emplace("DequeueMaxCount", std::to_string(m_state.dequeue_max_count));
        state_map.emplace("Whitelist", m_state.whitelist);
        state_map.emplace("Blacklist", m_state.blacklist);
        auto ec = glz::write_file_json(state_map, m_cfg_path.string(), std::string{});
    }

    auto Client::load_state() -> void
    {
        if (!std::filesystem::exists(m_cfg_path) || !std::filesystem::is_regular_file(m_cfg_path)) return;
        std::unordered_map<std::string, std::string> state_map{};
        auto ec = glz::read_file_json(state_map, m_cfg_path.string(), std::string{});
        if (ec.ec != glz::error_code::none) return;
        try
        {
            m_state.enabled = state_map.at("Enabled") != "0";
            m_state.show_tick = state_map.at("ShowTick") != "0";
            m_state.hook_target = static_cast<EMiddlewareHookTarget>(std::stoi(state_map.at("HookTarget")));
            m_state.thread_scheme = static_cast<EMiddlewareThreadScheme>(std::stoi(state_map.at("ThreadScheme")));
            m_state.mode = static_cast<EMode>(std::stoi(state_map.at("Mode")));
            m_state.dequeue_max_ms = std::stoi(state_map.at("DequeueMaxMs"));
            m_state.dequeue_max_count = std::stoi(state_map.at("DequeueMaxCount"));
            m_state.whitelist = state_map.at("Whitelist");
            m_state.blacklist = state_map.at("Blacklist");
            m_state.whitelist_tokens = split_string_by_comma(m_state.whitelist);
            m_state.blacklist_tokens = split_string_by_comma(m_state.blacklist);
        }
        catch (...) {}
    }

    auto Client::check_save_request() -> bool
    {
        if (m_state.needs_save.test(std::memory_order_acquire))
        {
            save_state();
            m_state.needs_save.clear();
            return true;
        }
        return false;
    }

    // if there's only a whitelist and no blacklist, only entries that have a whitelist token as a substring are allowed
    // if there's only a blacklist and no whitelist, only entries that don't have a blacklist token as a substring are allowed
    // if there's both, then the whitelist applies before the blacklist
    auto Client::apply_filters_to_history(bool whitelist_changed, bool blacklist_changed, bool tick_changed) -> void
    {
        if (whitelist_changed)
        {
            m_state.whitelist_tokens = split_string_by_comma(m_state.whitelist);
        }
        if (blacklist_changed)
        {
            m_state.blacklist_tokens = split_string_by_comma(m_state.blacklist);
        }

        auto filter_fn = [this, whitelist_changed, blacklist_changed, tick_changed](const auto& entry) {
            if (tick_changed)
            {
                if (entry->is_tick)
                {
                    entry->is_disabled = m_state.show_tick;
                    if (!m_state.show_tick) return;
                }
            }
            if (!whitelist_changed && !blacklist_changed) return; // neither list changed, just tick, so early out
            if (whitelist_changed && !blacklist_changed) // if only the whitelist changed, we only need to look at disabled entries to potentially enable them
            {
                if (!entry->is_disabled) return;
                entry->is_disabled = passes_filters(entry->text);
            }
            else if (!whitelist_changed && blacklist_changed) // if only the blacklist changed, we only need to look at the enabled entries to potentially disable them
            {
                if (entry->is_disabled) return;
                entry->is_disabled = passes_filters(entry->text);
            }
            else // both somehow changed, reapply to all filters
            {
                entry->is_disabled = passes_filters(entry->text);
            }
        };

        for (auto& [threads, current_thread]: m_state.targets)
        {
            for (auto& thread : threads)
            {
                std::for_each(std::execution::par_unseq, thread.call_stack.begin(), thread.call_stack.end(), filter_fn);
                std::for_each(std::execution::par_unseq, thread.call_frequencies.begin(), thread.call_frequencies.end(), filter_fn);
            }
        }
    }

    // must be called after render_cfg so whitelist/blacklist tokens are populated
    auto Client::dequeue() -> void
    {
        m_middleware->dequeue(m_state.dequeue_max_ms, m_state.dequeue_max_count, [this](CallStackEntry* entry) {
            // resolve which lists the entry should go in
            const auto& entry_thread = entry->thread_id;
            const auto enum_target = m_middleware->get_hook_target();
            auto& target = m_state.targets[static_cast<int>(enum_target)];
            auto& target_threads = target.threads;
            auto thread_it = std::ranges::find_if(target_threads, [&entry_thread](const auto& info){ return info.thread_id == entry_thread; });
            auto& thread = thread_it != target_threads.end() ? *thread_it : target_threads.emplace_back(entry_thread);

            // set whether it should be disabled given the current filters
            const auto disabled = (entry->is_tick && !m_state.show_tick) || !passes_filters(entry->text);

            // add to frequency tracking
            auto freq_it = std::ranges::find_if(thread.call_frequencies, [entry](const auto& freq_entry) -> bool { return entry->text == freq_entry->text; });
            if (freq_it != thread.call_frequencies.end()) [[likely]]
            {
                auto& freq = *freq_it;
                ++freq->frequency;
                freq->is_disabled = disabled; // might as well update its disabled field since we know it

                //move up as needed, searching from before the freq entry to beginning of list for first entry that's greater than freq's frequency (very fast)
                if (freq_it != thread.call_frequencies.begin())
                {
                    auto before_freq_it = freq_it;
                    --before_freq_it;
                    const auto current_before_freq_it = before_freq_it;
                    auto limit = --thread.call_frequencies.begin(); //work around ranges iterator not being compatible with rbegin
                    for (; before_freq_it != limit; --before_freq_it)
                    {
                        if ((*before_freq_it)->frequency > freq->frequency) break;
                    }
                    if (current_before_freq_it != before_freq_it)
                    {
                        auto freq_holder = std::move(freq);
                        thread.call_frequencies.erase(freq_it);
                        thread.call_frequencies.insert(before_freq_it, std::move(freq_holder));
                    }
                }
            }
            else
            {
                thread.call_frequencies.emplace_back(std::make_unique<CallFrequencyEntry>(entry->text, entry->is_tick))->is_disabled = disabled;
            }

            // add to call stack trace
            thread.call_stack.emplace_back(entry)->is_disabled = disabled;
        });
    }

    auto Client::passes_filters(const std::string& test_str) const -> bool
    {
        auto passes_whitelist = m_state.whitelist_tokens.empty();
        for (const auto& token : m_state.whitelist_tokens) //only one whitelist token needs to be present in the test_str to pass
        {
            if (test_str.contains(token))
            {
                passes_whitelist = true;
                break;
            }
        }
        if (!passes_whitelist) return false;

        for (const auto& token : m_state.blacklist_tokens) //only one blacklist token needs to be present in the test_str to fail
        {
            if (test_str.contains(token)) return false;
        }
        return true;
    }

    auto Client::save(ESaveMode mode) -> void
    {
        if (mode == ESaveMode::none) return;

        const auto now = std::chrono::system_clock::now();
        const std::time_t now_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm{};
        localtime_s(&local_tm, &now_t);
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
        std::ofstream file{};

        if (mode == ESaveMode::current)
        {
            auto& target = m_state.targets[static_cast<int>(m_state.hook_target)];
            if (target.threads.empty()) return;
            file.open(m_dump_path / ("EventViewerMod Capture-"s
                + EMiddlewareHookTarget_NameArray[static_cast<int>(m_state.hook_target)]
                + "-" + EMode_NameArray[static_cast<int>(m_state.mode)] + " "
                + oss.str()));
            file << EMiddlewareHookTarget_NameArray[static_cast<int>(m_state.hook_target)] << " ";
            serialize_view(target.threads[target.current_thread], m_state.mode, file);
            return file.close();
        }

        if (mode == ESaveMode::all)
        {
            for (const auto& target : m_state.targets)
            {
                if (target.threads.empty()) return;
            }

            file.open(m_dump_path / ("EventViewerMod Capture-All "s + oss.str()));
            serialize_all_views(file);
            return file.close();
        }
    }

    auto Client::serialize_view(ThreadInfo& info, EMode mode, std::ofstream& out) const -> void
    {
        auto serialize_fn = [&out](auto& vector) {
            if (vector.empty())
            {
                out << "No captures.\n\n\n" << std::endl;
                return;
            }

            for (auto& entry : vector)
            {
                out << entry->text << std::endl;
            }

            out << "\n\n\n";
        };
        out << fmt::format("Thread {} {}\n\n", info.id_string(), EMode_NameArray[static_cast<int>(mode)]);
        mode == EMode::Stack ? serialize_fn(info.call_stack) : serialize_fn(info.call_frequencies);
    }

    auto Client::serialize_all_views(std::ofstream& out) -> void
    {
        for (int target_idx = 0; target_idx < m_state.targets.size(); ++target_idx)
        {
            auto& target = m_state.targets[target_idx];
            for (auto& thread : target.threads)
            {
                out << EMiddlewareHookTarget_NameArray[target_idx] << " ";
                serialize_view(thread, EMode::Stack, out);
                out << EMiddlewareHookTarget_NameArray[target_idx] << " ";
                serialize_view(thread, EMode::Frequency, out);
            }
        }
    }

    auto Client::request_save_state() -> void
    {
        m_state.needs_save.test_and_set(std::memory_order_release);
    }
}