# Internal game-client click helper

This C++ utility is an optional, guarded input helper. It is separate
from M1 Arc's menu keyboard and mouse handling.

`m1arc::input::ClickGameClient(HWND, ClientPoint, MouseButton, gameplayContextConfirmed)`

Calls must supply a verified game controller context. With
`gameplayContextConfirmed=false`, the helper rejects the click without
moving the cursor. It also rejects clicks while M1 Arc's menu or
keybinding capture is active.

On Windows, the implementation checks foreground game-window ownership,
visibility and client bounds before translating coordinates and using
`SetCursorPos` and `SendInput`. If a mouse-down event only partially
succeeds, it attempts to send the corresponding release.

The `m1arc_mouse_click_tests` suite uses a fake backend to check bounds,
focus, ordering, injection failure and release fallback; these tests do
not prove that Cyberpunk's hacking controller accepts mouse events.

Game-specific Skip Breach experiments are tracked separately in
[BREACH_INPUT.md](./BREACH_INPUT.md). Do not change the framework's
working input router as part of this helper.
