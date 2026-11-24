# M1 Arc

Native in-game settings menu for Cyberpunk 2077 mods, built with RED4ext and Dear ImGui.

## Downloads

- [Nexus Mods](https://www.nexusmods.com/cyberpunk2077/mods/39311)
- [GitHub Releases](https://github.com/m1nsook/m1arc/releases)

## Requirements

- Cyberpunk 2077 2.50
- RED4ext
- REDscript

## Menu shortcut

On first launch, click **Capture**, press your preferred key (optionally with Ctrl and/or Shift), then click **Save**. Updating from 1.0.0 to 1.0.1 prompts for a new menu shortcut once. All mod settings, profiles, theme and language preferences are preserved; do not delete settings.json. If you accidentally choose a key or forget your shortcut, press **Ctrl + Shift + F11** in the focused game to reopen the shortcut setup. This combination is reserved for recovery.

## Logs and support diagnostics (1.0.1+)

Open M1 Arc, click the clipboard icon in the header, select the affected mod and choose **Copy diagnostics**. The report contains detected versions, current mod settings, pending actions, startup rejection reasons and recent session logs. Review it before sharing with the mod author. Mod developers can call `M1Arc_Log(modId, level, message)` to record actual failure reasons.

[Logs and diagnostics guide](https://m1nsook.github.io/m1arc/diagnostics)

## Documentation

[Installation, mod manifests, and native API](https://m1nsook.github.io/m1arc/)
