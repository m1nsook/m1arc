#pragma once
// Development-only, opt-in visual regression fixture.
// It is registered through the SAME Registry API as real REDscript mods.
// No game effects, new scripting language, or permanent demo mod.
#include "registry.hpp"

#include <filesystem>
#include <atomic>
#include <string_view>
#include <system_error>
#include <string>

namespace m1arc::dev {
inline std::atomic_bool previewEnabled{false};
inline constexpr std::string_view kToastDemoId="m1arc.ui-test.toasts";
inline constexpr int kDemoCount=10;
inline nlohmann::json Schema() {
    using json = nlohmann::json;
    json rows = json::array({
        {{"id","enabled"},{"kind","boolean"},{"default",true},
         {"label","Master enabled switch"},
         {"description","Toggle OFF to check that every child control is visibly disabled, but this switch remains usable."},
         {"group","States"}},
        {{"id","off_by_default"},{"kind","boolean"},{"default",false},
         {"label","Off by default"},
         {"description","Compare OFF/ON normal, hovered, held, and selected colors in light and dark themes."},
         {"group","States"}},
        {{"id","on_by_default"},{"kind","boolean"},{"default",true},
         {"label","On by default"},
         {"description","The highlighted ON side must remain readable after a press and during hover."},
         {"group","States"}},
        // Weapons: sliders, queued action buttons, and standalone checkboxes.
        // The UI Test NEVER spawns an item; it only previews controls.
        {{"id","ammo_amount"},{"kind","integer"},{"min",1},{"max",999},{"default",50},
         {"label","Ammunition"},{"group","Weapons"},
         {"description","Select the ammunition amount before pressing Spawn."},
         {"button",{{"id","spawn_ammo"},{"label","Spawn"}}}},
        {{"id","spawn_hotkey"},{"kind","hotkey"},{"default",0},
         {"label","Spawn hotkey"},{"group","Weapons"},
         {"description","Assign a key, close M1 Arc, and press it in-game. UI Test only: no game items are spawned."}},
        {{"id","light_ammo"},{"kind","boolean"},{"default",true},
         {"label","Light Ammo"},{"group","Weapons"},{"presentation","checkbox"}},
        {{"id","heavy_ammo"},{"kind","boolean"},{"default",true},
         {"label","Heavy Ammo"},{"group","Weapons"},{"presentation","checkbox"}},
        {{"id","spawn_selected"},{"kind","action"},{"label","Spawn selected ammunition"},
         {"buttonLabel","Spawn all"},{"group","Weapons"},
         {"description","UI preview only: clicking this button does not affect the game."}},
        {{"id","short_integer"},{"kind","integer"},{"min",0},{"max",10},{"default",4},
         {"label","Integer: 0 to 10"},{"description","Test slider grab, hover, press, and displaying low integer values."},
         {"group","Sliders"}},
        {{"id","large_integer"},{"kind","integer"},{"min",0},{"max",1000},{"default",875},
         {"label","Integer: 0 to 1000"},{"description","Test three/four-digit values and slider alignment."},
         {"group","Sliders"}},
        {{"id","negative_integer"},{"kind","integer"},{"min",-100},{"max",100},{"default",-25},
         {"label","Integer: negative to positive"},{"description","Signed integers must not overlap controls."},
         {"group","Sliders"}},
        {{"id","fraction"},{"kind","float"},{"min",0.0},{"max",1.0},{"default",0.375},
         {"label","Float: 0.00 to 1.00"},{"description","Test decimal values and responsive widths."},
         {"group","Sliders"}},
        {{"id","multiplier"},{"kind","float"},{"min",-5.0},{"max",5.0},{"default",1.75},
         {"label","Float: negative and positive"},{"description","Check the float slider across its zero point."},
         {"group","Sliders"}},
        {{"id","short_choice"},{"kind","choice"},{"choices",json::array({"Disabled","Low","Medium","High","Ultra"})},
         {"default","High"},{"label","Choice: short values"},
         {"description","Use both arrows and the center control. The saved ID stays the original English value."},
         {"group","Choices"}},
        {{"id","long_choice"},{"kind","choice"},
         {"choices",json::array({"Short","A very long selectable option to expose right-edge clipping","Longest option currently selected: check that the text does not escape the control"})},
         {"default","Longest option currently selected: check that the text does not escape the control"},
         {"label","Choice: deliberately long values"},
         {"description","Ensure choice text truncates or scrolls without overlapping the arrows."},
         {"group","Choices"}},
        {{"id","long_label"},{"kind","boolean"},{"default",true},
         {"label","A deliberately long setting label designed to wrap onto multiple lines on a narrower window and remain separate from its controls"},
         {"description","Tooltip: this sentence is deliberately longer to check popup foreground color, padding, and readable text in both themes."},
         {"group","Typography"}},
        {{"id","polish_label"},{"kind","boolean"},{"default",true},
         {"label","Polski: Zażółć gęślą jaźń — ąćęłńóśźż"},
         {"description","Latin Extended glyph coverage and baseline alignment; not a full localization test."},
         {"group","Typography"}},
        {{"id","korean_label"},{"kind","boolean"},{"default",true},
         {"label","한국어: 글꼴 및 텍스트 확인"},
         {"description","한글 글자와 도구 설명이 올바르게 표시되는지 확인합니다."},
         {"group","Typography"}},
        {{"id","long_description"},{"kind","integer"},{"min",0},{"max",100},{"default",50},
         {"label","Long description tooltip"},
         {"description","This tooltip must display readable words, not an empty rectangle. It should have proper contrast in light and dark modes and remain within the game viewport even when the panel is at the edge."},
         {"group","Typography"}}
    });
    for (int i=1;i<=24;i++) {
        const std::string n=std::to_string(i);
        rows.push_back({{"id","scroll_"+n},{"kind","boolean"},{"default",i%2==0},
                        {"label","Scrolling / stress row "+n},
                        {"description","This row exercises scrolling, tooltips, hit targets and footer containment."},
                        {"group","Scrolling"}});
    }
    return rows;
}

// Separate pages make sidebar scrolling, scope icons and selected state
// testable even when no actual gameplay mods are installed.
inline nlohmann::json ScopeSchema(std::string_view scope) {
    using json=nlohmann::json;
    json rows=json::array({
        {{"id","enabled"},{"kind","boolean"},{"default",true},
         {"label","Enable preview"},{"description","Only changes this fixture's saved value. No gameplay effects."},{"group","General"}},
        {{"id","strength"},{"kind","float"},{"min",0.0},{"max",10.0},{"default",2.5},
         {"label","Intensity"},{"description","Drag the slider and release to review the real saved-value toast."},{"group","Options"}},
        {{"id","mode"},{"kind","choice"},{"choices",json::array({"Low","Balanced","High"})},{"default","Balanced"},
         {"label","Preview mode"},{"description","Check both arrows and the selected value."},{"group","Options"}},
        {{"id","check"},{"kind","boolean"},{"default",false},{"presentation","checkbox"},
         {"label","Optional setting"},{"description","Check checkbox appearance, spacing and notifications."},{"group","Options"}},
        {{"id","request"},{"kind","action"},{"label","Preview action"},{"buttonLabel","Request"},
         {"description","Development fixture only. No game items or effects are created."},{"group","Actions"}}
    });
    if (scope=="Miscellaneous") {
        rows.push_back({{"id","no_description"},{"kind","boolean"},{"default",false},
                        {"label","Setting with no description"},{"group","Options"}});
        rows.push_back({{"id","long_heading"},{"kind","boolean"},{"default",true},
                        {"label","An unusually long setting name to check wrapping and control alignment"},
                        {"description","A longer heading should wrap cleanly without covering the control or tooltip body."},
                        {"group","A deliberately long section heading for checking layout"}});
    }
    return rows;
}

inline nlohmann::json ToastSchema() {
    using json=nlohmann::json;
    json rows=json::array();
    const auto add=[&](const char* id,const char* label,const char* button,const char* description,const char* group) {
        rows.push_back({{"id",id},{"kind","action"},{"label",label},{"buttonLabel",button},
                        {"description",description},{"group",group}});
    };
    add("toast_info","Information notification","Preview info","Check information icon, pastel background and status text.","Notification types");
    add("toast_success","Success notification","Preview success","Check success icon and contrast in both themes.","Notification types");
    add("toast_warning","Warning notification","Preview warning","Check warning icon, text and background.","Notification types");
    add("toast_error","Error notification","Preview error","Simulated error only: no real settings or game failure.","Notification types");
    add("toast_long","Long notification","Preview long","Check wrapping and clipping when a notification contains several sentences.","Layout and timing");
    add("toast_unicode","Polish / Korean notification","Preview Unicode","Check glyphs and text alignment in a mixed-language toast.","Layout and timing");
    add("toast_short","Short duration","Preview 1 sec","Review entrance and exit animation on a short notification.","Layout and timing");
    add("toast_burst","Queue and duplicates","Preview queue","Enqueues all four types and repeats one identical message. The duplicate should not appear.","Layout and timing");
    add("toast_close","Notification outside the menu","Preview in game","Closes M1 Arc and displays a development notification without capturing the cursor.","Layout and timing");
    return rows;
}

inline bool RegisterIfEnabled(const std::filesystem::path& pluginFolder,
                              Registry& registry) {
    std::error_code ec;
    const auto marker=pluginFolder / L"dev-preview.enabled";
    previewEnabled.store(false);
    if (!std::filesystem::is_regular_file(marker,ec) || ec) return false;
    // All pages use the production registry, without any gameplay effects.
    bool ok=registry.Register("m1arc.ui-test",
      "M1 ARC UI Test — very long mod name to check sidebar clipping",Schema(),
      "UI/UX","Native settings and typography stress test");
    ok=registry.Register(std::string(kToastDemoId),"UI Test // Notifications",ToastSchema(),
        "UI/UX","All four notification styles, queue behavior, wrapping and feedback outside the menu") && ok;
    struct Sample { const char* id; const char* title; const char* scope; const char* description; };
    constexpr Sample samples[]={
        {"gameplay","UI Test // Gameplay","Gameplay","Short title, game controller icon and normal controls"},
        {"crafting","UI Test // Crafting","Crafting","Wrench icon, checkboxes, sliders and action request"},
        {"cheats","UI Test // Cheats","Cheats","Terminal icon and safe preview controls"},
        {"weapons","UI Test // Weapons","Weapons","Sword icon and switch/choice alignment"},
        {"locations","UI Test // Locations","Locations","Map pin icon and footer containment"},
        {"appearance","UI Test // Appearance","Appearance","Clothing icon and longer text for testing sidebar descriptions and clipping"},
        {"characters","UI Test // Characters","Characters","Users icon and localized labels"},
        {"misc","UI Test // Miscellaneous — deliberately long sidebar title with additional text","Miscellaneous","Toolbox icon, long section header, missing description and wrapping"}
    };
    for (const auto& sample:samples)
        ok=registry.Register(std::string("m1arc.ui-test.")+sample.id,sample.title,ScopeSchema(sample.scope),
                             sample.scope,sample.description) && ok;
    previewEnabled.store(ok);
    return ok;
}
} // namespace m1arc::dev
