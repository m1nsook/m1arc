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
    // A hovered slider must NOT turn into a solid amber bar.
    // Only the thumb uses the amber accent; the track remains neutral.
    c[ImGuiCol_FrameBgHovered] = dark ? ImVec4(.23f,.27f,.23f,1.f)
                                          : ImVec4(.83f,.80f,.68f,1.f);
    c[ImGuiCol_FrameBgActive] = dark ? ImVec4(.28f,.31f,.27f,1.f)
                                         : ImVec4(.78f,.74f,.61f,1.f);
    c[ImGuiCol_Button] = p.fill;
    c[ImGuiCol_ButtonHovered] = hover;
    c[ImGuiCol_ButtonActive] = pressed;
    c[ImGuiCol_Header] = dark ? p.fill : p.paper;
    c[ImGuiCol_HeaderHovered] = dark ? ImVec4(.24f,.28f,.23f,1.f) : p.fill;
    c[ImGuiCol_HeaderActive] = dark ? ImVec4(.32f,.36f,.29f,1.f) : p.fill;
    c[ImGuiCol_CheckMark] = p.amber;
    c[ImGuiCol_SliderGrab] = p.amber;
    c[ImGuiCol_SliderGrabActive] = ImVec4(.92f,.63f,.10f,1.f);
    c[ImGuiCol_ScrollbarBg] = p.fill;
    c[ImGuiCol_ScrollbarGrab] = p.ink;
    c[ImGuiCol_ScrollbarGrabHovered] = p.amber;
    c[ImGuiCol_ScrollbarGrabActive] = p.amber;
    c[ImGuiCol_NavHighlight] = p.amber;
    // Tooltips use the correct foreground even after colored header text.
    c[ImGuiCol_PlotHistogram] = p.amber;
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

void ShowTooltip(const std::string& message, const Palette& p) {
    if (message.empty()) return;
    const auto* viewport = ImGui::GetMainViewport();
    const float wrap = std::max(150.f,std::min(360.f,viewport->Size.x-80.f));
    ImGui::PushStyleColor(ImGuiCol_Text,p.ink);
    ImGui::PushStyleColor(ImGuiCol_PopupBg,p.paper);
    ImGui::PushStyleColor(ImGuiCol_Border,p.ink);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(14.f,12.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,1.f);
    // A tooltip with unbounded SetTooltip() escaped across the viewport.
    // The wrap position is WINDOW-LOCAL, not the screen-space X coordinate.
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+wrap);
    ImGui::TextUnformatted(message.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
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
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip("Change M1 Arc's menu shortcut (independent of CET)",p);

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
        std::size_t index=label.size()-1;
        while (index>0 &&
               (static_cast<unsigned char>(label[index]) & 0xC0u) == 0x80u)
            --index;
        label.resize(index); // trim exactly one UTF-8 codepoint
    }
    return label + "...";
}

