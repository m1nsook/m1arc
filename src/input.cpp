#include "input.hpp"
#include "graphics.hpp"
#include "state.hpp"
#include "backends/win32.h"

#include <imgui.h>
#include <RED4ext/CName.hpp>
#include <RED4ext/GameEngine.hpp>
#include <algorithm>
#include <cstring>
#include <vector>

namespace m1arc {
namespace {
constexpr UINT kCursorSync = WM_APP + 0x6A1;
constexpr std::uint32_t kForceCursorHash = 2130646213u;
using ForceCursor = void (*)(RED4ext::CBaseEngine::UnkD0*, RED4ext::CName, bool);
bool PassThroughOS(UINT msg, WPARAM w, LPARAM l)
{
    if (msg == WM_CLOSE || msg == WM_DESTROY || msg == WM_NCDESTROY ||
        msg == WM_QUIT) return true;
    if (msg == WM_SYSCOMMAND && (w & 0xFFF0u) == SC_CLOSE) return true;
    return msg == WM_SYSKEYDOWN && w == VK_F4 &&
           (static_cast<std::uintptr_t>(l) & (1ull << 29)) != 0;
}
bool Bindable(int key)
{
    if (key < VK_BACK || key >= VK_PACKET) return false;
    switch (key) {
    case VK_ESCAPE: case VK_SHIFT: case VK_CONTROL: case VK_MENU:
    case VK_LSHIFT: case VK_RSHIFT: case VK_LCONTROL: case VK_RCONTROL:
    case VK_LMENU: case VK_RMENU: case VK_LWIN: case VK_RWIN:
    case VK_SLEEP: case VK_PROCESSKEY: return false;
    default: return true;
    }
}
}

InputRouter& InputRouter::Get() noexcept
{
    static InputRouter input;
    return input;
}

void InputRouter::ReadKeyFromSettings() noexcept
{
    try {
        const auto doc = nlohmann::json::parse(Store().Snapshot());
        if (doc.contains("overlay") && doc["overlay"].is_object()) {
            const auto& key = doc["overlay"].value("hotkey", 0);
            if (Bindable(key)) hotkey_.store(key);
        }
    } catch (...) {}
}

bool InputRouter::Attach(HWND hwnd) noexcept
{
    if (window_.load() == hwnd && oldProc_) return true;
    if (!IsWindow(hwnd)) return false;
    if (window_.load()) Detach();
    SetLastError(0);
    const LONG_PTR original = SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                            reinterpret_cast<LONG_PTR>(&WindowProc));
    if (original == 0 && GetLastError() != 0) return false;
    oldProc_ = reinterpret_cast<WNDPROC>(original);
    if (!oldProc_) {
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, original);
        return false;
    }
    {
        std::lock_guard lock(stateMutex_);
        held_.fill(false);
        justAssigned_ = 0;
        skipAssignedKeyRelease_ = false;
    }
    window_.store(hwnd);
    ReadKeyFromSettings();
    // Never take over input on initial DLL load. First-run UI is armed
    // only once RED4ext reports that base game initialization is complete.
    SyncCursorOnWindowThread();
    return true;
}

void InputRouter::Detach() noexcept
{
    auto* hwnd = window_.load();
    open_.store(false);
    recording_.store(false);
    rendererReady_.store(false);
    if (hwnd && IsWindow(hwnd)) {
        // Release our cursor reason *before* restoring the original WndProc,
        // on the HWND-owning thread as in the previously working code.
        DWORD_PTR ignored{};
        SendMessageTimeoutW(hwnd,kCursorSync,0,0,SMTO_ABORTIFHUNG,
                            200,&ignored);
        if (oldProc_) SetWindowLongPtrW(hwnd,GWLP_WNDPROC,
                                        reinterpret_cast<LONG_PTR>(oldProc_));
    }
    window_.store(nullptr);
    oldProc_ = nullptr;
}

bool InputRouter::WantsCursor() const noexcept
{
    const auto hwnd = window_.load();
    return hwnd && GetForegroundWindow() == hwnd &&
           rendererReady_.load() && open_.load();
}

bool InputRouter::CanCapture() const noexcept
{
    // Never swallow gameplay input until the native REDengine cursor is
    // actually acquired, even if the menu has already drawn its first frame.
    return WantsCursor() && cursorOwned_.load();
}

void InputRouter::SyncCursorOnWindowThread() noexcept
{
    const bool requested = WantsCursor();
    if (cursorOwned_.load() == requested) return;
    auto* engine = RED4ext::CGameEngine::Get();
    const auto address = ResolveGameSymbol(kForceCursorHash);
    if (!engine || !engine->unkD0 || !address) return;
    static constexpr RED4ext::CName reason("M1Arc_UI");
    reinterpret_cast<ForceCursor>(address)(engine->unkD0, reason, requested);
    cursorOwned_.store(requested);
}

