#include "ui.hpp"
#include "input.hpp"
#include "registry.hpp"
#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <string>

namespace m1arc {
namespace {
void DrawSetting(const ModView& mod, const SettingView& setting)
{
    ImGui::PushID(setting.id.c_str());
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(setting.label.c_str());
    if (!setting.description.empty() &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s",setting.description.c_str());
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-FLT_MIN);
    switch (setting.kind) {
        case SettingKind::Boolean: {
            bool value = setting.value.get<bool>();
            if (ImGui::Checkbox("##value",&value))
                Settings().Set(mod.id,setting.id,value);
            break;
        }
        case SettingKind::Integer: {
            int value = setting.value.get<int>();
            if (ImGui::SliderInt("##value",&value,
                static_cast<int>(setting.minimum),static_cast<int>(setting.maximum)))
                Settings().Set(mod.id,setting.id,value);
            break;
        }
        case SettingKind::Float: {
            float value = setting.value.get<float>();
            if (ImGui::SliderFloat("##value",&value,
                static_cast<float>(setting.minimum),static_cast<float>(setting.maximum)))
                Settings().Set(mod.id,setting.id,static_cast<double>(value));
            break;
        }
        case SettingKind::Choice: {
            const auto selected = setting.value.get<std::string>();
            if (ImGui::BeginCombo("##value",selected.c_str())) {
                for (const auto& candidate : setting.choices) {
                    const bool current = selected == candidate;
                    if (ImGui::Selectable(candidate.c_str(),current))
                        Settings().Set(mod.id,setting.id,candidate);
                    if (current) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            break;
        }
    }
    ImGui::PopID();
}

void DrawRegisteredMods()
{
    const auto mods = Settings().Snapshot();
    if (mods.empty()) {
        ImGui::TextDisabled("No registered mods yet.");
        ImGui::TextWrapped("REDscript mods can register controls through the M1Arc native functions.");
        return;
    }
    for (const auto& mod : mods) {
        ImGui::PushID(mod.id.c_str());
        if (ImGui::CollapsingHeader(mod.title.c_str(),ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("##setting-table",2,
                ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
                ImGui::TableSetupColumn("Setting",ImGuiTableColumnFlags_WidthStretch,0.62f);
                ImGui::TableSetupColumn("Value",ImGuiTableColumnFlags_WidthStretch,0.38f);
                for (const auto& setting : mod.settings) DrawSetting(mod,setting);
                ImGui::EndTable();
            }
        }
        ImGui::PopID();
    }
}
}
void DrawArcUI()
{
    auto& input = InputRouter::Get();
    // Submit at least one ImGui frame before enabling input capture.
    // The renderer signals readiness only after the GPU draw succeeds.
    if (!input.IsOpen()) return;
    ImGui::SetNextWindowSize(ImVec2(680,430),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                            ImGuiCond_FirstUseEver,ImVec2(0.5f,0.5f));
    if (ImGui::Begin("M1 ARC",nullptr,ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginTabBar("##arc-tabs")) {
            if (ImGui::BeginTabItem("Mod settings")) {
                DrawRegisteredMods();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("About")) {
                ImGui::TextUnformatted("M1 Arc");
                ImGui::TextDisabled("Native RED4ext / REDscript extension");
                ImGui::Separator();
                ImGui::TextUnformatted("Version 0.1.0");
                ImGui::TextUnformatted("Author: MISO @m1nsook");
                ImGui::TextWrapped("Mod settings and native REDscript functions. No Lua runtime, no additional scripting language.");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::Separator();
        ImGui::Text("Menu key: %s",input.KeyLabel().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Change key")) input.BeginRebind();
        ImGui::SameLine();
        if (ImGui::Button("Close")) input.Close();

        if (input.IsRecording()) ImGui::OpenPopup("M1 ARC - CHOOSE MENU KEY");
        if (ImGui::BeginPopupModal("M1 ARC - CHOOSE MENU KEY",
            nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Press your preferred keyboard key.");
            ImGui::TextDisabled("This key belongs to M1 Arc, not CET.");
            ImGui::TextDisabled("Windows keys, modifiers and Escape are reserved.");
            if (input.HasKey() && ImGui::Button("Cancel")) {
                // Cancelling a rebind must leave the previous key usable.
                input.CancelRebind();
                ImGui::CloseCurrentPopup();
            }
            if (!input.IsRecording()) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}
} // namespace m1arc
