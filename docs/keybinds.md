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

## Per-mod gameplay hotkeys

Mods can now declare a persistent native hotkey control in their
`settings.json`. It is separate from the framework's menu shortcut,
and does **not** require the Lua `registerHotkey` API or CET.

~~~json
{
  "id": "spawn_hotkey",
  "kind": "hotkey",
  "label": "Spawn hotkey",
  "group": "Weapons",
  "default": 0
}
~~~

Open M1 Arc to bind a key or clear the binding. `default: 0`
means unassigned. Escape cancels key recording. Modifier keys,
Windows keys and the M1 Arc menu key are reserved.

The assigned key is saved with the mod's other settings, survives
game restarts, and participates in profiles. While the game is focused
and M1 Arc is closed, a key-down queues a one-shot action for the mod.
The mod consumes it from its normal REDscript code:

~~~reds
if M1Arc_ConsumeAction("example.my-mod", "spawn_hotkey") {
  // Handle the action using the owning mod's REDscript.
}
~~~

Key presses are **not** commands, and M1 Arc does not execute
scripts from JSON or intercept normal gameplay input.
See [mod manifests](/mod-manifest) for details.
