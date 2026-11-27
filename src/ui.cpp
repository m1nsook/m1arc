#include "ui.hpp"
#include "fonts.hpp"
#include "upgrade.hpp"
#include "nexus_icon.hpp"
#include "arc_brand.hpp"
#include <Windows.h>
#include <shellapi.h>
#ifdef M1ARC_ENABLE_DEV_PREVIEW
#include "dev_fixture.hpp"
#endif
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
    (void)Settings().AppendLog(mod.id,LogLevel::Info,"M1 Arc: setting saved: "+setting.id+" = "+value.dump());
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
#ifdef M1ARC_ENABLE_DEV_PREVIEW
// The development fixture uses actual action registration and consumption.
// These notifications are deliberately simulated and cannot run without
// the opt-in marker having registered the fixture.
bool HandleDemoAction(const ModView& mod,const std::string& id) {
    if (!dev::previewEnabled.load()) return false;
    if (mod.id==dev::kToastDemoId) {
        if (!Settings().ConsumeAction(mod.id,id)) return false;
        if (id=="toast_info") ShowToast("DEMO // An informational notification.",3.2f,ToastKind::Info);
        else if (id=="toast_success") ShowToast("DEMO // Changes saved successfully.",3.2f,ToastKind::Success);
        else if (id=="toast_warning") ShowToast("DEMO // Check this setting before continuing.",3.2f,ToastKind::Warning);
        else if (id=="toast_error") ShowToast("DEMO // Simulated failure. No real error occurred.",3.2f,ToastKind::Error);
        else if (id=="toast_long") ShowToast("DEMO // This longer notification checks line wrapping, vertical spacing and clipping. The message should remain legible without overlapping the status label.",5.f,ToastKind::Info);
        else if (id=="toast_unicode") ShowToast("DEMO // Zażółć gęślą jaźń — 한국어 알림 테스트",4.f,ToastKind::Info);
        else if (id=="toast_short") ShowToast("DEMO // Short notification.",1.f,ToastKind::Success);
        else if (id=="toast_burst") {
            ShowToast("DEMO // Queue 1: Information",2.f,ToastKind::Info);
            ShowToast("DEMO // Queue 2: Success",2.f,ToastKind::Success);
            ShowToast("DEMO // Queue 3: Warning",2.f,ToastKind::Warning);
            ShowToast("DEMO // Queue 4: Error",2.f,ToastKind::Error);
            ShowToast("DEMO // Queue 4: Error",2.f,ToastKind::Error);
        } else if (id=="toast_close") {
            InputRouter::Get().Close();
            ShowToast("DEMO // M1 Arc is closed. Input should remain with the game.",4.f,ToastKind::Info);
        }
        return true;
    }
    if (mod.id=="m1arc.ui-test" || mod.id.starts_with("m1arc.ui-test.")) {
        if (!Settings().ConsumeAction(mod.id,id)) return false;
        ShowToast("DEMO // Action consumed: "+id+" (no gameplay effect)",2.8f,ToastKind::Info);
        return true;
    }
    return false;
}

#endif

void RequestAction(const ModView& mod,const std::string& id,const std::string& label) {
    const bool queued=Settings().QueueAction(mod.id,id);
#ifdef M1ARC_ENABLE_DEV_PREVIEW
    if (queued && HandleDemoAction(mod,id)) return;
#endif
    ShowToast((queued ? Tr("toast.action_requested","Action requested: ") :
                        Tr("toast.action_failed","Action could not be queued: ")) + label,
              2.8f,queued ? ToastKind::Info : ToastKind::Error);
}

bool& LogsMode() { static bool value=false; return value; }
std::string& PendingLogClear() { static std::string value; return value; }

