// The DLL target uses its established precompiled header. The isolated
// native test target needs only Win32 and must not pull in the full runtime.
#ifndef M1ARC_MOUSE_CLICK_STANDALONE_TEST
#include "input.hpp"
#endif

#include "mouse_click.hpp"

namespace m1arc::input {
namespace {

class Win32MouseBackend final : public MouseBackend
{
public:
    ClickResult Resolve(HWND target, ClientPoint location, POINT& screen) override
    {
        if (!target || !IsWindow(target) || !IsWindowVisible(target) || IsIconic(target))
            return ClickResult::InvalidTarget;

        DWORD process = 0;
        GetWindowThreadProcessId(target, &process);
        if (process != GetCurrentProcessId())
            return ClickResult::InvalidTarget;
        if (GetForegroundWindow() != target)
            return ClickResult::NotForeground;

        RECT client{};
        if (!GetClientRect(target, &client))
            return ClickResult::InvalidTarget;
        if (location.x < client.left || location.y < client.top ||
            location.x >= client.right || location.y >= client.bottom)
            return ClickResult::OutsideClient;

        POINT position{location.x, location.y};
        if (!ClientToScreen(target, &position))
            return ClickResult::InvalidTarget;
        screen = position;
        return ClickResult::Sent;
    }

    bool MoveCursor(POINT screen) override
    {
        return SetCursorPos(screen.x, screen.y) != 0;
    }

    UINT Inject(const INPUT* events, UINT count) override
    {
        return SendInput(count, const_cast<INPUT*>(events), sizeof(INPUT));
    }
};

} // namespace

ClickResult ClickClient(MouseBackend& backend, HWND target, ClientPoint location,
                        MouseButton button, bool gameplayContextConfirmed)
{
    // A future feature must first validate its in-game panel and verify
    // that m1arc isn't capturing input. Disabled unless explicitly allowed.
    if (!gameplayContextConfirmed)
        return ClickResult::NotAllowed;

    POINT position{};
    const auto resolved = backend.Resolve(target, location, position);
    if (resolved != ClickResult::Sent)
        return resolved;
    if (!backend.MoveCursor(position))
        return ClickResult::CursorFailed;

    const DWORD down = button == MouseButton::Left
        ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
    const DWORD up = button == MouseButton::Left
        ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;

    INPUT events[2]{};
    events[0].type = INPUT_MOUSE;
    events[0].mi.dwFlags = down;
    events[1].type = INPUT_MOUSE;
    events[1].mi.dwFlags = up;

    if (backend.Inject(events, 2) != 2)
    {
        // If only the down event got through, attempt a release to avoid
        // leaving a button logically held in the target application.
        INPUT release{};
        release.type = INPUT_MOUSE;
        release.mi.dwFlags = up;
        (void)backend.Inject(&release, 1);
        return ClickResult::InjectionFailed;
    }
    return ClickResult::Sent;
}

ClickResult ClickGameClient(HWND target, ClientPoint location, MouseButton button,
                            bool gameplayContextConfirmed)
{
    if (!gameplayContextConfirmed)
        return ClickResult::NotAllowed;
#ifndef M1ARC_MOUSE_CLICK_STANDALONE_TEST
    // An open/recording M1 Arc menu owns the mouse. Never click into the game
    // under that overlay even when a caller mistakenly declares a valid UI.
    if (m1arc::InputRouter::Get().IsOpen() ||
        m1arc::InputRouter::Get().IsRecording())
        return ClickResult::NotAllowed;
#endif
    Win32MouseBackend backend;
    return ClickClient(backend, target, location, button, true);
}

} // namespace m1arc::input