void DrawSidebar(const Palette& p, const std::vector<ModView>& mods,
                 float width, float height) {
    ImGui::BeginChild("##arc-nav",ImVec2(width,height),ImGuiChildFlags_None,0);
    ImDrawList* draw=ImGui::GetWindowDrawList();
    const float right=ImGui::GetWindowPos().x+ImGui::GetWindowWidth()-1.f;
    draw->AddLine(ImVec2(right,ImGui::GetWindowPos().y),
                  ImVec2(right,ImGui::GetWindowPos().y+height),Color(p.ink),2.f);
    if (mods.empty()) {
        // The empty state is intentionally padded, but real mod rows
        // must start flush with the top of the sidebar under the header.
        ImGui::SetCursorPos(ImVec2(16.f,12.f));
        ImGui::TextDisabled("NO REGISTERED MODS");
    } else {
        ImGui::SetCursorPos(ImVec2(0.f,0.f));
        for (const auto& mod : mods) {
            const bool selected=SelectedMod()==mod.id;
            ImGui::PushID(mod.id.c_str());
            const bool dark=p.paper.x<.2f;
            const ImVec4 hoverBg=selected
                ? (dark ? p.amber : ImVec4(.22f,.25f,.22f,1.f))
                : p.fill;
            const ImVec4 pressedBg=selected
                ? (dark ? ImVec4(.85f,.59f,.12f,1.f) : ImVec4(.30f,.34f,.28f,1.f))
                : p.fill;
            ImGui::PushStyleColor(ImGuiCol_Header,selected ? p.ink : p.paper);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered,hoverBg);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive,pressedBg);

            const ImVec2 row=ImGui::GetCursorScreenPos();
            const float rowWidth=std::max(1.f,ImGui::GetContentRegionAvail().x-2.f);
            constexpr float rowHeight=58.f;
            if (ImGui::Selectable("##mod-row",selected,0,ImVec2(rowWidth,rowHeight)))
                SelectedMod()=mod.id;
            const bool hovered=ImGui::IsItemHovered();
            const bool held=ImGui::IsItemActive();

            // Selectable registers a full-width hitbox; draw the label inside
            // it with a 16px inset. Regular Selectable text had ZERO padding.
            ImVec4 foreground=p.ink;
            if (selected) {
                foreground=(dark && (hovered||held)) ? p.paper :
                           (!dark && (hovered||held)) ? p.paper : p.paper;
            }
            // For selected dark rows the base is pale; on hover it is amber.
            // Both need dark lettering (p.paper in the dark palette).
            const std::string prefix=fonts::HasIcons() ? ARC_ICON_CUBE "  " : "";
            const float usable=std::max(30.f,rowWidth-16.f-30.f);
            const std::string label=FitLabel(prefix+mod.title,usable);
            const float baseline=row.y+(rowHeight-ImGui::GetFontSize())*.5f;
            draw->AddText(ImVec2(row.x+16.f,baseline),Color(foreground),label.c_str());

            const ImVec2 bottom(row.x+rowWidth,row.y+rowHeight);
            draw->AddLine(ImVec2(row.x,bottom.y-1.f),
                          ImVec2(bottom.x,bottom.y-1.f),Color(p.ink),1.f);
            const ImVec2 a(bottom.x-24.f,row.y+(rowHeight-10.f)*.5f);
            draw->AddRect(a,ImVec2(a.x+10.f,a.y+10.f),
                          Color(selected ? p.paper : p.ink),0.f,0,2.f);
            if (Enabled(mod))
                draw->AddRectFilled(ImVec2(a.x+2.f,a.y+2.f),
                                    ImVec2(a.x+8.f,a.y+8.f),Color(p.amber));
            if (hovered)
                ShowTooltip(mod.title,p);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void DrawSwitch(const ModView& mod,const SettingView& setting,
                const Palette& p,float width) {
    const bool enabled=setting.value.get<bool>();
    const bool dark=p.paper.x<.2f;
    const float half=width*.5f;
    const auto side=[&](const char* label,bool target,float buttonWidth) {
        const bool selected=enabled==target;
        const ImVec4 background=selected ? (target ? p.amber : p.ink) : p.fill;
        // Hover on an unselected switch is neutral, NOT amber / enabled.
        const ImVec4 hover=selected
            ? (target ? ImVec4(.95f,.68f,.16f,1.f)
                      : dark ? ImVec4(.93f,.89f,.73f,1.f)
                             : ImVec4(.22f,.25f,.22f,1.f))
            : (dark ? ImVec4(.24f,.27f,.23f,1.f)
                    : ImVec4(.83f,.80f,.68f,1.f));
        const ImVec4 active=selected
            ? (target ? ImVec4(.84f,.59f,.08f,1.f)
                      : dark ? ImVec4(.85f,.80f,.64f,1.f)
                             : ImVec4(.29f,.32f,.27f,1.f))
            : (dark ? ImVec4(.28f,.32f,.27f,1.f)
                    : ImVec4(.78f,.74f,.61f,1.f));
        const ImVec4 foreground=selected
            ? (target ? ImVec4(.10f,.11f,.09f,1.f) : p.paper)
            : p.ink;
        ImGui::PushStyleColor(ImGuiCol_Button,background);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,active);
        ImGui::PushStyleColor(ImGuiCol_Text,foreground);
        if (ImGui::Button(label,ImVec2(buttonWidth,32.f)) && enabled!=target)
            (void)Settings().Set(mod.id,setting.id,target);
        ImGui::PopStyleColor(4);
    };
    side("OFF##switch",false,half);
    ImGui::SameLine(0.f,0.f);
    side("ON##switch",true,width-half);
}

void DrawSliderGrabOutline(const Palette& p,double value,
                           double minimum,double maximum,bool integer) {
    const ImVec2 a=ImGui::GetItemRectMin();
    const ImVec2 b=ImGui::GetItemRectMax();
    const float frameWidth=b.x-a.x;
    if (frameWidth<=8.f || b.y-a.y<=8.f) return;
    const float padding=2.f;
    const float size=frameWidth-2.f*padding;
    float grab=ImGui::GetStyle().GrabMinSize;
    if (integer && maximum>minimum)
        grab=std::max(grab,size/static_cast<float>(maximum-minimum+1.0));
    grab=std::min(grab,size);
    const float usable=std::max(0.f,size-grab);
    const double fraction=maximum>minimum ?
        std::clamp((value-minimum)/(maximum-minimum),0.0,1.0):0.0;
    const float x=a.x+padding+static_cast<float>(fraction)*usable;
    // ImGui's native thumb is still used for behavior. The outline only
    // makes its boundaries visible against neutral tracks on hover.
    ImGui::GetWindowDrawList()->AddRect(
        ImVec2(x,a.y+padding),
        ImVec2(x+grab,b.y-padding),Color(p.ink),0.f,0,1.5f);
}

