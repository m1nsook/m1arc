#pragma once
#include <imgui.h>
#include <cstdint>
#include <string_view>

namespace m1arc {
// Rasterized from the owner's supplied nexus.svg (163 x 163 viewBox).
// Native 24px icon with an alpha-aware palette; no external image loader,
// texture allocation or renderer changes required.
inline void DrawNexusIcon(ImDrawList* draw, ImVec2 origin) {
    constexpr std::uint32_t palette[]={
        0x01000000u,0x72000000u,0xCE000000u,0xFB000000u,0xAF000000u,0x34000000u,0x8D000000u,0x51000000u,0xFF131416u,0xFF838282u,0xFFCCD1DAu,0xFF66778Eu,0xFD2B5997u,0xFF0C3F88u,0xFF0B2954u,0xFF327BD3u,0xFF3288F7u,0xFF5278A9u,0xFFE5EAF2u,0xFFF4F3F4u,0xFCABBFDEu,0xFF92A8C9u,0xFF6AA9F9u,0xFF1774F9u,0xFF3C91FAu,0xFF4B9AFAu,0xFF7EB2F7u,0xFD2E6CB9u,0xFF59677Du,0xFED8E4F4u,0xFFA9CBF6u,0xFDC7DCF5u,0xFF0C4290u,0xFF103566u,0xFF909090u,0xFF97C1F7u,0xFFB8D4F6u,0xFF08182Cu,0xFF204D84u,0xFF89BAF8u,0xFF3581DDu,0xFF59A1FAu,0xFF1E487Cu,0xFF47668Cu,0xFEB7C3D3u,0xFE1056BCu,0xFF408CF9u,0xFF4B586Cu,0xF4497DC8u,0xFFC8C9CBu,0xFF2B4C72u,0xFF265DA1u,0xFF676666u
    };
    constexpr std::string_view alphabet="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_";
    constexpr std::string_view pixels[]={
        "000000000000000123450000",
        "000144671233333333336000",
        "00433333333333389AB33400",
        "0133CDE3ECFGGFHIJJJK8360",
        "533LMNNGOOOOOPIJJJIQR320",
        "43SJTGOOOOOOOUJJJVGNW320",
        "33AJJVPOOOOPPJJJTPGNX360",
        "23YJJJVPOPOMIJJIPOOG3370",
        "138IJJJIZaOPJJJUOOOOb310",
        "033HIJJJJJPOVJJVPOOOc340",
        "033FPaJJJJZOdZPOOOOOR320",
        "033eOOMIJTdOOOOPdPOOF330",
        "033FOOPdPOOOOZTJIfOOF330",
        "023ROOOOOMUQOZJJJJaPR320",
        "043gOOOfTJJaOPJJJJJIh320",
        "013bOOOOaJJIOOadIJJJI831",
        "013bGOOMJJJIMOPOPTJJJ932",
        "023WNGfIJJIPPOOOOPVJJi33",
        "033jNkTJJJdOOOOOOGGIJl34",
        "033mZJJJJVOOOOOGGNNdL335",
        "0638iJJJnopRRpX33EWC3310",
        "00433Biq3333333333333400",
        "000633333223324776441000",
        "000054327000000000000000"
    };
    static_assert(sizeof(pixels)/sizeof(pixels[0])==24);
    static_assert([](const auto& rows) constexpr {
        for (const auto row:rows) if (row.size()!=24) return false;
        return true;
    }(pixels));
    for (int y=0;y<24;++y) {
        for (int x=0;x<24;) {
            const char c=pixels[y][x];
            int end=x+1;
            while (end<24 && pixels[y][end]==c) ++end;
            const auto i=alphabet.find(c);
            if (i>0 && i<sizeof(palette)/sizeof(palette[0]))
                draw->AddRectFilled(ImVec2(origin.x+x,origin.y+y),
                    ImVec2(origin.x+end,origin.y+y+1.f),palette[i]);
            x=end;
        }
    }
}
} // namespace m1arc