void InputRouter::SetRendererReady(bool ready) noexcept
{
    if (rendererReady_.exchange(ready) == ready) return;
    if (auto* hwnd = window_.load())
        PostMessageW(hwnd, kCursorSync, 0, 0);
}

void InputRouter::ArmFirstRun() noexcept
{
    if (baseReady_.exchange(true)) return;
    // RED4ext can report BaseInitialization BEFORE the game's HWND is
    // attached to our Win32 input router. Loading only from Attach()
    // caused the setup dialog on EVERY launch despite a saved hotkey.
    // The StateStore is configured in Main() before this callback is armed.
    ReadKeyFromSettings();
    if (!HasKey()) {
        recording_.store(true);
        open_.store(true);
        if (const auto hwnd = window_.load())
            PostMessageW(hwnd, kCursorSync, 0, 0);
    }
}

void InputRouter::Toggle() noexcept
{
    if (!HasKey() || recording_.load()) return;
    open_.store(!open_.load());
    if (const auto hwnd = window_.load())
        PostMessageW(hwnd, kCursorSync, 0, 0);
}

void InputRouter::BeginRebind() noexcept
{
    if (open_.load()) recording_.store(true);
}

void InputRouter::CancelRebind() noexcept
{
    if (HasKey()) recording_.store(false);
}

void InputRouter::Close() noexcept
{
    if (recording_.load() && !HasKey()) return; // First-run binding mandatory.
    recording_.store(false);
    open_.store(false);
    if (const auto hwnd = window_.load())
        PostMessageW(hwnd, kCursorSync, 0, 0);
}

void InputRouter::CommitKey(int key) noexcept
{
    if (!Bindable(key)) return;
    const bool firstRun = !HasKey();
    if (!Store().Update([&](nlohmann::json& data) {
        data["overlay"]["hotkey"] = key;
    })) return;
    hotkey_.store(key);
    recording_.store(false);
    justAssigned_ = key;
    skipAssignedKeyRelease_ = true;
    // Match the working m1core first-run UX: bind key, close setup, restore game.
    if (firstRun) open_.store(false);
    if (const auto hwnd = window_.load())
        PostMessageW(hwnd, kCursorSync, 0, 0);
}

void InputRouter::KeyEdge(unsigned code, bool down) noexcept
{
    if (code >= held_.size()) return;
    std::lock_guard lock(stateMutex_);
    if (held_[code] == down) return;
    held_[code] = down;
    if (recording_.load() && down) {
        CommitKey(static_cast<int>(code));
        return;
    }
    if (!down && skipAssignedKeyRelease_ && code == static_cast<unsigned>(justAssigned_)) {
        skipAssignedKeyRelease_ = false;
        justAssigned_ = 0;
        return;
    }
    // A dedicated M1 Arc shortcut, never CET's binding or key namespace.
    if (!down && static_cast<int>(code) == hotkey_.load())
        Toggle();
}

void InputRouter::HandleRawMouse(HWND hwnd, const RAWMOUSE& mouse) noexcept
{
    if (!CanCapture() || !ImGui::GetCurrentContext()) return;
    struct MouseAction { USHORT flag; UINT event; };
    constexpr MouseAction actions[] = {
        {RI_MOUSE_LEFT_BUTTON_DOWN, WM_LBUTTONDOWN},
        {RI_MOUSE_LEFT_BUTTON_UP, WM_LBUTTONUP},
        {RI_MOUSE_RIGHT_BUTTON_DOWN, WM_RBUTTONDOWN},
        {RI_MOUSE_RIGHT_BUTTON_UP, WM_RBUTTONUP},
        {RI_MOUSE_MIDDLE_BUTTON_DOWN, WM_MBUTTONDOWN},
        {RI_MOUSE_MIDDLE_BUTTON_UP, WM_MBUTTONUP},
    };
    for (const auto& action : actions)
        if (mouse.usButtonFlags & action.flag)
            ImGui_ImplWin32_WndProcHandler(hwnd, action.event, 0, 0);
    if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
        ImGui_ImplWin32_WndProcHandler(hwnd, WM_MOUSEWHEEL,
            MAKEWPARAM(0,mouse.usButtonData),0);
    if (mouse.usButtonFlags & RI_MOUSE_HWHEEL)
        ImGui_ImplWin32_WndProcHandler(hwnd, WM_MOUSEHWHEEL,
            MAKEWPARAM(0,mouse.usButtonData),0);
}

