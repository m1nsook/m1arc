#include "input.hpp"
#include "ui.hpp"
#include "localization.hpp"
#include "graphics.hpp"
#include "registry.hpp"
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
bool Bindable(int key) { return IsMenuKey(key); }
// Normalize RAWINPUT and legacy events to the same physical modifier.
unsigned NormalizeKey(unsigned key,unsigned scan,bool extended)
{
    if (key==VK_SHIFT) {
        const auto mapped=MapVirtualKeyW(scan,MAPVK_VSC_TO_VK_EX);
        if (mapped==VK_LSHIFT || mapped==VK_RSHIFT) return mapped;
    }
    if (key==VK_CONTROL) return extended ? VK_RCONTROL : VK_LCONTROL;
    if (key==VK_MENU) return extended ? VK_RMENU : VK_LMENU;
    return key;
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
            const auto& overlay=doc["overlay"];
            const int key=overlay.value("hotkey",0);
            const int mods=overlay.value("hotkey_modifiers",0);
            if (ValidMenuShortcut(key,mods))
                shortcut_.store(PackMenuShortcut(key,mods));
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
        menuPress_.Reset();
        menuCapture_.Reset();
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
    ClearWheel();
    auto* hwnd = window_.load();
    open_.store(false);
    recording_.store(false);
    CancelModRebind();
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
        std::lock_guard lock(stateMutex_);
        OpenRebindLocked();
    }
}

void InputRouter::Toggle() noexcept
{
    if (!HasKey() || recording_.load()) return;
    open_.store(!open_.load());
    if (const auto hwnd = window_.load())
        PostMessageW(hwnd, kCursorSync, 0, 0);
}

bool InputRouter::BeginModRebind(std::string modId, std::string settingId) noexcept
{
    if (!open_.load() || recording_.load() || modRecording_.load() || !HasKey())
        return false;
    const auto mods = Settings().Snapshot();
    const bool valid = std::any_of(mods.begin(), mods.end(), [&](const auto& mod) {
        return mod.id == modId && std::any_of(mod.settings.begin(),
            mod.settings.end(), [&](const auto& setting) {
                return setting.id == settingId && setting.kind == SettingKind::Hotkey;
            });
    });
    if (!valid) return false;
    std::lock_guard lock(stateMutex_);
    if (modRecording_.load() || recording_.load()) return false;
    modCaptureId_ = std::move(modId);
    modCaptureSetting_ = std::move(settingId);
    modRecording_.store(true);
    return true;
}

void InputRouter::CancelModRebind() noexcept
{
    std::lock_guard lock(stateMutex_);
    modRecording_.store(false);
    modCaptureId_.clear();
    modCaptureSetting_.clear();
}

bool InputRouter::RecordingModKey(const std::string& modId,
                                  const std::string& settingId) const noexcept
{
    std::lock_guard lock(stateMutex_);
    return modRecording_.load() && modCaptureId_ == modId &&
           modCaptureSetting_ == settingId;
}

void InputRouter::OpenRebindLocked() noexcept
{
    if (!recording_.load()) closeAfterBinding_=!open_.load();
    modRecording_.store(false);
    modCaptureId_.clear();
    modCaptureSetting_.clear();
    menuCapture_.Reset();
    menuPress_.Reset();
    recording_.store(true);
    open_.store(true);
    if (const auto hwnd=window_.load()) PostMessageW(hwnd,kCursorSync,0,0);
}

void InputRouter::BeginRebind() noexcept
{
    std::lock_guard lock(stateMutex_);
    if (open_.load()) OpenRebindLocked();
}

void InputRouter::CancelRebind() noexcept
{
    std::lock_guard lock(stateMutex_);
    if (!HasKey()) return;
    menuCapture_.Reset();
    menuPress_.Reset();
    recording_.store(false);
    if (closeAfterBinding_) open_.store(false);
    closeAfterBinding_=false;
    if (const auto hwnd=window_.load()) PostMessageW(hwnd,kCursorSync,0,0);
}

void InputRouter::StartCapture() noexcept
{
    std::lock_guard lock(stateMutex_);
    if (!recording_.load() || !CanCapture()) return;
    // Ignore keys already held when the user explicitly starts capture.
    for (unsigned key=VK_BACK; key<held_.size(); ++key)
        held_[key]=(GetAsyncKeyState(static_cast<int>(key)) & 0x8000)!=0;
    held_[VK_SHIFT]=held_[VK_CONTROL]=held_[VK_MENU]=false;
    menuCapture_.Arm();
}

