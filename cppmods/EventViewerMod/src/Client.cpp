#include <Client.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <execution>
#include <ctime>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_map>

#include <UE4SSProgram.hpp>

#include <fmt/core.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <EntryCallStackRenderer.hpp>
#include <glaze/glaze.hpp>

#include <QueueProfiler.hpp>
#include <HelpStrings.hpp>


// ASCII-only lowercasing for case-insensitive filtering.
// (Unreal names are typically ASCII; if this becomes a problem we'll revisit.)
static auto to_lower_ascii_copy(std::string_view s) -> std::string
{
    std::string out;
    out.reserve(s.size());
    for (const unsigned char ch : s)
    {
        out.push_back(static_cast<char>(std::tolower(ch)));
    }
    return out;
}


// Returns lower-cased tokens (copied strings).
static std::vector<std::string> split_string_by_comma(const std::string& string)
{
    std::vector<std::string> result;
    if (string.empty())
    {
        return result;
    }

    const std::string_view sv{string};
    size_t start = 0;

    auto trim = [](std::string_view v) -> std::string_view {
        const auto leading = v.find_first_not_of(" \t\n\r\f\v");
        if (leading == std::string_view::npos)
        {
            return {};
        }
        v.remove_prefix(leading);

        const auto trailing = v.find_last_not_of(" \t\n\r\f\v");
        if (trailing == std::string_view::npos)
        {
            return {};
        }
        v = v.substr(0, trailing + 1);
        return v;
    };

    while (start <= sv.size())
    {
        size_t end = sv.find(',', start);
        if (end == std::string_view::npos)
        {
            end = sv.size();
        }

        auto token = trim(sv.substr(start, end - start));
        if (!token.empty())
        {
            result.emplace_back(to_lower_ascii_copy(token));
        }

        if (end == sv.size())
        {
            break;
        }
        start = end + 1;
    }

    return result;
}

namespace RC::EventViewerMod
{
    using namespace std::literals::string_literals;

    Client::Client() : m_middleware(Middleware::GetInstance())
    {
        const auto wd = std::filesystem::path{StringType{UE4SSProgram::get_program().get_working_directory()}};
        const auto mod_root = wd / "Mods" / "EventViewerMod";

        m_cfg_path = mod_root / "config" / "settings.json";
        m_dump_dir = mod_root / "captures";

        std::error_code ec;
        std::filesystem::create_directories(m_cfg_path.parent_path(), ec);
        std::filesystem::create_directories(m_dump_dir, ec);

        load_state();
    }

