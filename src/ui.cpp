#include "ui.hpp"
#include "fonts.hpp"
#include "phosphor_icons.hpp"
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
            amber, ImVec4(202.f/255.f,193.f/255.f,159.f/255.f,1.f),
            ImVec4(10.f/255.f,11.f/255.f,9.f/255.f,1.f),
            ImVec4(10.f/255.f,11.f/255.f,9.f/255.f,1.f),
            ImVec4(227.f/255.f,220.f/255.f,184.f/255.f,1.f)
        };
    }
    return {
        ImVec4(234.f/255.f,228.f/255.f,200.f/255.f,1.f),
        ImVec4(221.f/255.f,213.f/255.f,174.f/255.f,1.f),
        ImVec4(26.f/255.f,27.f/255.f,24.f/255.f,1.f),
        amber, ImVec4(95.f/255.f,89.f/255.f,64.f/255.f,1.f),
        ImVec4(17.f/255.f,18.f/255.f,15.f/255.f,1.f),
        ImVec4(21.f/255.f,22.f/255.f,19.f/255.f,1.f),
        ImVec4(234.f/255.f,228.f/255.f,200.f/255.f,1.f)
    };
}
ImU32 Color(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

struct ThemeState {
    bool loaded = false;
    bool dark = false;
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
                    state.dark = overlay["theme"] == "dark";
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
    // ImGui's default disabled opacity can make thin strokes disappear.
    style.DisabledAlpha = 0.88f;
    auto& c = style.Colors;
    c[ImGuiCol_WindowBg] = p.paper;
    c[ImGuiCol_ChildBg] = p.paper;
    c[ImGuiCol_PopupBg] = p.paper;
    // Replace ImGui's default dark-blue title bar in modals.
    c[ImGuiCol_TitleBg] = p.paper;
    c[ImGuiCol_TitleBgActive] = p.paper;
    c[ImGuiCol_TitleBgCollapsed] = p.paper;
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.f,0.f,0.f,.55f);
    c[ImGuiCol_Text] = p.ink;
    c[ImGuiCol_TextDisabled] = p.muted;
    c[ImGuiCol_Border] = p.ink;
    c[ImGuiCol_Separator] = p.ink;
    c[ImGuiCol_FrameBg] = p.fill;
    const bool dark = p.paper.x < 0.2f;
    const ImVec4 hover = dark ? ImVec4(.22f,.25f,.22f,1.f) : p.amber;
    const ImVec4 pressed = dark ? ImVec4(.29f,.32f,.28f,1.f)
                                : ImVec4(.84f,.60f,.12f,1.f);
    c[ImGuiCol_FrameBgHovered] = hover;
    c[ImGuiCol_FrameBgActive] = pressed;
    c[ImGuiCol_Button] = p.fill;
    c[ImGuiCol_ButtonHovered] = hover;
    c[ImGuiCol_ButtonActive] = pressed;
    c[ImGuiCol_Header] = dark ? p.fill : p.paper;
    c[ImGuiCol_HeaderHovered] = dark ? ImVec4(.24f,.28f,.23f,1.f) : p.fill;
    c[ImGuiCol_HeaderActive] = dark ? ImVec4(.32f,.36f,.29f,1.f) : p.fill;
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
    const bool compact = width < 570.f;

    const std::string status = std::to_string(
        std::count_if(mods.begin(), mods.end(), Enabled)) +
        "/" + std::to_string(mods.size()) + " ONLINE";
    const std::string keyText =
        (fonts::HasIcons() ? ARC_ICON_KEY " " : "KEY: ") + input.KeyLabel() + "##arc-bind";
    const char* closeLabel = fonts::HasIcons() ? ARC_ICON_CLOSE " CLOSE##arc" : "CLOSE##arc";
    const float themeWidth = std::max(140.f, ImGui::CalcTextSize("TOGGLE THEME").x + 24.f);
    const float shortcutWidth = std::clamp(ImGui::CalcTextSize(keyText.c_str()).x + 22.f,
                                          75.f, 116.f);
    const float closeWidth = std::max(90.f, ImGui::CalcTextSize(closeLabel).x + 24.f);
    const float statusWidth = ImGui::CalcTextSize(status.c_str()).x;

    const float headingFontSize = fonts::titleBold ? fonts::titleBold->FontSize :
        ImGui::GetFontSize();
    float brandWidth = 16.f + (fonts::HasIcons() ? 27.f : 0.f);
    if (fonts::titleBold) ImGui::PushFont(fonts::titleBold);
    if (!compact)
        brandWidth += ImGui::CalcTextSize("CONTROL PANEL").x +
                      ImGui::CalcTextSize("//").x + 12.f;
    brandWidth += ImGui::CalcTextSize("M1 ARC").x + 6.f;
    if (fonts::titleBold) ImGui::PopFont();
    (void)headingFontSize;

    const float actionsWidth = themeWidth + 10.f + shortcutWidth;
    const float rightWidth = statusWidth + 16.f + closeWidth;
    const bool secondRow = brandWidth + actionsWidth + rightWidth + 46.f > width;
    const bool thirdRow = secondRow && actionsWidth + rightWidth + 48.f > width;
    const float height = thirdRow ? 140.f : secondRow ? 98.f : 56.f;
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x+width, origin.y+height),
                        Color(p.bar));

    ImGui::PushStyleColor(ImGuiCol_Text,p.barText);
    ImGui::SetCursorScreenPos(ImVec2(origin.x+16.f, origin.y+13.f));
    if (fonts::HasIcons()) {
        ImGui::TextUnformatted(ARC_ICON_SLIDERS);
        ImGui::SameLine(0.f,6.f);
    }
    // Use ONE font/weight for the entire title: CONTROL PANEL // M1 ARC.
    if (fonts::titleBold) ImGui::PushFont(fonts::titleBold);
    if (!compact) {
        ImGui::TextUnformatted("CONTROL PANEL");
        ImGui::SameLine(0.f,6.f);
        ImGui::PushStyleColor(ImGuiCol_Text,p.amber);
        ImGui::TextUnformatted("//");
        ImGui::PopStyleColor();
        ImGui::SameLine(0.f,6.f);
    }
    ImGui::TextUnformatted("M1 ARC");
    if (fonts::titleBold) ImGui::PopFont();

    // All header controls have light text on dark backgrounds in both themes.
    const auto action = [&](const char* label, float x, float y, float w) {
        ImGui::SetCursorScreenPos(ImVec2(origin.x+x,origin.y+y));
        ImGui::PushStyleColor(ImGuiCol_Button,p.bar);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.21f,.24f,.20f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(.29f,.32f,.26f,1.f));
        ImGui::PushStyleColor(ImGuiCol_Text,p.barText);
        ImGui::PushStyleColor(ImGuiCol_Border,p.barText);
        const bool clicked=ImGui::Button(label,ImVec2(w,34.f));
        ImGui::PopStyleColor(5);
        return clicked;
    };
    const float buttonsY = secondRow ? 56.f : 10.f;
    const float themeX = secondRow ? 16.f : brandWidth+12.f;
    if (action("TOGGLE THEME##arc-theme",themeX,buttonsY,themeWidth))
        ToggleTheme();
    const float shortcutX = themeX + themeWidth + 10.f;
    if (action(keyText.c_str(),shortcutX,buttonsY,shortcutWidth))
        input.BeginRebind();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        // Do not inherit header cream foreground onto the cream tooltip.
        ImGui::PushStyleColor(ImGuiCol_Text,p.ink);
        ImGui::PushStyleColor(ImGuiCol_PopupBg,p.paper);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(12.f,9.f));
        ImGui::SetTooltip("Change M1 Arc's menu shortcut (independent of CET)");
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    const float rightY = thirdRow ? 100.f : secondRow ? 56.f : 10.f;
    const float closeX = std::max(8.f,width-closeWidth-14.f);
    const float statusX = std::max(8.f,closeX-statusWidth-16.f);
    ImGui::SetCursorScreenPos(ImVec2(origin.x+statusX,origin.y+rightY+8.f));
    ImGui::TextUnformatted(status.c_str());
    if (action(closeLabel,closeX,rightY,closeWidth))
        input.Close();
    ImGui::PopStyleColor();
    ImGui::SetCursorScreenPos(ImVec2(origin.x,origin.y+height));
}

