#include "breach_select.hpp"
#include "input.hpp"
#include "mouse_click.hpp"
#include "graphics.hpp"
#include "registry.hpp"

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
#include <cstdio>
#include <cstring>
#include <mutex>
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
// No native action or game-code signature scan on normal M1 Arc launch.
// The standalone Skip Breach mod explicitly arms this capability via its
// existing persisted "enabled" setting, default OFF and absent otherwise.
std::once_flag g_selectorOnce;
std::atomic<bool> g_supportedGameVersion{false};

bool SkipBreachEnabled() noexcept {
    const auto setting=m1arc::Settings().Get("m1nsook.skipbreach","enabled");
    return setting.has_value() && setting->is_boolean()
        && setting->get<bool>();
}

enum class SelectorStatus {
    Uninitialized,
    UnsupportedVersion,
    InvalidExecutable,
    SignatureNotFound,
    UnsafeFunctionEntry,
    Ready
};
std::atomic<SelectorStatus> g_status{SelectorStatus::Uninitialized};
// A single message per changed rejection state, not one per UI frame.
std::atomic<int> g_lastActionState{-1};

void ActionState(int state,const char* message) noexcept {
    const int previous=g_lastActionState.exchange(state,std::memory_order_acq_rel);
    if (previous!=state) {
        m1arc::RenderBridge::Get().LogInfo(message);
        try {
            (void)m1arc::Settings().AppendLog("m1nsook.skipbreach",
                state==0 ? m1arc::LogLevel::Info : m1arc::LogLevel::Warning,message);
        } catch (...) {} // diagnostics must never break the native bridge
    }
}

NativeSelector Reject(SelectorStatus reason) noexcept {
    g_status.store(reason,std::memory_order_release);
    return nullptr;
}

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
    if (!module) return Reject(SelectorStatus::InvalidExecutable);
    wchar_t exe[MAX_PATH+1]{};
    const DWORD length=GetModuleFileNameW(module,exe,MAX_PATH);
    if (!length || length>=MAX_PATH)
        return Reject(SelectorStatus::InvalidExecutable);
    if (!ExactGameVersion(exe))
        return Reject(SelectorStatus::UnsupportedVersion);

    const auto* base=reinterpret_cast<const std::uint8_t*>(module);
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE ||
        dos->e_lfanew<0 || dos->e_lfanew>(1<<16))
        return Reject(SelectorStatus::InvalidExecutable);
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(
        base+dos->e_lfanew);
    if (nt->Signature!=IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.SizeOfImage==0 ||
        nt->OptionalHeader.SizeOfImage>(1u<<30))
        return Reject(SelectorStatus::InvalidExecutable);
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
                return Reject(SelectorStatus::UnsafeFunctionEntry);
            if (found) return Reject(SelectorStatus::UnsafeFunctionEntry);
            found=candidate;
        }
    }
    if (!found) return Reject(SelectorStatus::SignatureNotFound);
    g_status.store(SelectorStatus::Ready,std::memory_order_release);
    return reinterpret_cast<NativeSelector>(
        const_cast<std::uint8_t*>(found));
}

void SelectCell(RED4ext::IScriptable*,
                RED4ext::CStackFrame* frame,bool* output,std::int64_t) {
    // Use the global native route rather than @addMethod on a game RTTI
    // class, which triggered RED4ext invalid native definitions at launch.
    RED4ext::WeakHandle<RED4ext::IScriptable> weakController;
    RED4ext::Vector2 position{};
    RED4ext::GetParameter(frame,&weakController);
    RED4ext::GetParameter(frame,&position);
    frame->code++;
    if (output) *output=false;
    auto controller=weakController.Lock();
    auto* context=controller.GetPtr();
    if (!output || !context) {
        ActionState(1,"m1arc Breach: cell rejected (missing controller handle)");
        return;
    }
    if (!std::isfinite(position.X) || !std::isfinite(position.Y) ||
        position.X<0.f || position.Y<0.f ||
        position.X>7.f || position.Y>7.f ||
        std::floor(position.X)!=position.X ||
        std::floor(position.Y)!=position.Y) {
        ActionState(2,"m1arc Breach: cell rejected (invalid row or column)");
        return;
    }
    if (!Available()) {
        ActionState(3,"m1arc Breach: internal selection rejected (mod disabled, unsupported game or unavailable input)");
        return;
    }
    const auto& input=InputRouter::Get();
    const HWND game=input.Window();
    if (!game || !IsWindow(game) || GetForegroundWindow()!=game) {
        ActionState(3,"m1arc Breach: cell rejected (game window not foreground or not attached)");
        return;
    }
    if (input.IsOpen() || input.IsRecording()) {
        ActionState(4,"m1arc Breach: cell rejected (M1 Arc overlay or keybind capture active)");
        return;
    }
    auto* rtti=RED4ext::CRTTISystem::Get();
    auto* hackingClass=rtti ? rtti->GetClass(
        "gameuiHackingMinigameLogicController") : nullptr;
    const auto* contextType=context->GetType();
    if (!hackingClass || !contextType || !contextType->IsA(hackingClass)) {
        ActionState(5,"m1arc Breach: cell rejected (wrong hacking logic controller RTTI)");
        return;
    }
    // The old internal selector is retained as an opt-in compatibility route,
    // but scanning game code at M1 Arc startup is unnecessary. Skip Breach's
    // physical input path does NOT require this signature at all.
    std::call_once(g_selectorOnce, [] {
        g_selector.store(ResolveSelector(),std::memory_order_release);
    });
    const auto selector=g_selector.load(std::memory_order_acquire);
    if (!selector) {
        ActionState(6,"m1arc Breach: cell rejected (selector unavailable for current game build)");
        return;
    }
    ActionState(0,"m1arc Breach: selector ready to dispatch");
    // Each confirmed dispatch is worth one log line. Unlike ActionState,
    // this must NOT de-duplicate a later puzzle's identical selection.
    char detail[160]{};
    std::snprintf(detail,sizeof(detail),
                  "[Skip Breach] Native cell dispatch row=%d col=%d; awaiting game acknowledgement",
                  static_cast<int>(position.X),static_cast<int>(position.Y));
    m1arc::RenderBridge::Get().LogInfo(detail);
    selector(context,&position);
    *output=true; // means dispatched; REDscript still awaits acknowledgement
}

