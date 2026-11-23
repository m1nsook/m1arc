# Panel shortcut and keyboard handling

M1 Arc uses a **user-selected** shortcut for its native settings panel. You choose it the first time you launch
Cyberpunk with M1 Arc installed, and can rebind it using the panel.

The chosen key is stored in
`red4ext/plugins/m1arc/settings.json` as `overlay.hotkey`.
The menu key is chosen by the player.

The same chosen key opens and closes the panel. Escape is **not**
required to close it. The framework also includes an emergency
recovery combination (`Ctrl+Shift+F11`) intended for stuck input.

When the panel is closed, normal keyboard and mouse input continue to reach the game.

## Per-mod gameplay hotkeys

Mods can declare their own persistent hotkeys in `settings.json`, separately
from the panel shortcut.

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

The mod implements the action in its own REDscript code; M1 Arc queues the
key press as an event.
See [mod manifests](/mod-manifest) for details.
