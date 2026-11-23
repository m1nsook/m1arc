#include "fonts.hpp"
#include <Windows.h>
#include <filesystem>
#include <string>
#include <system_error>

namespace m1arc::fonts {
namespace {
std::filesystem::path Directory() {
    HMODULE module = nullptr;
    const auto location = reinterpret_cast<LPCWSTR>(&Directory);
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,location,&module))
        return {};
    wchar_t path[MAX_PATH+1]{};
    const DWORD len=GetModuleFileNameW(module,path,MAX_PATH);
    if (!len || len>=MAX_PATH) return {};
    return std::filesystem::path(path).parent_path() / L"fonts";
}
std::string Utf8(const std::filesystem::path& path) {
    const auto wide=path.wstring();
    const int n=WideCharToMultiByte(CP_UTF8,0,wide.c_str(),-1,nullptr,0,nullptr,nullptr);
    if (n<=0) return {};
    std::string output(static_cast<std::size_t>(n),'\0');
    if (!WideCharToMultiByte(CP_UTF8,0,wide.c_str(),-1,output.data(),n,
                            nullptr,nullptr)) return {};
    return output;
}
ImFont* Add(ImFontAtlas* atlas,const std::filesystem::path& dir,
            const wchar_t* file,float size,const ImWchar* ranges) {
    const auto path=dir / file;
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path,ec)) return nullptr;
    const auto filename=Utf8(path);
    if (filename.empty()) return nullptr;
    ImFontConfig config{};
    config.OversampleH=2;
    config.OversampleV=1;
    return atlas->AddFontFromFileTTF(filename.c_str(),size,&config,ranges);
}
} // namespace

void Clear() noexcept {
    body=titleMedium=titleBold=titleHeavy=nullptr;
    iconsAvailable=false;
}
void Load() noexcept {
    Clear();
    auto& io=ImGui::GetIO();
    try {
        auto* atlas=io.Fonts;
        const auto folder=Directory();
        static const ImWchar textRanges[] = {
            0x0020,0x024F,0x2000,0x206F,0x20AC,0x20AC,0
        };
        // Order is intentional: merge Phosphor into Karla BEFORE headings.
        body=Add(atlas,folder,L"Karla-Regular.ttf",16.f,textRanges);
        if (!body) body=atlas->AddFontDefault();
        io.FontDefault=body;

        static const ImWchar iconRanges[] = {
            0xE038,0xE038, 0xE184,0xE184, 0xE1DA,0xE1DA,
            0xE2D8,0xE2D8, 0xE434,0xE434, 0xE4F6,0xE4F6, 0
        };
        const auto iconFile=folder / L"Phosphor-Regular.ttf";
        std::error_code ec;
        if (std::filesystem::is_regular_file(iconFile,ec)) {
            const auto filename=Utf8(iconFile);
            ImFontConfig merged{};
            merged.MergeMode=true;
            merged.GlyphMinAdvanceX=13.f;
            merged.GlyphOffset=ImVec2(0.f,2.f);
            merged.OversampleH=2;
            merged.OversampleV=1;
            iconsAvailable=!filename.empty() &&
                atlas->AddFontFromFileTTF(filename.c_str(),16.f,&merged,iconRanges)!=nullptr;
        }

        // Blender Pro comes from the owner's supplied fonts.zip. It is
        // proprietary and is deliberately NOT in our public repo/artifacts.
        // If it exists locally it enhances headings; otherwise use Karla.
        titleMedium=Add(atlas,folder,L"BlenderPro-Medium.ttf",18.f,textRanges);
        titleBold=Add(atlas,folder,L"BlenderPro-Bold.ttf",19.f,textRanges);
        titleHeavy=Add(atlas,folder,L"BlenderPro-Heavy.ttf",22.f,textRanges);
        if (!titleMedium) titleMedium=Add(atlas,folder,L"Karla-Regular.ttf",18.f,textRanges);
        if (!titleBold) titleBold=Add(atlas,folder,L"Karla-Regular.ttf",19.f,textRanges);
        if (!titleHeavy) titleHeavy=Add(atlas,folder,L"Karla-Regular.ttf",22.f,textRanges);
    } catch (...) {
        // Damaged optional files must not stop the game.
        if (!body && io.Fonts->Fonts.Size==0)
            body=io.Fonts->AddFontDefault();
        if (body) io.FontDefault=body;
        iconsAvailable=false;
    }
}
} // namespace m1arc::fonts