void InputRouter::HandleRawInput(HRAWINPUT raw) noexcept
{
    UINT size = 0;
    if (GetRawInputData(raw,RID_INPUT,nullptr,&size,sizeof(RAWINPUTHEADER)) != 0 ||
        size < sizeof(RAWINPUTHEADER) || size > 4096) return;
    alignas(RAWINPUT) std::array<BYTE,4096> buffer{};
    if (GetRawInputData(raw,RID_INPUT,buffer.data(),&size,
                        sizeof(RAWINPUTHEADER)) != size) return;
    const auto* value = reinterpret_cast<const RAWINPUT*>(buffer.data());
    if (value->header.dwType == RIM_TYPEKEYBOARD &&
        size < sizeof(RAWINPUTHEADER) + sizeof(RAWKEYBOARD)) return;
    if (value->header.dwType == RIM_TYPEMOUSE &&
        size < sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE)) return;
    if (value->header.dwType == RIM_TYPEKEYBOARD) {
        const auto& key = value->data.keyboard;
        const bool down = key.Message == WM_KEYDOWN || key.Message == WM_SYSKEYDOWN;
        const bool up = key.Message == WM_KEYUP || key.Message == WM_SYSKEYUP;
        if (down || up) {
            KeyEdge(key.VKey,down);
            if (CanCapture() && ImGui::GetCurrentContext()) {
                ImGui_ImplWin32_WndProcHandler(window_.load(),
                    down ? WM_KEYDOWN : WM_KEYUP,key.VKey,
                    static_cast<LPARAM>(key.MakeCode << 16));
            }
        }
    } else if (value->header.dwType == RIM_TYPEMOUSE) {
        HandleRawMouse(window_.load(),value->data.mouse);
    }
}

std::string InputRouter::KeyLabel() const
{
    const int key = hotkey_.load();
    if (!key) return "Not assigned";
    wchar_t buf[96]{};
    UINT code = MapVirtualKeyW(static_cast<UINT>(key),MAPVK_VK_TO_VSC);
    if (key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN)
        code |= 0xE000;
    if (GetKeyNameTextW(static_cast<LONG>(code << 16),buf,96) > 0) {
        char utf8[192]{};
        if (WideCharToMultiByte(CP_UTF8,0,buf,-1,utf8,192,nullptr,nullptr))
            return utf8;
    }
    return "Key " + std::to_string(key);
}

LRESULT CALLBACK InputRouter::WindowProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    return Get().Process(hwnd,msg,w,l);
}

LRESULT InputRouter::Process(HWND hwnd, UINT msg, WPARAM w, LPARAM l) noexcept
{
    const auto next = oldProc_;
    if (!next) return DefWindowProcW(hwnd,msg,w,l);
    if (PassThroughOS(msg,w,l))
        return CallWindowProcW(next,hwnd,msg,w,l);

    if (msg == kCursorSync) {
        SyncCursorOnWindowThread();
        return 0;
    }
    if (msg == WM_KILLFOCUS || msg == WM_ACTIVATEAPP) {
        if (msg == WM_KILLFOCUS || !w) {
            {
                std::lock_guard lock(stateMutex_);
                held_.fill(false);
            }
            // Restore game cursor without modifying menu-open intent.
            if (cursorOwned_.load()) {
                auto* engine = RED4ext::CGameEngine::Get();
                const auto address = ResolveGameSymbol(kForceCursorHash);
                if (engine && engine->unkD0 && address) {
                    static constexpr RED4ext::CName reason("M1Arc_UI");
                    reinterpret_cast<ForceCursor>(address)(engine->unkD0,reason,false);
                    cursorOwned_.store(false);
                }
            }
        } else SyncCursorOnWindowThread();
    }
    if (msg == WM_INPUT)
        HandleRawInput(reinterpret_cast<HRAWINPUT>(l));
    // Retry delayed cursor acquisition/release on the actual HWND thread.
    if (cursorOwned_.load() != WantsCursor())
        SyncCursorOnWindowThread();

    // The original m1core registry used RAWINPUT for physical key edges;
    // fallback to legacy if the game supplies them, without double toggles.
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN ||
        msg == WM_KEYUP || msg == WM_SYSKEYUP)
        KeyEdge(static_cast<unsigned>(w),
                msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

    // Do not swallow game input unless ImGui has a completed GPU frame and
    // this menu owns the foreground game window / cursor.
    if (CanCapture()) {
        if (ImGui::GetCurrentContext() && msg != WM_INPUT)
            ImGui_ImplWin32_WndProcHandler(hwnd,msg,w,l);
        if (msg == WM_INPUT ||
            (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) ||
            (msg >= WM_KEYFIRST && msg <= WM_KEYLAST))
            return 0;
    }
    return CallWindowProcW(next,hwnd,msg,w,l);
}
} // namespace m1arc