void DrawHeader(const Palette& p, const std::vector<ModView>& mods) {
    auto& input = InputRouter::Get();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const bool compact = width < 570.f;

    const std::string status = std::to_string(
        std::count_if(mods.begin(), mods.end(), Enabled)) +
        "/" + std::to_string(mods.size()) + " " + Tr("ui.online", "ONLINE");
    const std::string keyText =
        (fonts::HasIcons() ? ARC_ICON_KEY "  |  " : "KEY  |  ") + input.KeyLabel() + "##arc-bind";
    const std::string closeLabel = (fonts::HasIcons() ? std::string(ARC_ICON_CLOSE) + " " : "") +
        Tr("ui.close", "CLOSE") + "##arc";
    const float themeWidth = 38.f;
    const float shortcutWidth = std::clamp(ImGui::CalcTextSize(keyText.c_str(),nullptr,true).x + 22.f,
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
    const std::string modeLabel=std::string(fonts::HasIcons() ?
        (LogsMode() ? ARC_ICON_SLIDERS : ARC_ICON_LOGS) :
        (LogsMode() ? Tr("ui.logs_settings","Settings").c_str() : Tr("ui.logs_title","Logs").c_str()))+"##arc-mode";
    const float modeWidth=fonts::HasIcons() ? 38.f : std::max(62.f,ImGui::CalcTextSize(modeLabel.c_str()).x+20.f);
    const float rightWidth = modeWidth + 10.f + statusWidth + 16.f + closeWidth;
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
    ImGui::TextUnformatted("M1 ARC");
    if (!compact) {
        ImGui::SameLine(0.f,6.f);
        ImGui::PushStyleColor(ImGuiCol_Text,p.amber);
        ImGui::TextUnformatted("//");
        ImGui::PopStyleColor();
        ImGui::SameLine(0.f,6.f);
        ImGui::TextUnformatted(Tr("ui.panel", "CONTROL PANEL").c_str());
    }
    if (fonts::titleBold) ImGui::PopFont();

    // All header controls have light text on dark backgrounds in both themes.
    const auto action = [&](const char* label, float x, float y, float w,bool active=false) {
        ImGui::SetCursorScreenPos(ImVec2(origin.x+x,origin.y+y));
        ImGui::PushStyleColor(ImGuiCol_Button,active ? p.amber : p.bar);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,active ? ImVec4(.98f,.76f,.28f,1.f) : ImVec4(.21f,.24f,.20f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,active ? ImVec4(.84f,.59f,.08f,1.f) : ImVec4(.29f,.32f,.26f,1.f));
        ImGui::PushStyleColor(ImGuiCol_Text,active ? ImVec4(.10f,.11f,.09f,1.f) : p.barText);
        ImGui::PushStyleColor(ImGuiCol_Border,p.barText);
        const bool clicked=ImGui::Button(label,ImVec2(w,34.f));
        ImGui::PopStyleColor(5);
        return clicked;
    };
    const float buttonsY = secondRow ? 56.f : 10.f;
    const float themeX = secondRow ? 16.f : brandWidth+12.f;
    const std::string themeLabel = std::string(fonts::HasIcons() ?
        (Theme().dark ? ARC_ICON_SUN : ARC_ICON_MOON) : (Theme().dark ? "L" : "D")) + "##arc-theme";
    if (action(themeLabel.c_str(),themeX,buttonsY,themeWidth))
        ToggleTheme();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(Tr("ui.theme","TOGGLE THEME"), Theme().dark ?
            Tr("ui.theme_light","Switch to light theme") : Tr("ui.theme_dark","Switch to dark theme"),p);
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
    const float modeX=std::max(8.f,statusX-modeWidth-10.f);
    if (action(modeLabel.c_str(),modeX,rightY,modeWidth,LogsMode())) {
        LogsMode()=!LogsMode();
        PendingLogClear().clear();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(LogsMode() ? Tr("ui.logs_settings","Settings") : Tr("ui.logs_title","Logs"),
                    LogsMode() ? Tr("ui.logs_back_help","Return to mod settings.") :
                                 Tr("ui.logs_switch_help","View diagnostic logs for the selected mod."),p);
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
    const float width = std::min(680.f, std::max(1.f, viewport->Size.x - 36.f));
    const std::string statusLabel = Tr(labelKey, label);
    ImFont* statusFont=fonts::label ? fonts::label : ImGui::GetFont();
    ImFont* messageFont=fonts::body ? fonts::body : ImGui::GetFont();
    const float statusSize=14.f;
    constexpr float kIconBox=42.f;
    constexpr float padding=16.f;
    constexpr float gap=16.f;
    const float textOffset=padding+kIconBox+gap;
    const float messageWidth=std::max(20.f,width-textOffset-padding);
    const ImVec2 statusBounds=statusFont->CalcTextSizeA(statusSize,10000.f,messageWidth,statusLabel.c_str());
    const ImVec2 textBounds=messageFont->CalcTextSizeA(messageFont->FontSize,10000.f,messageWidth,toast.text.c_str());
    const float contentHeight=statusBounds.y+5.f+textBounds.y;
    const float kHeight=std::min(std::max(1.f,viewport->Size.y-36.f),
        std::max(76.f,contentHeight+2.f*padding));
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
    ImDrawList* draw = ImGui::GetForegroundDrawList(viewport);
    const ImVec2 topLeft(x, y), bottomRight(x + width, y + kHeight);
    draw->AddRectFilled(topLeft, bottomRight, faded(background), 4.f);
    draw->AddRect(topLeft, bottomRight, dark, 4.f, 0, 1.4f);

    // An outlined status module containing an authentic Phosphor glyph,
    // both dark on the pastel card. Plain letters are the safe fallback.
    const float iconX = x + padding;
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

    // Status is above the message, not competing for horizontal space.
    const float textX=x+textOffset;
    const float textY=y+std::max(8.f,(kHeight-contentHeight)*.5f);
    const ImVec4 clip(textX,y+8.f,x+width-padding,y+kHeight-8.f);
    draw->AddText(statusFont,statusSize,ImVec2(textX,textY),dark,
                  statusLabel.c_str(),nullptr,messageWidth,&clip);
    draw->AddText(messageFont,messageFont->FontSize,
                  ImVec2(textX,textY+statusBounds.y+5.f),dark,
                  toast.text.c_str(),nullptr,messageWidth,&clip);
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
    // Fixed header above an independently scrollable list. Keep the
    // outer child stationary; only the inner list owns the scrollbar.
    ImGui::BeginChild("##arc-nav",ImVec2(width,height),ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 origin=ImGui::GetWindowPos();
    constexpr float headerHeight=36.f;
    auto* frame=ImGui::GetWindowDrawList();
    frame->AddRectFilled(origin,ImVec2(origin.x+width,origin.y+headerHeight),Color(p.bar));
    ImFont* headerFont=fonts::caption ? fonts::caption : ImGui::GetFont();
    const std::string count=std::to_string(mods.size());
    const std::string heading=Tr("ui.registered_mods","REGISTERED MODS");
    ImGui::PushFont(headerFont);
    const float countWidth=ImGui::CalcTextSize(count.c_str()).x;
    const std::string clipped=FitLabel(heading,std::max(20.f,width-countWidth-40.f));
    ImGui::PopFont();
    const float textY=origin.y+(headerHeight-headerFont->FontSize)*.5f;
    frame->AddText(headerFont,headerFont->FontSize,ImVec2(origin.x+16.f,textY),Color(p.barText),clipped.c_str());
    frame->AddText(headerFont,headerFont->FontSize,ImVec2(origin.x+width-16.f-countWidth,textY),Color(p.barText),count.c_str());
    ImGui::SetCursorPos(ImVec2(0.f,headerHeight));
    // Native ImGui scrollbar appears only when rows exceed available height.
    ImGui::BeginChild("##arc-nav-scroll",ImVec2(std::max(1.f,width-2.f),
                      std::max(1.f,height-headerHeight)),ImGuiChildFlags_None,0);
    ImDrawList* draw=ImGui::GetWindowDrawList();
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
            const std::string metadata=mod.author + (!mod.author.empty() && !mod.version.empty() ? "  |  " : "") +
                (mod.version.empty() ? "" : "v"+mod.version);
            const bool hasMetadata=!metadata.empty();
            const float rowHeight=hasDescription && hasMetadata ? 78.f :
                (hasDescription || hasMetadata ? 70.f : 58.f);
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
            const float titleY=(hasDescription || hasMetadata) ? row.y+10.f
                : row.y+(rowHeight-nameFont->FontSize)*.5f;
            if (fonts::HasIcons()) {
                const char* icon=ModScopeIcon(mod.scope);
                draw->AddText(ImVec2(row.x+16.f,titleY),Color(foreground),icon);
            }
            draw->AddText(nameFont,nameFont->FontSize,ImVec2(nameX,titleY),Color(foreground),label.c_str());
            if (hasMetadata) {
                ImFont* small=fonts::caption ? fonts::caption : ImGui::GetFont();
                ImGui::PushFont(small);
                const auto fitted=FitLabel(metadata,usable);
                ImGui::PopFont();
                draw->AddText(small,small->FontSize,ImVec2(nameX,row.y+32.f),
                              Color(foreground),fitted.c_str());
            }
            if (hasDescription) {
                ImFont* small=fonts::caption ? fonts::caption : ImGui::GetFont();
                const float smallSize=small->FontSize;
                ImGui::PushFont(small);
                const std::string detail=FitLabel(mod.shortDesc,usable);
                ImGui::PopFont();
                ImVec4 descriptionColor=foreground;
                descriptionColor.w*=.82f;
                draw->AddText(small,smallSize,ImVec2(nameX,row.y+(hasMetadata ? 50.f : 36.f)),
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
                ShowTooltip(mod.title,metadata + (metadata.empty() ? "" : "\n") + mod.shortDesc,p);
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    const float right=origin.x+width-1.f;
    frame->AddLine(ImVec2(right,origin.y),ImVec2(right,origin.y+height),Color(p.ink),2.f);
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


void OpenOfficialLink(const std::string& url) {
    // All callers use fixed HTTPS destinations or a validated profile segment.
    const std::wstring wide(url.begin(),url.end());
    const auto result=ShellExecuteW(nullptr,L"open",wide.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result)<=32)
        ShowToast(Tr("toast.link_failed","Could not open link"),3.2f,ToastKind::Error);
}

// Shared visual primitives for the control panel and every secondary window.
void AngularHeading(const Palette& p,const std::string& label) {
    const ImVec2 pos=ImGui::GetCursorScreenPos();
    const float w=std::max(1.f,ImGui::GetContentRegionAvail().x);
    constexpr float h=30.f;
    ImFont* font=fonts::titleBold ? fonts::titleBold : ImGui::GetFont();
    ImGui::PushFont(font);
    const float tab=std::min(w,std::max(40.f,ImGui::CalcTextSize(label.c_str()).x+58.f));
    const auto fitted=FitLabel(label,std::max(1.f,tab-50.f));
    ImGui::PopFont();
    auto* d=ImGui::GetWindowDrawList();
    d->AddLine(ImVec2(pos.x,pos.y+h-1),ImVec2(pos.x+w,pos.y+h-1),Color(p.muted));
    d->AddQuadFilled(pos,ImVec2(pos.x+tab,pos.y),ImVec2(pos.x+tab-std::min(14.f,tab/3),pos.y+h),ImVec2(pos.x,pos.y+h),Color(p.bar));
    d->PushClipRect(pos,ImVec2(pos.x+tab-std::min(14.f,tab/3),pos.y+h),true);
    d->AddText(ImVec2(pos.x+8.f,pos.y+6.f),Color(p.amber),"//");
    d->AddText(font,font->FontSize,ImVec2(pos.x+30.f,pos.y+(h-font->FontSize)*.5f),Color(p.barText),fitted.c_str());
    d->PopClipRect();
    ImGui::Dummy(ImVec2(w,h+6.f));
}

void ModalTitle(const Palette& p,const std::string& label,bool close) {
    const ImVec2 pos=ImGui::GetCursorScreenPos();
    const float w=std::max(1.f,ImGui::GetContentRegionAvail().x);
    constexpr float h=44.f;
    auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(pos,ImVec2(pos.x+w,pos.y+h),Color(p.bar));
    ImFont* font=fonts::titleBold ? fonts::titleBold : ImGui::GetFont();
    const auto closeText=Tr("ui.close","CLOSE");
    const std::string closeLabel=(fonts::HasIcons() ? ARC_ICON_CLOSE "  " : "")+closeText+"##modal-close";
    const float cw=close ? std::min(w*.45f,std::max(88.f,ImGui::CalcTextSize(closeLabel.c_str(),nullptr,true).x+20.f)) : 0.f;
    const float end=pos.x+w-(close ? cw+18.f : 8.f);
    ImGui::PushFont(font);
    const auto fitted=FitLabel(label,std::max(1.f,end-pos.x-36.f));
    ImGui::PopFont();
    d->PushClipRect(pos,ImVec2(std::max(pos.x+1.f,end),pos.y+h),true);
    d->AddText(ImVec2(pos.x+10.f,pos.y+(h-ImGui::GetFontSize())*.5f),Color(p.amber),"//");
    d->AddText(font,font->FontSize,ImVec2(pos.x+34.f,pos.y+(h-font->FontSize)*.5f),Color(p.barText),fitted.c_str());
    d->PopClipRect();
    if (close) {
        ImGui::SetCursorScreenPos(ImVec2(pos.x+w-cw-7.f,pos.y+6.f));
        ImGui::PushStyleColor(ImGuiCol_Button,p.bar);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.24f,.26f,.22f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(.32f,.34f,.28f,1.f));
        ImGui::PushStyleColor(ImGuiCol_Text,p.barText);
        ImGui::PushStyleColor(ImGuiCol_Border,p.barText);
        if (ImGui::Button(closeLabel.c_str(),ImVec2(cw,32.f))) ImGui::CloseCurrentPopup();
        ImGui::PopStyleColor(5);
    }
    ImGui::SetCursorScreenPos(pos);
    ImGui::Dummy(ImVec2(w,h+10.f));
}

bool LinkButton(const Palette& p,const char* id,const std::string& label,
                const char* icon,float width,float height=36.f) {
    ImGui::PushID(id);
    const bool clicked=ImGui::Button("##link",ImVec2(std::max(1.f,width),height));
    const auto a=ImGui::GetItemRectMin(), b=ImGui::GetItemRectMax();
    auto* d=ImGui::GetWindowDrawList();
    const bool hasIcons=fonts::HasIcons();
    const float left=hasIcons ? 34.f : 10.f;
    const float right=hasIcons ? 32.f : 18.f;
    const std::string fitted=FitLabel(label,std::max(1.f,b.x-a.x-left-right));
    const float y=a.y+(height-ImGui::GetFontSize())*.5f;
    const auto color=Color(p.ink);
    d->PushClipRect(a,b,true);
    if (hasIcons) d->AddText(ImVec2(a.x+10.f,y),color,icon);
    d->AddText(ImVec2(a.x+left,y),color,fitted.c_str());
    d->AddText(ImVec2(b.x-right+7.f,y),color,hasIcons ? ARC_ICON_EXTERNAL : ">");
    d->PopClipRect();
    ImGui::PopID();
    return clicked;
}

void DrawModHeader(const Palette& p,const ModView& mod) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(16.f,14.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8.f,7.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize,1.f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,p.paper);
    ImGui::PushStyleColor(ImGuiCol_Border,p.muted);
    ImGui::BeginChild("##mod-header",ImVec2(0,0),
        ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (fonts::HasIcons()) {
        ImGui::TextUnformatted(ModScopeIcon(mod.scope));
        ImGui::SameLine(0.f,8.f);
    }
    if (fonts::titleHeavy) ImGui::PushFont(fonts::titleHeavy);
    ImGui::TextWrapped("%s",mod.title.c_str());
    if (fonts::titleHeavy) ImGui::PopFont();
    const auto& description=mod.description.empty() ? mod.shortDesc : mod.description;
    if (!description.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text,p.muted);
        ImGui::TextWrapped("%s",description.c_str());
        ImGui::PopStyleColor();
    }
    if (!mod.author.empty() || !mod.version.empty() || !mod.nexusProfile.empty()) {
        ImGui::Separator();
        const auto label=Tr("ui.author_profile","View author profile");
        const float available=ImGui::GetContentRegionAvail().x;
        const std::string author=(fonts::HasIcons() ? ARC_ICON_USER "  " : "")+mod.author;
        const std::string version=std::string(fonts::HasIcons() ? ARC_ICON_TAG "  " : "")+"v"+mod.version;
        const float aw=mod.author.empty()?0.f:ImGui::CalcTextSize(author.c_str()).x;
        const float vw=mod.version.empty()?0.f:ImGui::CalcTextSize(version.c_str()).x;
        const float lw=mod.nexusProfile.empty()?0.f:ImGui::CalcTextSize(label.c_str()).x+72.f;
        bool previous=false;
        float used=0.f;
        const auto next=[&](float itemWidth) {
            if (previous && used+24.f+itemWidth<=available) {
                ImGui::SameLine(0.f,8.f);
                ImGui::TextColored(p.muted,"|");
                ImGui::SameLine(0.f,8.f);
                used+=24.f+itemWidth;
            } else used=itemWidth;
            previous=true;
        };
        if (!mod.author.empty()) {
            next(aw); ImGui::AlignTextToFramePadding(); ImGui::TextWrapped("%s",author.c_str());
        }
        if (!mod.version.empty()) {
            next(vw); ImGui::AlignTextToFramePadding(); ImGui::TextWrapped("%s",version.c_str());
        }
        if (!mod.nexusProfile.empty()) {
            next(lw);
            const std::string url="https://www.nexusmods.com/profile/"+mod.nexusProfile;
            if (LinkButton(p,"author-profile",label,ARC_ICON_USER,
                    std::min(lw,ImGui::GetContentRegionAvail().x),32.f)) OpenOfficialLink(url);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ShowTooltip(label,url,p);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    ImGui::Dummy(ImVec2(1.f,10.f));
}

constexpr const char* kAboutPopup="##arc-about";
void DrawAbout(const Palette& p) {
    // Do not alter the existing modal ownership or input router.
    if (!ImGui::IsPopupOpen(kAboutPopup)) return;
    const auto* viewport=ImGui::GetMainViewport();
    const float width=std::max(180.f,std::min(800.f,viewport->Size.x-36.f));
    ImGui::SetNextWindowPos(viewport->GetCenter(),ImGuiCond_Appearing,ImVec2(.5f,.5f));
    ImGui::SetNextWindowSize(ImVec2(width,0.f),ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(width,0.f),ImVec2(width,std::max(100.f,viewport->Size.y-36.f)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(18.f,16.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8.f,8.f));
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(14.f,8.f));
    if (ImGui::BeginPopupModal(kAboutPopup,nullptr,ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ModalTitle(p,Tr("ui.about","ABOUT")+" // M1 ARC",true);
        const float available=ImGui::GetContentRegionAvail().x;
        const auto branding=[&] {
            const float w=ImGui::GetContentRegionAvail().x;
            const float logo=std::min(132.f,w);
            const auto at=ImGui::GetCursorScreenPos();
            DrawArcBrand(ImGui::GetWindowDrawList(),ImVec2(at.x+(w-logo)*.5f,at.y),logo);
            ImGui::Dummy(ImVec2(w,logo+6.f));
            ImFont* font=fonts::titleHeavy ? fonts::titleHeavy : ImGui::GetFont();
            const float size=font->FontSize*1.45f;
            const auto titleAt=ImGui::GetCursorScreenPos();
            const auto textSize=font->CalcTextSizeA(size,10000.f,0.f,"M1 ARC");
            ImGui::GetWindowDrawList()->AddText(font,size,
                ImVec2(titleAt.x+std::max(0.f,(w-textSize.x)*.5f),titleAt.y),Color(p.ink),"M1 ARC");
            ImGui::Dummy(ImVec2(w,size+3.f));
            const auto subtitle=Tr("ui.about_subtitle","Mod configuration framework for Cyberpunk 2077");
            if (fonts::caption) ImGui::PushFont(fonts::caption);
            const auto textAt=ImGui::GetCursorScreenPos();
            const auto bounds=ImGui::CalcTextSize(subtitle.c_str(),nullptr,false,w);
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),ImGui::GetFontSize(),
                ImVec2(textAt.x+std::max(0.f,(w-bounds.x)*.5f),textAt.y),Color(p.muted),
                subtitle.c_str(),nullptr,w);
            ImGui::Dummy(ImVec2(w,bounds.y+4.f));
            if (fonts::caption) ImGui::PopFont();
            ImGui::Separator();
            const auto info=[&](const std::string& key,const char* value) {
                const float rowWidth=ImGui::GetContentRegionAvail().x;
                const float valueWidth=ImGui::CalcTextSize(value).x;
                const auto clipped=FitLabel(key,std::max(1.f,rowWidth-valueWidth-16.f));
                ImGui::TextColored(p.muted,"%s",clipped.c_str());
                ImGui::SameLine();
                const float edge=ImGui::GetCursorPosX()+ImGui::GetContentRegionAvail().x;
                ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(),edge-valueWidth));
                ImGui::TextUnformatted(value);
                ImGui::Separator();
            };
            info(Tr("ui.build_version","Build version"),M1ARC_BUILD_VERSION);
            info(Tr("ui.developer","Developer"),"m1nsook");
        };
        const auto links=[&] {
            AngularHeading(p,Tr("ui.official_links","OFFICIAL LINKS"));
            struct Entry { const char* key; const char* fallback; const char* url; const char* icon; };
            constexpr Entry entries[]={
                {"ui.about_nexus","M1 Arc on Nexus Mods","https://www.nexusmods.com/cyberpunk2077/mods/39311",ARC_SCOPE_GAME_CONTROLLER},
                {"ui.about_profile","My profile","https://www.nexusmods.com/profile/m1nsook/",ARC_ICON_USER},
                {"ui.about_source","Source code","https://github.com/m1nsook/m1arc",ARC_ICON_CODE},
                {"ui.about_docs","Documentation","https://m1nsook.github.io/m1arc/",ARC_ICON_BOOK},
                {"ui.about_discord","Official Discord server","https://discord.gg/mAYhepremH",ARC_ICON_DISCORD}
            };
            for (const auto& entry:entries) {
                const auto label=Tr(entry.key,entry.fallback);
                if (LinkButton(p,entry.key,label,entry.icon,ImGui::GetContentRegionAvail().x,38.f))
                    OpenOfficialLink(entry.url);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                    ShowTooltip(label,entry.url,p);
            }
        };
        if (available>=590.f && ImGui::BeginTable("##about-columns",2,
                ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings)) {
            ImGui::TableNextColumn(); branding();
            ImGui::TableNextColumn(); links();
            ImGui::EndTable();
        } else { branding(); links(); }
        ImGui::Separator();
        ImGui::TextDisabled("2026 (c) M1NSOOK");
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(3);
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
        DrawModHeader(p,*selected);
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
                AngularHeading(p,setting.group);
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
    ImGui::SetCursorScreenPos(ImVec2(foot.x+pad,foot.y+12.f));
    ImGui::PushStyleColor(ImGuiCol_Button,p.ink);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,Theme().dark ? p.amber : ImVec4(.23f,.27f,.21f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,Theme().dark ? p.amber : ImVec4(.31f,.35f,.28f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,p.paper);
    if (ImGui::Button(fonts::HasIcons() ? ARC_ICON_INFO "##about" : "i##about",ImVec2(38.f,32.f)))
        ImGui::OpenPopup(kAboutPopup);
    ImGui::PopStyleColor(4);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(Tr("ui.about","ABOUT"),"M1 ARC",p);
    const auto langs = Translations().Languages();
    const std::string currentLang = Translations().Current();
    const auto currentLangIt = std::find_if(langs.begin(), langs.end(),
        [&](const auto& lang) { return lang.code == currentLang; });
    const std::string nativeName = currentLangIt == langs.end() ? "English" : currentLangIt->name;
    ImGui::SetCursorScreenPos(ImVec2(foot.x+pad+48.f, foot.y+12.f));
    ImGui::SetNextItemWidth(std::max(55.f,std::min(140.f,width-2.f*pad-48.f-(selected ? 124.f : 0.f))));
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
            if (restored) (void)Settings().AppendLog(selected->id,LogLevel::Info,"M1 Arc: defaults restored");
            ShowToast(restored ? Tr("toast.defaults_restored_prefix", "Defaults restored for ") + selected->title
                               : Tr("toast.defaults_failed_prefix", "Failed to reset ") + selected->title,
                      3.2f, restored ? ToastKind::Success : ToastKind::Error);
        }
        ImGui::PopStyleColor(4);
    }
    DrawAbout(p);
    ImGui::EndChild();
}
ImVec4 LogColor(LogLevel level,const Palette& p) {
    const bool dark=p.paper.x<.2f;
    switch(level) {
    case LogLevel::Debug: return p.muted;
    case LogLevel::Info: return dark ? ImVec4(.65f,.80f,.94f,1.f) : ImVec4(.18f,.32f,.52f,1.f);
    case LogLevel::Warning: return dark ? p.amber : ImVec4(.49f,.30f,.04f,1.f);
    case LogLevel::Error: return dark ? ImVec4(1.f,.55f,.53f,1.f) : ImVec4(.65f,.12f,.12f,1.f);
    }
    return p.ink;
}

void DrawLogs(const Palette& p,const std::vector<ModView>& mods,float width,float height) {
    ImGui::BeginChild("##arc-logs",ImVec2(width,height),ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const auto selected=std::find_if(mods.begin(),mods.end(),[](const ModView& mod){
        return mod.id==SelectedMod();
    });
    constexpr float pad=24.f, footerHeight=42.f;
    ImGui::SetCursorPos(ImVec2(pad,16.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8.f,7.f));
    ImGui::BeginChild("##log-header",ImVec2(std::max(1.f,width-2.f*pad),0.f),
                      ImGuiChildFlags_AutoResizeY,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    AngularHeading(p,Tr("ui.logs_title","Logs")+" // "+
        (selected==mods.end() ? Tr("ui.no_mods","NO REGISTERED MODS") : selected->title));
    if (fonts::caption) ImGui::PushFont(fonts::caption);
    ImGui::TextDisabled("%s",Tr("ui.logs_session","Current session · Newest first").c_str());
    if (fonts::caption) ImGui::PopFont();

    std::vector<LogEntry> rows;
    if (selected!=mods.end()) rows=Settings().Logs(selected->id);
    auto& confirmation=PendingLogClear();
    if (selected==mods.end() || confirmation!=selected->id) confirmation.clear();
    const bool confirming=!confirmation.empty();
    const std::string copyLabel=Tr("ui.logs_copy","Copy diagnostics");
    const std::string clearLabel=confirming ? Tr("ui.logs_confirm","Confirm clear") : Tr("ui.logs_clear","Clear logs");
    const std::string cancelLabel=Tr("ui.cancel","Cancel");
    const float copyWidth=ImGui::CalcTextSize(copyLabel.c_str()).x+24.f;
    const float clearWidth=ImGui::CalcTextSize(clearLabel.c_str()).x+24.f;
    const float cancelWidth=ImGui::CalcTextSize(cancelLabel.c_str()).x+24.f;
    const float available=ImGui::GetContentRegionAvail().x;
    // Even with no selected/registered mod or cleared history, the report
    // can diagnose startup/manifest failures and settings configuration.
    if (ImGui::Button(copyLabel.c_str(),ImVec2(std::min(copyWidth,available),32.f))) {
        const std::string text=Settings().DiagnosticReport(selected==mods.end() ? std::string_view{} : std::string_view(selected->id));
        ImGui::SetClipboardText(text.c_str());
        const char* copied=ImGui::GetClipboardText();
        const bool ok=copied && text==copied;
        ShowToast(ok ? Tr("toast.logs_copied","Diagnostics copied to clipboard") :
                       Tr("toast.logs_copy_failed","Could not copy diagnostics to clipboard"),
                  2.8f,ok ? ToastKind::Success : ToastKind::Error);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        ShowTooltip(Tr("ui.logs_copy","Copy diagnostics"),
            Tr("ui.logs_copy_help","Copies the selected mod's settings, logs and framework startup diagnostics. Review before sharing."),p);
    if (copyWidth+8.f+clearWidth<=available) ImGui::SameLine();
    ImGui::BeginDisabled(rows.empty());
    if (ImGui::Button(clearLabel.c_str(),ImVec2(std::min(clearWidth,available),32.f))) {
        if (confirming) {
            if (Settings().ClearLogs(selected->id)) {
                rows.clear();
                ShowToast(Tr("toast.logs_cleared","Selected mod logs cleared"),2.8f,ToastKind::Success);
            } else ShowToast(Tr("toast.logs_clear_failed","Could not clear logs"),3.2f,ToastKind::Error);
            confirmation.clear();
        } else confirmation=selected->id;
    }
    ImGui::EndDisabled();
    if (confirming && !confirmation.empty()) {
        const float used=copyWidth+8.f+clearWidth<=available ? copyWidth+8.f+clearWidth : clearWidth;
        if (used+8.f+cancelWidth<=available) ImGui::SameLine();
        if (ImGui::Button(cancelLabel.c_str(),ImVec2(std::min(cancelWidth,available),32.f)))
            confirmation.clear();
    }
    ImGui::Separator();
    ImGui::EndChild();

    ImGui::SetCursorPosX(pad);
    if (selected!=mods.end()) ImGui::PushID(selected->id.c_str());
    const float scrollHeight=std::max(1.f,height-ImGui::GetCursorPosY()-footerHeight);
    ImGui::BeginChild("##log-scroll",ImVec2(std::max(1.f,width-2.f*pad),scrollHeight),
                      ImGuiChildFlags_None,0);
    if (rows.empty()) {
        ImGui::TextWrapped("%s",Tr("ui.logs_empty","No logs for this mod in the current session.").c_str());
    } else {
        for (const auto& entry:rows) {
            if (fonts::caption) ImGui::PushFont(fonts::caption);
            ImGui::TextDisabled("%s",entry.timestamp.c_str());
            ImGui::SameLine(0.f,12.f);
            if (fonts::caption) ImGui::PopFont();
            if (fonts::label) ImGui::PushFont(fonts::label);
            ImGui::TextColored(LogColor(entry.level,p),"%s",LogLevelName(entry.level));
            if (fonts::label) ImGui::PopFont();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::max(1.f,ImGui::GetContentRegionAvail().x-6.f));
            ImGui::TextUnformatted(entry.message.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PushStyleColor(ImGuiCol_Separator,p.muted);
            ImGui::Separator();
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();
    if (selected!=mods.end()) ImGui::PopID();
    ImGui::SetCursorPos(ImVec2(pad,std::max(0.f,height-footerHeight+8.f)));
    if (fonts::caption) ImGui::PushFont(fonts::caption);
    ImGui::TextDisabled("%s",(std::to_string(rows.size())+" / "+
        std::to_string(Registry::kLogsPerMod)+" · "+Tr("ui.logs_entries","entries")).c_str());
    if (fonts::caption) ImGui::PopFont();
    ImGui::PopStyleVar();
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
        ModalTitle(p,Tr("ui.rebind_heading","M1 ARC  //  MENU SHORTCUT"),false);
        const auto upgradeDoc=nlohmann::json::parse(Store().Snapshot(),nullptr,false);
        if (upgrade::NeedsShortcut(upgradeDoc))
            ImGui::TextWrapped("%s",Tr("ui.upgrade_rebind","M1 Arc has been updated. Choose your menu shortcut again to complete the update. Your mod settings are preserved.").c_str());
        else
            ImGui::TextWrapped("%s",Tr("ui.rebind_intro","Choose your preferred menu shortcut.").c_str());
        if (fonts::caption) ImGui::PushFont(fonts::caption);
        ImGui::TextWrapped("%s",Tr("ui.rebind_press","Click Capture, press a key or combination, then Save.").c_str());
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
        if (ImGui::BeginChild("##shortcut-preview",ImVec2(0.f,60.f),
                          ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            if (fonts::titleHeavy) ImGui::PushFont(fonts::titleHeavy);
            const float available=ImGui::GetContentRegionAvail().x;
            const float textWidth=ImGui::CalcTextSize(label.c_str()).x;
            if (textWidth<available)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX()+(available-textWidth)*.5f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()+6.f);
            ImGui::TextWrapped("%s",label.c_str());
            if (fonts::titleHeavy) ImGui::PopFont();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        if (fonts::caption) ImGui::PushFont(fonts::caption);
        if (fonts::HasIcons()) {
            // Icon exists in the body atlas, not the caption atlas.
            if (fonts::caption) ImGui::PopFont();
            ImGui::TextColored(p.muted,"%s",ARC_ICON_INFO);
            ImGui::SameLine(0.f,6.f);
            if (fonts::caption) ImGui::PushFont(fonts::caption);
        }
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

void DrawWelcome(const Palette& p) {
    auto& input=InputRouter::Get();
    if (!input.IsOpen() || input.IsRecording() || !input.HasKey()) return;
    // Read the durable state once per frame only while the welcome is pending.
    static bool dismissedThisSession=false;
    if (dismissedThisSession) return;
    const auto doc=nlohmann::json::parse(Store().Snapshot(),nullptr,false);
    if (!upgrade::WelcomePending(doc)) return;
    const std::string from=upgrade::Field(doc,"previous_version");
    constexpr const char* popup="##m1arc-upgrade-welcome";
    if (!ImGui::IsPopupOpen(popup)) ImGui::OpenPopup(popup);
    const auto* viewport=ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(),ImGuiCond_Appearing,ImVec2(.5f,.5f));
    ImGui::SetNextWindowSize(ImVec2(std::max(160.f,std::min(480.f,viewport->Size.x-36.f)),0.f),ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(24.f,22.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(8.f,10.f));
    if (ImGui::BeginPopupModal(popup,nullptr,ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
        ModalTitle(p,Tr(from.empty() ? "ui.welcome_title" : "ui.update_title",
            from.empty() ? "M1 ARC // WELCOME" : "M1 ARC // UPDATE COMPLETE"),false);
        if (fonts::caption) ImGui::PushFont(fonts::caption);
        ImGui::TextDisabled("%s",(from.empty() ? std::string(upgrade::Current) :
            from+"  ->  "+std::string(upgrade::Current)).c_str());
        if (fonts::caption) ImGui::PopFont();
        ImGui::Separator();
        ImGui::TextWrapped("%s",Tr(from.empty() ? "ui.welcome_message" : "ui.update_message",
            from.empty() ? "Thank you for choosing M1 Arc!" :
                "Thank you for updating M1 Arc! Your existing mod settings have been preserved.").c_str());
        if (fonts::caption) ImGui::PushFont(fonts::caption);
        ImGui::TextWrapped("%s",Tr("ui.welcome_discussion","Share feedback, report issues or join the discussion on Nexus Mods.").c_str());
        if (fonts::caption) ImGui::PopFont();
        ImGui::Spacing();
        const std::string discussion=Tr("ui.welcome_nexus","Join discussion on Nexus Mods");
        const bool visitNexus=ImGui::Button("##welcome-nexus",ImVec2(-1.f,36.f));
        const ImVec2 buttonMin=ImGui::GetItemRectMin(), buttonMax=ImGui::GetItemRectMax();
        const float available=std::max(1.f,buttonMax.x-buttonMin.x-24.f-8.f-16.f);
        const std::string fitted=FitLabel(discussion,available);
        const ImVec2 textSize=ImGui::CalcTextSize(fitted.c_str());
        const float startX=buttonMin.x+std::max(8.f,(buttonMax.x-buttonMin.x-24.f-8.f-textSize.x)*.5f);
        auto* draw=ImGui::GetWindowDrawList();
        draw->PushClipRect(buttonMin,buttonMax,true);
        DrawNexusIcon(draw,ImVec2(std::floor(startX),std::floor(buttonMin.y+(buttonMax.y-buttonMin.y-24.f)*.5f)));
        draw->AddText(ImVec2(startX+32.f,buttonMin.y+(buttonMax.y-buttonMin.y-textSize.y)*.5f),
                      ImGui::GetColorU32(ImGuiCol_Text),fitted.c_str());
        draw->PopClipRect();
        if (visitNexus) {
            const auto result=ShellExecuteW(nullptr,L"open",
                L"https://www.nexusmods.com/cyberpunk2077/mods/39311?tab=posts",nullptr,nullptr,SW_SHOWNORMAL);
            if (reinterpret_cast<INT_PTR>(result)<=32)
                ShowToast(Tr("toast.welcome_link_failed","Could not open Nexus Mods"),3.2f,ToastKind::Error);
        }
        ImGui::PushStyleColor(ImGuiCol_Button,p.amber);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.98f,.76f,.28f,1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(.84f,.59f,.08f,1.f));
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(.10f,.11f,.09f,1.f));
        if (ImGui::Button(Tr("ui.welcome_continue","Continue").c_str(),ImVec2(-1.f,36.f))) {
            if (!upgrade::Acknowledge(Store()))
                ShowToast(Tr("toast.welcome_save_failed","Could not save update acknowledgement; it may appear next launch."),4.f,ToastKind::Warning);
            dismissedThisSession=true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::EndPopup();
    }
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
        if (LogsMode()) DrawLogs(palette,mods,std::max(0.f,area.x-navWidth),area.y);
        else DrawDetail(palette,mods,std::max(0.f,area.x-navWidth),area.y);
        ImGui::PopStyleVar();
    }
    ImGui::End();
    // Floating toast, centered below the panel when display height permits.
    const ImVec2 windowPos(viewport->GetCenter().x - width * 0.5f,
                           viewport->GetCenter().y - height * 0.5f);
    DrawToasts(viewport, windowPos, ImVec2(width, height));
    // First-run, rebind and emergency recovery share the same modal.
    DrawRebindModal(palette);
    DrawWelcome(palette);
}
} // namespace m1arc
