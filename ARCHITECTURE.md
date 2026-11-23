# M1 Arc architecture

M1 Arc is a native RED4ext plugin for Cyberpunk 2077 2.50.

- `src/plugin.cpp` registers the plugin and native REDscript functions.
- `src/input.cpp` handles panel and mod keybinding input.
- `src/graphics.cpp` and `src/ui.cpp` render the ImGui control panel.
- `src/mod_manifest.cpp` and `src/registry.cpp` load and validate mod settings.
- `src/state.cpp` persists settings and profiles.
- `src/localization.cpp` loads framework and per-mod JSON translations.
- `r6/scripts/m1arc/M1Arc.reds` declares the native functions available to mods.

Mod authors provide data-only manifests and ordinary REDscript logic.
M1 Arc does not run scripts from JSON. Its own keybinds and settings are
persistent, while button presses are one-shot events.

For user and mod-author guides, see
[the published documentation](https://m1nsook.github.io/m1arc/).
