#pragma once
#include <imgui.h>
#include <initializer_list>

namespace m1arc {
// Native rendering of assets/m1arc-option-b.svg. The R and C curves are
// approximated by small polygons; no textures or decoder are required.
inline void DrawArcBrand(ImDrawList* d,ImVec2 origin,float size) {
    const auto pt=[&](float x,float y){return ImVec2(origin.x+x*size/222.f,origin.y+y*size/222.f);};
    const auto cream=IM_COL32(234,228,200,255), ink=IM_COL32(22,23,19,255);
    const float rounding=size*50.f/222.f;
    d->AddRectFilled(pt(3,3),pt(218,218),ink,rounding);
    d->AddRectFilled(pt(3,3),pt(111,218),cream,rounding,ImDrawFlags_RoundCornersLeft);
    d->AddRect(pt(3,3),pt(218,218),cream,rounding,0,size*2.f/222.f);
    const auto poly=[&](ImU32 color,std::initializer_list<ImVec2> coords) {
        for (auto v:coords) d->PathLineTo(pt(v.x,v.y));
        d->PathFillConcave(color);
    };
    const auto monogram=[&](ImU32 col){
        poly(col,{{38,55},{58,55},{82,80},{108,55},{147,55},{147,128},
            {126,128},{126,88},{84,130},{58,103},{58,125},{38,145}});
    };
    d->PushClipRect(pt(0,0),pt(111,222),true);monogram(ink);d->PopClipRect();
    d->PushClipRect(pt(111,0),pt(222,222),true);monogram(cream);d->PopClipRect();
    poly(cream,{{155,64},{165,55},{186,55},{186,128},{165,128},{165,73},{155,73}});
    poly(ink,{{26,177},{69,134},{205,134},{173,177}});
    poly(cream,{{47,171},{75,142},{198,142},{171,171}});
    poly(ink,{{75,166},{85,148},{94,148},{103,166},{96,166},{94,162},{84,162},{82,166}});
    poly(cream,{{86,157},{92,157},{89,151}});
    poly(ink,{{112,148},{126,148},{132,149},{135,154},{134,157},{130,159},
        {137,166},{128,166},{122,160},{119,160},{119,166},{112,166}});
    poly(cream,{{119,153},{125,153},{128,154},{128,155},{125,156},{119,156}});
    poly(ink,{{170,152},{164,155},{160,153},{154,153},{151,156},{151,159},
        {154,162},{160,162},{164,159},{170,162},{167,165},{161,167},{153,167},
        {147,165},{144,159},{144,156},{147,150},{153,148},{161,148},{167,150}});
}
} // namespace m1arc