bool InputRouter::IsCapturing() const noexcept
{
    std::lock_guard lock(stateMutex_);
    return menuCapture_.Armed();
}

bool InputRouter::HasCandidate() const noexcept
{
    std::lock_guard lock(stateMutex_);
    return menuCapture_.Candidate()!=0;
}

std::string InputRouter::CandidateLabel() const
{
    std::lock_guard lock(stateMutex_);
    return DescribeShortcut(menuCapture_.Candidate());
}

void InputRouter::Close() noexcept
{
    std::lock_guard lock(stateMutex_);
    modRecording_.store(false);
    modCaptureId_.clear();
    modCaptureSetting_.clear();
    if (recording_.load() && !HasKey()) return;
    menuCapture_.Reset();
    menuPress_.Reset();
    closeAfterBinding_=false;
    recording_.store(false);
    open_.store(false);
    if (const auto hwnd=window_.load()) PostMessageW(hwnd,kCursorSync,0,0);
}

bool InputRouter::ConfirmCandidate() noexcept
{
    std::lock_guard lock(stateMutex_);
    const int candidate=menuCapture_.Candidate();
    if (!recording_.load() || !candidate || !CanCapture()) return false;
    if (!Store().Update([&](nlohmann::json& data) {
        if (!data["overlay"].is_object()) data["overlay"]=nlohmann::json::object();
        data["overlay"]["hotkey"]=MenuKey(candidate);
        data["overlay"]["hotkey_modifiers"]=MenuMods(candidate);
    })) return false;
    const bool close=!HasKey() || closeAfterBinding_;
    shortcut_.store(candidate);
    menuCapture_.Reset();
    menuPress_.Reset();
    recording_.store(false);
    closeAfterBinding_=false;
    if (close) open_.store(false);
    if (const auto hwnd=window_.load()) PostMessageW(hwnd,kCursorSync,0,0);
    return true;
}

int InputRouter::ModifiersLocked() const noexcept
{
    const bool ctrl=held_[VK_CONTROL] || held_[VK_LCONTROL] || held_[VK_RCONTROL];
    const bool shift=held_[VK_SHIFT] || held_[VK_LSHIFT] || held_[VK_RSHIFT];
    return (ctrl ? kMenuCtrl : 0) | (shift ? kMenuShift : 0);
}

bool InputRouter::BlockedModifiersLocked() const noexcept
{
    return held_[VK_MENU] || held_[VK_LMENU] || held_[VK_RMENU] ||
        held_[VK_LWIN] || held_[VK_RWIN] ||
        (GetAsyncKeyState(VK_MENU) & 0x8000) ||
        (GetAsyncKeyState(VK_LWIN) & 0x8000) ||
        (GetAsyncKeyState(VK_RWIN) & 0x8000);
}

void InputRouter::KeyEdge(unsigned code, bool down) noexcept
{
    if (code >= held_.size() || window_.load()!=GetForegroundWindow()) return;
    std::lock_guard lock(stateMutex_);
    if (held_[code]==down) return;
    held_[code]=down;
    if (!baseReady_.load() || !rendererReady_.load()) return;
    const int mods=ModifiersLocked();
    if (BlockedModifiersLocked()) { menuPress_.Reset(); return; }
    if (down && PackMenuShortcut(static_cast<int>(code),mods)==kRecoveryShortcut) {
        OpenRebindLocked();
        return;
    }
    if (recording_.load()) {
        if (down && CanCapture()) {
            if (code==VK_ESCAPE) menuCapture_.Reset();
            else menuCapture_.Offer(static_cast<int>(code),mods);
        }
        return;
    }
    if (modRecording_.load()) {
        // Key capture happens only while the M1 Arc menu is visible.
        // Escape cancels this capture only; it never closes the menu.
        if (down && code == VK_ESCAPE) {
            modRecording_.store(false);
            modCaptureId_.clear();
            modCaptureSetting_.clear();
        } else if (down && Bindable(static_cast<int>(code)) &&
                   !MatchesMenuShortcut(shortcut_.load(),static_cast<int>(code),0)) {
            if (Settings().Set(modCaptureId_,modCaptureSetting_,static_cast<int>(code))) {
                ShowToast(Translations().Text("toast.mod_shortcut_saved","Mod shortcut saved: ") +
                          DescribeKey(static_cast<int>(code)),2.8f,ToastKind::Success);
                modRecording_.store(false);
                modCaptureId_.clear();
                modCaptureSetting_.clear();
                // Do not fire the newly assigned shortcut on release.
                justAssigned_ = static_cast<int>(code);
                skipAssignedKeyRelease_ = true;
            } else {
                ShowToast(Translations().Text("toast.setting_failed","Could not save: ") +
                          modCaptureSetting_,3.5f,ToastKind::Error);
            }
        }
        return;
    }
    if (!down && skipAssignedKeyRelease_ && code == static_cast<unsigned>(justAssigned_)) {
        skipAssignedKeyRelease_ = false;
        justAssigned_ = 0;
        return;
    }
    // Match on press; release modifiers in either order.
    if (down && MatchesMenuShortcut(shortcut_.load(),static_cast<int>(code),mods)) {
        menuPress_.Down(shortcut_.load(),static_cast<int>(code),mods);
        return;
    }
    if (!down && menuPress_.Up(static_cast<int>(code))) {
        Toggle();
        return;
    }
    // Dispatch mod shortcuts only on the physical key-down edge, while
    // gameplay owns focus. Leave all OS/game input passing through as before.
    if (down && baseReady_.load() && HasKey() && !open_.load() &&
        window_.load() == GetForegroundWindow() &&
        !(GetKeyState(VK_MENU) & 0x8000) &&
        !(GetKeyState(VK_CONTROL) & 0x8000) &&
        !(GetKeyState(VK_LWIN) & 0x8000) &&
        !(GetKeyState(VK_RWIN) & 0x8000)) {
        Settings().QueueHotkeyKey(static_cast<int>(code));
    }
}

