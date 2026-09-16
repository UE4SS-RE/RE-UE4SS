#include <GUI/Mods.hpp>

#include <set>
#include <limits>

#include <DynamicOutput/DynamicOutput.hpp>
#include <UE4SSProgram.hpp>
#include <Mod/LuaMod.hpp>
#include <GUI/GUI.hpp>
#include <GUI/ImGuiUtility.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/Hooks/Hooks.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome5.h>

namespace RC::GUI
{
    class ModActor : public AActor
    {
    public:
        struct ModMenuButtonPressed_Params
        {
            int32_t Index; // 0x0
        };

    public:
        auto ModMenuButtonPressed(int32_t Index) -> void
        {
            auto Function = GetFunctionByNameInChain(FromCharTypePtr<TCHAR>(STR("ModMenuButtonPressed")));
            if (!Function)
            {
                return;
            }

            ModMenuButtonPressed_Params Params{Index};
            ProcessEvent(Function, &Params);
        }
    };

    static auto mod_type_to_string(ModType mod_type)
    {
        switch (mod_type)
        {
        case ModType::Invalid:
            return "Invalid";
        case ModType::LuaMod:
            return "LuaMod";
        case ModType::CppMod:
            return "CppMod";
        case ModType::BPMod:
            return "BPMod";
        }
        Output::send<LogLevel::Error>(STR("Invalid value for ModType\n"));
        return "Corrupted";
    }