// Alternative to the game-internal controller selector: physical Win32 input
// at the centre of an actual live ink grid widget. The caller supplies a
// screen-normalized point resolved from that widget's current draw bounds.
// Unlike an internal RTTI selector dispatch, SendInput follows the ordinary
// player pointer route; dispatch success is NOT acceptance in the game.
void ClickAt(RED4ext::IScriptable*,
             RED4ext::CStackFrame* frame,bool* output,std::int64_t) {
    RED4ext::WeakHandle<RED4ext::IScriptable> weakController;
    RED4ext::Vector2 normalized{};
    RED4ext::GetParameter(frame,&weakController);
    RED4ext::GetParameter(frame,&normalized);
    frame->code++;
    if (output) *output=false;
    if (!output) return;
    // A direct native invocation is never sufficient to arm input. The mod
    // must be installed, registered in M1 Arc, and explicitly enabled.
    if (!Available()) {
        ActionState(16,"m1arc Breach: mouse click not armed (Skip Breach OFF or game unavailable)");
        return;
    }

    auto controller=weakController.Lock();
    auto* context=controller.GetPtr();
    if (!context || !std::isfinite(normalized.X) ||
        !std::isfinite(normalized.Y) ||
        normalized.X<0.02f || normalized.X>0.98f ||
        normalized.Y<0.02f || normalized.Y>0.98f) {
        ActionState(10,"m1arc Breach: physical click rejected (controller or coordinates)");
        return;
    }
    auto* rtti=RED4ext::CRTTISystem::Get();
    auto* hackingClass=rtti ? rtti->GetClass(
        "gameuiHackingMinigameLogicController") : nullptr;
    const auto* contextType=context->GetType();
    if (!hackingClass || !contextType || !contextType->IsA(hackingClass)) {
        ActionState(11,"m1arc Breach: physical click rejected (not hacking controller)");
        return;
    }

    const auto& input=InputRouter::Get();
    const HWND game=input.Window();
    if (!game || !IsWindow(game) || GetForegroundWindow()!=game ||
        input.IsOpen() || input.IsRecording()) {
        ActionState(12,"m1arc Breach: physical click rejected (focus or overlay)");
        return;
    }
    RECT client{};
    if (!GetClientRect(game,&client)) {
        ActionState(13,"m1arc Breach: physical click rejected (client geometry)");
        return;
    }
    const long width=client.right-client.left;
    const long height=client.bottom-client.top;
    if (width<640 || height<480) {
        ActionState(14,"m1arc Breach: physical click rejected (invalid viewport)");
        return;
    }
    const long x=static_cast<long>(std::lround(
        static_cast<double>(normalized.X)*static_cast<double>(width-1)));
    const long y=static_cast<long>(std::lround(
        static_cast<double>(normalized.Y)*static_cast<double>(height-1)));
    const auto sent=m1arc::input::ClickGameClient(
        game,{x,y},m1arc::input::MouseButton::Left,true);
    if (sent!=m1arc::input::ClickResult::Sent) {
        ActionState(15,"m1arc Breach: physical click rejected (Win32 input failed)");
        return;
    }
    m1arc::RenderBridge::Get().LogInfo(
        "[Skip Breach] Physical grid click sent; awaiting native OnPositionSelected acknowledgement");
    *output=true; // SendInput submitted, NEVER proof that a cell was accepted
}

void AvailableNative(RED4ext::IScriptable*,
                     RED4ext::CStackFrame* frame,bool* output,std::int64_t) {
    frame->code++;
    if (output) *output=Available();
}