void DrawChoice(const ModView& mod,const SettingView& setting,const Palette& p,float width) {
    const auto value = setting.value.get<std::string>();
    const auto it = std::find(setting.choices.begin(),setting.choices.end(),value);
    const int count = static_cast<int>(setting.choices.size());
    if (count <= 0) return;
    const int index = it == setting.choices.end() ? 0 :
        static_cast<int>(std::distance(setting.choices.begin(),it));
    if (ImGui::Button("<##previous",ImVec2(32.f,32.f)))
        (void)Settings().Set(mod.id,setting.id,setting.choices[(index+count-1)%count]);
    ImGui::SameLine(0,0);
    // The center is a functional next-choice button, not a fake display.
    const std::string clipped=FitLabel(value,std::max(15.f,width-92.f));
    if (ImGui::Button((clipped+"##current").c_str(),
                      ImVec2(std::max(16.f,width-64.f),32.f)))
        (void)Settings().Set(mod.id,setting.id,setting.choices[(index+1)%count]);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(value,p);
    ImGui::SameLine(0,0);
    if (ImGui::Button(">##next",ImVec2(32.f,32.f)))
        (void)Settings().Set(mod.id,setting.id,setting.choices[(index+1)%count]);
}

void DrawSetting(const ModView& mod,const SettingView& setting,
                 const Palette& p,float width,bool disabled) {
    ImGui::PushID(setting.id.c_str());
    const bool stacked=width<430.f;
    const float controls=stacked ? std::min(250.f,width) :
        std::min(250.f,std::max(155.f,width*.38f));
    const float labelWidth=stacked ? width :
        std::max(25.f,width-controls-24.f);
    const float textHeight=ImGui::CalcTextSize(
        setting.label.c_str(),nullptr,false,labelWidth).y;
    const float rowHeight=stacked ?
        std::max(84.f,textHeight+62.f) :
        std::max(62.f,textHeight+22.f);
    const ImVec2 row=ImGui::GetCursorScreenPos();

    if (disabled) ImGui::BeginDisabled();
    ImGui::SetCursorScreenPos(ImVec2(row.x,row.y+12.f));
    // PushTextWrapPos uses WINDOW-LOCAL X, not screen X (previously wrong).
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+labelWidth);
    ImGui::TextUnformatted(setting.label.c_str());
    ImGui::PopTextWrapPos();
    if (!setting.description.empty() &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(setting.description,p);

    const float x=stacked ? row.x : row.x+width-controls;
    const float y=stacked ? row.y+textHeight+22.f :
        row.y+std::max(12.f,(rowHeight-32.f)*.5f);
    ImGui::SetCursorScreenPos(ImVec2(x,y));
    ImGui::SetNextItemWidth(controls);
    switch (setting.kind) {
    case SettingKind::Boolean:
        DrawSwitch(mod,setting,p,controls);
        break;
    case SettingKind::Integer: {
        int value=setting.value.get<int>();
        const int lower=static_cast<int>(setting.minimum);
        const int upper=static_cast<int>(setting.maximum);
        if (ImGui::SliderInt("##value",&value,lower,upper,"%d",
                             ImGuiSliderFlags_AlwaysClamp))
            (void)Settings().Set(mod.id,setting.id,value);
        DrawSliderGrabOutline(p,value,lower,upper,true);
        break;
    }
    case SettingKind::Float: {
        float value=setting.value.get<float>();
        const float lower=static_cast<float>(setting.minimum);
        const float upper=static_cast<float>(setting.maximum);
        if (ImGui::SliderFloat("##value",&value,lower,upper,"%.2f",
                               ImGuiSliderFlags_AlwaysClamp))
            (void)Settings().Set(mod.id,setting.id,static_cast<double>(value));
        DrawSliderGrabOutline(p,value,lower,upper,false);
        break;
    }
    case SettingKind::Choice:
        DrawChoice(mod,setting,p,controls);
        break;
    }
    if (disabled) ImGui::EndDisabled();

    // Explicitly register row height with ImGui: SetCursorScreenPos alone
    // did not contribute reliable scrollable content height.
    ImGui::SetCursorScreenPos(row);
    ImGui::Dummy(ImVec2(std::max(1.f,width),rowHeight));
    ImGui::PopID();
}

void DrawDetail(const Palette& p, const std::vector<ModView>& mods,
                float width, float height) {
    ImGui::BeginChild("##arc-detail",ImVec2(width,height),ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar);
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
                      ImGuiChildFlags_None,0); // native scrollbar appears when content overflows
    // Keep 10px of top breathing room without indenting only the first
    // setting: ImGui advances subsequent rows back to the child's x=0.
    ImGui::SetCursorPosY(10.f);
    if (selected) {
        const bool active = Enabled(*selected);
        // Settings are native live controls, derived from each REDscript mod's
        // own registered schema. Never substitute the HTML's demo mod data.
        for (const auto& setting : selected->settings)
            DrawSetting(*selected,setting,p,
                std::max(1.f,ImGui::GetContentRegionAvail().x-8.f),
                !active && !IsMasterSwitch(setting));
    } else {
        ImGui::TextDisabled("No registered mods yet.");
        ImGui::Spacing();
        ImGui::TextWrapped(
            "REDscript mods appear here after they register their settings through M1 Arc.");
    }
    if (selected) ImGui::Dummy(ImVec2(1.f,16.f));
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