    auto ModsWidget::render() -> void
    {
        const auto can_process = UnrealInitializer::StaticStorage::bIsInitialized || UE4SSProgram::get_program().can_process_events();
        if (!can_process)
        {
            ImGui::BeginDisabled();
        }

        if (m_mods_list_dirty)
        {
            refresh_mods_list();
        }

        if (ImGui::Button(ICON_FA_SYNC " Refresh"))
        {
            m_mods_list_dirty = true;
        }

        ImGui::SameLine();

        if (m_create_new_mod_button_handler)
        {
            if (ImGui::Button(ICON_FA_PLUS " New Mod"))
            {
                m_show_create_mod_popup = true;
                m_new_mod_name.clear();
            }
        }

        ImGui::SameLine();

        auto& gui = UE4SSProgram::get_program().get_debugging_ui();
        bool event_busy = gui.m_event_thread_busy || !UE4SSProgram::get_program().can_process_events();
        if (event_busy)
        {
            ImGui::BeginDisabled(true);
        }
        if (ImGui::Button(ICON_FA_REDO " Restart All Mods"))
        {
            gui.m_event_thread_busy = true;
            UE4SSProgram::get_program().queue_event([&gui, this]() {
                UE4SSProgram::get_program().queue_reinstall_mods();
                gui.m_event_thread_busy = false;
                m_mods_list_dirty = true;
            });
        }
        if (event_busy)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("(%zu mods)", m_discovered_mods.size());

        ImGui::Separator();

        ImGui::BeginChild("ModsList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (size_t i = 0; i < m_discovered_mods.size(); ++i)
        {
            auto& mod = m_discovered_mods[i];

            ImGui::PushID(static_cast<int>(i));

            bool enabled = mod.is_enabled();
            if (ImGui::Checkbox("##enabled", &enabled))
            {
                set_mod_enabled(mod.path, enabled);
                mod.enabled_via_txt = enabled;
            }
            const auto checkbox_width = scaled(ImGui::GetItemRectSize().x);

            if (ImGui::IsItemHovered())
            {
                std::string tooltip = "Enable/disable on next reload";
                if (mod.enabled_via_mods_txt && !mod.enabled_via_txt)
                {
                    tooltip += "\n(Currently enabled via mods.txt)";
                }
                ImGui::SetTooltip("%s", tooltip.c_str());
            }

            ImGui::SameLine();

            if (mod.is_running)
            {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), ICON_FA_PLAY);
                ImGui::SameLine();
            }

            // TODO: Consider making the mod types a different color to make them stand out.
            //       Maybe put them at the left-most part of the right side of the window, so right before all the buttons.
            const auto mod_name = fmt::format("{} ({})", mod.name, mod_type_to_string(mod.mod_type));
            if (mod.enabled_via_mods_txt && !mod.enabled_via_txt)
            {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", mod_name.c_str());
            }
            else
            {
                ImGui::Text("%s", mod_name.c_str());
            }

            static constexpr auto open_text = ICON_FA_FOLDER_OPEN " Open";
            static constexpr auto new_text = ICON_FA_FILE_ALT " New";
            static constexpr auto restart_text = ICON_FA_SYNC " Restart";
            static constexpr auto uninstall_text = ICON_FA_STOP " Uninstall";
            static constexpr auto start_text = ICON_FA_PLAY " Start";
            const auto frame_padding = ImGui::GetStyle().FramePadding;
            const auto open_text_size = ImGui::CalcTextSize(open_text);
            const auto new_text_size = ImGui::CalcTextSize(new_text);
            const auto restart_text_size = ImGui::CalcTextSize(restart_text);
            const auto uninstall_text_size = ImGui::CalcTextSize(uninstall_text);
            const auto start_text_size = ImGui::CalcTextSize(start_text);
            const auto open_button_size = ImVec2{open_text_size.x + frame_padding.x * 2, open_text_size.y + frame_padding.y * 2};
            const auto new_button_size = ImVec2{new_text_size.x + frame_padding.x * 2, new_text_size.y + frame_padding.y * 2};
            const auto restart_button_size = ImVec2{restart_text_size.x + frame_padding.x * 2, restart_text_size.y + frame_padding.y * 2};
            const auto uninstall_button_size = ImVec2{uninstall_text_size.x + frame_padding.x * 2, uninstall_text_size.y + frame_padding.y * 2};
            const auto start_button_size = ImVec2{start_text_size.x + frame_padding.x * 2, start_text_size.y + frame_padding.y * 2};

            const auto button_width = scaled(open_button_size.x + new_button_size.x + restart_button_size.x + uninstall_button_size.x + start_button_size.x);
            if (mod.mod_type == ModType::BPMod)
            {
                ImGui::SameLine(checkbox_width);
                static constexpr auto click_to_expand_text = "<Click to expand>";
                const auto text_size = ImGui::CalcTextSize(click_to_expand_text);
                ImGui::SetNextItemAllowOverlap();
                ImGui::InvisibleButton("##line-item-hitbox", {-button_width, ImGui::GetFrameHeight()});
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    if (m_expanded_mod == s_invalid_mod_id || m_expanded_mod != i)
                    {
                        m_expanded_mod = i;
                    }
                    else
                    {
                        m_expanded_mod = s_invalid_mod_id;
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    // Make it obvious that there's an on-click action available by changing the cursor when hovering over the text.
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    ImGui::SetTooltip("Click to expand details");
                }
                const auto text_start_offset = ImGui::GetContentRegionAvail().x / 2 - scaled(text_size.x);
                ImGui::SameLine(text_start_offset);
                ImGui::Text(click_to_expand_text);
            }
            // Use consistent button width for alignment (widest case: Open + New + Restart + Uninstall)
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - button_width);

