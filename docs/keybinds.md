# Panel shortcut and keyboard handling

M1 Arc uses an **independent, user-selected** shortcut for its
native settings panel. You choose it the first time you launch
Cyberpunk with M1 Arc installed, and can rebind it using the panel.

The chosen key is stored in
`red4ext/plugins/m1arc/settings.json` as `overlay.hotkey`.
It is not hardcoded to F8, F10, F11 or the CET menu shortcut.

The same chosen key opens and closes the panel. Escape is **not**
required to close it. The framework also includes an emergency
recovery combination (`Ctrl+Shift+F11`) intended for stuck input.

M1 Arc does not overwrite CET's own hotkey. When the control panel is
not open, normal keyboard and mouse input should continue to reach the game.

The current public settings manifest supports Boolean, integer, float
and choice controls. Do not claim a standalone hotkey-control schema
or a Lua `registerHotkey` API; neither is part of the current M1 Arc
native manifest specification.