void InputRouter::ClearWheel() noexcept
{
    rawWheelX_.exchange(0);
    rawWheelY_.exchange(0);
    legacyWheelX_.exchange(0);
    legacyWheelY_.exchange(0);
}

void InputRouter::FlushWheel() noexcept
{
    // WndProc only accumulates wheel deltas. Submit on the frame thread so
    // NewFrame cannot race a wheel event being appended to ImGui's queue.
    const int rawX=rawWheelX_.exchange(0), rawY=rawWheelY_.exchange(0);
    const int legacyX=legacyWheelX_.exchange(0), legacyY=legacyWheelY_.exchange(0);
    if (!CanCapture() || !ImGui::GetCurrentContext()) return;
    // A physical wheel tick can arrive via both RAWINPUT and legacy Win32.
    const int x=rawX ? rawX : legacyX, y=rawY ? rawY : legacyY;
    if (x || y) ImGui::GetIO().AddMouseWheelEvent(
        -static_cast<float>(x)/WHEEL_DELTA,static_cast<float>(y)/WHEEL_DELTA);
}

void InputRouter::HandleRawMouse(HWND hwnd, const RAWMOUSE& mouse) noexcept
{
    if (!CanCapture()) return;
    if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
        rawWheelY_.fetch_add(static_cast<SHORT>(mouse.usButtonData));
    if (mouse.usButtonFlags & RI_MOUSE_HWHEEL)
        rawWheelX_.fetch_add(static_cast<SHORT>(mouse.usButtonData));
    if (!ImGui::GetCurrentContext()) return;
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
            KeyEdge(NormalizeKey(key.VKey,key.MakeCode,(key.Flags & RI_KEY_E0)!=0),down);
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
    return DescribeShortcut(shortcut_.load());
}

std::string InputRouter::DescribeShortcut(int shortcut)
{
    std::string label;
    if (MenuMods(shortcut) & kMenuCtrl) label+="Ctrl + ";
    if (MenuMods(shortcut) & kMenuShift) label+="Shift + ";
    return label+DescribeKey(MenuKey(shortcut));
}

std::string InputRouter::DescribeKey(int key)
{
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
            ClearWheel();
            {
                std::lock_guard lock(stateMutex_);
                held_.fill(false);
                menuCapture_.Reset();
                menuPress_.Reset();
                justAssigned_=0;
                skipAssignedKeyRelease_=false;
                modRecording_.store(false);
                modCaptureId_.clear();
                modCaptureSetting_.clear();
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
        KeyEdge(NormalizeKey(static_cast<unsigned>(w),
                    (static_cast<std::uintptr_t>(l)>>16)&0xff,
                    (static_cast<std::uintptr_t>(l)&(1ull<<24))!=0),
                msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

    // Do not swallow game input unless ImGui has a completed GPU frame and
    // this menu owns the foreground game window / cursor.
    if (CanCapture()) {
        if (msg==WM_MOUSEWHEEL || msg==WM_MOUSEHWHEEL) {
            auto& wheel=msg==WM_MOUSEWHEEL ? legacyWheelY_ : legacyWheelX_;
            wheel.fetch_add(static_cast<SHORT>(HIWORD(w)));
            return 0;
        }
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