            if (m_open_button_handler)
            {
                if (mod.mod_type != ModType::LuaMod)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::SmallButton(open_text))
                {
                    m_open_button_handler(m_handler_context, mod);
                }
                if (mod.mod_type != ModType::LuaMod)
                {
                    ImGui::EndDisabled();
                }
            }
            else
            {
                ImGui::InvisibleButton("##open_button_placeholder_for_layout", open_button_size);
            }

            ImGui::SameLine();

            if (m_create_file_button_handler)
            {
                if (mod.mod_type != ModType::LuaMod)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::SmallButton(new_text))
                {
                    m_show_create_file_popup = true;
                    m_new_file_name.clear();
                    m_add_require_to_main = true;
                    m_create_file_mod_path = mod.path;
                }
                if (mod.mod_type != ModType::LuaMod)
                {
                    ImGui::EndDisabled();
                }
            }
            else
            {
                ImGui::InvisibleButton("##new_button_placeholder_for_layout", new_button_size);
            }

            // Show different buttons based on whether mod is running
            if (mod.is_running)
            {
                ImGui::SameLine();

                if (event_busy)
                {
                    ImGui::BeginDisabled(true);
                }

                if (mod.mod_type == ModType::BPMod)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::SmallButton(restart_text))
                {
                    restart_mod_by_name(mod.name, mod.mod_type);
                }
                if (mod.mod_type == ModType::BPMod)
                {
                    ImGui::EndDisabled();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Uninstall and reinstall this mod");
                }

                ImGui::SameLine();

                if (mod.mod_type == ModType::BPMod)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::SmallButton(uninstall_text))
                {
                    uninstall_mod_by_name(mod.name, mod.mod_type);
                }
                if (mod.mod_type == ModType::BPMod)
                {
                    ImGui::EndDisabled();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Uninstall this mod (will not run until next full reload)");
                }

                if (event_busy)
                {
                    ImGui::EndDisabled();
                }
            }
            else
            {
                // Show Start button for mods that are not running
                ImGui::SameLine();

                if (event_busy)
                {
                    ImGui::BeginDisabled(true);
                }

                if (mod.mod_type == ModType::BPMod)
                {
                    ImGui::BeginDisabled();
                }
                if (ImGui::SmallButton(start_text))
                {
                    start_mod_by_path(mod.path, mod.mod_type);
                }
                if (mod.mod_type == ModType::BPMod)
                {
                    ImGui::EndDisabled();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Start this mod without reloading all mods");
                }

                if (event_busy)
                {
                    ImGui::EndDisabled();
                }
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Path: %s", mod.path.string().c_str());
                ImGui::EndTooltip();
            }

            if (mod.mod_type == ModType::BPMod && m_expanded_mod == i)
            {
                const auto& style = ImGui::GetStyle();
                const auto size = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
                ImGui::BeginChild("ModExpandedSection", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::Indent(size);
                ImGui::Text("Author: %s", mod.bp_info.author.c_str());
                ImGui::Text("Description: %s", mod.bp_info.description.c_str());
                ImGui::Text("Version: %s", mod.bp_info.version.c_str());
                static constexpr auto max_buttons_per_line = 3;
                for (int32_t i2 = 0; i2 < mod.bp_info.buttons.size(); ++i2)
                {
                    const auto& mod_button = mod.bp_info.buttons[i2];
                    if (ImGui::Button(fmt::format("{}", mod_button).c_str()))
                    {
                        mod.bp_info.actor->ModMenuButtonPressed(i2);
                    }
                    if (i2 % max_buttons_per_line != 0)
                    {
                        ImGui::SameLine();
                    }
                }
                ImGui::Unindent();
                ImGui::EndChild();
            }

            ImGui::PopID();
        }

        ImGui::EndChild();

        if (m_show_create_mod_popup)
        {
            ImGui::OpenPopup("Create New Mod");
            m_show_create_mod_popup = false;
        }

        if (m_show_create_file_popup)
        {
            ImGui::OpenPopup("Create New File");
            m_show_create_file_popup = false;
        }

        if (ImGui::BeginPopupModal("Create New Mod", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter name for new mod:");
            ImGui::Separator();

            ImGui::SetNextItemWidth(scaled(300.0f));
            bool enter_pressed = ImGui::InputText("##newmodname", &m_new_mod_name, ImGuiInputTextFlags_EnterReturnsTrue);

            if ((ImGui::Button("Create", ImVec2(scaled(120.0f), 0)) || enter_pressed) && !m_new_mod_name.empty())
            {
                if (m_create_new_mod_button_handler && m_create_new_mod_button_handler(m_handler_context, m_new_mod_name))
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(scaled(120.0f), 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Create New File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::string mod_name = m_create_file_mod_path.stem().string();
            ImGui::Text("Create new file in %s:", mod_name.c_str());
            ImGui::Separator();

            ImGui::SetNextItemWidth(scaled(300.0f));
            bool enter_pressed = ImGui::InputText("##newfilename", &m_new_file_name, ImGuiInputTextFlags_EnterReturnsTrue);

            if (!m_new_file_name.empty() && m_new_file_name.find(".lua") == std::string::npos)
            {
                ImGui::SameLine();
                ImGui::TextDisabled(".lua");
            }

            ImGui::Checkbox("Add require() to main.lua", &m_add_require_to_main);

            if (m_create_file_button_handler)
            {
                if ((ImGui::Button("Create", ImVec2(scaled(120.0f), 0)) || enter_pressed) && !m_new_file_name.empty())
                {
                    if (m_create_file_button_handler(m_handler_context, m_create_file_mod_path.string(), m_new_file_name, m_add_require_to_main))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(scaled(120.0f), 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (!can_process)
        {
            ImGui::EndDisabled();
        }
    }

    auto ModsWidget::refresh_mods_list() -> void
    {
        m_discovered_mods.clear();
        m_expanded_mod = s_invalid_mod_id;

        auto& program = UE4SSProgram::get_program();
        auto& mods_dirs = program.get_mods_directories();
        auto mods_txt_entries = program.get_mods_txt_entries();

        std::set<std::string> running_mod_names;
        for (const auto& mod : program.m_mods)
        {
            if (mod->is_started())
            {
                auto running_identifier = to_string(mod->get_name());
                if (dynamic_cast<LuaMod*>(mod.get()))
                {
                    running_identifier += "_LuaMod";
                }
                else
                {
                    running_identifier += "_CppMod";
                }
                running_mod_names.insert(running_identifier);
            }
        }

        std::set<std::string> seen_mods;

        for (const auto& mods_dir : mods_dirs)
        {
            if (!std::filesystem::exists(mods_dir))
            {
                continue;
            }

            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(mods_dir, ec))
            {
                if (!entry.is_directory(ec) || ec)
                {
                    continue;
                }

                std::string mod_name = entry.path().stem().string();

                if (seen_mods.count(mod_name))
                {
                    continue;
                }
                seen_mods.insert(mod_name);

                ModInfo info;
                info.name = mod_name;
                info.path = entry.path();
                info.enabled_via_txt = std::filesystem::exists(entry.path() / "enabled.txt", ec);

                const bool has_main_lua = std::filesystem::exists(entry.path() / "Scripts" / "main.lua", ec);
                const bool has_main_dll = std::filesystem::exists(entry.path() / "dlls" / "main.dll", ec) ||
                    std::filesystem::exists(entry.path() / "dlls" / fmt::format("{}.dll", mod_name), ec);

                if (has_main_lua || has_main_dll)
                {
                    auto it = mods_txt_entries.find(mod_name);
                    if (it != mods_txt_entries.end())
                    {
                        info.enabled_via_mods_txt = it->second;
                    }
                }

                if (has_main_lua)
                {
                    info.mod_type = ModType::LuaMod;
                    info.is_running = running_mod_names.contains(mod_name + "_LuaMod");
                    m_discovered_mods.push_back(info);
                }

                if (has_main_dll)
                {
                    info.mod_type = ModType::CppMod;
                    info.is_running = running_mod_names.contains(mod_name + "_CppMod");
                    m_discovered_mods.push_back(info);
                }
            }
        }

        static const auto mod_actor_name = FName(STR("ModActor_C"), FNAME_Add);
        std::vector<UObject*> mod_actors{};
        UObjectGlobals::FindAllOf(mod_actor_name, mod_actors);
        for (auto mod_actor : mod_actors)
        {
            const auto mod_class = mod_actor->GetClassPrivate();

            ModInfo info;
            info.mod_type = ModType::BPMod;
            info.is_running = true;
            info.bp_info.actor = static_cast<ModActor*>(mod_actor);

            static auto mod_author_name = FName(STR("ModAuthor"), FNAME_Add);
            auto mod_author_property = mod_class->FindProperty(mod_author_name);
            info.bp_info.author = mod_author_property ? to_string(**mod_author_property->ContainerPtrToValuePtr<FString>(mod_actor)) : "Unknown";

            static auto mod_description_name = FName(STR("ModDescription"), FNAME_Add);
            auto mod_description_property = mod_class->FindProperty(mod_description_name);
            info.bp_info.description =
                    mod_description_property ? to_string(**mod_description_property->ContainerPtrToValuePtr<FString>(mod_actor)) : "Unknown";

            static auto mod_version_name = FName(STR("ModVersion"), FNAME_Add);
            auto mod_version_property = mod_class->FindProperty(mod_version_name);
            info.bp_info.version = mod_version_property ? to_string(**mod_version_property->ContainerPtrToValuePtr<FString>(mod_actor)) : "Unknown";

            static auto mod_buttons_name = FName(STR("ModButtons"), FNAME_Add);
            auto mod_buttons_property = mod_class->FindProperty(mod_buttons_name);
            const auto mod_buttons = mod_buttons_property ? mod_buttons_property->ContainerPtrToValuePtr<TArray<FString>>(mod_actor) : nullptr;
            if (mod_buttons)
            {
                for (FString& button_name : *mod_buttons)
                {
                    info.bp_info.buttons.emplace_back(to_string(*button_name));
                }
            }

            auto mod_full_name = mod_class->GetFullName();
            auto mod_name_parts = explode_by_occurrence(mod_full_name, STR('/'));
            info.name = to_string(mod_name_parts[mod_name_parts.size() - 2]);

            m_discovered_mods.push_back(info);
        }

        std::sort(m_discovered_mods.begin(),
                  m_discovered_mods.end(),
                  [](const auto& a, const auto& b) {
                      return a.name < b.name;
                  });

        m_mods_list_dirty = false;

        static bool sco_hooked = false;
        if (!sco_hooked)
        {
            Hook::RegisterStaticConstructObjectPostCallback([this] (const auto& data, const Unreal::FStaticConstructObjectParameters&) {
                static const auto mod_actor_fname = FName(STR("ModActor_C"));
                auto constructed_object = data.GetCurrentResolvedReturnValue();
                if (!constructed_object->GetNamePrivate().Equals(mod_actor_fname))
                {
                    return;
                }
                m_mods_list_dirty = true;
            }, {false, true, STR("UE4SS"), STR("ModsTabBPModDirtyDetector")});
        }
    }

    auto ModsWidget::set_mod_enabled(const std::filesystem::path& mod_path, bool enabled) -> void
    {
        std::filesystem::path enabled_file = mod_path / "enabled.txt";

        if (enabled)
        {
            std::ofstream file(enabled_file);
            file.close();
        }
        else
        {
            std::error_code ec;
            std::filesystem::remove(enabled_file, ec);
            if (ec)
            {
                Output::send<LogLevel::Error>(STR("Error: Unable to remove file: '{}', message: {}\n"), ensure_str(enabled_file), ensure_str(ec.message()));
            }
        }

        m_mods_list_dirty = true;
    }

    auto ModsWidget::uninstall_mod_by_name(const std::string& mod_name, ModType mod_type) -> void
    {
        if (mod_type == ModType::LuaMod)
        {
            UE4SSProgram::get_program().queue_uninstall_lua_mod_by_name(mod_name);
        }
        else
        {
            UE4SSProgram::get_program().queue_uninstall_cpp_mod_by_name(mod_name);
        }
        m_mods_list_dirty = true;
    }

    auto ModsWidget::restart_mod_by_name(const std::string& mod_name, ModType mod_type) -> void
    {
        if (mod_type == ModType::LuaMod)
        {
            UE4SSProgram::get_program().queue_reinstall_lua_mod_by_name(mod_name);
        }
        else
        {
            UE4SSProgram::get_program().queue_reinstall_cpp_mod_by_name(mod_name);
        }
        m_mods_list_dirty = true;
    }

    auto ModsWidget::start_mod_by_path(const std::filesystem::path& mod_path, ModType mod_type) -> void
    {
        if (mod_type == ModType::LuaMod)
        {
            UE4SSProgram::get_program().queue_start_lua_mod_by_path(mod_path);
        }
        else
        {
            UE4SSProgram::get_program().queue_start_cpp_mod_by_path(mod_path);
        }
        m_mods_list_dirty = true;
    }
} // namespace RC::GUI::Mods