std::string FitLabel(std::string label, float available) {
    if (available <= 12.f) return "...";
    if (ImGui::CalcTextSize(label.c_str()).x <= available) return label;
    while (!label.empty() &&
           ImGui::CalcTextSize((label + "...").c_str()).x > available) {
        // Trim one complete UTF-8 codepoint, never cut inside a byte sequence.
        label.pop_back();
        while (!label.empty() &&
            (static_cast<unsigned char>(label.back()) & 0xC0u) == 0x80u)
            label.pop_back();
        if (!label.empty()) label.pop_back(); // remove UTF-8 lead byte as well
    }
    return label + "...";
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
            // Keep selected rows readable AND visibly interactive on both themes.
            const bool darkTheme = p.paper.x < .2f;
            const ImVec4 selectedHover = darkTheme ? p.amber : ImVec4(.22f,.25f,.22f,1.f);
            const ImVec4 selectedPress = darkTheme ? ImVec4(.86f,.60f,.12f,1.f)
                                                  : ImVec4(.30f,.34f,.28f,1.f);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                selected ? selectedHover : p.fill);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive,
                selected ? selectedPress : p.fill);
            const ImVec2 row = ImGui::GetCursorScreenPos();
            const float rowWidth = std::max(1.f, ImGui::GetContentRegionAvail().x-2.f);
            constexpr float rowHeight = 54.f;
            const std::string prefix = fonts::HasIcons() ? ARC_ICON_CUBE "  " : "";
            const float prefixWidth = ImGui::CalcTextSize(prefix.c_str()).x;
            const std::string label=prefix +
                FitLabel(mod.title,std::max(15.f,rowWidth-prefixWidth-40.f));
            if (ImGui::Selectable(label.c_str(),selected,0,ImVec2(rowWidth,rowHeight)))
                SelectedMod() = mod.id;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s",mod.title.c_str());
            ImGui::PopStyleColor(4);
            const ImVec2 rowEnd(row.x+rowWidth,row.y+54.f);
            draw->AddLine(ImVec2(row.x,rowEnd.y-1.f),
                          ImVec2(rowEnd.x,rowEnd.y-1.f),Color(p.ink),2.f);
            const ImVec4 outline = selected ? p.paper : p.ink;
            const ImVec2 a(rowEnd.x-25.f,row.y+22.f);
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
    const bool dark = p.paper.x < .2f;
    const ImVec4 altHover = dark ? ImVec4(.22f,.25f,.22f,1.f) : p.amber;
    const ImVec4 altPress = dark ? ImVec4(.29f,.32f,.28f,1.f) : ImVec4(.84f,.60f,.12f,1.f);
    ImGui::PushStyleColor(ImGuiCol_Button,!value ? p.ink : p.fill);
    // OFF is an interactive selected button too: give it a contrast-safe
    // hover/press treatment instead of reusing its base color.
    const ImVec4 offHover = dark ? ImVec4(.98f,.94f,.77f,1.f)
                                 : ImVec4(.22f,.25f,.22f,1.f);
    const ImVec4 offPress = dark ? ImVec4(.87f,.80f,.58f,1.f)
                                 : ImVec4(.30f,.34f,.28f,1.f);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,!value ? offHover : altHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,!value ? offPress : altPress);
    ImGui::PushStyleColor(ImGuiCol_Text,!value ? p.paper : p.ink);
    if (ImGui::Button("OFF##switch",ImVec2(half,30.f)))
        if (value) (void)Settings().Set(mod.id,setting.id,false);
    ImGui::PopStyleColor(4);
    ImGui::SameLine(0,0);
    ImGui::PushStyleColor(ImGuiCol_Button,value ? p.amber : p.fill);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,value ? ImVec4(.98f,.75f,.24f,1.f) : altHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,value ? ImVec4(.86f,.60f,.08f,1.f) : altPress);
    ImGui::PushStyleColor(ImGuiCol_Text,value ? ImVec4(.10f,.11f,.09f,1.f) : p.ink);
    if (ImGui::Button("ON##switch",ImVec2(width-half,30.f)))
        if (!value) (void)Settings().Set(mod.id,setting.id,true);
    ImGui::PopStyleColor(4);
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
    const bool stacked = width < 430.f;
    const float controls = stacked ? std::min(250.f,width) :
        std::min(250.f,std::max(155.f,width*.38f));
    const float labelWidth = stacked ? width :
        std::max(25.f,width-controls-22.f);
    const float textHeight = ImGui::CalcTextSize(
        setting.label.c_str(), nullptr, false, labelWidth).y;
    const float height = stacked ?
        std::max(72.f,textHeight+54.f) : std::max(54.f,textHeight+18.f);
    const ImVec2 row = ImGui::GetCursorScreenPos();
    if (disabled) ImGui::BeginDisabled();

    ImGui::SetCursorScreenPos(ImVec2(row.x,row.y+8.f));
    ImGui::PushTextWrapPos(row.x+labelWidth);
    ImGui::TextWrapped("%s",setting.label.c_str());
    ImGui::PopTextWrapPos();
    if (!setting.description.empty() &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ImGui::SetTooltip("%s",setting.description.c_str());

    const float controlX = stacked ? row.x : row.x+width-controls;
    const float controlY = stacked ? row.y+textHeight+17.f :
        row.y+std::max(8.f,(height-34.f)*.5f);
    ImGui::SetCursorScreenPos(ImVec2(controlX,controlY));
    ImGui::SetNextItemWidth(controls);
    switch (setting.kind) {
    case SettingKind::Boolean:
        DrawSwitch(mod,setting,p,controls);
        break;
    case SettingKind::Integer: {
        int value = setting.value.get<int>();
        if (ImGui::SliderInt("##value",&value,
            static_cast<int>(setting.minimum),
            static_cast<int>(setting.maximum),"%d",
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

    // Rule spans the entire detail area, touching the sidebar divider and
    // the right edge. Footer TEXT retains a comfortable 24px inset.
    const ImVec2 foot(ImGui::GetWindowPos().x+1.f,
                      ImGui::GetWindowPos().y+height-footerHeight);
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddLine(foot,ImVec2(ImGui::GetWindowPos().x+width-1.f,foot.y),
                  Color(p.ink),2.f);
    ImGui::SetCursorScreenPos(ImVec2(foot.x+pad,foot.y+20.f));
    ImGui::TextDisabled("2026 (c) M1NSOOK");
    if (!Theme().message.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s",Theme().message.c_str());
    }
    if (selected) {
        ImGui::SetCursorScreenPos(ImVec2(foot.x+width-116.f-pad,foot.y+12.f));
        const bool dark = p.paper.x < .2f;
        ImGui::PushStyleColor(ImGuiCol_Button,p.ink);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            dark ? ImVec4(.98f,.76f,.28f,1.f) : ImVec4(.23f,.27f,.21f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            dark ? p.amber : ImVec4(.31f,.35f,.28f,1.f));
        ImGui::PushStyleColor(ImGuiCol_Text,p.paper);
        if (ImGui::Button(fonts::HasIcons() ? ARC_ICON_RESET " DEFAULTS##mod" : "DEFAULTS##mod",
                          ImVec2(116.f,32.f)))
            (void)Settings().ResetMod(selected->id);
        ImGui::PopStyleColor(4);
    }
    ImGui::EndChild();
}
void DrawRebindModal() {
    auto& input = InputRouter::Get();
    if (input.IsRecording()) ImGui::OpenPopup("M1 ARC - CHOOSE MENU KEY");
    ImGui::SetNextWindowSize(ImVec2(390,0),ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(18.f,18.f));
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_Text));
    if (ImGui::BeginPopupModal("M1 ARC - CHOOSE MENU KEY",nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize |
                              ImGuiWindowFlags_NoTitleBar)) {
        if (fonts::titleBold) ImGui::PushFont(fonts::titleBold);
        ImGui::TextUnformatted("M1 ARC  //  CHOOSE MENU KEY");
        if (fonts::titleBold) ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextUnformatted("Press your preferred keyboard key.");
        ImGui::TextDisabled("Independent of Cyber Engine Tweaks.");
        ImGui::TextDisabled("Windows keys, modifiers and Escape are reserved.");
        if (input.HasKey() && ImGui::Button("Cancel"))
            input.CancelRebind();
        if (!input.IsRecording()) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
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

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    // CET-like dimming is visual only and never owns gameplay input.
    // The background draw list is below the native control panel.
    ImGui::GetBackgroundDrawList(viewport)->AddRectFilled(
        viewport->Pos,
        ImVec2(viewport->Pos.x+viewport->Size.x,
               viewport->Pos.y+viewport->Size.y),
        IM_COL32(0,0,0,206));
    const float width = std::max(1.f,std::min(1280.f,viewport->Size.x-36.f));
    const float height = std::max(1.f,std::min(720.f,viewport->Size.y-36.f));
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
        const float navWidth = std::min(320.f,std::max(100.f,area.x*.26f));
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
