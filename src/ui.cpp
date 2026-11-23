#include "ui.hpp"
#include "input.hpp"
#include "registry.hpp"
#include "state.hpp"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace m1arc {
namespace {

struct Palette {
    ImVec4 paper, fill, ink, amber, muted, stage, bar, barText;
};

// Colors are taken from the supplied m1arc.html; no HTML/browser is loaded.
Palette Colors(bool dark) {
    const ImVec4 amber(242.f/255.f,178.f/255.f,41.f/255.f,1.f);
    const ImVec4 muted(138.f/255.f,132.f/255.f,95.f/255.f,1.f);
    if (dark) {
        return {
            ImVec4(30.f/255.f,34.f/255.f,31.f/255.f,1.f),
            ImVec4(42.f/255.f,47.f/255.f,42.f/255.f,1.f),
            ImVec4(227.f/255.f,220.f/255.f,184.f/255.f,1.f),
            amber, muted,
            ImVec4(10.f/255.f,11.f/255.f,9.f/255.f,1.f),
            ImVec4(10.f/255.f,11.f/255.f,9.f/255.f,1.f),
            ImVec4(227.f/255.f,220.f/255.f,184.f/255.f,1.f)
        };
    }
    return {
        ImVec4(234.f/255.f,228.f/255.f,200.f/255.f,1.f),
        ImVec4(221.f/255.f,213.f/255.f,174.f/255.f,1.f),
        ImVec4(26.f/255.f,27.f/255.f,24.f/255.f,1.f),
        amber, muted,
        ImVec4(17.f/255.f,18.f/255.f,15.f/255.f,1.f),
        ImVec4(21.f/255.f,22.f/255.f,19.f/255.f,1.f),
        ImVec4(234.f/255.f,228.f/255.f,200.f/255.f,1.f)
    };
}
ImU32 Color(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

struct ThemeState {
    bool loaded = false;
    bool dark = true;
    std::string message;
};
ThemeState& Theme() {
    static ThemeState state;
    if (!state.loaded) {
        state.loaded = true;
        try {
            const auto saved = nlohmann::json::parse(Store().Snapshot());
            if (saved.contains("overlay") && saved["overlay"].is_object()) {
                const auto& overlay = saved["overlay"];
                if (overlay.contains("theme") && overlay["theme"].is_string())
                    state.dark = overlay["theme"] != "light";
            }
        } catch (...) { /* Keep the safe dark default; do not rewrite corrupt state. */ }
    }
    return state;
}
void ToggleTheme() {
    auto& theme = Theme();
    const bool next = !theme.dark;
    if (Store().Update([&](nlohmann::json& state) {
        state["overlay"]["theme"] = next ? "dark" : "light";
    })) {
        theme.dark = next;
        theme.message.clear();
    } else {
        theme.message = "Theme could not be saved";
    }
}
void ApplyTheme(const Palette& p) {
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.f;
    style.ChildRounding = 0.f;
    style.PopupRounding = 0.f;
    style.FrameRounding = 0.f;
    style.GrabRounding = 0.f;
    style.ScrollbarRounding = 0.f;
    style.TabRounding = 0.f;
    style.WindowBorderSize = 2.f;
    style.ChildBorderSize = 0.f;
    style.FrameBorderSize = 1.f;
    style.WindowPadding = ImVec2(0,0);
    style.FramePadding = ImVec2(9,6);
    style.ItemSpacing = ImVec2(8,7);
    style.ScrollbarSize = 12.f;
    style.GrabMinSize = 10.f;
    auto& c = style.Colors;
    c[ImGuiCol_WindowBg] = p.paper;
    c[ImGuiCol_ChildBg] = p.paper;
    c[ImGuiCol_PopupBg] = p.paper;
    c[ImGuiCol_Text] = p.ink;
    c[ImGuiCol_TextDisabled] = p.muted;
    c[ImGuiCol_Border] = p.ink;
    c[ImGuiCol_Separator] = p.ink;
    c[ImGuiCol_FrameBg] = p.fill;
    c[ImGuiCol_FrameBgHovered] = p.paper;
    c[ImGuiCol_FrameBgActive] = p.fill;
    c[ImGuiCol_Button] = p.fill;
    c[ImGuiCol_ButtonHovered] = p.amber;
    c[ImGuiCol_ButtonActive] = p.amber;
    c[ImGuiCol_Header] = p.ink;
    c[ImGuiCol_HeaderHovered] = p.fill;
    c[ImGuiCol_HeaderActive] = p.ink;
    c[ImGuiCol_CheckMark] = p.amber;
    c[ImGuiCol_SliderGrab] = p.amber;
    c[ImGuiCol_SliderGrabActive] = p.amber;
    c[ImGuiCol_ScrollbarBg] = p.fill;
    c[ImGuiCol_ScrollbarGrab] = p.ink;
    c[ImGuiCol_ScrollbarGrabHovered] = p.amber;
    c[ImGuiCol_ScrollbarGrabActive] = p.amber;
    c[ImGuiCol_NavHighlight] = p.amber;
}

bool Enabled(const ModView& mod) {
    for (const auto& row : mod.settings)
        if ((row.id == "enabled" || row.id == "active") &&
            row.kind == SettingKind::Boolean &&
            row.value.is_boolean())
            return row.value.get<bool>();
    // A registered mod without a master Enabled switch is still online.
    return true;
}
bool IsMasterSwitch(const SettingView& setting) {
    return (setting.id == "enabled" || setting.id == "active") &&
        setting.kind == SettingKind::Boolean;
}
std::string& SelectedMod() {
    static std::string selected;
    return selected;
}

void DrawHeader(const Palette& p, const std::vector<ModView>& mods) {
    auto& input = InputRouter::Get();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    constexpr float height = 42.f;
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin,ImVec2(origin.x+width,origin.y+height),Color(p.bar));

    ImGui::SetCursorScreenPos(ImVec2(origin.x+16.f, origin.y+12.f));
    ImGui::PushStyleColor(ImGuiCol_Text,p.barText);
    ImGui::TextUnformatted("CONTROL PANEL");
    ImGui::SameLine(0.f,5.f);
    ImGui::PushStyleColor(ImGuiCol_Text,p.amber);
    ImGui::TextUnformatted("//");
    ImGui::PopStyleColor();
    ImGui::SameLine(0.f,5.f);
    ImGui::TextUnformatted("M1 ARC");

    // Keep the theme button independent of the game's overlay hotkey.
    ImGui::SetCursorScreenPos(ImVec2(origin.x+224.f,origin.y+5.f));
    ImGui::PushStyleColor(ImGuiCol_Button,p.bar);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,p.ink);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,p.amber);
    ImGui::PushStyleColor(ImGuiCol_Border,p.barText);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,2.f);
    if (ImGui::Button("TOGGLE THEME##arc-theme",ImVec2(140.f,30.f)))
        ToggleTheme();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    // Rebinding must remain available after first run, regardless of theme.
    ImGui::SetCursorScreenPos(ImVec2(origin.x+372.f,origin.y+5.f));
    ImGui::PushStyleColor(ImGuiCol_Button,p.bar);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,p.ink);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,p.amber);
    const std::string keyText = "KEY: " + input.KeyLabel() + "##arc-bind";
    if (ImGui::Button(keyText.c_str(),ImVec2(144.f,30.f)))
        input.BeginRebind();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Change M1 Arc's menu key (independent of CET)");
    ImGui::PopStyleColor(3);

    // The source mockup is visual only; these are real registered mods.
    const auto online = std::count_if(mods.begin(),mods.end(),Enabled);
    const std::string status = std::to_string(online) + "/" +
        std::to_string(mods.size()) + " ONLINE";
    const float statusWidth = ImGui::CalcTextSize(status.c_str()).x;
    ImGui::SetCursorScreenPos(ImVec2(origin.x+width-statusWidth-98.f,origin.y+12.f));
    ImGui::TextUnformatted(status.c_str());

    ImGui::SetCursorScreenPos(ImVec2(origin.x+width-79.f,origin.y+5.f));
    ImGui::PushStyleColor(ImGuiCol_Button,p.bar);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,p.ink);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,p.amber);
    if (ImGui::Button("CLOSE##arc",ImVec2(68.f,30.f))) input.Close();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleColor(); // Header foreground
    ImGui::SetCursorScreenPos(ImVec2(origin.x,origin.y+height));
}