    auto Client::render() -> void
    {
        // Ensure middleware knows the correct ImGui thread, including after scheme swaps.
        if (!m_imgui_thread_id_set)
        {
            m_middleware.set_imgui_thread_id(std::this_thread::get_id());
            m_imgui_thread_id_set = true;
        }

        const auto saved = check_save_request();

        if (ImGui::Checkbox("Enable", &m_state.enabled))
        {
            request_save_state();

            if (m_state.enabled)
            {
                // enabling
                m_middleware.set_imgui_thread_id(std::this_thread::get_id());
                m_imgui_thread_id_set = true;
                QueueProfiler::Reset();
            }
            else
            {
                // disabling
                m_middleware.stop();
                m_state.started = false;
                m_state.needs_save.clear(std::memory_order_release);
                clear_threads();
                if (!saved)
                {
                    save_state();
                }
                QueueProfiler::Reset();
                return;
            }
        }

        if (!m_state.enabled) return;

        render_cfg();

        if (ImGui::TreeNode("Performance Options"))
        {
            render_perf_opts();
            ImGui::TreePop();
        }

        dequeue();

        render_view();

        if (ImGui::BeginPopupModal("Saved File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::Text("Saved file to %s", m_state.last_save_path.c_str());
            ImGui::PopTextWrapPos();
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    auto Client::render_cfg() -> void
    {
        int hook_target_idx = 0;
        for (int i = 0; i < EMiddlewareHookTarget_Size; ++i)
        {
            if (EMiddlewareHookTarget_ValueArray[i] == m_state.hook_target)
            {
                hook_target_idx = i;
                break;
            }
        }

        if (combo_with_flags("Target", &hook_target_idx, EMiddlewareHookTarget_NameArray, EMiddlewareHookTarget_Size, ImGuiComboFlags_WidthFitPreview))
        {
            request_save_state();
            m_state.hook_target = EMiddlewareHookTarget_ValueArray[hook_target_idx];
        }
        HelpMarker(HelpStrings::HELP_TARGET);
        ImGui::SameLine();

        if (combo_with_flags("Mode", reinterpret_cast<int*>(&m_state.mode), EMode_NameArray, EMode_Size, ImGuiComboFlags_WidthFitPreview))
        {
            request_save_state();
        }
        HelpMarker(HelpStrings::HELP_MODE);

        bool whitelist_changed = false;
        bool blacklist_changed = false;
        bool tick_changed = false;

        // whitelist
        const auto wl_input_changed = ImGui::InputText("Whitelist", &m_state.whitelist, ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (wl_input_changed || ImGui::Button("Apply##Whitelist"))
        {
            whitelist_changed = true;
            request_save_state();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##Whitelist"))
        {
            if (!m_state.whitelist.empty())
            {
                m_state.whitelist.clear();
                whitelist_changed = true;
                request_save_state();
            }
        }
        HelpMarker(HelpStrings::HELP_LIST_FILTER);

        // blacklist
        const auto bl_input_changed = ImGui::InputText("Blacklist", &m_state.blacklist, ImGuiInputTextFlags_ElideLeft | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (bl_input_changed || ImGui::Button("Apply##Blacklist"))
        {
            blacklist_changed = true;
            request_save_state();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear##Blacklist"))
        {
            if (!m_state.blacklist.empty())
            {
                m_state.blacklist.clear();
                blacklist_changed = true;
                request_save_state();
            }
        }
        auto& threads = m_state.threads;

        if (!threads.empty())
        {
            if (m_state.current_thread < 0)
            {
                m_state.current_thread = 0;
            }
            if (static_cast<size_t>(m_state.current_thread) >= threads.size())
            {
                m_state.current_thread = static_cast<int>(threads.size() - 1);
            }

            ThreadInfo* game_thread = nullptr;
            if (ImGui::BeginCombo("Thread", threads[m_state.current_thread].id_string(), ImGuiComboFlags_WidthFitPreview))
            {
                for (size_t idx = 0; idx < threads.size(); ++idx)
                {
                    const bool selected = static_cast<int>(idx) == m_state.current_thread;
                    auto& thread = threads[idx];
                    if (ImGui::Selectable(thread.id_string(), selected))
                    {
                        m_state.thread_explicitly_chosen = true;
                        m_state.current_thread = static_cast<int>(idx);
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                    if (thread.is_game_thread)
                    {
                        game_thread = &thread;
                    }
                }
                ImGui::EndCombo();
            }
            else if (!m_state.thread_implicitly_set)
            {
                for (auto& thread : threads)
                {
                    if (thread.is_game_thread) game_thread = &thread;
                }

                if (game_thread && !m_state.thread_explicitly_chosen)
                {
                    m_state.current_thread = static_cast<int>(game_thread - threads.data());
                    ImGui::SetItemDefaultFocus();
                    m_state.thread_implicitly_set = true;
                }
            }
            HelpMarker(HelpStrings::HELP_THREAD);
        }

        auto save_mode = ESaveMode::none;

        // controls
        if (ImGui::Button(m_state.started ? "Stop" : "Start"))
        {
            m_state.started = !m_state.started;
            if (m_state.started)
            {
                m_middleware.start();
                m_state.text_temp_virtualization_count = m_state.text_virtualization_count;
            }
            else
            {
                m_middleware.stop();
            }
            QueueProfiler::Reset();
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear##CurrentThread") && !threads.empty())
        {
            auto& thread = threads[m_state.current_thread];
            thread.call_frequencies.clear();
            thread.call_stack.clear();
        }
        HelpMarker(HelpStrings::HELP_CLEAR);
        ImGui::SameLine();
        if (ImGui::Button("Clear All##AllThreads") && !threads.empty())
        {
            clear_threads();
        }
        HelpMarker(HelpStrings::HELP_CLEAR_ALL);
        ImGui::SameLine();
        if (ImGui::Button("Save##Current"))
        {
            save_mode = ESaveMode::current;
        }
        HelpMarker(HelpStrings::HELP_SAVE);
        ImGui::SameLine();
        if (ImGui::Button("Save All##All"))
        {
            save_mode = ESaveMode::all;
        }
        HelpMarker(HelpStrings::HELP_SAVE_ALL);
        ImGui::SameLine();
        if (ImGui::Checkbox("Show Builtin Tick Functions", &m_state.show_tick))
        {
            tick_changed = true;
            request_save_state();
        }
        HelpMarker(HelpStrings::HELP_SHOW_BUILTIN_TICK);
        ImGui::SameLine();
        if (ImGui::Checkbox("Disable Indent Colors", &m_state.disable_indent_colors))
        {
            request_save_state();
        }

        apply_filters_to_history(whitelist_changed, blacklist_changed, tick_changed);
        save(save_mode);
    }

    auto Client::render_perf_opts() -> void
    {
        static uint16_t step = 1;
        if (ImGui::InputScalar("Max MS Read Time", ImGuiDataType_U16, &m_state.dequeue_max_ms, &step, 0, 0))
        {
            request_save_state();
            if (!m_state.dequeue_max_ms)
            {
                m_state.dequeue_max_ms = 1;
            }
        }
        HelpMarker(HelpStrings::HELP_MAX_MS_READ_TIME);

        if (ImGui::InputScalar("Max Count Per Iteration", ImGuiDataType_U32, &m_state.dequeue_max_count, &step))
        {
            request_save_state();
            if (!m_state.dequeue_max_count)
            {
                m_state.dequeue_max_count = 1;
            }
        }
        HelpMarker(HelpStrings::HELP_MAX_COUNT_PER_ITERATION);

        if (ImGui::InputScalar("Text Virtualization Count", ImGuiDataType_U16, &m_state.text_virtualization_count, &step))
        {
            request_save_state();
            if (!m_state.text_virtualization_count)
            {
                m_state.text_virtualization_count = 1;
            }
            m_state.text_temp_virtualization_count = m_state.text_virtualization_count;
            clear_threads();
        }
        HelpMarker(HelpStrings::HELP_TEXT_VIRTUALIZATION_COUNT);

        ImGui::Text("Enqueue Avg: %f Dequeue Avg: %f Pending Avg: %f Time Slot Exceeded Count: %llu", QueueProfiler::GetEnqueueAverage(), QueueProfiler::GetDequeueAverage(), QueueProfiler::GetPendingAverage(), QueueProfiler::GetTimeExceededCount());
        HelpMarker(HelpStrings::HELP_QUEUE_PROFILE_VALUES);
    }

    auto Client::render_view() -> void
    {
        auto& threads = m_state.threads;
        if (threads.empty())
        {
            return;
        }

        if (m_state.current_thread < 0)
        {
            m_state.current_thread = 0;
        }
        if (static_cast<size_t>(m_state.current_thread) >= threads.size())
        {
            m_state.current_thread = static_cast<int>(threads.size() - 1);
        }

        auto& thread = threads[m_state.current_thread];

        const auto selected_flags = static_cast<uint32_t>(m_state.hook_target);

        auto area = ImGui::GetContentRegionAvail();
        auto& padding = ImGui::GetStyle().WindowPadding;
        auto& scroll_size = ImGui::GetStyle().ScrollbarSize;
        area.y -= ((padding.y + scroll_size) * 2);
        area.x -= (padding.x + scroll_size);
        ImGui::BeginChild("##view", area, ImGuiChildFlags_Borders | ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar);
        if (m_state.mode == EMode::Stack)
        {
            if (thread.call_stack.empty())
            {
                ImGui::EndChild();
                return;
            }

            int prev_depth = 0;
            bool have_prev = false;
            int current_indent = 0;
            int id = 0;
            uint8_t entry_flags = 0;
            if (!m_state.disable_indent_colors) entry_flags |= ECallStackEntryRenderFlags_IndentColors;
            if (!m_state.started) entry_flags |= ECallStackEntryRenderFlags_WithSupportMenusCallStackModal;

            auto r_start = thread.call_stack.rbegin();
            size_t count = 0;
            for (; r_start != thread.call_stack.rend() && count < m_state.text_temp_virtualization_count; ++r_start)
            {
                if (r_start->is_disabled) continue;
                ++count;
            }
            bool needs_scroll_here = false;
            if (!m_state.started && count == m_state.text_temp_virtualization_count)
            {
                if (ImGui::Button("Load More..."))
                {
                    m_state.text_temp_virtualization_count *= 2;
                    needs_scroll_here = true;
                }
            }
            for (auto start = r_start.base(); start != thread.call_stack.end(); ++start)
            {
                auto& entry = *start;
                if ((static_cast<uint32_t>(entry.hook_target) & selected_flags) == 0)
                {
                    continue;
                }

                if (entry.is_disabled)
                {
                    continue;
                }

                const int depth = static_cast<int>(entry.depth);
                const int delta = have_prev ? (depth - prev_depth) : depth;
                ImGui::PushID(id++);
                entry.render(delta, static_cast<ECallStackEntryRenderFlags_>(entry_flags));
                ImGui::PopID();
                current_indent += delta;
                prev_depth = depth;
                have_prev = true;
            }

            // Reset indent state for safety.
            while (current_indent > 0)
            {
                ImGui::Unindent();
                --current_indent;
            }

            if (m_state.started || needs_scroll_here) ImGui::SetScrollHereY(1.0f);
        }
        else
        {
            if (ImGui::BeginTable("##frequency", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV))
            {
                int id = 0;
                for (const auto& entry : thread.call_frequencies)
                {
                    if ((entry.source_flags & selected_flags) == 0)
                    {
                        continue;
                    }

                    if (entry.is_disabled)
                    {
                        continue;
                    }
                    ImGui::PushID(id++);
                    entry.render(m_state.started ? ECallFrequencyEntryRenderFlags_None : ECallFrequencyEntryRenderFlags_WithSupportMenus);
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

        ImGui::EndChild();

        if (m_entry_call_stack_renderer)
        {
            if (!m_entry_call_stack_renderer->render())
            {
                m_entry_call_stack_renderer = nullptr;
            }
        }
    }

    auto Client::combo_with_flags(const char* label,
            int* current_item,
            const char* const items[],
            const int items_count,
            const ImGuiComboFlags_ flags) -> bool
    {
        bool changed = false;
        if (ImGui::BeginCombo(label, items[*current_item], flags))
        {
            for (int i = 0; i < items_count; ++i)
            {
                const auto is_selected = i == *current_item;
                if (ImGui::Selectable(items[i], is_selected))
                {
                    *current_item = i;
                    changed = true;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    auto Client::save_state() -> void
    {
        std::error_code ec;
        std::filesystem::create_directories(m_cfg_path.parent_path(), ec);

        std::unordered_map<std::string, std::string> state_map;
        state_map.emplace("Enabled", std::to_string(m_state.enabled));
        state_map.emplace("ShowTick", std::to_string(m_state.show_tick));
        int hook_target_idx = 0;
        for (int i = 0; i < EMiddlewareHookTarget_Size; ++i)
        {
            if (EMiddlewareHookTarget_ValueArray[i] == m_state.hook_target)
            {
                hook_target_idx = i;
                break;
            }
        }
        state_map.emplace("HookTarget", std::to_string(hook_target_idx));
        state_map.emplace("Mode", std::to_string(static_cast<int>(m_state.mode)));
        state_map.emplace("DequeueMaxMs", std::to_string(m_state.dequeue_max_ms));
        state_map.emplace("DequeueMaxCount", std::to_string(m_state.dequeue_max_count));
        state_map.emplace("Whitelist", m_state.whitelist);
        state_map.emplace("Blacklist", m_state.blacklist);
        state_map.emplace("DisableIndentColors", std::to_string(m_state.disable_indent_colors));
        state_map.emplace("TextVirtualizationCount", std::to_string(m_state.text_virtualization_count));

        (void)glz::write_file_json(state_map, m_cfg_path.string(), std::string{});
    }

    auto Client::load_state() -> void
    {
        if (!std::filesystem::exists(m_cfg_path) || !std::filesystem::is_regular_file(m_cfg_path))
        {
            return;
        }

        std::unordered_map<std::string, std::string> state_map{};
        auto ec = glz::read_file_json(state_map, m_cfg_path.string(), std::string{});
        if (ec.ec != glz::error_code::none)
        {
            return;
        }

        try
        {
            m_state.enabled = state_map.at("Enabled") != "0";
            m_state.show_tick = state_map.at("ShowTick") != "0";
            m_state.disable_indent_colors = state_map.at("DisableIndentColors") != "0";
            const int hook_target_idx = std::stoi(state_map.at("HookTarget"));
            if (hook_target_idx >= 0 && hook_target_idx < EMiddlewareHookTarget_Size)
            {
                m_state.hook_target = EMiddlewareHookTarget_ValueArray[hook_target_idx];
            }
            else
            {
                m_state.hook_target = EMiddlewareHookTarget::All;
            }
            m_state.mode = static_cast<EMode>(std::stoi(state_map.at("Mode")));
            m_state.dequeue_max_ms = static_cast<uint16_t>(std::stoi(state_map.at("DequeueMaxMs")));
            m_state.dequeue_max_count = static_cast<uint32_t>(std::stoul(state_map.at("DequeueMaxCount")));
            m_state.whitelist = state_map.at("Whitelist");
            m_state.blacklist = state_map.at("Blacklist");
            m_state.whitelist_tokens = split_string_by_comma(m_state.whitelist);
            m_state.blacklist_tokens = split_string_by_comma(m_state.blacklist);
            m_state.text_virtualization_count = static_cast<uint16_t>(std::stoi(state_map.at("TextVirtualizationCount")));
            m_state.text_temp_virtualization_count = m_state.text_virtualization_count;
            clear_threads(); // just to be safe, since if text_virtualization_count changes while scrolling it could cause problems
        }
        catch (...)
        {
            Output::send<LogLevel::Verbose>(STR("[EventViewerMod] Failed to load state from file due to exception!"));
        }
    }

    auto Client::check_save_request() -> bool
    {
        if (m_state.needs_save.test(std::memory_order_acquire))
        {
            save_state();
            m_state.needs_save.clear(std::memory_order_release);
            return true;
        }
        return false;
    }

    auto Client::apply_filters_to_history(const bool whitelist_changed, const bool blacklist_changed, const bool tick_changed) -> void
    {
        if (!(whitelist_changed || blacklist_changed || tick_changed))
        {
            return;
        }
        if (whitelist_changed)
        {
            m_state.whitelist_tokens = split_string_by_comma(m_state.whitelist);
        }
        if (blacklist_changed)
        {
            m_state.blacklist_tokens = split_string_by_comma(m_state.blacklist);
        }

        // Recompute disabled state for all history. This only runs when the user changes filters/tick setting.
        const bool show_tick = m_state.show_tick;

        for (auto& thread : m_state.threads)
        {
                // Stack history can get very large; leverage parallel execution on random-access iterators.
                std::for_each(std::execution::par_unseq,
                              thread.call_stack.begin(),
                              thread.call_stack.end(),
                              [this, show_tick](CallStackEntry& entry) {
                                  entry.is_disabled = (entry.is_tick && !show_tick) || !passes_filters(entry.lower_cased_full_name);
                              });

                // Frequency view is smaller and is a list (non-random-access).
                for (auto& entry : thread.call_frequencies)
                {
                    entry.is_disabled = (entry.is_tick && !show_tick) || !passes_filters(entry.lower_cased_function_name);
                }
        }
    }

    auto Client::dequeue() -> void
    {
        if (!m_state.enabled || !m_state.started)
        {
            return;
        }

        m_middleware.dequeue(m_state.dequeue_max_ms, m_state.dequeue_max_count, [this](CallStackEntry&& entry) {
            // Thread lookup/creation (unified across hook targets).
            auto& threads = m_state.threads;

            const auto entry_thread = entry.thread_id;
            auto thread_it = std::ranges::find_if(threads, [&entry_thread](const ThreadInfo& info) {
                return info.thread_id == entry_thread;
            });

            ThreadInfo* thread_ptr = nullptr;
            if (thread_it != threads.end())
            {
                thread_ptr = &(*thread_it);
            }
            else
            {
                thread_ptr = &threads.emplace_back(entry_thread);
                if (m_state.current_thread < 0)
                {
                    m_state.current_thread = 0;
                }
            }

            auto& thread = *thread_ptr;

            // Determine disabled state under current filters.
            // TODO if it doesn't pass freq, it won't pass stack
            const auto stack_disabled = (entry.is_tick && !m_state.show_tick) || !passes_filters(entry.lower_cased_full_name);
            const auto freq_disabled = (entry.is_tick && !m_state.show_tick) || !passes_filters(entry.lower_cased_function_name);

            // Frequency tracking: bump existing, or add. TODO combine into 1 loop iteration?
            auto freq_it = std::ranges::find_if(thread.call_frequencies, [&entry](const CallFrequencyEntry& freq_entry) -> bool {
                return entry.function_hash == freq_entry.function_hash;
            });

            const auto entry_source_flags = static_cast<uint32_t>(entry.hook_target);

            if (freq_it != thread.call_frequencies.end()) [[likely]]
            {
                auto& freq = *freq_it;
                ++freq.frequency;
                freq.is_disabled = freq_disabled;
                freq.source_flags |= entry_source_flags;

                // Maintain descending order by frequency using list::splice (fast, no alloc).
                auto new_pos = freq_it;
                while (new_pos != thread.call_frequencies.begin())
                {
                    auto prev = std::prev(new_pos);
                    if (prev->frequency > freq.frequency)
                    {
                        break;
                    }
                    new_pos = prev;
                }
                if (new_pos != freq_it)
                {
                    thread.call_frequencies.splice(new_pos, thread.call_frequencies, freq_it);
                }
            }
            else
            {
                thread.call_frequencies.emplace_back(static_cast<const FunctionNameStringViews&>(entry), entry.is_tick);
                auto& freq = thread.call_frequencies.back();
                freq.is_disabled = freq_disabled;
                freq.source_flags = entry_source_flags;
            }

            // Call stack history.
            entry.is_disabled = stack_disabled;
            thread.call_stack.emplace_back(std::move(entry));
        });
    }

    // todo there's definitely room for improvement, like diffing white/blacklists to tell if test_str needs to be checked
    // (would be done by callers probably) or keeping track of an 'enabled' and 'disabled' unordered_map of
    // hashes (that the StringPool could be altered to provide) to skip string parsing, but this is good enough for now.
    auto Client::passes_filters(const std::string_view test_str) const -> bool
    {
        bool passes_whitelist = m_state.whitelist_tokens.empty();
        for (const auto& token : m_state.whitelist_tokens) // any whitelist token present => pass
        {
            if (test_str.contains(token))
            {
                passes_whitelist = true;
                break;
            }
        }
        if (!passes_whitelist)
        {
            return false;
        }

        for (const auto& token : m_state.blacklist_tokens) // any blacklist token present => fail
        {
            if (test_str.contains(token))
            {
                return false;
            }
        }
        return true;
    }

    auto Client::save(ESaveMode mode) -> void
    {
        if (mode == ESaveMode::none)
        {
            return;
        }

        std::error_code ec;
        std::filesystem::create_directories(m_dump_dir, ec);

        const auto now = std::chrono::system_clock::now();
        const std::time_t now_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm{};
        localtime_s(&local_tm, &now_t);

        std::ostringstream oss;
        // Windows filenames cannot contain ':'.
        oss << std::put_time(&local_tm, "%Y-%m-%d %H-%M-%S");

        if (mode == ESaveMode::current)
        {
            auto& threads = m_state.threads;
            if (threads.empty())
            {
                return;
            }

            if (m_state.current_thread < 0)
            {
                m_state.current_thread = 0;
            }
            if (static_cast<size_t>(m_state.current_thread) >= threads.size())
            {
                m_state.current_thread = static_cast<int>(threads.size() - 1);
            }

            const auto filename =
                "EventViewerMod Capture-"s +
                to_string(m_state.hook_target) +
                "-" + EMode_NameArray[static_cast<int>(m_state.mode)] + " " +
                oss.str() + ".txt";
            const auto path = m_dump_dir / filename;
            std::ofstream file{path};
            if (!file.is_open())
            {
                return;
            }

            file << to_string(m_state.hook_target) << " ";
            serialize_view(threads[m_state.current_thread], m_state.mode, m_state.hook_target, file);
            file.close();
            m_state.last_save_path = path.string();
            ImGui::OpenPopup("Saved File");
            return;
        }

        if (mode == ESaveMode::all)
        {
            if (m_state.threads.empty())
            {
                return;
            }


            const auto filename = "EventViewerMod Capture-All "s + oss.str() + ".txt";
            const auto path = m_dump_dir / filename;
            std::ofstream file{path};
            if (!file.is_open())
            {
                return;
            }

            serialize_all_views(file);
            file.close();
            m_state.last_save_path = path.string();
            ImGui::OpenPopup("Saved File");
        }
    }

    auto Client::serialize_view(ThreadInfo& info, const EMode mode, const EMiddlewareHookTarget hook_target, std::ofstream& out) const -> void
    {
        out << fmt::format("Thread {} {}\n\n", info.id_string(), EMode_NameArray[static_cast<int>(mode)]);

        const uint32_t selected_flags = static_cast<uint32_t>(hook_target);

        if (mode == EMode::Stack)
        {
            if (info.call_stack.empty())
            {
                out << "No captures.\n\n\n";
                return;
            }

            for (const auto& entry : info.call_stack)
            {
                if ((static_cast<uint32_t>(entry.hook_target) & selected_flags) == 0)
                {
                    continue;
                }

                if (entry.is_disabled)
                {
                    continue;
                }
                for (auto i = 0u; i < entry.depth; ++i) out << "\t";
                out << entry.full_name << '\n';
            }

            out << "\n\n\n";
            return;
        }

        if (info.call_frequencies.empty())
        {
            out << "No captures.\n\n\n";
            return;
        }

        for (const auto& entry : info.call_frequencies)
        {
            if ((entry.source_flags & selected_flags) == 0)
            {
                continue;
            }

            if (entry.is_disabled)
            {
                continue;
            }
            out << entry.function_name << '\t' << entry.frequency << '\n';
        }

        out << "\n\n\n";
    }

    auto Client::serialize_all_views(std::ofstream& out) -> void
    {
        for (auto& thread : m_state.threads)
        {
            out << to_string(EMiddlewareHookTarget::All) << " ";
            serialize_view(thread, EMode::Stack, EMiddlewareHookTarget::All, out);
            out << to_string(EMiddlewareHookTarget::All) << " ";
            serialize_view(thread, EMode::Frequency, EMiddlewareHookTarget::All, out);
        }
    }

    auto Client::clear_threads() -> void
    {
        m_state.current_thread = 0;
        m_state.threads.clear();
        m_state.thread_explicitly_chosen = false;
        m_state.thread_implicitly_set = false;
    }

    auto Client::request_save_state() -> void
    {
        m_state.needs_save.test_and_set(std::memory_order_release);
    }

auto Client::add_to_white_list(const std::string_view item) -> void
{
    if (m_state.whitelist.empty())
    {
        m_state.whitelist += item;
    }
    else
    {
        m_state.whitelist += ", ";
        m_state.whitelist += item;
    }
    request_save_state();
    apply_filters_to_history(true, false, false);
}

auto Client::add_to_black_list(std::string_view item) -> void
{
    if (m_state.blacklist.empty())
    {
        m_state.blacklist += item;
    }
    else
    {
        m_state.blacklist += ", ";
        m_state.blacklist += item;
    }
    request_save_state();
    apply_filters_to_history(false, true, false);
}

auto Client::render_entry_stack_modal(const CallStackEntry* entry) -> void
{
    if (m_entry_call_stack_renderer) return;
    // find root entry and next root entry
    // finding the root entry also reveals all callers, so go ahead and bookkeep it
    const auto& stack = m_state.threads[m_state.current_thread].call_stack;
    const size_t target_abs_idx = entry - stack.data();
    size_t root_abs_idx = target_abs_idx;
    std::vector<size_t> idxs_relevant_to_target{}; // added in reverse-view order
    if (entry->depth)
    {
        uint32_t last_lowest_depth = entry->depth;
        for (auto idx = target_abs_idx - 1; idx >= 0; --idx)
        {
            auto this_depth = stack[idx].depth;
            if (this_depth < last_lowest_depth)
            {
                idxs_relevant_to_target.push_back(idx);
                last_lowest_depth = this_depth;
                if (this_depth == 0)
                {
                    root_abs_idx = idx;
                    break;
                }
            }
        }
    }

    size_t next_root_abs_idx = target_abs_idx + 1;
    bool out_of_target_scope = false;
    for (; next_root_abs_idx < stack.size(); ++next_root_abs_idx) // find callers and callees of target
    {
        const auto this_entry_depth = stack[next_root_abs_idx].depth;
        if (this_entry_depth == 0) break;

        if (this_entry_depth <= entry->depth) out_of_target_scope = true;
        if (!out_of_target_scope) idxs_relevant_to_target.push_back(next_root_abs_idx);
    }

    std::vector<CallStackEntry> context{ stack.begin() + root_abs_idx, stack.begin() + next_root_abs_idx }; // copy entries
    for (auto& abs_idx : idxs_relevant_to_target) // make idxs_relevant_to_target relative to context
    {
        abs_idx -= root_abs_idx;
    }
    const auto target_rel_idx = target_abs_idx - root_abs_idx; // find context index for target
    idxs_relevant_to_target.push_back(target_rel_idx);

    // make any irrelevant entry disabled by default, and assert that any relevant entry is enabled. the modal won't change them, but
    // will use is_disabled as a flag to indicate its relevance for the checkbox.
    for (size_t context_idx = 0; context_idx < context.size(); ++context_idx)
    {
        if (std::ranges::find(idxs_relevant_to_target, static_cast<int64_t>(context_idx)) != idxs_relevant_to_target.end())
        {
            context[context_idx].is_disabled = false;
            continue;
        }
        context[context_idx].is_disabled = true;
    }

    m_entry_call_stack_renderer = std::make_unique<EntryCallStackRenderer>(target_rel_idx, std::move(context));
}

auto Client::GetInstance() -> Client&
    {
        static Client client{};
        return client;
    }
} // namespace RC::EventViewerMod
