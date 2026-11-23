#include "breach_select.hpp"
#include "input.hpp"

#include <RED4ext/Scripting/Natives/Generated/Vector2.hpp>
#include <RED4ext/Scripting/IScriptable.hpp>
#include <RED4ext/RTTITypes.hpp>

#include <Windows.h>
#include <winver.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#pragma comment(lib, "version.lib")

namespace m1arc::breach {
namespace {
// Interface fingerprint observed from the user-provided CP2077 2.5
// AutoBreach reference. This is a machine-code entry fingerprint, NOT a
// redistributed DLL, a CET dependency, or a function offset.
// Do not execute it if the signature is absent/ambiguous or version differs.
constexpr std::array<std::uint8_t,37> kSelectorEntry{
    0x48,0x89,0x5C,0x24,0x10,0x55,0x56,0x57,0x48,0x83,0xEC,0x30,
    0x48,0x8B,0x81,0x40,0x04,0x00,0x00,0x0F,0x57,0xC0,0x48,0x8B,
    0xF2,0x48,0x8B,0xF9,0xF3,0x0F,0x7F,0x44,0x24,0x20,0x48,0x85,
    0xC0
};
using NativeSelector = void(__fastcall*)(RED4ext::IScriptable*,
                                           const RED4ext::Vector2*);
std::atomic<NativeSelector> g_selector{nullptr};

bool ExactGameVersion(const std::wstring& exe) noexcept {
    DWORD unused=0;
    const DWORD size=GetFileVersionInfoSizeW(exe.c_str(),&unused);
    if (size==0 || size>(1u<<20)) return false;
    std::vector<std::uint8_t> data(size);
    if (!GetFileVersionInfoW(exe.c_str(),0,size,data.data())) return false;
    VS_FIXEDFILEINFO* info=nullptr;
    UINT bytes=0;
    if (!VerQueryValueW(data.data(),L"\\",
                        reinterpret_cast<void**>(&info),&bytes) ||
        bytes<sizeof(VS_FIXEDFILEINFO) || !info ||
        info->dwSignature!=0xFEEF04BD) return false;
    return HIWORD(info->dwFileVersionMS)==3 &&
           LOWORD(info->dwFileVersionMS)==0 &&
           HIWORD(info->dwFileVersionLS)==88 &&
           LOWORD(info->dwFileVersionLS)==46235;
}

NativeSelector ResolveSelector() noexcept {
    HMODULE module=GetModuleHandleW(nullptr);
    if (!module) return nullptr;
    wchar_t exe[MAX_PATH+1]{};
    const DWORD length=GetModuleFileNameW(module,exe,MAX_PATH);
    if (!length || length>=MAX_PATH || !ExactGameVersion(exe))
        return nullptr;

    const auto* base=reinterpret_cast<const std::uint8_t*>(module);
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE ||
        dos->e_lfanew<0 || dos->e_lfanew>(1<<16))
        return nullptr;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        base+dos->e_lfanew);
    if (nt->Signature!=IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.SizeOfImage==0 ||
        nt->OptionalHeader.SizeOfImage>(1u<<30))
        return nullptr;
    const auto size=nt->OptionalHeader.SizeOfImage;
    const auto* sections=IMAGE_FIRST_SECTION(nt);
    const std::uint8_t* found=nullptr;
    for (WORD i=0;i<nt->FileHeader.NumberOfSections;++i) {
        const auto& section=sections[i];
        if (!(section.Characteristics&IMAGE_SCN_MEM_EXECUTE) ||
            section.VirtualAddress>=size) continue;
        const auto available=std::min<std::size_t>(
            section.Misc.VirtualSize,size-section.VirtualAddress);
        if (available<kSelectorEntry.size()) continue;
        const auto* const begin=base+section.VirtualAddress;
        for (std::size_t offset=0;
             offset+kSelectorEntry.size()<=available;++offset) {
            const auto* candidate=begin+offset;
            if (std::memcmp(candidate,kSelectorEntry.data(),
                            kSelectorEntry.size())!=0) continue;
            // A duplicate or a matching sequence in the *middle* of a
            // function is unsafe, even if it is in executable memory.
            DWORD64 imageBase=0;
            auto* unwind=RtlLookupFunctionEntry(
                reinterpret_cast<DWORD64>(candidate),&imageBase,nullptr);
            if (!unwind || imageBase+unwind->BeginAddress!=
                               reinterpret_cast<DWORD64>(candidate))
                return nullptr;
            if (found) return nullptr;
            found=candidate;
        }
    }
    return found ? reinterpret_cast<NativeSelector>(
                       const_cast<std::uint8_t*>(found)) : nullptr;
}

void SelectCell(RED4ext::IScriptable* context,
                RED4ext::CStackFrame* frame,bool* output,std::int64_t) {
    RED4ext::Vector2 position{};
    RED4ext::GetParameter(frame,&position);
    frame->code++;
    if (output) *output=false;
    if (!output || !context || !std::isfinite(position.X) ||
        !std::isfinite(position.Y) ||
        position.X<0.f || position.Y<0.f ||
        position.X>7.f || position.Y>7.f ||
        std::floor(position.X)!=position.X ||
        std::floor(position.Y)!=position.Y)
        return;
    const auto& input=InputRouter::Get();
    const HWND game=input.Window();
    if (!game || !IsWindow(game) || GetForegroundWindow()!=game ||
        input.IsOpen() || input.IsRecording())
        return;
    auto* rtti=RED4ext::CRTTISystem::Get();
    auto* hackingClass=rtti ? rtti->GetClass(
        "gameuiHackingMinigameLogicController") : nullptr;
    if (!hackingClass || context->GetType()!=hackingClass)
        return;
    const auto selector=g_selector.load(std::memory_order_acquire);
    if (!selector) return;
    selector(context,&position);
    *output=true; // means dispatched; REDscript still awaits acknowledgement
}

void AvailableNative(RED4ext::IScriptable*,
                     RED4ext::CStackFrame* frame,bool* output,std::int64_t) {
    frame->code++;
    if (output) *output=Available();
}
} // namespace

void Initialize() noexcept {
    g_selector.store(ResolveSelector(),std::memory_order_release);
}
bool Available() noexcept {
    return g_selector.load(std::memory_order_acquire)!=nullptr;
}

void RegisterNative() noexcept {
    auto* rtti=RED4ext::CRTTISystem::Get();
    if (!rtti) return;

    auto* available=RED4ext::CGlobalFunction::Create(
        "M1Arc_BreachInputAvailable","M1Arc_BreachInputAvailable",
        reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&AvailableNative));
    available->flags=RED4ext::CBaseFunction::Flags{
        .isNative=true,.isStatic=true
    };
    available->SetReturnType("Bool");
    rtti->RegisterFunction(available);

    auto* logic=rtti->GetClass("inkLogicController");
    if (!logic) return;
    auto* select=RED4ext::CClassFunction::Create(
        logic,"M1Arc_BreachSelectCell","M1Arc_BreachSelectCell",
        reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&SelectCell));
    select->flags=RED4ext::CBaseFunction::Flags{.isNative=true};
    select->AddParam("Vector2","position");
    select->SetReturnType("Bool");
    logic->RegisterFunction(select);
}
} // namespace m1arc::breach