void DrawSidebar(const Palette& p, const std::vector<ModView>& mods, float width, float height) {
    ImGui::BeginChild("##arc-nav",ImVec2(width,height),ImGuiChildFlags_None,0);
    auto* draw = ImGui::GetWindowDrawList();
    const float right = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - 1.f;
    draw->AddLine(ImVec2(right,ImGui::GetWindowPos().y),
                  ImVec2(right,ImGui::GetWindowPos().y+height),Color(p.ink),2.f);
    if (mods.empty()) {
        ImGui::SetCursorPos(ImVec2(16,16));
        ImGui::TextDisabled("NO REGISTERED MODS");
    } else {
        for (const auto& mod : mods) {
            const bool selected = SelectedMod() == mod.id;
            ImGui::PushID(mod.id.c_str());
            ImGui::PushStyleColor(ImGuiCol_Text,selected ? p.paper : p.ink);
            ImGui::PushStyleColor(ImGuiCol_Header,selected ? p.ink : p.paper);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered,selected ? p.ink : p.fill);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive,selected ? p.ink : p.fill);
            const ImVec2 row = ImGui::GetCursorScreenPos();
            const float rowWidth = ImGui::GetContentRegionAvail().x-2.f;
            if (ImGui::Selectable(mod.title.c_str(),selected,0,ImVec2(rowWidth,44.f)))
                SelectedMod() = mod.id;
            ImGui::PopStyleColor(4);
            const ImVec2 rowEnd(row.x+rowWidth,row.y+44.f);
            draw->AddLine(ImVec2(row.x,rowEnd.y-1.f),
                          ImVec2(rowEnd.x,rowEnd.y-1.f),Color(p.ink),2.f);
            const ImVec4 outline = selected ? p.paper : p.ink;
            const ImVec2 a(rowEnd.x-25.f,row.y+18.f);
            const ImVec2 b(a.x+9.f,a.y+9.f);
            draw->AddRect(a,b,Color(outline),0.f,0,2.f);
            if (Enabled(mod))
                draw->AddRectFilled(ImVec2(a.x+2.f,a.y+2.f),
                                    ImVec2(b.x-2.f,b.y-2.f),Color(p.amber));
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}
void DrawSwitch(const ModView& mod, const SettingView& setting, const Palette& p, float width) {
    const bool value = setting.value.get<bool>();
    const float half = width * .5f;
    ImGui::PushStyleColor(ImGuiCol_Button,!value ? p.ink : p.fill);
    ImGui::PushStyleColor(ImGuiCol_Text,!value ? p.paper : p.ink);
    if (ImGui::Button("OFF##switch",ImVec2(half,30.f)))
        if (value) (void)Settings().Set(mod.id,setting.id,false);
    ImGui::PopStyleColor(2);
    ImGui::SameLine(0,0);
    ImGui::PushStyleColor(ImGuiCol_Button,value ? p.amber : p.fill);
    ImGui::PushStyleColor(ImGuiCol_Text,p.ink);
    if (ImGui::Button("ON##switch",ImVec2(width-half,30.f)))
        if (!value) (void)Settings().Set(mod.id,setting.id,true);
    ImGui::PopStyleColor(2);
}
void DrawChoice(const ModView& mod,const SettingView& setting,float width) {
    const auto value = setting.value.get<std::string>();
    const auto it = std::find(setting.choices.begin(),setting.choices.end(),value);
    const int count = static_cast<int>(setting.choices.size());
    if (count <= 0) return;
    const int index = it == setting.choices.end() ? 0 :
        static_cast<int>(std::distance(setting.choices.begin(),it));
    if (ImGui::Button("<##previous",ImVec2(32.f,30.f)))
        (void)Settings().Set(mod.id,setting.id,setting.choices[(index+count-1)%count]);
    ImGui::SameLine(0,0);
    // The center is a functional next-choice button, not a fake display.
    if (ImGui::Button((value+"##current").c_str(),ImVec2(std::max(16.f,width-64.f),30.f)))
        (void)Settings().Set(mod.id,setting.id,setting.choices[(index+1)%count]);
    ImGui::SameLine(0,0);
    if (ImGui::Button(">##next",ImVec2(32.f,30.f)))
        (void)Settings().Set(mod.id,setting.id,setting.choices[(index+1)%count]);
}

