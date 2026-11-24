#include "ui.hpp"
#include "fonts.hpp"
#include "localization.hpp"
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
#include <deque>
#include <utility>
#include <mutex>
#include <cstdio>

namespace m1arc {
namespace {

std::string Tr(std::string_view key, const char* fallback) {
    return Translations().Text(key, fallback);
}

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

struct Toast {
    std::string text;
    float elapsed  = 0.f;
    float duration = 3.2f;
    ToastKind kind = ToastKind::Info;
};
std::mutex& ToastMutex() { static std::mutex mutex; return mutex; }
std::deque<Toast>& ToastQueue() {
    static std::deque<Toast> queue;
    return queue;
}

void ToggleTheme() {
    auto& theme = Theme();
    const bool next = !theme.dark;
    if (Store().Update([&](nlohmann::json& state) {
        state["overlay"]["theme"] = next ? "dark" : "light";
    })) {
        theme.dark = next;
        theme.message.clear();
        ShowToast(next ? Tr("toast.theme_dark", "Your theme has been updated to Dark mode!")
                       : Tr("toast.theme_light", "Your theme has been updated to Light mode!"),
                  3.2f, ToastKind::Success);
    } else {
        theme.message = Tr("toast.theme_failed", "Theme could not be saved");
        ShowToast(theme.message, 3.2f, ToastKind::Error);
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

void ShowTooltip(const std::string& title,const std::string& message,const Palette& p) {
    if (title.empty() && message.empty()) return;
    const float wrap=std::max(100.f,std::min(360.f,ImGui::GetMainViewport()->Size.x-72.f));
    ImGui::PushStyleColor(ImGuiCol_Text,p.ink);
    ImGui::PushStyleColor(ImGuiCol_PopupBg,p.paper);
    ImGui::PushStyleColor(ImGuiCol_Border,p.ink);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(16.f,13.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8.f,7.f));
    if (ImGui::BeginTooltip()) {
        const float right=ImGui::GetCursorPosX()+wrap;
        ImGui::TextUnformatted(fonts::HasIcons() ? ARC_ICON_INFO : "[i]");
        ImGui::SameLine(0.f,8.f);
        ImGui::TextUnformatted("//");
        ImGui::SameLine(0.f,8.f);
        if (fonts::label) ImGui::PushFont(fonts::label);
        ImGui::PushTextWrapPos(right);
        ImGui::TextUnformatted(title.c_str());
        ImGui::PopTextWrapPos();
        if (fonts::label) ImGui::PopFont();
        if (!message.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Separator,p.muted);
            ImGui::Separator();
            ImGui::PopStyleColor();
            if (fonts::caption) ImGui::PushFont(fonts::caption);
            ImGui::PushStyleColor(ImGuiCol_Text,p.muted);
            ImGui::PushTextWrapPos(right);
            ImGui::TextUnformatted(message.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
            if (fonts::caption) ImGui::PopFont();
        }
        ImGui::EndTooltip();
    }
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);
}

std::string SettingValueLabel(const ModView& mod,const SettingView& setting,const nlohmann::json& value) {
    if (value.is_boolean()) return value.get<bool>() ? Tr("ui.on","ON") : Tr("ui.off","OFF");
    if (setting.kind==SettingKind::Choice && value.is_string())
        return Translations().Choice(mod.id,setting.id,value.get<std::string>());
    if (setting.kind==SettingKind::Hotkey && value.is_number_integer())
        return value.get<int>() == 0 ? Tr("ui.hotkey.unbound","Not assigned") : InputRouter::DescribeKey(value.get<int>());
    if (value.is_number_float()) {
        char buffer[48]{};
        std::snprintf(buffer,sizeof(buffer),"%.2f",value.get<double>());
        return buffer;
    }
    return value.is_string() ? value.get<std::string>() : value.dump();
}

void NotifySaved(const ModView& mod,const SettingView& setting,const nlohmann::json& value) {
    ShowToast(Tr("toast.setting_saved","Saved: ") + setting.label + " — " +
              SettingValueLabel(mod,setting,value),2.8f,ToastKind::Success);
}
bool SetFromUI(const ModView& mod,const SettingView& setting,const nlohmann::json& value,bool notify=true) {
    if (!Settings().Set(mod.id,setting.id,value)) {
        ShowToast(Tr("toast.setting_failed","Could not save: ") + setting.label,3.5f,ToastKind::Error);
        return false;
    }
    if (notify && value!=setting.value) {
        const auto committed=Settings().Get(mod.id,setting.id);
        if (committed) NotifySaved(mod,setting,*committed);
    }
    return true;
}
void RequestAction(const ModView& mod,const std::string& id,const std::string& label) {
    const bool queued=Settings().QueueAction(mod.id,id);
    ShowToast((queued ? Tr("toast.action_requested","Action requested: ") :
                        Tr("toast.action_failed","Action could not be queued: ")) + label,
              2.8f,queued ? ToastKind::Info : ToastKind::Error);
}

void DrawHeader(const Palette& p, const std::vector<ModView>& mods) {
    auto& input = InputRouter::Get();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const bool compact = width < 570.f;

    const std::string status = std::to_string(
        std::count_if(mods.begin(), mods.end(), Enabled)) +
        "/" + std::to_string(mods.size()) + " " + Tr("ui.online", "ONLINE");
    const std::string keyText =
        (fonts::HasIcons() ? ARC_ICON_KEY " " : "KEY: ") + input.KeyLabel() + "##arc-bind";
    const std::string closeLabel = (fonts::HasIcons() ? std::string(ARC_ICON_CLOSE) + " " : "") +
        Tr("ui.close", "CLOSE") + "##arc";
    const float themeWidth = std::max(140.f, ImGui::CalcTextSize(Tr("ui.theme", "TOGGLE THEME").c_str()).x + 24.f);
    const float shortcutWidth = std::clamp(ImGui::CalcTextSize(keyText.c_str()).x + 22.f,
                                          75.f, std::max(116.f,width*.36f));
    const float closeWidth = std::max(90.f, ImGui::CalcTextSize(closeLabel.c_str()).x + 24.f);
    const float statusWidth = ImGui::CalcTextSize(status.c_str()).x;

    const float headingFontSize = fonts::titleBold ? fonts::titleBold->FontSize :
        ImGui::GetFontSize();
    float brandWidth = 16.f + (fonts::HasIcons() ? 27.f : 0.f);
    if (fonts::titleBold) ImGui::PushFont(fonts::titleBold);
    if (!compact)
        brandWidth += ImGui::CalcTextSize(Tr("ui.panel", "CONTROL PANEL").c_str()).x +
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
        ImGui::TextUnformatted(Tr("ui.panel", "CONTROL PANEL").c_str());
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
    const std::string themeLabel = Tr("ui.theme", "TOGGLE THEME") + "##arc-theme";
    if (action(themeLabel.c_str(),themeX,buttonsY,themeWidth))
        ToggleTheme();
    const float shortcutX = themeX + themeWidth + 10.f;
    if (action(keyText.c_str(),shortcutX,buttonsY,shortcutWidth))
        input.BeginRebind();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(Tr("ui.shortcut_help", "Change M1 Arc's menu shortcut"),
                    input.KeyLabel() + "\n" +
                    Tr("ui.rebind_recovery", "Recovery: Ctrl + Shift + F11"),p);

    const float rightY = thirdRow ? 100.f : secondRow ? 56.f : 10.f;
    const float closeX = std::max(8.f,width-closeWidth-14.f);
    const float statusX = std::max(8.f,closeX-statusWidth-16.f);
    ImGui::SetCursorScreenPos(ImVec2(origin.x+statusX,origin.y+rightY+8.f));
    ImGui::TextUnformatted(status.c_str());
    if (action(closeLabel.c_str(),closeX,rightY,closeWidth))
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

// These Cynosure-inspired notifications use the ENTIRE surface as the
// status color. All text and Phosphor icons are dark for contrast. Drawing
// directly into the foreground list adds no focus/input-capturing window.
void DrawToasts(ImGuiViewport* viewport, const ImVec2& panelPos,
                const ImVec2& panelSize) {
    std::lock_guard lock(ToastMutex());
    auto& queue = ToastQueue();
    if (queue.empty() || !viewport) return;

    auto& toast = queue.front();
    toast.elapsed += std::max(0.f, ImGui::GetIO().DeltaTime);
    if (toast.elapsed >= toast.duration) {
        queue.pop_front();
        return;
    }

    constexpr float kHeight = 76.f;
    constexpr float kIntro = .28f;
    constexpr float kOutro = .42f;
    const float fadeStart = std::max(kIntro, toast.duration - kOutro);
    float alpha = 1.f;
    float movement = 0.f;
    if (toast.elapsed < kIntro) {
        float t = std::clamp(toast.elapsed / kIntro, 0.f, 1.f);
        t = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
        alpha = t;
        movement = (1.f - t) * 20.f;
    } else if (toast.elapsed > fadeStart) {
        const float t = std::clamp((toast.elapsed - fadeStart) / kOutro, 0.f, 1.f);
        alpha = 1.f - t;
        movement = -12.f * t;
    }

    // Full toast BACKGROUND colors, not accent strips. Success is the
    // complementary muted green; the other three are owner-specified.
    ImU32 background = IM_COL32(191, 198, 230, 255); // Info    #bfc6e6
    const char* label = "INFORMATION";
    const char* labelKey = "toast.info";
    const char* icon = ARC_ICON_INFO;
    const char* fallbackIcon = "i";
    switch (toast.kind) {
    case ToastKind::Info:
        break;
    case ToastKind::Warning:
        background = IM_COL32(243, 220, 167, 255); // Warning #f3dca7
        label = "WARNING";
        labelKey = "toast.warning";
        icon = ARC_ICON_WARNING;
        fallbackIcon = "!";
        break;
    case ToastKind::Success:
        background = IM_COL32(188, 218, 177, 255); // Success #bcdab1
        label = "SUCCESS";
        labelKey = "toast.success";
        icon = ARC_ICON_SUCCESS;
        fallbackIcon = "+";
        break;
    case ToastKind::Error:
        background = IM_COL32(232, 104, 104, 255); // Error   #e86868
        label = "ERROR";
        labelKey = "toast.error";
        icon = ARC_ICON_ERROR;
        fallbackIcon = "x";
        break;
    }

    // Prefer a floating card below the panel (user reference). If the
    // viewport is short, keep it visible rather than rendering offscreen.
    const float width = std::min(840.f, std::max(1.f, viewport->Size.x - 36.f));
    const float x = viewport->Pos.x + (viewport->Size.x - width) * .5f;
    const float idealY = panelPos.y + panelSize.y + 24.f;
    const float lastY = viewport->Pos.y + viewport->Size.y - kHeight - 18.f;
    const float y = std::max(viewport->Pos.y + 10.f,
                              std::min(idealY, lastY)) + movement;

    const auto faded = [alpha](ImU32 color) -> ImU32 {
        const auto original = (color >> 24) & 255u;
        const auto a = static_cast<ImU32>(original * alpha + .5f);
        return (color & 0x00FFFFFFu) | (a << 24);
    };
    const ImU32 dark = faded(IM_COL32(19, 25, 24, 255));
    const std::string statusLabel = Tr(labelKey, label);
    ImDrawList* draw = ImGui::GetForegroundDrawList(viewport);
    const ImVec2 topLeft(x, y), bottomRight(x + width, y + kHeight);
    draw->AddRectFilled(topLeft, bottomRight, faded(background), 4.f);
    draw->AddRect(topLeft, bottomRight, dark, 4.f, 0, 1.4f);

    // An outlined status module containing an authentic Phosphor glyph,
    // both dark on the pastel card. Plain letters are the safe fallback.
    constexpr float kIconBox = 42.f;
    const float iconX = x + 25.f;
    const float iconY = y + (kHeight - kIconBox) * .5f;
    draw->AddRect(ImVec2(iconX, iconY),
                  ImVec2(iconX + kIconBox, iconY + kIconBox),
                  dark, 3.f, 0, 1.4f);
    ImFont* iconFont = fonts::body ? fonts::body : ImGui::GetFont();
    const char* iconText = fonts::HasIcons() ? icon : fallbackIcon;
    const float glyphSize = fonts::HasIcons() ? 29.f : 22.f;
    const ImVec2 glyphBounds = iconFont->CalcTextSizeA(glyphSize, 1000.f,
                                                       0.f, iconText);
    draw->AddText(iconFont, glyphSize,
                  ImVec2(iconX + (kIconBox - glyphBounds.x) * .5f,
                         iconY + (kIconBox - glyphBounds.y) * .5f),
                  dark, iconText);

    const float textX = iconX + kIconBox + 28.f;
    ImFont* statusFont = fonts::titleBold ? fonts::titleBold : ImGui::GetFont();
    const float statusSize = 17.f;
    const ImVec2 statusBounds = statusFont->CalcTextSizeA(statusSize,
                                                         1000.f, 0.f, statusLabel.c_str());
    // Narrow displays prioritize readable message text over the label.
    const bool showStatus = width >= 460.f;
    const float statusX = x + width - 28.f - statusBounds.x;
    const float textEnd = showStatus ? statusX - 22.f : x + width - 20.f;
    ImFont* messageFont=fonts::body ? fonts::body : ImGui::GetFont();
    const float messageWidth=std::max(20.f,textEnd-textX);
    const ImVec2 textBounds=messageFont->CalcTextSizeA(messageFont->FontSize,10000.f,messageWidth,toast.text.c_str());
    const ImVec4 clip(textX,y+8.f,textEnd,y+kHeight-8.f);
    draw->AddText(messageFont,messageFont->FontSize,
                  ImVec2(textX,y+std::max(8.f,(kHeight-textBounds.y)*.5f)),
                  dark,toast.text.c_str(),nullptr,messageWidth,&clip);
    if (showStatus) {
        draw->AddText(statusFont, statusSize,
                      ImVec2(statusX, y + (kHeight - statusBounds.y) * .5f),
                      dark, statusLabel.c_str());
    }
}

// Phosphor Regular icon before each mod title, selected only by the
// mod author's scope field. A legacy manifest falls back to Toolbox.
const char* ModScopeIcon(std::string_view scope) {
    if (scope=="Gameplay") return ARC_SCOPE_GAME_CONTROLLER;
    if (scope=="Crafting") return ARC_SCOPE_WRENCH;
    if (scope=="Cheats") return ARC_SCOPE_TERMINAL;
    if (scope=="Weapons") return ARC_SCOPE_SWORD;
    if (scope=="Locations") return ARC_SCOPE_MAP_PIN;
    if (scope=="Appearance") return ARC_SCOPE_T_SHIRT;
    if (scope=="Characters") return ARC_SCOPE_USERS;
    if (scope=="UI/UX") return ARC_SCOPE_SLIDERS_SCOPE;
    return ARC_SCOPE_TOOLBOX;
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
        ImGui::TextDisabled("%s", Tr("ui.no_mods", "NO REGISTERED MODS").c_str());
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
            const bool hasDescription=!mod.shortDesc.empty();
            const float rowHeight=hasDescription ? 70.f : 58.f;
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
            const float nameX=row.x+(fonts::HasIcons() ? 45.f : 16.f);
            const float usable=std::max(20.f,rowWidth-(nameX-row.x)-30.f);
            ImFont* nameFont=fonts::label ? fonts::label : ImGui::GetFont();
            ImGui::PushFont(nameFont);
            const std::string label=FitLabel(mod.title,usable);
            ImGui::PopFont();
            const float titleY=hasDescription ? row.y+12.f
                : row.y+(rowHeight-nameFont->FontSize)*.5f;
            if (fonts::HasIcons()) {
                const char* icon=ModScopeIcon(mod.scope);
                draw->AddText(ImVec2(row.x+16.f,titleY),Color(foreground),icon);
            }
            draw->AddText(nameFont,nameFont->FontSize,ImVec2(nameX,titleY),Color(foreground),label.c_str());
            if (hasDescription) {
                ImFont* small=fonts::caption ? fonts::caption : ImGui::GetFont();
                const float smallSize=small->FontSize;
                ImGui::PushFont(small);
                const std::string detail=FitLabel(mod.shortDesc,usable);
                ImGui::PopFont();
                ImVec4 descriptionColor=foreground;
                descriptionColor.w*=.82f;
                draw->AddText(small,smallSize,ImVec2(nameX,row.y+38.f),
                              Color(descriptionColor),detail.c_str());
            }

            const ImVec2 bottom(row.x+rowWidth,row.y+rowHeight);
            draw->AddLine(ImVec2(row.x,bottom.y-1.f),
                          ImVec2(bottom.x,bottom.y-1.f),Color(p.ink),1.f);
            const ImVec2 a(bottom.x-24.f,row.y+(rowHeight-10.f)*.5f);
            draw->AddRect(a,ImVec2(a.x+10.f,a.y+10.f),
                          Color(selected ? p.paper : p.ink),0.f,0,2.f);
            if (Enabled(mod))
                draw->AddRectFilled(ImVec2(a.x+2.f,a.y+2.f),
                                    ImVec2(a.x+8.f,a.y+8.f),Color(p.amber));
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                ShowTooltip(mod.title,mod.shortDesc,p);
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
            (void)SetFromUI(mod,setting,target);
        ImGui::PopStyleColor(4);
    };
    const std::string off = Tr("ui.off", "OFF") + "##switch";
    const std::string on = Tr("ui.on", "ON") + "##switch";
    side(off.c_str(),false,half);
    ImGui::SameLine(0.f,0.f);
    side(on.c_str(),true,width-half);
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
        (void)SetFromUI(mod,setting,setting.choices[(index+count-1)%count]);
    ImGui::SameLine(0,0);
    // The center is a functional next-choice button, not a fake display.
    const std::string display = Translations().Choice(mod.id, setting.id, value);
    const std::string clipped=FitLabel(display,std::max(15.f,width-92.f));
    if (ImGui::Button((clipped+"##current").c_str(),
                      ImVec2(std::max(16.f,width-64.f),32.f)))
        (void)SetFromUI(mod,setting,setting.choices[(index+1)%count]);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(setting.label,display,p);
    ImGui::SameLine(0,0);
    if (ImGui::Button(">##next",ImVec2(32.f,32.f)))
        (void)SetFromUI(mod,setting,setting.choices[(index+1)%count]);
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
        std::max(54.f,textHeight+22.f);
    const ImVec2 row=ImGui::GetCursorScreenPos();

    if (disabled) ImGui::BeginDisabled();
    ImGui::SetCursorScreenPos(ImVec2(row.x,row.y+12.f));
    // PushTextWrapPos uses WINDOW-LOCAL X, not screen X (previously wrong).
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+labelWidth);
    ImGui::TextUnformatted(setting.label.c_str());
    ImGui::PopTextWrapPos();
    if (!setting.description.empty() &&
        ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled))
        ShowTooltip(setting.label,setting.description,p);

