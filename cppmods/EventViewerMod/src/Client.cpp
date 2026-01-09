#include "../include/Client.h"

#include <span>

#include <UE4SSProgram.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

namespace RC::EventViewerMod
{

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
                m_middleware->set_imgui_thread_id(std::this_thread::get_id()); //TODO test to make sure thread id doesnt change when closing and reopening console
            }

            // enabled -> disabled, TODO any non-savable values should be reset
            else
            {
                m_middleware->stop();
                m_state.started = false;
                //for (auto& entry : m_state.entries) entry.clear();
                if (!saved) save_state();
                return;
            }
        }

        render_cfg();

        if (ImGui::TreeNode("Performance Options")) render_perf_opts();

        dequeue();

        render_view();
    }

    auto Client::render_cfg() -> void //TODO may need to take in a bool arg for init to signal to apply current regardless of input
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

        if (ImGui::InputText("Whitelist", &m_state.whitelist, ImGuiInputTextFlags_ElideLeft))
        {
            whitelist_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) // TODO maybe also add 'apply' button?
        {
            m_state.whitelist.clear();
            whitelist_changed = true;
        }

        if (ImGui::InputText("Blacklist", &m_state.blacklist, ImGuiInputTextFlags_ElideLeft))
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
            //TODO save thread's filtered callstack/frequency
        }
        ImGui::SameLine();
        if (ImGui::Button("Save All"))
        {
            //TODO save thread's filtered callstack/frequency
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Show Tick Functions", &m_state.show_tick))
        {
            tick_changed = true;
        }

        apply_filters_to_history(whitelist_changed, blacklist_changed, tick_changed);
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
        //TODO use begin child
    }

    auto Client::save_state() -> void
    {
    }

    auto Client::load_state() -> void
    {
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

    auto Client::apply_filters_to_history(bool whitelist_changed, bool blacklist_changed, bool tick_changed) -> void
    {
    }

    // must be called after render_cfg so whitelist/blacklist tokens are populated
    auto Client::dequeue() -> void
    {
        m_middleware->dequeue(m_state.dequeue_max_ms, m_state.dequeue_max_count, [this](CallStackEntry* entry) {
            const auto& entry_thread = entry->thread_id;
            const auto enum_target = m_middleware->get_hook_target();
            auto& target = m_state.targets[static_cast<int>(enum_target)];
            auto& target_threads = target.threads;
            auto thread_it = std::ranges::find_if(target_threads, [&entry_thread](const auto& info){ return info.thread_id == entry_thread; });
            const auto is_thread_iterator_valid = thread_it != target_threads.end();
            if (!is_thread_iterator_valid) [[unlikely]]
            {
                target_threads.emplace_back(entry_thread);
            }
            auto& thread = is_thread_iterator_valid ? *thread_it : target_threads.back();

            const auto disabled = !passes_filters(entry->entry_as_std_string());
            if (thread.call_stack.empty()) [[unlikely]]
            {
                thread.call_frequencies.emplace_back(std::make_unique<CallFrequencyEntry>(entry->entry_as_string()));
                thread.call_frequencies.back()->is_disabled = disabled;
                thread.call_stack.emplace_back(entry);
                thread.call_stack.back()->is_disabled = disabled;
                return;
            }

            // add to frequency tracking
            auto freq_it = std::ranges::find_if(thread.call_frequencies, [entry](const auto& freq_entry) -> bool { return entry->entry_as_std_string() == freq_entry->function_name; });
            if (freq_it != thread.call_frequencies.end()) [[likely]]
            {
                auto& freq = *freq_it;
                ++freq->frequency;
                //TODO swap up as needed
            }
            else
            {
                thread.call_frequencies.emplace_back(std::make_unique<CallFrequencyEntry>(entry->entry_as_string()));
                thread.call_frequencies.back()->is_disabled = disabled;
            }

            // add to call stack trace
            thread.call_stack.emplace_back(entry);
            thread.call_stack.back()->is_disabled = disabled;
        });
    }

    auto Client::passes_filters(const std::string& test_str) const -> bool //TODO fix: only needs to match one wl/bl token, not all
    {
        for (const auto& token : m_state.whitelist_tokens)
        {
            if (test_str.contains(token)) return false;
        }
        for (const auto& token : m_state.blacklist_tokens)
        {
            if (test_str.contains(token)) return false;
        }
        return true;
    }

    auto Client::request_save_state() -> void
    {
        m_state.needs_save.test_and_set(std::memory_order_release);
    }
}