void DrawSetting(const ModView& mod, const SettingView& setting,
                 const Palette& p, float width, bool disabled) {
    ImGui::PushID(setting.id.c_str());
    const float controls = std::min(250.f,std::max(160.f,width*.38f));
    const ImVec2 row = ImGui::GetCursorScreenPos();
    constexpr float height = 40.f;
    if (disabled) ImGui::BeginDisabled();
    ImGui::SetCursorScreenPos(ImVec2(row.x,row.y+9.f));
    ImGui::TextUnformatted(setting.label.c_str());
    if (!setting.description.empty() &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s",setting.description.c_str());

    ImGui::SetCursorScreenPos(ImVec2(row.x+width-controls,row.y+4.f));
    ImGui::SetNextItemWidth(controls);
    switch (setting.kind) {
    case SettingKind::Boolean:
        DrawSwitch(mod,setting,p,controls);
        break;
    case SettingKind::Integer: {
        int value = setting.value.get<int>();
        const int min = static_cast<int>(setting.minimum);
        const int max = static_cast<int>(setting.maximum);
        if (ImGui::SliderInt("##value",&value,min,max,"%d",
                             ImGuiSliderFlags_AlwaysClamp))
            (void)Settings().Set(mod.id,setting.id,value);
        break;
    }
    case SettingKind::Float: {
        float value = setting.value.get<float>();
        if (ImGui::SliderFloat("##value",&value,
            static_cast<float>(setting.minimum),
            static_cast<float>(setting.maximum),"%.2f",
            ImGuiSliderFlags_AlwaysClamp))
            (void)Settings().Set(mod.id,setting.id,static_cast<double>(value));
        break;
    }
    case SettingKind::Choice:
        DrawChoice(mod,setting,controls);
        break;
    }
    if (disabled) ImGui::EndDisabled();
    ImGui::SetCursorScreenPos(ImVec2(row.x,row.y+height));
    ImGui::PopID();
}

void DrawDetail(const Palette& p, const std::vector<ModView>& mods,
                float width, float height) {
    ImGui::BeginChild("##arc-detail",ImVec2(width,height),ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    auto& current = SelectedMod();
    const auto it = std::find_if(mods.begin(),mods.end(),
        [&](const auto& mod){return mod.id==current;});
    const ModView* selected = it == mods.end() ? nullptr : &*it;
    constexpr float footerHeight = 60.f;
    constexpr float pad = 24.f;
    const float interior = std::max(0.f,ImGui::GetContentRegionAvail().x-2*pad);
    ImGui::SetCursorPos(ImVec2(pad,16.f));
    ImGui::BeginChild("##settings-scroll",
                      ImVec2(interior,std::max(20.f,height-footerHeight-32.f)),
                      ImGuiChildFlags_None,0);
    if (selected) {
        const bool active = Enabled(*selected);
        // Settings are native live controls, derived from each REDscript mod's
        // own registered schema. Never substitute the HTML's demo mod data.
        for (const auto& setting : selected->settings)
            DrawSetting(*selected,setting,p,
                std::max(0.f,ImGui::GetContentRegionAvail().x),
                !active && !IsMasterSwitch(setting));
    } else {
        ImGui::TextDisabled("No registered mods yet.");
        ImGui::Spacing();
        ImGui::TextWrapped(
            "REDscript mods appear here after they register their settings through M1 Arc.");
    }
    ImGui::EndChild();

    const ImVec2 foot(ImGui::GetWindowPos().x+pad,
                      ImGui::GetWindowPos().y+height-footerHeight);
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddLine(foot,ImVec2(foot.x+interior,foot.y),Color(p.ink),2.f);
    ImGui::SetCursorScreenPos(ImVec2(foot.x,foot.y+20.f));
    ImGui::TextDisabled("2026 (c) M1NSOOK");
    if (!Theme().message.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s",Theme().message.c_str());
    }
    if (selected) {
        ImGui::SetCursorScreenPos(ImVec2(foot.x+interior-116.f,foot.y+12.f));
        ImGui::PushStyleColor(ImGuiCol_Button,p.ink);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,p.amber);
        ImGui::PushStyleColor(ImGuiCol_Text,p.paper);
        if (ImGui::Button("DEFAULTS##mod",ImVec2(116.f,32.f)))
            (void)Settings().ResetMod(selected->id);
        ImGui::PopStyleColor(3);
    }
    ImGui::EndChild();
}
void DrawRebindModal() {
    auto& input = InputRouter::Get();
    if (input.IsRecording()) ImGui::OpenPopup("M1 ARC - CHOOSE MENU KEY");
    ImGui::SetNextWindowSize(ImVec2(390,0),ImGuiCond_Always);
    if (ImGui::BeginPopupModal("M1 ARC - CHOOSE MENU KEY",nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Press your preferred keyboard key.");
        ImGui::TextDisabled("Independent of Cyber Engine Tweaks.");
        ImGui::TextDisabled("Windows keys, modifiers and Escape are reserved.");
        if (input.HasKey() && ImGui::Button("Cancel"))
            input.CancelRebind();
        if (!input.IsRecording()) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

} // namespace

void DrawArcUI() {
    auto& input = InputRouter::Get();
    if (!input.IsOpen()) return;

    const auto& theme = Theme();
    const Palette palette = Colors(theme.dark);
    ApplyTheme(palette);
    const auto mods = Settings().Snapshot();
    if (SelectedMod().empty() ||
        std::none_of(mods.begin(),mods.end(),[](const ModView& mod) {
            return mod.id == SelectedMod();
        }))
        SelectedMod() = mods.empty() ? std::string{} : mods.front().id;

    const auto* viewport = ImGui::GetMainViewport();
    const float width = std::max(320.f,std::min(1280.f,viewport->Size.x-48.f));
    const float height = std::max(260.f,std::min(720.f,viewport->Size.y-48.f));
    ImGui::SetNextWindowPos(viewport->GetCenter(),ImGuiCond_Always,ImVec2(.5f,.5f));
    ImGui::SetNextWindowSize(ImVec2(width,height),ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("##M1ArcControlPanel",nullptr,flags)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,0));
        DrawHeader(palette,mods);
        const ImVec2 area = ImGui::GetContentRegionAvail();
        const float navWidth = std::min(320.f,std::max(130.f,area.x*.26f));
        DrawSidebar(palette,mods,navWidth,area.y);
        ImGui::SameLine(0.f,0.f);
        DrawDetail(palette,mods,std::max(0.f,area.x-navWidth),area.y);
        ImGui::PopStyleVar();
    }
    ImGui::End();

    // Keep existing first-run setup/rebind behavior, independently of the
    // visual theme. Does not touch the previously working input/router code.
    DrawRebindModal();
}
} // namespace m1arc