    const float x=stacked ? row.x : row.x+width-controls;
    const float y=stacked ? row.y+textHeight+22.f :
        row.y+std::max(12.f,(rowHeight-32.f)*.5f);
    ImGui::SetCursorScreenPos(ImVec2(x,y));
    ImGui::SetNextItemWidth(controls);
    const bool hasInlineAction = !setting.buttonId.empty();
    const float sliderWidth = hasInlineAction ? std::max(56.f,controls-92.f) : controls;
    const auto drawInlineAction = [&] {
        if (!hasInlineAction) return;
        ImGui::SameLine(0.f,8.f);
        const auto caption = FitLabel(setting.buttonLabel,76.f);
        if (ImGui::Button((caption+"##inline-action").c_str(),
                          ImVec2(std::max(32.f,controls-sliderWidth-8.f),32.f)))
            RequestAction(mod,setting.buttonId,setting.buttonLabel);
    };
    switch (setting.kind) {
    case SettingKind::Boolean:
        if (setting.checkbox) {
            bool checked = setting.value.get<bool>();
            if (ImGui::Checkbox("##checkbox", &checked))
                (void)SetFromUI(mod,setting,checked);
        } else {
            DrawSwitch(mod,setting,p,controls);
        }
        break;
    case SettingKind::Integer: {
        int value=setting.value.get<int>();
        const int lower=static_cast<int>(setting.minimum);
        const int upper=static_cast<int>(setting.maximum);
        ImGui::SetNextItemWidth(sliderWidth);
        static bool saved=true;
        const bool changed=ImGui::SliderInt("##value",&value,lower,upper,"%d",
                             ImGuiSliderFlags_AlwaysClamp);
        if (ImGui::IsItemActivated()) saved=true;
        if (changed) saved=SetFromUI(mod,setting,value,false);
        if (ImGui::IsItemDeactivatedAfterEdit() && saved) {
            const auto committed=Settings().Get(mod.id,setting.id);
            if (committed) NotifySaved(mod,setting,*committed);
        }
        DrawSliderGrabOutline(p,value,lower,upper,true);
        drawInlineAction();
        break;
    }
    case SettingKind::Float: {
        float value=setting.value.get<float>();
        const float lower=static_cast<float>(setting.minimum);
        const float upper=static_cast<float>(setting.maximum);
        ImGui::SetNextItemWidth(sliderWidth);
        static bool saved=true;
        const bool changed=ImGui::SliderFloat("##value",&value,lower,upper,"%.2f",
                               ImGuiSliderFlags_AlwaysClamp);
        if (ImGui::IsItemActivated()) saved=true;
        if (changed) saved=SetFromUI(mod,setting,static_cast<double>(value),false);
        if (ImGui::IsItemDeactivatedAfterEdit() && saved) {
            const auto committed=Settings().Get(mod.id,setting.id);
            if (committed) NotifySaved(mod,setting,*committed);
        }
        DrawSliderGrabOutline(p,value,lower,upper,false);
        drawInlineAction();
        break;
    }
    case SettingKind::Choice:
        DrawChoice(mod,setting,p,controls);
        break;
    case SettingKind::Action:
        if (ImGui::Button((FitLabel(setting.buttonLabel,controls-15.f)+"##action").c_str(),
                          ImVec2(controls,32.f)))
            RequestAction(mod,setting.id,setting.buttonLabel);
        break;
    case SettingKind::Hotkey: {
        auto& input = InputRouter::Get();
        const bool capturing = input.RecordingModKey(mod.id, setting.id);
        const int key = setting.value.get<int>();
        const std::string display = capturing
            ? Tr("ui.hotkey.press", "Press a key...")
            : key == 0 ? Tr("ui.hotkey.unbound", "Not assigned")
                       : InputRouter::DescribeKey(key);
        const float keyWidth = std::max(65.f, controls - 78.f);
        if (ImGui::Button((FitLabel(display,keyWidth-18.f)+"##mod-key").c_str(),
                          ImVec2(keyWidth,32.f)) && !capturing)
            if (!input.BeginModRebind(mod.id, setting.id))
                ShowToast(Tr("toast.capture_failed","Could not start shortcut capture"),3.2f,ToastKind::Warning);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ShowTooltip(setting.label,Tr("ui.hotkey.help",
                "Press a keyboard key to bind. Escape cancels; the M1 Arc menu key is reserved."),p);
        ImGui::SameLine(0.f,8.f);
        const std::string auxiliary = capturing
            ? Tr("ui.cancel", "Cancel") : Tr("ui.hotkey.clear", "Clear");
        if (ImGui::Button((auxiliary+"##mod-key-clear").c_str(),
                          ImVec2(std::max(40.f,controls-keyWidth-8.f),32.f))) {
            if (capturing) input.CancelModRebind();
            else (void)SetFromUI(mod,setting,0);
        }
        break;
    }
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
        if (fonts::titleHeavy) ImGui::PushFont(fonts::titleHeavy);
        ImGui::TextWrapped("%s",selected->title.c_str());
        if (fonts::titleHeavy) ImGui::PopFont();
        if (!selected->shortDesc.empty()) {
            if (fonts::caption) ImGui::PushFont(fonts::caption);
            ImGui::PushStyleColor(ImGuiCol_Text,p.muted);
            ImGui::TextWrapped("%s",selected->shortDesc.c_str());
            ImGui::PopStyleColor();
            if (fonts::caption) ImGui::PopFont();
        }
        ImGui::Dummy(ImVec2(1.f,12.f));
        // Settings are native live controls, derived from each REDscript mod's
        // own registered schema. Never substitute the HTML's demo mod data.
        // Group headers use the mod author's existing "group" field.
        // Legacy, ungrouped mods keep their exact previous row layout.
        const bool showGroups = std::any_of(selected->settings.begin(),
            selected->settings.end(), [](const SettingView& s) {
                return s.group != "General";
            });
        std::string previousGroup;
        for (const auto& setting : selected->settings) {
            if (showGroups && setting.group != previousGroup) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text,p.ink);
                if (fonts::titleBold) ImGui::PushFont(fonts::titleBold);
                ImGui::TextUnformatted(setting.group.c_str());
                if (fonts::titleBold) ImGui::PopFont();
                ImGui::PushStyleColor(ImGuiCol_Separator,p.muted);
                ImGui::Separator();
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                ImGui::Spacing();
                previousGroup = setting.group;
            }
            DrawSetting(*selected,setting,p,
                std::max(1.f,ImGui::GetContentRegionAvail().x-8.f),
                !active && !IsMasterSwitch(setting));
        }
    } else {
        ImGui::TextDisabled("%s", Tr("ui.no_mods_detail", "No registered mods yet.").c_str());
        ImGui::Spacing();
        ImGui::TextWrapped(
            "%s", Tr("ui.no_mods_help",
                "REDscript mods appear here after they register their settings through M1 Arc.").c_str());
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
    if (fonts::caption) ImGui::PushFont(fonts::caption);
    if (width >= 520.f) ImGui::TextDisabled("2026 (c) M1NSOOK");
    if (fonts::caption) ImGui::PopFont();
    const auto langs = Translations().Languages();
    const std::string currentLang = Translations().Current();
    const auto currentLangIt = std::find_if(langs.begin(), langs.end(),
        [&](const auto& lang) { return lang.code == currentLang; });
    const std::string nativeName = currentLangIt == langs.end() ? "English" : currentLangIt->name;
    ImGui::SetCursorScreenPos(ImVec2(foot.x+pad+(width>=520.f ? 175.f : 0.f), foot.y+12.f));
    ImGui::SetNextItemWidth(140.f);
    if (ImGui::BeginCombo("##m1arc-language",nativeName.c_str())) {
        for (const auto& lang : langs) {
            const bool selected = lang.code == currentLang;
            if (ImGui::Selectable(lang.name.c_str(),selected) && !selected) {
                const bool changed=Translations().Select(lang.code);
                ShowToast((changed ? Tr("toast.language_saved","Language updated: ") :
                                     Tr("toast.language_failed","Could not change language: ")) + lang.name,
                          3.2f,changed ? ToastKind::Success : ToastKind::Error);
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(Tr("ui.language", "Language"),nativeName,p);

    if (selected) {
        ImGui::SetCursorScreenPos(ImVec2(foot.x+width-116.f-pad,foot.y+12.f));
        const bool dark = p.paper.x < .2f;
        ImGui::PushStyleColor(ImGuiCol_Button,p.ink);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            dark ? ImVec4(.98f,.76f,.28f,1.f) : ImVec4(.23f,.27f,.21f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            dark ? p.amber : ImVec4(.31f,.35f,.28f,1.f));
        ImGui::PushStyleColor(ImGuiCol_Text,p.paper);
        if (ImGui::Button((std::string(fonts::HasIcons() ? ARC_ICON_RESET " " : "") +
                           Tr("ui.defaults", "DEFAULTS") + "##mod").c_str(),
                          ImVec2(116.f,32.f))) {
            const bool restored = Settings().ResetMod(selected->id);
            ShowToast(restored ? Tr("toast.defaults_restored_prefix", "Defaults restored for ") + selected->title
                               : Tr("toast.defaults_failed_prefix", "Failed to reset ") + selected->title,
                      3.2f, restored ? ToastKind::Success : ToastKind::Error);
        }
        ImGui::PopStyleColor(4);
    }
    ImGui::EndChild();
}
void DrawRebindModal(const Palette& p) {
    auto& input = InputRouter::Get();
    if (input.IsRecording()) ImGui::OpenPopup("M1 ARC - CHOOSE MENU KEY");
    const float width=std::max(160.f,std::min(460.f,ImGui::GetMainViewport()->Size.x-36.f));
    ImGui::SetNextWindowSize(ImVec2(width,0),ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(22.f,20.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8.f,9.f));
    ImGui::PushStyleColor(ImGuiCol_Text,ImGui::GetStyleColorVec4(ImGuiCol_Text));
    if (ImGui::BeginPopupModal("M1 ARC - CHOOSE MENU KEY",nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize |
                              ImGuiWindowFlags_NoTitleBar |
                              ImGuiWindowFlags_NoNavInputs)) {
        if (fonts::titleHeavy) ImGui::PushFont(fonts::titleHeavy);
        ImGui::TextWrapped("%s",Tr("ui.rebind_heading", "M1 ARC  //  CHOOSE MENU KEY").c_str());
        if (fonts::titleHeavy) ImGui::PopFont();
        ImGui::Separator();
        ImGui::TextWrapped("%s",Tr("ui.rebind_press","Click Capture, press a key or combination, then Save.").c_str());
        if (fonts::caption) ImGui::PushFont(fonts::caption);
        ImGui::PushStyleColor(ImGuiCol_Text,p.muted);
        ImGui::TextWrapped("%s",Tr("ui.rebind_reserved","Ctrl and Shift may be combined with a key. Alt, Windows and Escape are reserved.").c_str());
        ImGui::PopStyleColor();
        if (fonts::caption) ImGui::PopFont();
        ImGui::Spacing();
        const bool capturing=input.IsCapturing();
        const bool candidate=input.HasCandidate();
        const std::string label=candidate ? input.CandidateLabel() :
            capturing ? Tr("ui.hotkey.press","Press a key...") :
            input.HasKey() ? input.KeyLabel() : Tr("ui.hotkey.unbound","Not assigned");
        if (fonts::caption) ImGui::PushFont(fonts::caption);
        ImGui::TextDisabled("%s",(candidate ? Tr("ui.rebind_candidate","NEW SHORTCUT") :
            capturing ? Tr("ui.rebind_listening","LISTENING FOR INPUT") :
            Tr("ui.rebind_current","CURRENT SHORTCUT")).c_str());
        if (fonts::caption) ImGui::PopFont();
        ImGui::PushStyleColor(ImGuiCol_ChildBg,p.fill);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize,1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(12.f,10.f));
        if (ImGui::BeginChild("##shortcut-preview",ImVec2(0.f,48.f),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            if (fonts::label) ImGui::PushFont(fonts::label);
            ImGui::TextWrapped("%s",label.c_str());
            if (fonts::label) ImGui::PopFont();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        if (fonts::caption) ImGui::PushFont(fonts::caption);
        ImGui::TextDisabled("%s",Tr("ui.rebind_recovery","Recovery: Ctrl + Shift + F11").c_str());
        if (fonts::caption) ImGui::PopFont();
        ImGui::Separator();
        // The keys being recorded must not also activate Capture or Save via ImGui navigation.
        ImGui::PushItemFlag(ImGuiItemFlags_NoNav,true);
        const float buttonWidth=std::max(40.f,(ImGui::GetContentRegionAvail().x-16.f)/3.f);
        if (ImGui::Button(Tr("ui.rebind_capture","Capture").c_str(),ImVec2(buttonWidth,34.f)))
            input.StartCapture();
        ImGui::SameLine();
        ImGui::BeginDisabled(!candidate);
        ImGui::PushStyleColor(ImGuiCol_Button,candidate ? p.amber : p.fill);
        ImGui::PushStyleColor(ImGuiCol_Text,candidate ? ImVec4(.10f,.11f,.09f,1.f) : p.muted);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.98f,.76f,.28f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(.84f,.59f,.08f,1.f));
        if (ImGui::Button(Tr("ui.rebind_save","Save").c_str(),ImVec2(buttonWidth,34.f))) {
            if (input.ConfirmCandidate())
                ShowToast(Tr("toast.shortcut_saved","Menu shortcut saved: ") + input.KeyLabel(),3.2f,ToastKind::Success);
            else
                ShowToast(Tr("ui.rebind_failed","Shortcut could not be saved"),3.2f,ToastKind::Error);
        }
        ImGui::PopStyleColor(4);
        ImGui::EndDisabled();
        if (input.HasKey()) {
            ImGui::SameLine();
            if (ImGui::Button(Tr("ui.cancel","Cancel").c_str(),ImVec2(buttonWidth,34.f)))
                input.CancelRebind();
        }
        ImGui::PopItemFlag();
        if (!input.IsRecording()) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

} // namespace

void ShowToast(std::string message, float seconds, ToastKind kind) {
    if (message.empty()) return;
    std::lock_guard lock(ToastMutex());
    auto& queue = ToastQueue();
    if (std::any_of(queue.begin(),queue.end(),[&](const Toast& toast) {
            return toast.kind==kind && toast.text==message;
        })) return;
    queue.push_back({std::move(message), 0.f, std::max(0.6f, seconds), kind});
    while (queue.size() > 4) queue.pop_front(); // bounded backlog
}

void DrawArcUI() {
    auto& input = InputRouter::Get();
    if (!input.IsOpen()) {
        // First-run/recovery Save closes the panel; feedback must still be visible.
        auto* viewport=ImGui::GetMainViewport();
        DrawToasts(viewport,viewport->Pos,viewport->Size);
        return;
    }

    const auto& theme = Theme();
    const Palette palette = Colors(theme.dark);
    ApplyTheme(palette);
    auto mods = Settings().Snapshot();
    for (auto& mod : mods) Translations().Localize(mod);
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
    // Floating toast, centered below the panel when display height permits.
    const ImVec2 windowPos(viewport->GetCenter().x - width * 0.5f,
                           viewport->GetCenter().y - height * 0.5f);
    DrawToasts(viewport, windowPos, ImVec2(width, height));
    // First-run, rebind and emergency recovery share the same modal.
    DrawRebindModal(palette);
}
} // namespace m1arc