// Explicitly opt-in addon diagnostic bridge, NOT a new scripting runtime.
// The script logs into the same RED4ext M1 Arc log as native dispatch.
// Keep a hard byte cap so malformed mods cannot flood the logger.
void TraceNative(RED4ext::IScriptable*,
                 RED4ext::CStackFrame* frame,bool* output,std::int64_t) {
    RED4ext::CString message;
    RED4ext::GetParameter(frame,&message);
    frame->code++;
    if (output) *output=false;
    const char* text=message.c_str();
    if (!text) return;
    std::size_t bytes=0;
    while (bytes<480 && text[bytes]) ++bytes;
    if (bytes==0) return;
    std::string line="[Skip Breach] ";
    line.append(text,bytes);
    m1arc::RenderBridge::Get().LogInfo(line.c_str());
    if (output) *output=true;
}
} // namespace

void Initialize() noexcept {
    // A read-only executable version check is harmless on M1 Arc startup.
    // No input is hooked, no solver is started and the legacy function
    // signature is NOT scanned until a mod explicitly requests that route.
    HMODULE module=GetModuleHandleW(nullptr);
    wchar_t exe[MAX_PATH+1]{};
    const DWORD length=module ? GetModuleFileNameW(module,exe,MAX_PATH) : 0;
    g_supportedGameVersion.store(
        length>0 && length<MAX_PATH && ExactGameVersion(exe),
        std::memory_order_release);
}
bool Available() noexcept {
    // Missing Skip Breach manifest or its default-OFF setting => inert.
    // This applies equally to the legacy selector and the Win32 click route.
    if (!SkipBreachEnabled()) return false;
    if (!g_supportedGameVersion.load(std::memory_order_acquire)) {
        ActionState(9,"m1arc Breach: optional input disabled (unsupported game executable)");
        return false;
    }

    // The physical click path only needs the game and UI to be available.
    // It must NOT depend on resolving the old game-internal selector.
    const auto& input=InputRouter::Get();
    const HWND game=input.Window();
    if (!game || !IsWindow(game) || GetForegroundWindow()!=game) {
        ActionState(7,"m1arc Breach: waiting for focused game window");
        return false;
    }
    if (input.IsOpen() || input.IsRecording()) {
        ActionState(8,"m1arc Breach: waiting for M1 Arc overlay to release input");
        return false;
    }
    return true;
}
const char* StatusMessage() noexcept {
    switch (g_status.load(std::memory_order_acquire)) {
    case SelectorStatus::Ready:
        return "m1arc Breach: optional legacy selector resolved (no auto-input without enabled Skip Breach)";
    case SelectorStatus::UnsupportedVersion:
        return "m1arc Skip Breach DISABLED: executable file version is not 3.0.88.46235 (2.5)";
    case SelectorStatus::InvalidExecutable:
        return "m1arc Skip Breach DISABLED: unable to inspect the game executable";
    case SelectorStatus::SignatureNotFound:
        return "m1arc Skip Breach DISABLED: native selector signature not found in this executable";
    case SelectorStatus::UnsafeFunctionEntry:
        return "m1arc Skip Breach DISABLED: native selector is ambiguous or not a safe function entry";
    default:
        return "m1arc Breach: native input bridge dormant; activated only by enabled Skip Breach mod";
    }
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

    // A matched global RTTI definition means missing engine methods do
    // not invalidate the entire installed M1Arc.reds API at startup.
    auto* select=RED4ext::CGlobalFunction::Create(
        "M1Arc_BreachSelectCell","M1Arc_BreachSelectCell",
        reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&SelectCell));
    select->flags=RED4ext::CBaseFunction::Flags{
        .isNative=true,.isStatic=true
    };
    select->AddParam("whandle:inkLogicController","controller");
    select->AddParam("Vector2","position");
    select->SetReturnType("Bool");
    rtti->RegisterFunction(select);

    // Real mouse-button path reuses the existing guarded m1core-derived
    // Win32 helper. No OCR/CMD/other DLL; game handles normal UI clicks.
    auto* click=RED4ext::CGlobalFunction::Create(
        "M1Arc_BreachClickAt","M1Arc_BreachClickAt",
        reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&ClickAt));
    click->flags=RED4ext::CBaseFunction::Flags{
        .isNative=true,.isStatic=true
    };
    click->AddParam("whandle:inkLogicController","controller");
    click->AddParam("Vector2","normalized");
    click->SetReturnType("Bool");
    rtti->RegisterFunction(click);

    // Paired addon-only declaration in M1ArcBreachNative.reds.
    // Never modify the known-working core M1Arc.reds native definitions.
    auto* trace=RED4ext::CGlobalFunction::Create(
        "M1Arc_BreachTrace","M1Arc_BreachTrace",
        reinterpret_cast<RED4ext::ScriptingFunction_t<void*>>(&TraceNative));
    trace->flags=RED4ext::CBaseFunction::Flags{
        .isNative=true,.isStatic=true
    };
    trace->AddParam("String","message");
    trace->SetReturnType("Bool");
    rtti->RegisterFunction(trace);
}
} // namespace m1arc::breach
