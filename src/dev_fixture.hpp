#pragma once
// Development-only, opt-in visual regression fixture.
// It is registered through the SAME Registry API as real REDscript mods.
// No game effects, new scripting language, or permanent demo mod.
#include "registry.hpp"

#include <filesystem>
#include <system_error>
#include <string>

namespace m1arc::dev {
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

inline bool RegisterIfEnabled(const std::filesystem::path& pluginFolder,
                              Registry& registry) {
    std::error_code ec;
    const auto marker=pluginFolder / L"dev-preview.enabled";
    if (!std::filesystem::is_regular_file(marker,ec) || ec) return false;
    // Do not touch mod files, alter gameplay or activate additional input.
    return registry.Register("m1arc.ui-test",
      "M1 ARC UI Test — very long mod name to check sidebar clipping",Schema(),
      "UI/UX","Native settings and typography stress test");
}
} // namespace m1arc::dev
