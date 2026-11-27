#pragma once
#include <imgui.h>

namespace m1arc {
// Option B concept, rebuilt as resolution-independent native geometry.
// No texture, font file, image decoder or graphics-backend changes.
inline void DrawArcBrand(ImDrawList* d,ImVec2 origin,float size) {
    const auto pt=[&](float x,float y){return ImVec2(origin.x+x*size/100.f,origin.y+y*size/100.f);};
    const auto cream=IM_COL32(234,228,200,255), ink=IM_COL32(21,22,19,255);
    const float rounding=size*.22f;
    d->AddRectFilled(pt(0,0),pt(100,100),ink,rounding);
    d->AddRectFilled(pt(0,0),pt(50,100),cream,rounding,ImDrawFlags_RoundCornersLeft);
    d->AddRect(pt(.8f,.8f),pt(99.2f,99.2f),cream,rounding,0,size*.012f);
    const auto quad=[&](float a,float b,float c,float e,float f,float g,float h,float i,ImU32 color){
        d->AddQuadFilled(pt(a,b),pt(c,e),pt(f,g),pt(h,i),color);
    };
    const auto monogram=[&](ImU32 col){
        quad(16,25,25,25,25,60,16,69,col);
        quad(25,25,46,45,46,57,25,39,col);
        quad(46,45,65,25,65,38,46,57,col);
        quad(65,25,72,25,72,59,62,59,col);
    };
    d->PushClipRect(pt(0,0),pt(50,100),true);monogram(ink);d->PopClipRect();
    d->PushClipRect(pt(50,0),pt(100,100),true);monogram(cream);d->PopClipRect();
    quad(76,32,84,25,89,25,89,37,cream);
    quad(79,32,89,32,89,59,79,59,cream);
    quad(12,82,33,62,94,62,76,82,ink);
    quad(23,79,36,65,91,65,77,79,cream);
    // ARC is drawn with line geometry rather than a font dependency.
    const float stroke=size*.021f;
    const auto line=[&](float a,float b,float c,float e){d->AddLine(pt(a,b),pt(c,e),ink,stroke);};
    line(35,76,40,68);line(40,68,45,76);line(37,73,43,73);
    line(50,76,50,68);line(50,68,58,68);line(58,68,58,72);
    line(58,72,50,72);line(54,72,59,76);
    line(72,68,65,68);line(65,68,64,69);line(64,69,64,75);
    line(64,75,65,76);line(65,76,72,76);
}
} // namespace m1arc
