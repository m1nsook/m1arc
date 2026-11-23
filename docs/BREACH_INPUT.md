# Breach Protocol input bridges (experimental)

M1 Arc (native RED4ext C++) contains two different hacking input routes:

- **Legacy game-internal selector:** `M1Arc_BreachSelectCell(controller, Vector2(row,col))` calls a game 2.5 function found by a guarded signature scan. The user's first stealth puzzle did not acknowledge this dispatch, while the second puzzle did.
- **New real Windows click:** `M1Arc_BreachClickAt(controller, normalizedPoint)` calls M1 Arc's already existing native `ClickGameClient` helper, based on Win32 `SetCursorPos` and `SendInput(MOUSEEVENTF_LEFTDOWN/UP)`. It does not call the internal game selector. This is the new Skip Breach route.

## Safety and integration

`M1Arc_BreachClickAt` requires a live `gameuiHackingMinigameLogicController` RTTI object, current Cyberpunk foreground HWND, an unoccupied M1 Arc overlay, a finite normalized position strictly within the client window, and a valid 640x480-or-larger client area. The existing click backend additionally checks process ID and client pixel bounds and sends a release on partial injection.

The REDscript addon derives the point from **live** `GetScreenPosition` rectangles in the base-game 2.5 widget hierarchy:

`AllParts/Top/NetworkCodeMapAndButton/wrapper/NetworkCodeMap/NetworkCodeMap/ContentWithBorder/ContentContainer/GridContainer/Grid`

The recovered original `hacking_minigame.inkwidget` has 48 `GridCell6` children and default `wrappingWidgetCount=6`. The consumer checks that the *live* grid wrap equals the currently captured puzzle dimension, validates that the first, right, and down cell positions form a coherent two-dimensional layout, and calculates the cell centre relative to the root bounds. **No fixed 1080p coordinates, OCR or CMD executable.** If any geometry is missing or inconsistent, it does not click.

`M1Arc_BreachInputAvailable` checks **only** the registered Skip Breach mod's `enabled` setting, supported game 2.5 executable version, focused Cyberpunk window, and absence of M1 Arc overlay/keybind capture. The Windows physical click does **not** need the old selector signature. The legacy selector checks its own signature only when explicitly used, and its signature scan no longer runs at M1 Arc startup.

**Mod-only activation:** M1 Arc registers the native functions for compatibility with the separately installed REDscript addon, but does not call them itself. Even if another script invokes a native entry directly, it refuses to send input unless the separately installed `m1nsook.skipbreach` manifest is registered and its `enabled` setting is `true`. Missing mod or default-OFF setting means a dormant bridge. Switching the setting OFF prevents new native input submissions; Skip Breach itself also stops sequencing. The mod owns puzzle capture, pathfinding, first-board lifecycle, click requests, and game acknowledgement handling.

A true native return means only that `SendInput` submitted events. The REDscript addon advances one cell **only after** a matching `HackingMinigameGameController.OnPositionSelected` acknowledgement. It never blindly repeats a submitted but unacknowledged click, changes quest facts, marks daemons complete, pauses the hacking timer, or closes/reopens the puzzle.

**Origin:** `yuri-sevatz/cyber_breach` (BSD-2-Clause) independently demonstrates real Win32 pointer input for Breach Protocol. M1 Arc reuses its own pre-existing mouse helper; it does not bundle that project's executable or copy its OCR/solver implementation. The game UI structure was checked against the user's extracted original 2.5 UI assets.

## Packaging and status

Install **matching** M1 Arc PR #1 DLL and CP2077 Skip Breach PR #46 source/addon. `M1Arc_BreachInputAvailable`, `M1Arc_BreachSelectCell`, `M1Arc_BreachClickAt`, and `M1Arc_BreachTrace` are registered globally by the M1 Arc DLL and declared only in the Skip Breach addon. The existing stable `r6/scripts/m1arc/M1Arc.reds` is unchanged. Wrong DLL/addon revision can cause REDscript native validation errors on startup.

The original game-internal selector is retained for diagnostics and compatibility but is not used by the current Skip Breach script.

**Important:** Windows C++ CI compilation and REDscript parser checks do **not** prove first-puzzle gameplay acceptance. This remains an experimental draft. The user's stealth mission fails on puzzle cancellation; **do not ask the user to repeat that mission test**. M1 Arc core settings functionality remains independent of this optional mod.
