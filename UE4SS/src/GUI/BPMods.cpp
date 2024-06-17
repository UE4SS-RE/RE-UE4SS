#include <GUI/BPMods.hpp>

#include <limits>
#include <unordered_map>
#include <vector>

#include <DynamicOutput/DynamicOutput.hpp>
#include <GUI/ImGuiUtility.hpp>
#include <Helpers/String.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/ReflectedFunction.hpp>
#include <Unreal/Core/Containers/Array.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UObjectGlobals.hpp>

#include <imgui.h>

namespace RC::GUI::BPMods
{
    using namespace RC::Unreal;

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
            auto Function = GetFunctionByNameInChain(STR("ModMenuButtonPressed"));
            if (!Function)
            {
                return;
            }

            ModMenuButtonPressed_Params Params{Index};
            ProcessEvent(Function, &Params);
        }
    };

    struct ModInfo
    {
        ModActor* ModActor{};
        std::string ModAuthor{};
        std::string ModDescription{};
        std::string ModVersion{};
        std::string ModName{};
        std::vector<std::string> ModButtons{};
    };

    auto render() -> void
    {
        static auto mod_actor_name = FName(STR("ModActor_C"), FNAME_Add);

        std::vector<UObject*> mod_actors{};
        UObjectGlobals::FindAllOf(mod_actor_name, mod_actors);
        for (size_t i = 0; i < mod_actors.size(); ++i)
        {
            const auto& mod_actor = mod_actors[i];
            auto mod_class = mod_actor->GetClassPrivate();

            auto mod_info = ModInfo{};

            mod_info.ModActor = static_cast<ModActor*>(mod_actor);

            static auto mod_author_name = FName(STR("ModAuthor"), FNAME_Add);
            auto mod_author_property = mod_class->FindProperty(mod_author_name);
            mod_info.ModAuthor = mod_author_property ? to_string(mod_author_property->ContainerPtrToValuePtr<FString>(mod_actor)->GetCharArray()) : "Unknown";

            static auto mod_description_name = FName(STR("ModDescription"), FNAME_Add);
            auto mod_description_property = mod_class->FindProperty(mod_description_name);
            mod_info.ModDescription =
                    mod_description_property ? to_string(mod_description_property->ContainerPtrToValuePtr<FString>(mod_actor)->GetCharArray()) : "Unknown";

            static auto mod_version_name = FName(STR("ModVersion"), FNAME_Add);
            auto mod_version_property = mod_class->FindProperty(mod_version_name);
            mod_info.ModVersion = mod_version_property ? to_string(mod_version_property->ContainerPtrToValuePtr<FString>(mod_actor)->GetCharArray()) : "Unknown";

            static auto mod_buttons_name = FName(STR("ModButtons"), FNAME_Add);
            auto mod_buttons_property = mod_class->FindProperty(mod_buttons_name);
            auto mod_buttons = mod_buttons_property ? mod_buttons_property->ContainerPtrToValuePtr<TArray<FString>>(mod_actor) : nullptr;
            if (mod_buttons)
            {
                for (FString& button_name : *mod_buttons)
                {
                    mod_info.ModButtons.emplace_back(to_string(button_name.GetCharArray()));
                }
            }

            auto mod_full_name = mod_class->GetFullName();
            auto mod_name_parts = explode_by_occurrence(mod_full_name, STR('/'));
            mod_info.ModName = to_string(mod_name_parts[mod_name_parts.size() - 1 - 1]);

            if (ImGui_TreeNodeEx(fmt::format("{}", mod_info.ModName).c_str(), fmt::format("{}_{}", mod_info.ModName, i).c_str(), ImGuiTreeNodeFlags_CollapsingHeader))
            {
                ImGui::Indent();
                ImGui::Text("Author: %s", mod_info.ModAuthor.c_str());
                ImGui::Text("Description: %s", mod_info.ModDescription.c_str());
                ImGui::Text("Version: %s", mod_info.ModVersion.c_str());
                if (ImGui_TreeNodeEx("Mod Buttons", fmt::format("{}_{}_ModButtons", mod_info.ModName, i).c_str(), ImGuiTreeNodeFlags_CollapsingHeader))
                {
                    ImGui::Indent();
                    for (size_t i2 = 0; i2 < mod_info.ModButtons.size(); ++i2)
                    {
                        if (i2 > std::numeric_limits<int32_t>::max())
                        {
                            continue;
                        }
                        const auto& mod_button = mod_info.ModButtons[i2];
                        if (ImGui::Button(fmt::format("{}", mod_button).c_str()))
                        {
                            Output::send(STR("Mod button {} hit.\n"), to_wstring(mod_button));
                            mod_info.ModActor->ModMenuButtonPressed(static_cast<int32_t>(i2));
                        }
                    }
                    ImGui::Unindent();
                }
                ImGui::Text("");
                ImGui::Unindent();
            }
        }
    }
} // namespace RC::GUI::BPMods
