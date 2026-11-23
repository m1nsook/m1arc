# M1 Arc internal mouse-click helper

This is the narrowly scoped input utility carried over from the **working
m1core backup** (`core/src/input/MouseClick.{hpp,cpp}` and
`tests/mouse_click_tests.cpp`). No replacement for REDscript or the
existing Win32/ImGui input router.

## API (internal C++, not a scripting extension)

`m1arc::input::ClickGameClient(HWND, ClientPoint, MouseButton,
gameplayContextConfirmed)`.

The caller must prove a supported in-game controller is active **before**
passing `gameplayContextConfirmed=true`. Passing `false` returns
`NotAllowed`, without moving the cursor or posting events. The
production wrapper additionally rejects clicks while M1 Arc's menu or
keybinding modal is open.

The Windows backend checks game window ownership, visibility, focus and
client coordinates, then translates client-to-screen and attempts
`SetCursorPos` + `SendInput` for LEFT/RIGHT down/up. If the first input
attempt injects only part of a pair, it attempts to release the button so
the game is not left with a logically held mouse button. Errors are
returned to the caller.

**Not supported:** unattended autonomous clicking, screen capture/OCR,
access to arbitrary desktop windows, reflection hooks into Cyberpunk,
or triggering clicks while the game is unfocused.

## Tests

The `m1arc_mouse_click_tests` CTest target uses a fake backend
(no real Windows window, no injected input) to exercise:

- safety gate: a rejected context produces zero backend calls
- focus and out-of-bounds rejections
- failure of pointer movement
- left/right event ordering and screen coordinate translation
- partial/failed injection and button-release fallback

This utility **does not solve puzzles** and **does not click in-game yet**.
The planned `Skip Breach` mod will independently read the live vanilla
matrix, daemon sequences and buffer, compute a valid path, verify each
still-selectable node, then submit node selections one at a time via the
game's accepted UI path. The solver and game-UI adapter have not been
implemented or verified here. No "fulfilled daemon" shortcut is accepted.
