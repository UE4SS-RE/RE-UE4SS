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

        if (ImGui::Button(ICON_FA_PLUS " New Mod"))
        {
            m_show_create_mod_popup = true;
            m_new_mod_name.clear();
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

            const auto button_width = scaled(320.0f);
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

            if (mod.mod_type != ModType::LuaMod)
            {
                ImGui::BeginDisabled();
            }
            ImGui::BeginDisabled();
            if (ImGui::SmallButton(ICON_FA_FOLDER_OPEN " Open"))
            {
                // TODO: Make this button open the LuaDebugger with the proper context for Lua mods, and gray it out for other mod types.
                //       This is disabled for now, and the code that's commented out is from the old LuaDebugger implementation.
                /*std::filesystem::path scripts_path = mod.path / "Scripts";
                std::filesystem::path main_lua = scripts_path / "main.lua";
                if (std::filesystem::exists(main_lua))
                {
                    // Clear selected state if mod isn't running so dropdown uses filesystem scan
                    if (!mod.is_running)
                    {
                        m_selected_state = nullptr;
                    }
                    m_script_edit_path = main_lua.string();
                    const LuaScriptFile* script = load_script(m_script_edit_path);
                    if (script && script->loaded)
                    {
                        m_script_editor.SetText(script->content);
                        m_script_original_content = m_script_editor.GetText();
                        m_script_is_dirty = false;
                        m_pending_editor_tab_switch = 1;
                    }
                }*/
            }
            ImGui::EndDisabled();
            if (mod.mod_type != ModType::LuaMod)
            {
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            if (mod.mod_type != ModType::LuaMod)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::SmallButton(ICON_FA_FILE_ALT " New"))
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
                if (ImGui::SmallButton(ICON_FA_SYNC " Restart"))
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
                if (ImGui::SmallButton(ICON_FA_STOP " Uninstall"))
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
                if (ImGui::SmallButton(ICON_FA_PLAY " Start"))
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
                if (create_new_mod(m_new_mod_name))
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

            // TODO: Disabled until Lua Debugger context switch is implemented.
            ImGui::BeginDisabled();
            if ((ImGui::Button("Create", ImVec2(scaled(120.0f), 0)) || enter_pressed) && !m_new_file_name.empty())
            {
                /*
                if (create_new_file(m_create_file_mod_path.string(), m_new_file_name, m_add_require_to_main))
                {
                    ImGui::CloseCurrentPopup();
                }
                //*/
            }
            ImGui::EndDisabled();

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
                    info.is_running = running_mod_names.contains(mod_name + "_LuaMod") > 0;
                    m_discovered_mods.push_back(info);
                }

                if (has_main_dll)
                {
                    info.mod_type = ModType::CppMod;
                    info.is_running = running_mod_names.contains(mod_name + "_CppMod") > 0;
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

    auto ModsWidget::create_new_mod(const std::string& name) -> bool
    {
        if (name.empty())
        {
            return false;
        }

        auto& program = UE4SSProgram::get_program();
        auto& mods_dirs = program.get_mods_directories();

        if (mods_dirs.empty())
        {
            return false;
        }

        std::filesystem::path mod_path = mods_dirs[0] / name;
        std::filesystem::path scripts_path = mod_path / "Scripts";
        std::filesystem::path main_lua_path = scripts_path / "main.lua";

        std::error_code ec;

        if (std::filesystem::exists(mod_path, ec))
        {
            Output::send<LogLevel::Warning>(STR("Mod '{}' already exists\n"), to_generic_string(name));
            return false;
        }

        std::filesystem::create_directories(scripts_path, ec);
        if (ec)
        {
            Output::send<LogLevel::Error>(STR("Failed to create mod directory: {}\n"), to_generic_string(ec.message()));
            return false;
        }

        std::ofstream main_file(main_lua_path);
        if (!main_file.is_open())
        {
            Output::send<LogLevel::Error>(STR("Failed to create main.lua\n"));
            return false;
        }

        main_file << "-- " << name << " mod\n";
        main_file << "-- Created by UE4SS Lua Debugger\n\n";
        main_file << "print(\"[" << name << "] Mod loaded!\")\n";
        main_file.close();

        std::ofstream enabled_file(mod_path / "enabled.txt");
        enabled_file.close();

        m_mods_list_dirty = true;

        // TODO: Decide how to implement the context switching to the Lua Debugger tab.
        //       This code belongs to that system.
        /*
        // New mod is not running, clear selected state so dropdown uses filesystem scan
        m_selected_state = nullptr;

        m_script_edit_path = main_lua_path.string();
        const LuaScriptFile* script = load_script(m_script_edit_path);
        if (script && script->loaded)
        {
            m_script_editor.SetText(script->content);
            m_script_original_content = m_script_editor.GetText();
            m_script_is_dirty = false;
        }

        m_pending_editor_tab_switch = 1;
        //*/

        Output::send(STR("Created new mod '{}'\n"), to_generic_string(name));
        return true;
    }

    // TODO: Code belongs to "Open" action, when we want to switch context to the Lua Debugger tab.
    //       Removed for now.
    /*
    auto ModsWidget::create_new_file(const std::string& mod_path, const std::string& filename, bool add_require_to_main) -> bool
    {
        if (filename.empty() || mod_path.empty())
        {
            return false;
        }

        std::filesystem::path scripts_path = std::filesystem::path(mod_path) / "Scripts";
        std::filesystem::path file_path = scripts_path / filename;

        if (!file_path.has_extension())
        {
            file_path += ".lua";
        }

        std::error_code ec;
        if (std::filesystem::exists(file_path, ec))
        {
            Output::send<LogLevel::Warning>(STR("File '{}' already exists\n"), to_generic_string(filename));
            return false;
        }

        std::ofstream file(file_path);
        if (!file.is_open())
        {
            Output::send<LogLevel::Error>(STR("Failed to create file '{}'\n"), to_generic_string(filename));
            return false;
        }

        std::string module_name = file_path.stem().string();
        file << "-- " << module_name << " module\n\n";
        file << "local M = {}\n\n";
        file << "return M\n";
        file.close();

        // Add require() to main.lua if requested
        if (add_require_to_main)
        {
            std::filesystem::path main_lua_path = scripts_path / "main.lua";
            if (std::filesystem::exists(main_lua_path))
            {
                std::ifstream main_file_read(main_lua_path);
                std::string main_content((std::istreambuf_iterator<char>(main_file_read)), std::istreambuf_iterator<char>());
                main_file_read.close();

                std::string require_line = "local " + module_name + " = require(\"" + module_name + "\")\n";

                // Check if require already exists
                if (main_content.find("require(\"" + module_name + "\")") == std::string::npos)
                {
                    // Find the best place to insert - after existing requires or at the top
                    size_t insert_pos = 0;
                    size_t last_require_pos = main_content.rfind("require(");
                    if (last_require_pos != std::string::npos)
                    {
                        // Find end of line after last require
                        size_t newline_pos = main_content.find('\n', last_require_pos);
                        if (newline_pos != std::string::npos)
                        {
                            insert_pos = newline_pos + 1;
                        }
                    }
                    else
                    {
                        // No existing requires, find end of any initial comments
                        size_t pos = 0;
                        while (pos < main_content.size())
                        {
                            // Skip whitespace
                            while (pos < main_content.size() && (main_content[pos] == ' ' || main_content[pos] == '\t' || main_content[pos] == '\n' || main_content[pos] == '\r'))
                                pos++;

                            if (pos < main_content.size() && main_content[pos] == '-' && pos + 1 < main_content.size() && main_content[pos + 1] == '-')
                            {
                                // Skip comment line
                                size_t newline = main_content.find('\n', pos);
                                if (newline != std::string::npos)
                                    pos = newline + 1;
                                else
                                    break;
                            }
                            else
                            {
                                break;
                            }
                        }
                        insert_pos = pos;
                    }

                    main_content.insert(insert_pos, require_line);

                    std::ofstream main_file_write(main_lua_path);
                    main_file_write << main_content;
                    main_file_write.close();

                    // Clear cache for main.lua so it reloads
                    {
                        std::lock_guard<std::mutex> lock(m_scripts_mutex);
                        m_script_cache.erase(main_lua_path.string());
                    }

                    Output::send(STR("Added require() for '{}' to main.lua\n"), to_generic_string(module_name));
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_scripts_mutex);
            m_script_cache.erase(file_path.string());
        }

        // Check if mod is running; if not, clear selected state so dropdown uses filesystem scan
        bool mod_is_running = false;
        std::string mod_name = std::filesystem::path(mod_path).stem().string();
        for (const auto& mod : UE4SSProgram::get_program().m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod && to_string(lua_mod->get_name()) == mod_name && lua_mod->is_started())
            {
                mod_is_running = true;
                break;
            }
        }
        if (!mod_is_running)
        {
            m_selected_state = nullptr;
        }

        m_script_edit_path = file_path.string();
        const LuaScriptFile* script = load_script(m_script_edit_path);
        if (script && script->loaded)
        {
            m_script_editor.SetText(script->content);
            m_script_original_content = m_script_editor.GetText();
            m_script_is_dirty = false;
        }

        m_pending_editor_tab_switch = 1;

        Output::send(STR("Created new file '{}'\n"), to_generic_string(file_path.string()));
        return true;
    }
    //*/
} // namespace RC::GUI::Mods